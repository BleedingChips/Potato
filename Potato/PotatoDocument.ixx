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
		virtual std::size_t StreamRead(std::byte* out, std::size_t byte) override;
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
		bool FillBuffer();
		std::optional<BomT> UpdateBom();
	};

	struct PlainTextWriter
	{
		enum class Encoding
		{
			UTF8,
			UTF16,
			UTF32
		};

		struct Config
		{
			BomT bom = BomT::NoBom;
			bool need_bom = true;
			std::size_t cache_buffer_size = 1024 * 1024 * 2;
			std::pmr::memory_resource* resource = std::pmr::get_default_resource();
		};

		PlainTextWriter(DocumentWriter& writer, Config config = {});
		~PlainTextWriter();
		BomT GetBom() const { return bom; }
		//std::size_t Write(std::u8string_view str);
		//std::size_t Write(std::string_view str) { return Write(std::u8string_view{ reinterpret_cast<char8_t const*>(str.data()), str.size() }); }
		std::size_t Flush();
		std::size_t Write(Encoding encode, std::byte const* string, std::size_t size_in_byte);
		std::size_t Write(std::u8string_view str) {
			return Write(Encoding::UTF8, reinterpret_cast<std::byte const*>(str.data()), str.size() * sizeof(char8_t));
		}
		std::size_t Write(std::string_view str) {
			return Write(Encoding::UTF8, reinterpret_cast<std::byte const*>(str.data()), str.size() * sizeof(char));
		}
		std::size_t Write(std::u16string_view str) {
			return Write(Encoding::UTF16, reinterpret_cast<std::byte const*>(str.data()), str.size() * sizeof(char16_t));
		}
		std::size_t Write(std::u32string_view str) {
			return Write(Encoding::UTF32, reinterpret_cast<std::byte const*>(str.data()), str.size() * sizeof(char32_t));
		}
		std::size_t Write(std::wstring_view str) {
			return Write(
				(sizeof(wchar_t) == sizeof(char16_t) ? Encoding::UTF16 : Encoding::UTF32),
				reinterpret_cast<std::byte const*>(str.data()), str.size() * sizeof(wchar_t)
			);
		}
		static std::size_t ByteswapUTF16(std::span<std::byte> data);
		static std::size_t ByteswapUTF32(std::span<std::byte> data);
		template<typename ...AT>
		void FormatWrite(const std::format_string<AT...> format_string, AT&& ...at)
		{
			return this->FormatWriteImp(format_string, std::forward<AT>(at)...);
		}
		template<typename ...AT>
		void FormatWrite(const std::wformat_string<AT...> format_string, AT&& ...at)
		{
			return this->FormatWriteImp(format_string, std::forward<AT>(at)...);
		}
	protected:
		template<typename CharT, typename ...AT>
		void FormatWriteImp(const std::basic_format_string<CharT, AT...> format_string, AT&& ...at);
		DocumentWriter& writer;
		BomT bom = BomT::NoBom;
		std::pmr::vector<std::byte> cache_buffer;
		std::size_t cache_buffer_used = 0;
	};

	template<typename CharT, typename ...AT>
	void PlainTextWriter::FormatWriteImp(const std::basic_format_string<CharT, AT...> format_string, AT&& ...at)
	{
		struct PlainTextWriterOutputIterator
		{
			using iterator_category = std::output_iterator_tag;
			using value_type = void;
			using pointer = void;
			using reference = void;

			using container_type = void;

			using difference_type = ptrdiff_t;

			PlainTextWriterOutputIterator(
				PlainTextWriter& writer,
				std::span<CharT> buffer,
				std::size_t& reference_index
			) : writer(writer), buffer(buffer), reference_index(reference_index)
			{
			}

			PlainTextWriterOutputIterator(PlainTextWriterOutputIterator const&) = default;
			PlainTextWriterOutputIterator(PlainTextWriterOutputIterator&&) = default;
			PlainTextWriterOutputIterator& operator++() {return *this; }
			PlainTextWriterOutputIterator operator++(int) { return *this; }

			PlainTextWriterOutputIterator& operator=(PlainTextWriterOutputIterator const& other)
			{
				return *this;
			}

			auto operator*() -> CharT&
			{
				if (reference_index + 1 >= buffer.size())
				{
					auto info = Encode::UnicodeEncoder<CharT, Encode::Unicode::CodePointT>::Statistics(
						std::span(buffer.data(), reference_index)
					);
					writer.Write(
						std::basic_string_view(buffer.data(), info.source_space)
					);
					std::memmove(buffer.data(), buffer.data() + info.source_space, reference_index - info.source_space);
					reference_index = reference_index - info.source_space;
				}
				return buffer[reference_index++];
			}
		protected:
			PlainTextWriter& writer;
			std::size_t& reference_index;
			std::span<CharT> buffer;
		};

		std::array<CharT, Encode::Unicode::temporary_cache_buffer_size * sizeof(CharT)> temporary_size;
		std::size_t index = 0;

		PlainTextWriterOutputIterator iterator(*this, temporary_size, index);

		iterator = std::format_to(
			iterator,
			format_string,
			std::forward<AT>(at)...
		);

		if (index != 0)
		{
			Write(
				std::basic_string_view{ temporary_size.data(), index - 1}
			);
		}
	}

}