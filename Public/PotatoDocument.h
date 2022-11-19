#pragma once
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include "PotatoStrEncode.h"

namespace Potato::Document
{

	using EncodeInfo = StrEncode::EncodeInfo;


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

	BomT DetectBom(std::span<std::byte const> bom) noexcept;
	std::span<std::byte const> ToBinary(BomT type) noexcept;

	struct ReaderBuffer
	{
		ReaderBuffer(ReaderBuffer const&) = default;

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

		BomT GetBom() const { return Bom; }

		explicit operator bool() const { return Available.Count() != 0; }

	private:

		ReaderBuffer(BomT Bom, std::span<std::byte> TemporaryBuffer) : Bom(Bom), DocumentSpan(TemporaryBuffer), Available{ 0, 0 } {}

		BomT Bom = BomT::NoBom;
		std::span<std::byte> DocumentSpan;
		Misc::IndexSpan<> Available;
		std::size_t TotalCharacter = 0;

		friend struct Reader;
	};

	template<typename UnicodeT, typename Function>
	auto ReaderBuffer::Read(Function&& Func, std::size_t MaxCharacter)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeInfo>)
	{
		if (Available.Count() == 0)
			return { StateT::EmptyBuffer, 0, 0 };
		std::span<std::byte> CurSpan = DocumentSpan.subspan(Available.Begin(), Available.Count());
		switch (GetBom())
		{
		case BomT::NoBom:
		case BomT::UTF8:
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
	auto ReaderBuffer::ReadLine(Function&& Func, bool KeepLine)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeInfo>)
	{
		if (Available.Count() == 0)
			return { StateT::EmptyBuffer, 0, 0 };
		std::span<std::byte> CurSpan = DocumentSpan.subspan(Available.Begin(), Available.Count());
		switch (GetBom())
		{
		case BomT::NoBom:
		case BomT::UTF8:
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

	struct Reader
	{
		Reader(std::filesystem::path path);
		Reader(Reader const&) = default;
		Reader& operator=(Reader const&) = default;
		explicit operator bool() const { return File.is_open(); }

		void ResetIterator();

		BomT GetBom() const { return Bom; }

		enum class FlushResult
		{
			UnSupport,
			Finish,
			EndOfFile,
			BadString
		};

		FlushResult Flush(ReaderBuffer& Reader);

		std::size_t RecalculateLastSize();

		ReaderBuffer CreateWrapper(std::span<std::byte> TemporaryBuffer) const { return ReaderBuffer{ Bom, TemporaryBuffer }; }

	private:

		BomT Bom = BomT::NoBom;
		std::ifstream File;
		std::size_t TextOffset = 0;
	};

	struct ImmediateReader
	{
		ImmediateReader(std::filesystem::path Path);
		bool IsGood() const { return Buffer.has_value(); }
		std::optional<std::u8string_view> TryCastU8() const;
	private:
		std::optional<std::vector<std::byte>> Buffer;
		BomT Bom = BomT::NoBom;
	};

	struct Encoder
	{
		static EncodeInfo RequireSpaceUnsafe(std::u32string_view Str, BomT Bom, bool WriteBom = false);
		static EncodeInfo RequireSpaceUnsafe(std::u16string_view Str, BomT Bom, bool WriteBom = false);
		static EncodeInfo RequireSpaceUnsafe(std::u8string_view Str, BomT Bom, bool WriteBom = false);
		static EncodeInfo RequireSpaceUnsafe(std::wstring_view Str, BomT Bom, bool WriteBom = false);

		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::u32string_view Str, BomT Bom, bool WriteBom = false);
		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::u16string_view Str, BomT Bom, bool WriteBom = false);
		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::u8string_view Str, BomT Bom, bool WriteBom = false);
		static EncodeInfo EncodeUnsafe(std::span<std::byte> Span, std::wstring_view Str, BomT Bom, bool WriteBom = false);
	};

	struct Writer
	{
		Writer(std::filesystem::path Path, BomT BomType = BomT::NoBom);
		explicit operator bool() const { return File.is_open(); }
		EncodeInfo Write(std::u32string_view Str);
		EncodeInfo Write(std::u16string_view Str);
		EncodeInfo Write(std::u8string_view Str);
		EncodeInfo Write(std::wstring_view Str);
		~Writer(){ Flush(); }
		void Flush() { if (File.good()) File.flush(); }
	private:
		std::ofstream File;
		BomT BomType;
		std::vector<std::byte> TemporaryBuffer;
	};

	struct SperateResult
	{
		std::u8string_view LineStr;
		std::u8string_view Last;

		enum class LineMode
		{
			N,
			RN
		}Mode;
	};

	struct LineSperater
	{
		enum class LineMode
		{
			N,
			RN
		};

		struct Result
		{
			std::optional<LineMode> Mode;
			std::u8string_view Str;
		};

		LineSperater(std::u8string_view InStr) : TotalStr(InStr), IteStr(InStr) {}

		Result Consume(bool KeepLineSymbol = true);
		std::size_t GetItePosition() const { return TotalStr.size() - IteStr.size(); }
		std::u8string_view GetTotalStr() const { return TotalStr; };
		operator bool() const { return !IteStr.empty(); }
		void Clear() { IteStr = TotalStr; }

	private:

		std::u8string_view TotalStr;
		std::u8string_view IteStr;
	};

}