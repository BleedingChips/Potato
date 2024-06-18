module;

#include <cassert>

export module PotatoEncode;

import std;
import PotatoMisc;

export namespace Potato::Encode
{

	struct EncodeInfo
	{
		bool GoodString = true;
		std::size_t CharacterCount = 0;
		std::size_t SourceSpace = 0;
		std::size_t TargetSpace = 0;
		explicit operator bool() const { return GoodString; }
	};

	template<typename ST, typename TT>
	struct CharEncoder
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
	struct CharEncoder<char32_t, char16_t>
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
	struct CharEncoder<char32_t, char8_t>
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
	struct CharEncoder<char32_t, char32_t>
	{
		using SourceT = char32_t;
		using TargetT = char32_t;
		static constexpr std::size_t MaxSourceBufferLength = 1;
		static constexpr std::size_t MaxTargetBufferLength = 1;
		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source){ return {true, 1, 1, 1}; }
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source);
		static EncodeInfo EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target){ Target[0] = Source[0]; return { true, 1, 1, 1}; }
	};

	template<>
	struct CharEncoder<char16_t, char32_t>
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
	struct CharEncoder<char16_t, char8_t>
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
	struct CharEncoder<char8_t, char32_t>
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
	struct CharEncoder<char8_t, char16_t>
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
	struct CharEncoder<UnicodeT, wchar_t>
	{
		using SourceT = UnicodeT;
		using TargetT = wchar_t;
	private:
		using Wrapper = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), CharEncoder<UnicodeT, char16_t>, CharEncoder<UnicodeT, char32_t>>;
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
	struct CharEncoder<wchar_t, UnicodeT>
	{
		using SourceT = wchar_t;
		using TargetT = UnicodeT;
	private:
		using Wrapper = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), CharEncoder<char16_t, UnicodeT>, CharEncoder<char32_t, UnicodeT>>;
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
	struct CharEncoder<UnicodeT, UnicodeT>
	{
		using SourceT = UnicodeT;
		using TargetT = UnicodeT;

		static constexpr std::size_t MaxSourceBufferLength = CharEncoder<UnicodeT, char32_t>::MaxSourceBufferLength;
		static constexpr std::size_t MaxTargetBufferLength = CharEncoder<char32_t, UnicodeT>::MaxTargetBufferLength;

		static EncodeInfo RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
		{
			auto Re = CharEncoder<UnicodeT, char32_t>::RequireSpaceOnceUnSafe(Source);
			Re.TargetSpace = Re.SourceSpace;
			return Re;
		}
		static EncodeInfo RequireSpaceOnce(std::span<SourceT const> Source)
		{
			auto Re = CharEncoder<UnicodeT, char32_t>::RequireSpaceOnce(Source);
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
	struct CharEncoder<wchar_t, wchar_t>
	{
		using SourceT = wchar_t;
		using TargetT = wchar_t;
	private:
		using Wrapper = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), CharEncoder<char16_t, char16_t>, CharEncoder<char32_t, char32_t>>;
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

	template<typename SourceT, typename TargetT>
	struct StrEncoder
	{
		static EncodeInfo RequireSpaceUnSafe(std::span<SourceT const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max()) {
			EncodeInfo Result;
			while (!Source.empty() && Result.CharacterCount < MaxCharacter)
			{
				auto Res = CharEncoder<SourceT, TargetT>::RequireSpaceOnceUnSafe(Source);
				assert(Res.SourceSpace != 0);
				Result.CharacterCount += 1;
				Result.TargetSpace += Res.TargetSpace;
				Result.SourceSpace += Res.SourceSpace;
				Source = Source.subspan(Res.SourceSpace);
			}
			return Result;
		}

		static EncodeInfo RequireSpace(std::span<SourceT const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo Result;
			while (!Source.empty() && Result.CharacterCount < MaxCharacter)
			{
				auto Res = CharEncoder<SourceT, TargetT>::RequireSpaceOnce(Source);
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

		static EncodeInfo RequireSpaceLineUnsafe(std::span<SourceT const> Source, bool KeepLine = true)
		{
			EncodeInfo Result;
			bool MeetR = false;
			while (!Source.empty())
			{
				EncodeInfo Res = CharEncoder<SourceT, TargetT>::RequireSpaceOnceUnSafe(Source);
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

		static EncodeInfo EncodeUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max()) {
			EncodeInfo Result;
			while (!Source.empty() && Result.CharacterCount < MaxCharacter)
			{
				auto Res = CharEncoder<SourceT, TargetT>::EncodeOnceUnSafe(Source, Target);
				Result.CharacterCount += 1;
				Result.TargetSpace += Res.TargetSpace;
				Result.SourceSpace += Res.SourceSpace;
				Source = Source.subspan(Res.SourceSpace);
				Target = Target.subspan(Res.TargetSpace);
			}
			return Result;
		}

		template<typename CharTraits, typename AllocatorT = std::allocator<TargetT>>
		static auto EncodeToString(std::basic_string_view<SourceT, CharTraits> Source, AllocatorT Allocator = {}) -> std::optional<std::basic_string<TargetT, std::char_traits<TargetT>, AllocatorT>>
		{
			auto Info = RequireSpaceUnSafe(Source);
			if (Info)
			{
				std::basic_string<TargetT, std::char_traits<TargetT>, AllocatorT> Result{ std::move(Allocator) };
				Result.resize(Info.TargetSpace);
				EncodeUnSafe(Source, Result);
				return Result;
			}
			return {};
		}
	};

	template<>
	struct StrEncoder<char, wchar_t>
	{
		static EncodeInfo RequireSpaceUnSafe(std::span<char const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max());
		static EncodeInfo RequireSpace(std::span<char const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max())
		{
			return RequireSpaceUnSafe(Source, MaxCharacter);
		}

		static EncodeInfo EncodeUnSafe(std::span<char const> Source, std::span<wchar_t> Target, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max());

		template<typename CharTraits, typename AllocatorT = std::allocator<wchar_t>>
		static auto EncodeToString(std::basic_string_view<char, CharTraits> Source, AllocatorT Allocator = {}) -> std::optional<std::basic_string<wchar_t, std::char_traits<wchar_t>, AllocatorT>>
		{
			auto Info = RequireSpaceUnSafe(Source);
			if (Info)
			{
				std::basic_string<wchar_t, std::char_traits<wchar_t>, AllocatorT> Result{ std::move(Allocator) };
				Result.resize(Info.TargetSpace);
				EncodeUnSafe(Source, Result);
				return Result;
			}
			return {};
		}
	};

	template<>
	struct StrEncoder<wchar_t, char>
	{
		static EncodeInfo RequireSpaceUnSafe(std::span<wchar_t const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max());
		static EncodeInfo RequireSpace(std::span<wchar_t const> Source, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max())
		{
			return RequireSpaceUnSafe(Source, MaxCharacter);
		}

		static EncodeInfo EncodeUnSafe(std::span<wchar_t const> Source, std::span<char> Target, std::size_t MaxCharacter = std::numeric_limits<std::size_t>::max());

		template<typename CharTraits, typename AllocatorT = std::allocator<char>>
		static auto EncodeToString(std::basic_string_view<wchar_t, CharTraits> Source, AllocatorT Allocator = {}) -> std::optional<std::basic_string<char, std::char_traits<char>, AllocatorT>>
		{
			auto Info = RequireSpaceUnSafe(Source);
			if (Info)
			{
				std::basic_string<char, std::char_traits<char>, AllocatorT> Result{ std::move(Allocator) };
				Result.resize(Info.TargetSpace);
				EncodeUnSafe(Source, Result);
				return Result;
			}
			return {};
		}
	};
}
