#pragma once
#include <optional>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <fstream>
#include <filesystem>
#include <array>
#include "PotatoMisc.h"

namespace Potato::StrEncode
{

	struct EncodeInfo
	{
		std::size_t SourceSpace = 0;
		std::size_t TargetSpace = 0;
	};

	template<typename ST, typename TT>
	struct CoreEncoder
	{
		using SourceT = ST;
		using TargetT = TT;
		static constexpr std::size_t MaxSourceBufferLength = 1;
		static constexpr std::size_t MaxTargetBufferLength = 1;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<>
	struct CoreEncoder<char32_t, char16_t>
	{
		using SourceT = char32_t;
		using TargetT = char16_t;
		static constexpr std::size_t MaxSourceBufferLength = 1;
		static constexpr std::size_t MaxTargetBufferLength = 1;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<>
	struct CoreEncoder<char32_t, char8_t>
	{
		using SourceT = char32_t;
		using TargetT = char8_t;
		static constexpr std::size_t MaxSourceBufferLength = 1;
		static constexpr std::size_t MaxTargetBufferLength = 4;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<>
	struct CoreEncoder<char32_t, char32_t>
	{
		using SourceT = char32_t;
		using TargetT = char32_t;
		static constexpr std::size_t MaxSourceBufferLength = 1;
		static constexpr std::size_t MaxTargetBufferLength = 1;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source){ return {1, 1}; }
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target){ Target[0] = Source[0]; return {1, 1}; }
	};

	template<>
	struct CoreEncoder<char16_t, char32_t>
	{
		using SourceT = char16_t;
		using TargetT = char32_t;
		static constexpr std::size_t MaxSourceBufferLength = 2;
		static constexpr std::size_t MaxTargetBufferLength = 1;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<>
	struct CoreEncoder<char16_t, char8_t>
	{
		using SourceT = char16_t;
		using TargetT = char8_t;
		static constexpr std::size_t MaxSourceBufferLength = 2;
		static constexpr std::size_t MaxTargetBufferLength = 2;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<>
	struct CoreEncoder<char8_t, char32_t>
	{
		using SourceT = char8_t;
		using TargetT = char32_t;
		static constexpr std::size_t MaxSourceBufferLength = 4;
		static constexpr std::size_t MaxTargetBufferLength = 1;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<>
	struct CoreEncoder<char8_t, char16_t>
	{
		using SourceT = char8_t;
		using TargetT = char16_t;
		static constexpr std::size_t MaxSourceBufferLength = 4;
		static constexpr std::size_t MaxTargetBufferLength = 2;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source);
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target);
	};

	template<typename UnicodeT>
	struct CoreEncoder<UnicodeT, wchar_t>
	{
		using SourceT = UnicodeT;
		using TargetT = wchar_t;
	private:
		using Wrapper = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), CoreEncoder<UnicodeT, char16_t>, CoreEncoder<UnicodeT, char32_t>>;
		static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));

	public:

		static constexpr std::size_t MaxSourceBufferLength = Wrapper::MaxSourceBufferLength;
		static constexpr std::size_t MaxTargetBufferLength = Wrapper::MaxTargetBufferLength;

		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
		{
			return Wrapper::RequireSpaceOnceUnSafe(Source);
		}
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source)
		{
			return Wrapper::RequireSpaceOnceUnSafe(Source);
		}
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
		{
			return Wrapper::EncodeOnceUnSafe(Source, std::span(reinterpret_cast<typename Wrapper::TargetT*>(Target.data()), Target.size()));
		}
	};

	template<typename UnicodeT>
	struct CoreEncoder<wchar_t, UnicodeT>
	{
		using SourceT = wchar_t;
		using TargetT = UnicodeT;
	private:
		using Wrapper = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), CoreEncoder<char16_t, UnicodeT>, CoreEncoder<char32_t, UnicodeT>>;
		static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));
	public:

		static constexpr std::size_t MaxSourceBufferLength = Wrapper::MaxSourceBufferLength;
		static constexpr std::size_t MaxTargetBufferLength = Wrapper::MaxTargetBufferLength;

		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
		{
			return Wrapper::RequireSpaceOnceUnSafe(std::span(reinterpret_cast<typename Wrapper::SourceT const*>(Source.data()), Source.size()));
		}
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source)
		{
			return Wrapper::RequireSpaceOnceUnSafe(std::span(reinterpret_cast<typename Wrapper::SourceT const*>(Source.data()), Source.size()));
		}
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
		{
			return Wrapper::EncodeOnceUnSafe(std::span(reinterpret_cast<typename Wrapper::SourceT const*>(Source.data()), Source.size()), Target);
		}
	};

	template<typename UnicodeT>
	struct CoreEncoder<UnicodeT, UnicodeT>
	{
		using SourceT = UnicodeT;
		using TargetT = UnicodeT;

		static constexpr std::size_t MaxSourceBufferLength = CoreEncoder<UnicodeT, char32_t>::MaxSourceBufferLength;
		static constexpr std::size_t MaxTargetBufferLength = CoreEncoder<char32_t, UnicodeT>::MaxTargetBufferLength;

		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
		{
			auto Re = CoreEncoder<UnicodeT, char32_t>::RequireSpaceOnceUnSafe(Source);
			Re.TargetSpace = Re.SourceSpace;
			return Re;
		}
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source)
		{
			auto Re = CoreEncoder<UnicodeT, char32_t>::RequireSpaceOnce(Source);
			if(Re.TargetSpace != 0)
				Re.TargetSpace = Re.SourceSpace;
			return Re;
		}
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
		{
			auto EncodeInfo = RequireSpaceOnceUnSafe(Source);
			std::memcpy(Target.data(), Source.data(), EncodeInfo.TargetSpace * sizeof(TargetT));
			return EncodeInfo;
		}
	};

	template<>
	struct CoreEncoder<wchar_t, wchar_t>
	{
		using SourceT = wchar_t;
		using TargetT = wchar_t;
	private:
		using Wrapper = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), CoreEncoder<char16_t, char16_t>, CoreEncoder<char32_t, char32_t>>;
		static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));
	public:
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
		{
			return Wrapper::RequireSpaceOnceUnSafe(std::span(reinterpret_cast<typename Wrapper::SourceT const*>(Source.data()), Source.size()));
		}
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source)
		{
			return Wrapper::RequireSpaceOnceUnSafe(std::span(reinterpret_cast<typename Wrapper::SourceT const*>(Source.data()), Source.size()));
		}
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
		{
			return Wrapper::EncodeOnceUnSafe(
				std::span(reinterpret_cast<typename Wrapper::SourceT const*>(Source.data()), Source.size()), 
				std::span(reinterpret_cast<typename Wrapper::TargetT*>(Target.data()), Target.size())
			);
		}
	};

	template<typename UnicodeT>
	struct UnicodeLineSymbol
	{
		//static constexpr UnicodeT R;
		//static constexpr UnicodeT L;
	};

	template<>
	struct UnicodeLineSymbol<char32_t>
	{
		static constexpr char32_t R = U'\r';
		static constexpr char32_t N = U'\n';
	};

	template<>
	struct UnicodeLineSymbol<char16_t>
	{
		static constexpr char16_t R = u'\r';
		static constexpr char16_t N = u'\n';
	};

	template<>
	struct UnicodeLineSymbol<char8_t>
	{
		static constexpr char8_t R = u8'\r';
		static constexpr char8_t N = u8'\n';
	};

	template<>
	struct UnicodeLineSymbol<wchar_t>
	{
		static constexpr wchar_t R = L'\r';
		static constexpr wchar_t N = L'\n';
	};

	struct EncodeStrInfo
	{
		bool GoodString = true;
		std::size_t CharacterCount = 0;
		std::size_t TargetSpace = 0;
		std::size_t SourceSpace = 0;
		explicit operator bool() const { return GoodString; }
	};

	template<typename SourceT, typename TargetT>
	struct StrCodeEncoder
	{
		static EncodeStrInfo RequireSpaceUnSafe(std::span<SourceT const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max()) {
			EncodeStrInfo Result;
			while (!Source.empty() && Result.CharacterCount < MaxCharacter)
			{
				auto Res = CoreEncoder<SourceT, TargetT>::RequireSpaceOnceUnSafe(Source);
				assert(Res.SourceSpace != 0);
				Result.CharacterCount += 1;
				Result.TargetSpace += Res.TargetSpace;
				Result.SourceSpace += Res.SourceSpace;
				Source = Source.subspan(Res.SourceSpace);
			}
			return Result;
		}

		static EncodeStrInfo RequireSpace(std::span<SourceT const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max())
		{
			EncodeStrInfo Result;
			while (!Source.empty() && Result.CharacterCount < MaxCharacter)
			{
				auto Res = CoreEncoder<SourceT, TargetT>::RequireSpaceOnce(Source);
				if (Res.SourceSpace == 0) [[unlikely]]
				{
					Result.GoodString = false;
					return Result;
				}
				Result.CharacterCount += 1;
				Result.TargetSpace += Res.TargetSpace;
				Result.SourceSpace += Res.SourceSpace;
				Source = Source.subspan(Res.SourceSpace);
			}
			return Result;
		}

		static EncodeStrInfo RequireSpaceLineUnsafe(std::span<SourceT const> Source, bool KeepLine = true)
		{
			EncodeStrInfo Result;
			bool MeetR = false;
			while (!Source.empty())
			{
				EncodeInfo Res = CoreEncoder<SourceT, TargetT>::RequireSpaceOnceUnSafe(Source);
				Result.SourceSpace += Res.SourceSpace;
				if (Res.SourceSpace == 1)
				{
					auto Cur = Source[0];
					switch (Cur)
					{
					case UnicodeLineSymbol<SourceT>::R:
					{
						if (!MeetR)
							MeetR = true;
						else {
							Result.CharacterCount += 1;
							Result.TargetSpace += 1;
						}
						break;
					}
					case UnicodeLineSymbol<SourceT>::N:
					{
						if (MeetR && KeepLine)
						{
							Result.CharacterCount += 2;
							Result.TargetSpace += 2;
						}
						else if (KeepLine)
						{
							Result.CharacterCount += 1;
							Result.TargetSpace += 1;
						}
						return Result;
					}
					default:
						Result.CharacterCount += 1;
						Result.TargetSpace += Res.TargetSpace;
						break;
					}
				}
				else {
					Result.CharacterCount += 1;
					Result.TargetSpace += Res.TargetSpace;
				}
				Source = Source.subspan(Res.SourceSpace);
			}
			return Result;
		}

		static EncodeStrInfo EncodeUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max()) {
			EncodeStrInfo Result;
			while (!Source.empty() && Result.CharacterCount < MaxCharacter)
			{
				auto Res = CoreEncoder<SourceT, TargetT>::EncodeOnceUnSafe(Source, Target);
				Result.CharacterCount += 1;
				Result.TargetSpace += Res.TargetSpace;
				Result.SourceSpace += Res.SourceSpace;
				Source = Source.subspan(Res.SourceSpace);
				Target = Target.subspan(Res.TargetSpace);
			}
			return Result;
		}
	};


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
		case Potato::StrEncode::DocumenetBomT::UTF16LE:
		case Potato::StrEncode::DocumenetBomT::UTF32LE:
			return std::endian::native == std::endian::little;
			break;
		case Potato::StrEncode::DocumenetBomT::UTF16BE:
		case Potato::StrEncode::DocumenetBomT::UTF32BE:
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

	struct DocumenetReaderWrapper
	{
		
		DocumenetReaderWrapper(DocumenetReaderWrapper const&) = default;

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
		auto Read(Function&& Func, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max())->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeStrInfo>);

		template<typename UnicodeT, typename CharTrai, typename Allocator>
		ReadResult Read(std::basic_string<UnicodeT, CharTrai, Allocator>& Output, std::size_t MaxCharactor = std::numeric_limits<std::size_t>::max())
		{
			std::size_t OldSize = Output.size();
			return this->Read<UnicodeT>([&](EncodeStrInfo Input) -> std::optional<std::span<UnicodeT>>{
				Output.resize(OldSize + Input.TargetSpace);
				return std::span(Output).subspan(OldSize);
			});
		}

		template<typename UnicodeT, typename Function>
		auto ReadLine(Function&& Func, bool KeepLine = true)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeStrInfo>);
		

		template<typename UnicodeT, typename CharTrai, typename Allocator>
		ReadResult ReadLine(std::basic_string<UnicodeT, CharTrai, Allocator>& Output, bool KeepLine = true)
		{
			std::size_t OldSize = Output.size();
			return this->ReadLine<UnicodeT>([&](EncodeStrInfo Input) -> std::optional<std::span<UnicodeT>> {
				Output.resize(OldSize + Input.TargetSpace);
				return std::span(Output).subspan(OldSize);
				}, KeepLine);
		}

		DocumenetBomT GetBom() const { return Bom; }

		explicit operator bool() const { return Available.Count() != 0; }

	private:

		DocumenetReaderWrapper(DocumenetBomT Bom, std::span<std::byte> TemporaryBuffer) : Bom(Bom), DocumentSpan(TemporaryBuffer), Available{ 0, 0 } {}

		DocumenetBomT Bom = DocumenetBomT::NoBom;
		std::span<std::byte> DocumentSpan;
		Misc::IndexSpan<> Available;
		std::size_t TotalCharacter = 0;

		friend struct DocumentReader;
	};

	template<typename UnicodeT, typename Function>
	auto DocumenetReaderWrapper::Read(Function&& Func, std::size_t MaxCharacter) ->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeStrInfo>)
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
			EncodeStrInfo Info = StrCodeEncoder<char8_t, UnicodeT>::RequireSpaceUnSafe(Cur, MaxCharacter);
			std::optional<std::span<UnicodeT>> OutputBuffer = Func(Info);
			if(OutputBuffer.has_value())
				StrCodeEncoder<char8_t, UnicodeT>::EncodeUnSafe(Cur, *OutputBuffer, MaxCharacter);
			Available = Available.Sub(Info.SourceSpace);
			TotalCharacter -= Info.CharacterCount;
			return { StateT::Normal, Info .CharacterCount, Info.TargetSpace};
			break;
		}
		default:
			return {StateT::UnSupport, 0, 0};
			break;
		}
	}

	template<typename UnicodeT, typename Function>
	auto DocumenetReaderWrapper::ReadLine(Function&& Func, bool KeepLine)->ReadResult requires(std::is_invocable_r_v<std::optional<std::span<UnicodeT>>, Function, EncodeStrInfo>)
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
			EncodeStrInfo Info = StrCodeEncoder<char8_t, UnicodeT>::RequireSpaceLineUnsafe(Cur, KeepLine);
			std::optional<std::span<UnicodeT>> OutputBuffer = Func(Info);
			if(OutputBuffer.has_value())
				StrCodeEncoder<char8_t, UnicodeT>::EncodeUnSafe(Cur, *OutputBuffer, Info.CharacterCount);
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

		FlushResult Flush(DocumenetReaderWrapper& Reader);

		std::size_t RecalculateLastSize();

		DocumenetReaderWrapper CreateWrapper(std::span<std::byte> TemporaryBuffer) const { return DocumenetReaderWrapper{ Bom, TemporaryBuffer };}

	private:

		DocumenetBomT Bom = DocumenetBomT::NoBom;
		std::ifstream File;
		std::size_t TextOffset = 0;
	};

	struct DocumentEncoder
	{

		static EncodeStrInfo RequireSpaceUnsafe(std::u32string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeStrInfo RequireSpaceUnsafe(std::u16string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeStrInfo RequireSpaceUnsafe(std::u8string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeStrInfo RequireSpaceUnsafe(std::wstring_view Str, DocumenetBomT Bom, bool WriteBom = false);

		static EncodeStrInfo EncodeUnsafe(std::span<std::byte> Span, std::u32string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeStrInfo EncodeUnsafe(std::span<std::byte> Span, std::u16string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeStrInfo EncodeUnsafe(std::span<std::byte> Span, std::u8string_view Str, DocumenetBomT Bom, bool WriteBom = false);
		static EncodeStrInfo EncodeUnsafe(std::span<std::byte> Span, std::wstring_view Str, DocumenetBomT Bom, bool WriteBom = false);
	};

	struct DocumenetWriter
	{
		DocumenetWriter(std::filesystem::path Path, DocumenetBomT BomType = DocumenetBomT::NoBom);
		explicit operator bool() const { return File.is_open(); }
		EncodeStrInfo Write(std::u32string_view Str);
		EncodeStrInfo Write(std::u16string_view Str);
		EncodeStrInfo Write(std::u8string_view Str);
		EncodeStrInfo Write(std::wstring_view Str);
	private:
		std::ofstream File;
		DocumenetBomT BomType;
		std::vector<std::byte> TemporaryBuffer;
	};
}
