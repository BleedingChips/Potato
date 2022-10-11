#pragma once
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include "PotatoStrEncode.h"

namespace Potato::Document
{

	using EncodeInfo = StrEncode::EncodeInfo;


	enum class DocumenetBomT
	{
		NoBom,
		UTF8,
		UTF16LE,
		UTF16BE,
		UTF32LE,
		UTF32BE
	};

	constexpr bool IsNativeEndian(DocumenetBomT T)
	{
		switch (T)
		{
		case DocumenetBomT::UTF16LE:
		case DocumenetBomT::UTF32LE:
			return std::endian::native == std::endian::little;
			break;
		case DocumenetBomT::UTF16BE:
		case DocumenetBomT::UTF32BE:
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
		static constexpr  DocumenetBomT Value = (std::endian::native == std::endian::little ? DocumenetBomT::UTF32LE : DocumenetBomT::UTF32BE);
	};

	template<>
	struct UnicodeTypeToBom<char16_t> {
		static constexpr  DocumenetBomT Value = (std::endian::native == std::endian::little ? DocumenetBomT::UTF16LE : DocumenetBomT::UTF16BE);
	};

	template<>
	struct UnicodeTypeToBom<wchar_t> : UnicodeTypeToBom<std::conditional_t<sizeof(wchar_t) == sizeof(char32_t), char32_t, char16_t>> {};

	template<>
	struct UnicodeTypeToBom<char8_t> {
		static constexpr  DocumenetBomT Value = DocumenetBomT::NoBom;
	};

	DocumenetBomT DetectBom(std::span<std::byte const> bom) noexcept;
	std::span<std::byte const> ToBinary(DocumenetBomT type) noexcept;

	struct DocumentReaderWrapper
	{

		DocumentReaderWrapper(DocumentReaderWrapper const&) = default;

		enum class StateT
		{
			Normal,
			EmptyBuffer,
			NotEnoughtSpace,
			UnSupport,
		};

		struct ReadResult
		{
			StateT State = StateT::Normal;
			std::size_t ReadCharacterCount = 0;
			std::size_t BufferSpace = 0;
		};

		template<typename UnicodeT, typename Function>
		auto Read(Function&& Func, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max())->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeInfo>);

		template<typename UnicodeT, typename CharTrai, typename Allocator>
		ReadResult Read(std::basic_string<UnicodeT, CharTrai, Allocator>& Output, std::size_t MaxCharactor = std::numeric_limits<std::size_t>::max())
		{
			std::size_t OldSize = Output.size();
			return this->Read<UnicodeT>([&](EncodeInfo Input) -> std::optional<std::span<UnicodeT>> {
				Output.resize(OldSize + Input.TargetSpace);
				return std::span(Output).subspan(OldSize);
				});
		}

		template<typename UnicodeT, typename Function>
		auto ReadLine(Function&& Func, bool KeepLine = true)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeInfo>);


		template<typename UnicodeT, typename CharTrai, typename Allocator>
		ReadResult ReadLine(std::basic_string<UnicodeT, CharTrai, Allocator>& Output, bool KeepLine = true)
		{
			std::size_t OldSize = Output.size();
			return this->ReadLine<UnicodeT>([&](EncodeInfo Input) -> std::optional<std::span<UnicodeT>> {
				Output.resize(OldSize + Input.TargetSpace);
				return std::span(Output).subspan(OldSize);
				}, KeepLine);
		}

		DocumenetBomT GetBom() const { return Bom; }

		explicit operator bool() const { return Available.Count() != 0; }

	private:

		DocumentReaderWrapper(DocumenetBomT Bom, std::span<std::byte> TemporaryBuffer) : Bom(Bom), DocumentSpan(TemporaryBuffer), Available{ 0, 0 } {}

		DocumenetBomT Bom = DocumenetBomT::NoBom;
		std::span<std::byte> DocumentSpan;
		Misc::IndexSpan<> Available;
		std::size_t TotalCharacter = 0;

		friend struct DocumentReader;
	};

	template<typename UnicodeT, typename Function>
	auto DocumentReaderWrapper::Read(Function&& Func, std::size_t MaxCharacter)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeInfo>)
	{
		if (Available.Count() == 0)
			return { StateT::EmptyBuffer, 0, 0 };
		std::span<std::byte> CurSpan = DocumentSpan.subspan(Available.Begin(), Available.Count());
		switch (GetBom())
		{
		case DocumenetBomT::NoBom:
		case DocumenetBomT::UTF8:
		{
			std::span<char8_t> Cur{ reinterpret_cast<char8_t*>(CurSpan.data()), CurSpan.size() / sizeof(char8_t) };
			EncodeInfo Info = StrEncoder<char8_t, UnicodeT>::RequireSpaceUnSafe(Cur, MaxCharacter);
			std::optional<std::span<UnicodeT>> OutputBuffer = Func(Info);
			if (OutputBuffer.has_value())
				StrEncoder<char8_t, UnicodeT>::EncodeUnSafe(Cur, *OutputBuffer, MaxCharacter);
			Available = Available.Sub(Info.SourceSpace);
			TotalCharacter -= Info.CharacterCount;
			return { StateT::Normal, Info.CharacterCount, Info.TargetSpace };
			break;
		}
		default:
			return { StateT::UnSupport, 0, 0 };
			break;
		}
	}

	template<typename UnicodeT, typename Function>
	auto DocumentReaderWrapper::ReadLine(Function&& Func, bool KeepLine)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeInfo>)
	{
		if (Available.Count() == 0)
			return { StateT::EmptyBuffer, 0, 0 };
		std::span<std::byte> CurSpan = DocumentSpan.subspan(Available.Begin(), Available.Count());
		switch (GetBom())
		{
		case DocumenetBomT::NoBom:
		case DocumenetBomT::UTF8:
		{
			std::span<char8_t> Cur{ reinterpret_cast<char8_t*>(CurSpan.data()), CurSpan.size() / sizeof(char8_t) };
			EncodeInfo Info = StrEncoder<char8_t, UnicodeT>::RequireSpaceLineUnsafe(Cur, KeepLine);
			std::optional<std::span<UnicodeT>> OutputBuffer = Func(Info);
			if (OutputBuffer.has_value())
				StrEncoder<char8_t, UnicodeT>::EncodeUnSafe(Cur, *OutputBuffer, Info.CharacterCount);
			Available = Available.Sub(Info.SourceSpace);
			TotalCharacter -= Info.CharacterCount;
			return { StateT::Normal, Info.CharacterCount, Info.TargetSpace };
			break;
		}
		default:
			return { StateT::UnSupport, 0, 0 };
			break;
		}
	}

	struct DocumentReader
	{
		DocumentReader(std::filesystem::path path);
		DocumentReader(DocumentReader const&) = default;
		DocumentReader& operator=(DocumentReader const&) = default;
		explicit operator bool() const { return File.is_open(); }

		void ResetIterator();

		DocumenetBomT GetBom() const { return Bom; }

		enum class FlushResult
		{
			UnSupport,
			Finish,
			EndOfFile,
			BadString
		};

		FlushResult Flush(DocumentReaderWrapper& Reader);

		std::size_t RecalculateLastSize();

		DocumentReaderWrapper CreateWrapper(std::span<std::byte> TemporaryBuffer) const { return DocumentReaderWrapper{ Bom, TemporaryBuffer }; }

	private:

		DocumenetBomT Bom = DocumenetBomT::NoBom;
		std::ifstream File;
		std::size_t TextOffset = 0;
	};

	struct ImmediateDocumentReader
	{
		ImmediateDocumentReader(std::filesystem::path Path);
		bool IsGood() const { return Buffer.has_value(); }
		std::optional<std::u8string_view> TryCastU8() const;
	private:
		std::optional<std::vector<std::byte>> Buffer;
		DocumenetBomT Bom = DocumenetBomT::NoBom;
	};

	struct DocumentEncoder
	{

		static EncodeInfo RequireSpaceUnsafe(std::u32string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeInfo RequireSpaceUnsafe(std::u16string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeInfo RequireSpaceUnsafe(std::u8string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeInfo RequireSpaceUnsafe(std::wstring_view Str, DocumenetBomT Bom, bool WriteBom = false);

		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::u32string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::u16string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::u8string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::wstring_view Str, DocumenetBomT Bom, bool WriteBom = false);
	};

	struct DocumentWriter
	{
		DocumentWriter(std::filesystem::path Path, DocumenetBomT BomType = DocumenetBomT::NoBom);
		explicit operator bool() const { return File.is_open(); }
		EncodeInfo Write(std::u32string_view Str);
		EncodeInfo Write(std::u16string_view Str);
		EncodeInfo Write(std::u8string_view Str);
		EncodeInfo Write(std::wstring_view Str);
	private:
		std::ofstream File;
		DocumenetBomT BomType;
		std::vector<std::byte> TemporaryBuffer;
	};
}