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


	struct BinaryStreamReader
	{
		BinaryStreamReader(std::filesystem::path const& path) { Open(path); }
		BinaryStreamReader() = default;
		BinaryStreamReader(BinaryStreamReader&& reader);
		operator bool() const;
		std::size_t Read(std::span<std::byte> output);
		std::size_t GetStreamSize() const;
		bool Open(std::filesystem::path const& path);
		void Close();
		std::optional<std::ptrdiff_t> SetPointerOffsetFromBegin(std::ptrdiff_t offset = 0);
		std::optional<std::ptrdiff_t> SetPointerOffsetFromEnd(std::ptrdiff_t offset = 0);
		std::optional<std::ptrdiff_t> SetPointerOffsetFromCurrent(std::ptrdiff_t offset = 0);
		~BinaryStreamReader();
	protected:
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
#endif
	};

	struct BinaryStreamWriter
	{
		enum class OpenMode
		{
			APPEND,
			CREATE,
			APPEND_OR_CREATE,
			CREATE_OR_EMPTY,
			EMPTY
		};
		BinaryStreamWriter(std::filesystem::path const& target_path, OpenMode mode) { Open(target_path, mode); }
		BinaryStreamWriter() = default;
		BinaryStreamWriter(BinaryStreamWriter&& reader);
		operator bool() const;
		bool Open(std::filesystem::path const& path, OpenMode mode);
		bool Write(std::byte const* data, std::size_t size);
		bool Write(std::span<std::byte const> input) { return Write(input.data(), input.size()); }
		template<typename Type>
		bool Write(std::span<Type const> input) { return this->Write(reinterpret_cast<std::byte const*>(input.data()),  sizeof(Type) * input.size()); }
		void Close();
		~BinaryStreamWriter();
	protected:
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
#endif
	};

	struct DocumentReader
	{
		DocumentReader(std::span<std::byte const> stream, std::optional<BomT> force_bom = std::nullopt);

		std::optional<Encode::EncodeInfo> ReadSome(std::span<char> output, std::size_t max_character_count = std::numeric_limits<std::size_t>::max());

		template<typename OutIterator>
		OutIterator Read(OutIterator out_iterator, std::size_t max_character_count = std::numeric_limits<std::size_t>::max());
		BomT GetBom() const { return bom; }
		operator bool() const { return !stream.empty(); }
	protected:

		std::span<std::byte const> stream;
		BomT bom = BomT::NoBom;
	};

	template<typename OutIterator>
	OutIterator DocumentReader::Read(OutIterator out_iterator, std::size_t max_character)
	{
		std::array<char, 2048> tem_buffer;
		while (true)
		{
			auto info = this->ReadSome(std::span(tem_buffer), max_character);
			if (info.has_value())
			{
				max_character -= info->character_count;
				out_iterator = std::copy_n(tem_buffer.begin(), info->target_space, out_iterator);
				if (!info->good)
				{
					return out_iterator;
				}
			}
			else {
				return out_iterator;
			}
		}
	}

	struct DocumentWriter
	{
		DocumentWriter(BomT force_bom = BomT::NoBom, bool need_bom = true, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		BomT GetBom() const { return bom; }
		std::size_t Write(std::string_view str);
		bool FlushTo(BinaryStreamWriter& writer);
	protected:
		std::pmr::vector<std::byte> cache_buffer;
		BomT bom = BomT::NoBom;
	};
}