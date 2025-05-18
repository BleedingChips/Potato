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
		DocumentReader(BinaryStreamReader& reader, std::optional<BomT> force_bom = std::nullopt);
		BomT GetBom() const { return bom; }
		operator bool() const { return reader; }
		template<typename OutIterator>
		OutIterator ReadLine(OutIterator out_iterator);
		~DocumentReader();
	protected:
		std::size_t FlushBuffer();
		BinaryStreamReader& reader;
		BomT bom = BomT::NoBom;
		std::array<wchar_t, 2048> temporary_buffer;
		Misc::IndexSpan<> available_buffer_index;
	};

	template<typename OutIterator>
	OutIterator DocumentReader::ReadLine(OutIterator out_iterator)
	{
		while (true)
		{
			if (available_buffer_index.Size() == 0)
			{
				if (FlushBuffer() == 0)
				{
					return out_iterator;
				}
			}
			auto span = available_buffer_index.Slice(std::wstring_view{ temporary_buffer });
			auto ite = std::find(span.begin(), span.end(), L'\n');
			bool has_switch_line = false;
			if (ite != span.end())
			{
				has_switch_line = true;
				ite += 1;
			}
			out_iterator = std::copy(span.begin(), ite, out_iterator);
			available_buffer_index = available_buffer_index.SubIndex(ite - span.begin());
			if (has_switch_line)
			{
				return out_iterator;
			}
		}
	}

	struct DocumentWriter
	{
		DocumentWriter(BinaryStreamWriter& writer, BomT force_bom = BomT::NoBom, bool append_bom = false);
		BomT GetBom() const { return bom; }
		operator bool() const { return writer; }
		void Write(std::wstring_view str);

		struct OutIterator
		{
			using iterator_category = std::output_iterator_tag;
			using value_type = void;
			using pointer = void;
			using reference = void;
			using difference_type = ptrdiff_t;
			using container_type = DocumentWriter;


			OutIterator(DocumentWriter& writer) : writer(&writer) {}
			OutIterator(OutIterator const&) = default;
			OutIterator& operator*() { return *this; }
			OutIterator& operator=(wchar_t const& symbol) { writer->Write({ &symbol, 1 }); return *this; }
			OutIterator operator++(int) { return *this; }
			OutIterator& operator++() { return *this; }
			OutIterator& operator=(OutIterator const&) = default;
		protected:
			DocumentWriter* writer = nullptr;
			friend struct DocumentWriter;
		};

		OutIterator AsOutputIterator() { return OutIterator{*this}; }
		~DocumentWriter() { TryFlush(); }

	protected:

		std::size_t TryFlush();
		std::array<wchar_t, 4096> cache_buffer;
		std::size_t buffer_space = 0;
		BinaryStreamWriter& writer;
		BomT bom = BomT::NoBom;
	};
}