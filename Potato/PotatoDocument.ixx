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
		std::span<std::byte> Read(std::span<std::byte> output);
		std::uint64_t GetStreamSize() const;
		bool Open(std::filesystem::path const& path);
		void Close();
		std::optional<std::int64_t> SetPointerOffsetFromBegin(std::int64_t offset = 0);
		std::optional<std::int64_t> SetPointerOffsetFromEnd(std::int64_t offset = 0);
		std::optional<std::int64_t> SetPointerOffsetFromCurrent(std::int64_t offset = 0);
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
		bool Write(std::span<std::byte const> output);
		void Close();
		~BinaryStreamWriter();
	protected:
#ifdef _WIN32
		HANDLE file = INVALID_HANDLE_VALUE;
#endif
	};

	template<typename Type>
	concept AcceptableCharType = TMP::IsOneOfV<Type, char8_t, wchar_t, char16_t, char32_t>;

	struct StringSerializer
	{
		template<typename CharT> struct Holder{};
		StringSerializer(BomT bom = BomT::NoBom) : bom(bom) {}
		StringSerializer(StringSerializer const&) = default;
		BomT GetBom() const { return bom; }
		void OverrideBom(BomT bom) { this->bom = bom; }

		std::span<std::byte const> ConsumeBom(std::span<std::byte const> input);
		std::span<std::byte const> GetBinaryBom() const;

		template<AcceptableCharType CharT>
		EncodeInfo RequireSpace(std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) { return RequireSpace(Holder<CharT>{}, input, max_character); }

		template<AcceptableCharType CharT>
		EncodeInfo SerializerToUnsafe(std::span<CharT> output, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) { return SerializerToUnsafe(Holder<CharT>{}, output, input, max_character); }

		template<AcceptableCharType CharT>
		std::optional<std::basic_string_view<CharT>> TryDirectCastToStringView(std::span<std::byte const> input) const { return TryDirectCastToStringView(Holder<CharT>{}, input); }

		bool SerializerBomTo(BinaryStreamWriter& writer) const { return writer.Write(GetBinaryBom()); }
		bool SerializerTo(BinaryStreamWriter& writer, std::span<char8_t const> input) const;
		bool IsNativeEnding() const { return IsNativeEndian(bom); }

	protected:

		EncodeInfo RequireSpace(Holder<char8_t> holder, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const;
		EncodeInfo RequireSpace(Holder<char16_t> holder, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return {false}; }
		EncodeInfo RequireSpace(Holder<char32_t> holder, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return { false }; }
		EncodeInfo RequireSpace(Holder<wchar_t> holder, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return {false }; }

		EncodeInfo SerializerToUnsafe(Holder<char8_t> holder, std::span<char8_t> output, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const;
		EncodeInfo SerializerToUnsafe(Holder<char16_t> holder, std::span<char16_t> output, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return {false}; }
		EncodeInfo SerializerToUnsafe(Holder<char32_t> holder, std::span<char32_t> output, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return { false }; }
		EncodeInfo SerializerToUnsafe(Holder<wchar_t> holder, std::span<wchar_t> output, std::span<std::byte const> input, std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return {false }; }

		std::optional<std::basic_string_view<char8_t>> TryDirectCastToStringView(Holder<char8_t> holder, std::span<std::byte const> input) const;
		std::optional<std::basic_string_view<char16_t>> TryDirectCastToStringView(Holder<char16_t> holder, std::span<std::byte const> input) const { return std::nullopt; }
		std::optional<std::basic_string_view<char32_t>> TryDirectCastToStringView(Holder<char32_t> holder, std::span<std::byte const> input) const { return std::nullopt; }
		std::optional<std::basic_string_view<wchar_t>> TryDirectCastToStringView(Holder<wchar_t> holder, std::span<std::byte const> input) const { return std::nullopt; }

		BomT bom = BomT::NoBom;
	};


	struct StringSplitter
	{
		struct Line
		{
			enum class Mode
			{
				None,
				N,
				RN
			};
			Mode mode = Mode::None;
			Misc::IndexSpan<> index_without_line;
			Misc::IndexSpan<> index_with_line;
			std::size_t GetLineEnd() const { return index_with_line.End(); }
			template<typename CharT>
			std::basic_string_view<CharT> SliceWithoutLine(std::basic_string_view<CharT> str) const { return index_without_line.Slice(str);  }
			template<typename CharT>
			std::basic_string_view<CharT> SliceWithLine(std::basic_string_view<CharT> str) const { return index_with_line.Slice(str);  }
			std::size_t SizeWithoutLine() const { return index_without_line.Size(); }
			std::size_t SizeWithLine() const { return index_with_line.Size(); }
		};

		static Line SplitLine(std::u8string_view str, std::size_t offset = 0);
	};
}