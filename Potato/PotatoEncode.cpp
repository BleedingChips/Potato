module;

#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#undef max
#endif

module PotatoEncode;

namespace Potato::Encode
{

	EncodeInfo CharEncoder<char32_t, char16_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		EncodeInfo Info;
		assert(!Source.empty());
		if (Source[0] <= 0x10000)
		{
			Info.TargetSpace = 1;
		}else
			Info.TargetSpace = 2;
		Info.SourceSpace = 1;
		Info.CharacterCount = 1;
		return Info;
	}

	EncodeInfo CharEncoder<char32_t, char16_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		EncodeInfo Info;
		if (!Source.empty())
		{
			auto Ite = Source[0];
			if (Ite <= 0x010000)
				Info.TargetSpace = 1;
			else if (Ite < 0x110000)
				Info.TargetSpace = 2;
			else
				return Info;
			Info.SourceSpace = 1;
			Info.CharacterCount = 1;
		}
		return Info;
	}

	EncodeInfo CharEncoder<char32_t, char16_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		auto Info = RequireSpaceOnceUnSafe(Source);
		switch (Info.TargetSpace)
		{
			case 1:
				assert(Target.size() >= 1);
				Target[0] = static_cast<char16_t>(Source[0]);
			break;
			case 2:
			{
				assert(Target.size() >= 2);
				auto Tem = Source[0];
				assert(Tem >= 0x10000);
				Tem -= 0x10000;
				Target[0] = (0xd800 | (Tem >> 10));
				Target[1] = (0xdc00 | (Tem & 0x3FF));
			}
			break;
			default:
				assert(false);
			break;
		}
		return Info;
	}

	EncodeInfo CharEncoder<char32_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		EncodeInfo Info;
		assert(!Source.empty());
		auto Ite = Source[0];
		if (Ite <= 0x7F)
			Info.TargetSpace = 1;
		else if (Ite <= 0x7FF)
			Info.TargetSpace = 2;
		else if (Ite <= 0xFFFF)
			Info.TargetSpace = 3;
		else
			Info.TargetSpace = 4;
		Info.SourceSpace = 1;
		Info.CharacterCount = 1;
		return Info;
	}

	EncodeInfo CharEncoder<char32_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		EncodeInfo Info;
		if (!Source.empty())
		{
			auto Ite = Source[0];
			if (Ite <= 0x7F)
				Info.TargetSpace = 1;
			else if (Ite <= 0x7FF)
				Info.TargetSpace = 2;
			else if (Ite <= 0xFFFF)
				Info.TargetSpace = 3;
			else if (Ite < 0x110000)
				Info.TargetSpace = 4;
			else
				return {false, 0, 0, 0};
			Info.SourceSpace = 1;
			Info.CharacterCount = 1;
		}
		return Info;
	}

	EncodeInfo CharEncoder<char32_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if (!Source.empty())
		{
			if(Source[0] < 0x110000)
				return {true, 1, 1, 1};
			else
				return {false, 0, 0, 0};
		}
		return {true, 0, 0, 0};
	}

	EncodeInfo CharEncoder<char32_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		auto Info = RequireSpaceOnceUnSafe(Source);
		auto Cur = Source[0];
		switch (Info.TargetSpace)
		{
		case 1:
			assert(Target.size() >= 1);
			Target[0] = static_cast<char8_t>(Cur);
		break;
		case 2:
			assert(Target.size() >= 2);
			Target[0] = 0xC0 | static_cast<char8_t>((Cur & 0x07C0) >> 6);
			Target[1] = 0x80 | static_cast<char8_t>((Cur & 0x3F));
		break;
		case 3:
			assert(Target.size() >= 3);
			Target[0] = 0xE0 | static_cast<char8_t>((Cur & 0xF000) >> 12);
			Target[1] = 0x80 | static_cast<char8_t>((Cur & 0xFC0) >> 6);
			Target[2] = 0x80 | static_cast<char8_t>((Cur & 0x3F));
		break;
		case 4:
			assert(Target.size() >= 4);
			Target[0] = 0xF0 | static_cast<char>((Cur & 0x1C0000) >> 18);
			Target[1] = 0x80 | static_cast<char>((Cur & 0x3F000) >> 12);
			Target[2] = 0x80 | static_cast<char>((Cur & 0xFC0) >> 6);
			Target[3] = 0x80 | static_cast<char>((Cur & 0x3F));
		break;
		default:
		assert(false);
		break;
		}
		return Info;
	}

	EncodeInfo CharEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		assert(!Source.empty());
		assert(Source.size() >= 1);
		auto Cur = Source[0];
		if ((Cur >> 10) == 0x36 && Source.size() >= 2 && (Source[1] >> 10) == 0x37)
		{
			return {true, 1, 2, 1};
		}else
			return { true, 1, 1, 1};
	}

	EncodeInfo CharEncoder<char16_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if(!Source.empty())
			return RequireSpaceOnceUnSafe(Source);
		else
			return {true, 0, 0, 0};
	}

	EncodeInfo CharEncoder<char16_t, char32_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		assert(!Source.empty());
		assert(Target.size() >= 1);
		auto Info = RequireSpaceOnceUnSafe(Source);
		switch (Info.SourceSpace)
		{
		case 2:
		{
			Target[0] = (static_cast<char32_t>(Source[0] & 0x3FF) << 10) + (Source[1] & 0x3FF) + 0x10000;
			return { true, 1, 2, 1 };
		}
		default:
			Target[0] = Source[0];
			return { true, 1, 1, 1 };
		}
	}

	EncodeInfo CharEncoder<char16_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		auto Re = CharEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(Source);
		assert(Re.SourceSpace != 0);
		if(Re.SourceSpace == 2)
			return {true, 1, 2, 4};
		else {
			auto Cur = Source[0];
			if(Cur <= 0x7f) return {true, 1, 1, 1};
			else if(Cur <= 0x7ff) return { true, 1, 1, 2};
			else if(Cur <= 0xffff) return { true, 1, 1, 3};
			else {
				assert(false);
				return {false, 0, 0, 0};
			}
		}
	}

	EncodeInfo CharEncoder<char16_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if (!Source.empty())
		{
			auto Re = CharEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(Source);
			assert(Re.SourceSpace != 0);
			if (Re.SourceSpace == 2)
				return { true, 1, 2, 4 };
			else {
				auto Cur = Source[0];
				if (Cur <= 0x7f) return { true, 1, 1, 1 };
				else if (Cur <= 0x7ff) return { true, 1, 1, 2 };
				else if (Cur <= 0xffff) return { true, 1, 1, 3 };
				else {
					return {false, 0, 0, 0};
				}
			}
		}
		else {
			return {true, 0, 0, 0};
		}
	}

	EncodeInfo CharEncoder<char16_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		std::array<char32_t, 1> Tem;
		auto Re1 = CharEncoder<char16_t, char32_t>::EncodeOnceUnSafe(Source, Tem);
		auto Re2 = CharEncoder<char32_t, char8_t>::EncodeOnceUnSafe(Tem, Target);
		return {true, 1, Re1.SourceSpace, Re2.TargetSpace};
	}

	EncodeInfo CharEncoder<char8_t, char16_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		auto Re = CharEncoder<char8_t, char32_t>::RequireSpaceOnceUnSafe(Source);
		if(Re.SourceSpace == 4)
			Re.TargetSpace = 2;
		return Re;
	}

	EncodeInfo CharEncoder<char8_t, char16_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		auto Re = CharEncoder<char8_t, char32_t>::RequireSpaceOnce(Source);
		if(Re.SourceSpace == 4)
			Re.TargetSpace = 2;
		return Re;
	}

	EncodeInfo CharEncoder<char8_t, char16_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		std::array<char32_t, 1> Tem;
		auto Re1 = CharEncoder<char8_t, char32_t>::EncodeOnceUnSafe(Source, Tem);
		auto Re2 = CharEncoder<char32_t, char16_t>::EncodeOnceUnSafe(Tem, Target);
		return { true, 1, Re1.SourceSpace, Re2.TargetSpace };
	}

	EncodeInfo CharEncoder<char8_t, char32_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		assert(!Source.empty());
		auto Cur = Source[0];
		if ((Cur & 0x80) == 0)
			return {true, 1, 1, 1};
		else if((Cur & 0xE0) == 0xC0)
			return { true, 1,2, 1};
		else if((Cur & 0xF0) == 0xE0)
			return { true, 1,3, 1};
		else
			return { true, 1, 4, 1};
	}

	EncodeInfo CharEncoder<char8_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if (!Source.empty())
		{
			auto Cur = Source[0];
			std::size_t Count = 0;
			if ((Cur & 0x80) == 0)
				Count = 1;
			else if ((Cur & 0xE0) == 0xC0)
				Count = 2;
			else if ((Cur & 0xF0) == 0xE0)
				Count = 3;
			else if((Cur & 0xF8) == 0xF0)
				Count = 4;
			if (Count <= Source.size() && Count != 0)
			{
				bool Succeed = true;
				for (std::size_t Index = 1; Index < Count; ++Index)
				{
					auto Ite = Source[Index];
					if ((Ite & 0x80) != 0x80)
					{
						Succeed = false;
						break;
					}
				}
				if(Succeed)
					return {true, 1, Count, 1};
				else
					return {false, 0, 0, 0};
			}
		}
		return {true, 0, 0, 0};
	}

	EncodeInfo CharEncoder<char8_t, char32_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		auto Info = RequireSpaceOnceUnSafe(Source);
		assert(!Target.empty());
		assert(Source.size() >= Info.SourceSpace);
		switch (Info.SourceSpace)
		{
		case 1:
			Target[0] = Source[0];
			break;
		case 2:
			Target[0] = (static_cast<char32_t>(Source[0] & 0x1F) << 6) | static_cast<char32_t>(Source[1] & 0x3F);
			break;
		case 3:
			Target[0] = (static_cast<char32_t>(Source[0] & 0x0F) << 12) | (static_cast<char32_t>(Source[1] & 0x3F) << 6)
				| static_cast<char32_t>(Source[2] & 0x3F);
			break;
		case 4:
			Target[0] = (static_cast<char32_t>(Source[0] & 0x07) << 18) | (static_cast<char32_t>(Source[1] & 0x3F) << 12)
				| (static_cast<char32_t>(Source[2] & 0x3F) << 6) | static_cast<char32_t>(Source[3] & 0x3F);
			break;
		default:
			assert(false);
		}
		return Info;
	}

#ifdef _WIN32
	EncodeInfo StrEncoder<char, wchar_t>::RequireSpaceUnSafe(std::span<char const> Source, std::size_t MaxCharacter)
	{
		int nWideLen = MultiByteToWideChar(CP_ACP, 0, Source.data(), Source.size(), NULL, 0);
		if (nWideLen > 0)
		{
			bool GoodString = true;
			std::size_t CharacterCount = 0;
			std::size_t SourceSpace = 0;
			std::size_t TargetSpace = 0;
			return EncodeInfo{
				true,
				0,
				Source.size(),
				static_cast<std::size_t>(nWideLen)
			};
		}
		else {
			return EncodeInfo{
				false,
				0,
				Source.size(),
				0
			};
		}
	}

	EncodeInfo StrEncoder<char, wchar_t>::RequireSpace(std::span<char const> Source, std::size_t MaxCharacter)
	{
		return RequireSpaceUnSafe(Source, MaxCharacter);
	}

	EncodeInfo StrEncoder<char, wchar_t>::EncodeUnSafe(std::span<char const> Source, std::span<wchar_t> Target, std::size_t MaxCharacter)
	{
		int nWideLen = MultiByteToWideChar(CP_ACP, 0, Source.data(), Source.size(), Target.data(), Target.size());
		if (nWideLen > 0)
		{
			bool GoodString = true;
			std::size_t CharacterCount = 0;
			std::size_t SourceSpace = 0;
			std::size_t TargetSpace = 0;
			return EncodeInfo{
				true,
				0,
				Source.size(),
				static_cast<std::size_t>(nWideLen)
			};
		}
		else {
			return EncodeInfo{
				false,
				0,
				Source.size(),
				0
			};
		}
	}

	EncodeInfo StrEncoder<wchar_t, char>::RequireSpaceUnSafe(std::span<wchar_t const> Source, std::size_t MaxCharacter)
	{
		int nWideLen = WideCharToMultiByte(CP_ACP, 0, Source.data(), Source.size(), NULL, 0, NULL, NULL);

		if (nWideLen > 0)
		{
			bool GoodString = true;
			std::size_t CharacterCount = 0;
			std::size_t SourceSpace = 0;
			std::size_t TargetSpace = 0;
			return EncodeInfo{
				true,
				0,
				Source.size(),
				static_cast<std::size_t>(nWideLen)
			};
		}
		else {
			return EncodeInfo{
				false,
				0,
				Source.size(),
				0
			};
		}
	}

	EncodeInfo StrEncoder<wchar_t, char>::RequireSpace(std::span<wchar_t const> Source, std::size_t MaxCharacter)
	{
		return RequireSpaceUnSafe(Source, MaxCharacter);
	}

	EncodeInfo StrEncoder<wchar_t, char>::EncodeUnSafe(std::span<wchar_t const> Source, std::span<char> Target, std::size_t MaxCharacter)
	{
		
		int nWideLen = WideCharToMultiByte(CP_ACP, 0, Source.data(), Source.size(), Target.data(), Target.size(), NULL, NULL);
		if (nWideLen > 0)
		{
			bool GoodString = true;
			std::size_t CharacterCount = 0;
			std::size_t SourceSpace = 0;
			std::size_t TargetSpace = 0;
			return EncodeInfo{
				true,
				0,
				Source.size(),
				static_cast<std::size_t>(nWideLen)
			};
		}
		else {
			return EncodeInfo{
				false,
				0,
				Source.size(),
				0
			};
		}
	}

#else
	EncodeInfo StrEncoder<char, wchar_t>::RequireSpaceUnSafe(std::span<char const> Source, std::size_t MaxCharacter)
	{
		return StrEncoder<char8_t, wchar_t>::RequireSpaceUnSafe(
			std::span<char8_t const>{ reinterpret_cast<char8_t const*>(Source.data()), Source.size() }, MaxCharacter
		);
	}

	EncodeInfo StrEncoder<char, wchar_t>::RequireSpace(std::span<char const> Source, std::size_t MaxCharacter)
	{
		return StrEncoder<char8_t, wchar_t>::RequireSpace(
			std::span<char8_t const>{ reinterpret_cast<char8_t const*>(Source.data()), Source.size() }, MaxCharacter
		);
	}

	EncodeInfo StrEncoder<char, wchar_t>::EncodeUnSafe(std::span<char const> Source, std::span<wchar_t> Target, std::size_t MaxCharacter)) {
		return StrEncoder<char8_t, wchar_t>::EncodeUnSafe(
			std::span<char8_t const>{ reinterpret_cast<char8_t const*>(Source.data()), Source.size() }, Target, MaxCharacter
		);
	}

	EncodeInfo StrEncoder<wchar_t, char>::RequireSpaceUnSafe(std::span<wchar_t const> Source, std::size_t MaxCharacter)
	{
		return StrEncoder<wchar_t, char8_t>::RequireSpaceUnSafe(
			std::span<wchar_t const>{ reinterpret_cast<wchar_t const*>(Source.data()), Source.size() }, MaxCharacter
		);
	}

	EncodeInfo StrEncoder<char, wchar_t>::RequireSpace(std::span<wchar_t const> Source, std::size_t MaxCharacter)
	{
		return StrEncoder<char8_t, wchar_t>::RequireSpace(
			std::span<wchar_t const>{ reinterpret_cast<wchar_t const*>(Source.data()), Source.size() }, MaxCharacter
		);
	}

	EncodeInfo StrEncoder<wchar_t, char>::EncodeUnSafe(std::span<wchar_t const> Source, std::span<char> Target, std::size_t MaxCharacter)) {
		return StrEncoder<char8_t, wchar_t>::EncodeUnSafe(
			std::span<wchar_t const>{ reinterpret_cast<wchar_t const*>(Source.data()), Source.size() }, Target, MaxCharacter
		);
	}
#endif
}