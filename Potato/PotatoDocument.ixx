module;

#ifdef _WIN32
#include <Windows.h>
#undef max
#endif

export module PotatoDocument;

import std;
import PotatoTMP;
import PotatoMisc;
import PotatoEncode;
import PotatoStreamer;


export namespace Potato::Document
{

	using EncodeInfo = Encode::EncodeInfo;

	enum class BomT
	{
		NoBom,
		UTF8,
		UTF16LE,
		UTF16BE,
		UTF32LE,
		UTF32BE
	};

	constexpr bool IsNativeEndian(BomT T)
	{
		switch (T)
		{
		case BomT::UTF16LE:
		case BomT::UTF32LE:
			return std::endian::native == std::endian::little;
			break;
		case BomT::UTF16BE:
		case BomT::UTF32BE:
			return std::endian::native == std::endian::big;
			break;
		default:
			return true;
			break;
		}
	}

	template<typename UnicodeT>
	struct UnicodeTypeToBom;

	template<>
	struct UnicodeTypeToBom<char32_t> {
		static constexpr  BomT Value = (std::endian::native == std::endian::little ? BomT::UTF32LE : BomT::UTF32BE);
	};

	template<>
	struct UnicodeTypeToBom<char16_t> {
		static constexpr  BomT Value = (std::endian::native == std::endian::little ? BomT::UTF16LE : BomT::UTF16BE);
	};

	template<>
	struct UnicodeTypeToBom<wchar_t> : UnicodeTypeToBom<std::conditional_t<sizeof(wchar_t) == sizeof(char32_t), char32_t, char16_t>> {};

	template<>
	struct UnicodeTypeToBom<char8_t> {
		static constexpr  BomT Value = BomT::NoBom;
	};


	struct DocumentReader : public Streamer::StreamRandomReader
	{
		DocumentReader(std::filesystem::path const& path) { Open(path); }
		DocumentReader() = default;
		DocumentReader(DocumentReader&& reader);
		operator bool() const;
		std::size_t Read(std::span<std::byte> output);
		virtual std::size_t StreamRead(std::span<std::byte> out_byte) override;
		std::size_t GetStreamSize() const;
		bool Open(std::filesystem::path const& path);
		void Close();
		virtual std::optional<std::ptrdiff_t> StreamSeek(std::ptrdiff_t offset, Streamer::SeekAnchor anchor = Streamer::SeekAnchor::Start) override;
		virtual Streamer::StreamState GetStreamState() const override { return state; }
		~DocumentReader();
		static std::optional<std::size_t> ReadToBuffer(std::filesystem::path const& path, TMP::FunctionRef<std::span<std::byte>(std::size_t)> allocate_func);
	protected:
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
#endif
		Streamer::StreamState state = Streamer::StreamState::NoExist;
	};

	struct DocumentWriter
	{
		enum class OpenMode
		{
			APPEND,
			CREATE,
			APPEND_OR_CREATE,
			CREATE_OR_EMPTY,
			EMPTY
		};
		DocumentWriter(std::filesystem::path const& target_path, OpenMode mode) { Open(target_path, mode); }
		DocumentWriter() = default;
		DocumentWriter(DocumentWriter&& reader);
		operator bool() const;
		bool Open(std::filesystem::path const& path, OpenMode mode);
		std::size_t Write(std::byte const* data, std::size_t size);
		std::size_t Write(std::span<std::byte const> input) { return Write(input.data(), input.size()); }
		template<typename Type>
		std::size_t Write(std::span<Type const> input) { return this->Write(reinterpret_cast<std::byte const*>(input.data()),  sizeof(Type) * input.size()); }
		void Close();
		~DocumentWriter();
	protected:
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
#endif
	};


	struct PlainTextReader
	{
		struct Config
		{
			std::size_t cache_buffer_size = 1024 * 1024 * 2;
			std::pmr::memory_resource* cache_buffer_resource = std::pmr::get_default_resource();
			std::optional<BomT> bom = std::nullopt;
		};

		using CutoffSetting = Encode::EncodeCutOffSetting;

		PlainTextReader(DocumentReader& reader_reference, Config config = {})
			: reader_reference(reader_reference), cache_buffer_size(config.cache_buffer_size), cache_buffer(config.cache_buffer_resource) 
		{
			cache_buffer.resize(cache_buffer_size);
			
			if (!config.bom.has_value())
			{
				FillBuffer();
				UpdateBom();
			}
			else {
				bom = *config.bom;
			}
			
		}
		PlainTextReader(PlainTextReader const&) = delete;

		std::optional<Encode::EncodeInfo> ReadPlainText(std::span<char8_t> output, CutoffSetting cutoff = {});
		std::optional<Encode::EncodeInfo> ReadPlainText(std::span<Encode::Unicode::CodePointT> output, CutoffSetting cutoff = {});

		template<std::output_iterator<char8_t> OutIterator>
		std::optional<Encode::EncodeInfo> ReadPlainText(OutIterator out_iterator, CutoffSetting cutoff = {})
		{
			std::array<char8_t, Encode::Unicode::temporary_cache_buffer_size* Encode::Unicode::UTF8::max_storage_size> temp_buffer;
			Encode::EncodeInfo info;
			while (info.character_count < cutoff.max_character_count && info.is_good_string && !info.reach_cutoff_character)
			{
				auto cur = ReadPlainText(std::span(temp_buffer), cutoff);
				if (cur.has_value())
				{
					info+= *cur;
					out_iterator = std::copy_n(
						temp_buffer.data(),
						cur->target_space * sizeof(char8_t),
						out_iterator
					);
				}
				else {
					if (info.character_count == 0)
					{
						return std::nullopt;
					}
					else {
						return info;
					}
				}
			}
			return info;
		}

		BomT GetBom() const { return bom; }
		static std::tuple<BomT, std::size_t> DetectBom(std::span<std::byte const> byte);
		static std::span<std::byte const> GetBomByteSpan(BomT bom);

	protected:
		
		Streamer::StreamRandomReader& reader_reference;
		std::size_t cache_buffer_size = 1024 * 1024 * 2;
		std::pmr::vector<std::byte> cache_buffer;
		Misc::IndexSpan<> buffer_index;
		Misc::IndexSpan<> hanlded_buffer_index;
		BomT bom = BomT::NoBom;
		void FillBuffer();
		std::optional<BomT> UpdateBom();
	};

	struct PlainTextWritter
	{
		struct Config
		{
			BomT bom = BomT::NoBom;
			bool need_bom = true;
			std::size_t cache_buffer_size = 1024 * 1024 * 2;
			std::pmr::memory_resource* resource = std::pmr::get_default_resource();
		};
		PlainTextWritter(DocumentWriter& writer, Config config = {});
		~PlainTextWritter();
		BomT GetBom() const { return bom; }
		std::size_t Write(std::u8string_view str);
		std::size_t Write(std::string_view str) { return Write(std::u8string_view{ reinterpret_cast<char8_t const*>(str.data()), str.size() }); }
		std::size_t Flush();
	protected:
		DocumentWriter& writer;
		std::size_t cache_buffer_size = 1024 * 1024 * 2;
		std::pmr::vector<std::byte> cache_buffer;
		std::size_t cache_buffer_used = 0;
		BomT bom = BomT::NoBom;
	};

}