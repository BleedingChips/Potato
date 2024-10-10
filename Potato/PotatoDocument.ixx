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

	BomT DetectBom(std::span<std::byte const> bom) noexcept;
	std::span<std::byte const> ToBinary(BomT type) noexcept;

	export struct Reader;

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

		explicit operator bool() const { return Available.Size() != 0; }

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
		if (Available.Size() == 0)
			return { StateT::EmptyBuffer, 0, 0 };
		std::span<std::byte> CurSpan = DocumentSpan.subspan(Available.Begin(), Available.Size());
		switch (GetBom())
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			std::span<char8_t> Cur{ reinterpret_cast<char8_t*>(CurSpan.data()), CurSpan.size() / sizeof(char8_t) };
			EncodeInfo Info = Encode::StrEncoder<char8_t, UnicodeT>::RequireSpaceUnSafe(Cur, MaxCharacter);
			std::optional<std::span<UnicodeT>> OutputBuffer = Func(Info);
			if (OutputBuffer.has_value())
				Encode::StrEncoder<char8_t, UnicodeT>::EncodeUnSafe(Cur, *OutputBuffer, MaxCharacter);
			Available = Available.SubIndex(Info.SourceSpace);
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
		if (Available.Size() == 0)
			return { StateT::EmptyBuffer, 0, 0 };
		std::span<std::byte> CurSpan = Available.Slice(DocumentSpan);
		switch (GetBom())
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			std::span<char8_t> Cur{ reinterpret_cast<char8_t*>(CurSpan.data()), CurSpan.size() / sizeof(char8_t) };
			EncodeInfo Info = Encode::StrEncoder<char8_t, UnicodeT>::RequireSpaceLineUnsafe(Cur, KeepLine);
			std::optional<std::span<UnicodeT>> OutputBuffer = Func(Info);
			if (OutputBuffer.has_value())
				Encode::StrEncoder<char8_t, UnicodeT>::EncodeUnSafe(Cur, *OutputBuffer, Info.CharacterCount);
			Available = Available.SubIndex(Info.SourceSpace);
			TotalCharacter -= Info.CharacterCount;
			return { StateT::Normal, Info.CharacterCount, Info.TargetSpace };
			break;
		}
		default:
			return { StateT::UnSupport, 0, 0 };
			break;
		}
	}

	export struct Reader
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
		BomT GetBom() const { return Bom; }
		std::span<std::byte const> GetBuffer() const { return std::span(*Buffer); }
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


	struct BinaryStreamReader
	{
		BinaryStreamReader(std::filesystem::path const& path) { Open(path); }
		BinaryStreamReader() = default;
		BinaryStreamReader(BinaryStreamReader&&) = default;
		operator bool() const;
		std::span<std::byte> Read(std::span<std::byte> output);
		std::int64_t GetStreamSize() const;
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

	struct StringSerializer
	{
		StringSerializer(std::span<std::byte const> input, BomT bom = BomT::NoBom) : reference(input), bom(bom) {}
		StringSerializer(BomT bom = BomT::NoBom) : bom(bom) {}
		StringSerializer(StringSerializer const&) = default;
		void SetBom(BomT bom = BomT::NoBom) { this->bom = bom; }
		BomT SerializeAndSetBom();

		template<typename Tar> EncodeInfo DetectCharacterCount(std::size_t max_character = std::numeric_limits<std::size_t>::max())
			requires(TMP::IsOneOfV<Tar, char8_t, char32_t, char16_t, wchar_t>) 
		{
			if constexpr (std::is_same_v<Tar, char8_t>)
			{
				return DetectUTF8Count();
			}else if constexpr (std::is_same_v<Tar, char16_t>)
			{
				return DetectUTF16Count();
			}else if constexpr (std::is_same_v<Tar, char32_t>)
			{
				return DetectUTF32Count();
			}else
			{
				static_assert(std::is_same_v<Tar, wchar_t>);
				return DetectWideCount();
			}
		}

		EncodeInfo DetectUTF8Count(std::size_t max_character = std::numeric_limits<std::size_t>::max()) const;
		EncodeInfo DetectUTF16Count(std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return {false}; }
		EncodeInfo DetectUTF32Count(std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return { false }; }
		EncodeInfo DetectWideCount(std::size_t max_character = std::numeric_limits<std::size_t>::max()) const { return {false }; }
		
		EncodeInfo SerializeToStringUnsafe(std::span<char8_t> output, std::size_t max_character = std::numeric_limits<std::size_t>::max());
		EncodeInfo SerializeToStringUnsafe(std::span<char16_t> output, std::size_t max_character = std::numeric_limits<std::size_t>::max()) { return {false }; }
		EncodeInfo SerializeToStringUnsafe(std::span<char32_t> output, std::size_t max_character = std::numeric_limits<std::size_t>::max()) { return {false }; }
		EncodeInfo SerializeToStringUnsafe(std::span<wchar_t> output, std::size_t max_character = std::numeric_limits<std::size_t>::max()) { return {false }; }
		static std::tuple<BomT, std::size_t> TryGetBom(std::span<std::byte const> input);

		template<typename Char, typename Trai, typename Allo>
		static EncodeInfo SerializeToBomAndString(std::span<std::byte const> input, std::basic_string<Char, Trai, Allo>& output, std::size_t max_character = std::numeric_limits<std::size_t>::max())
		{
			StringSerializer ser{input};
			ser.SerializeAndSetBom();
			auto re = ser.DetectCharacterCount<Char>(max_character);
			if(re)
			{
				auto old_size = output.size();
				output.resize(old_size + re.TargetSpace);
				auto out = std::span(output).subspan(old_size);
				return ser.SerializeToStringUnsafe(out, re.CharacterCount);
			}
			return re;
		}

	protected:

		BomT bom = BomT::NoBom;
		std::span<std::byte const> reference;
	};

	std::tuple<BomT, std::size_t> TryGetBomFromBinary(std::span<std::byte const> input);
}