module;

#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#undef max
#endif

module PotatoEncode;

namespace Potato::Encode
{
#ifdef _WIN32
	EncodeInfo StrEncoder<char, wchar_t>::Encode(std::span<char const> source, std::span<wchar_t> target, EncodeOption option)
	{
		if (option.predict)
		{
			int nWideLen = MultiByteToWideChar(CP_ACP, 0, source.data(), source.size(), NULL, 0);
			if (nWideLen > 0)
			{
				return EncodeInfo{
					true,
					0,
					source.size(),
					static_cast<std::size_t>(nWideLen)
				};
			}
			else {
				return EncodeInfo{
					false,
					0,
					source.size(),
					0
				};
			}
		}
		else {
			int nWideLen = MultiByteToWideChar(CP_ACP, 0, source.data(), source.size(), target.data(), target.size());
			if (nWideLen > 0)
			{
				return EncodeInfo{
					true,
					0,
					source.size(),
					static_cast<std::size_t>(nWideLen)
				};
			}
			else {
				return EncodeInfo{
					false,
					0,
					source.size(),
					0
				};
			}
		}
	}

	EncodeInfo StrEncoder<wchar_t, char>::Encode(std::span<wchar_t const> source, std::span<char> target, EncodeOption option)
	{
		if (option.predict)
		{
			int nWideLen = WideCharToMultiByte(CP_ACP, 0, source.data(), source.size(), NULL, 0, NULL, NULL);

			if (nWideLen > 0)
			{
				return EncodeInfo{
					true,
					0,
					source.size(),
					static_cast<std::size_t>(nWideLen)
				};
			}
			else {
				return EncodeInfo{
					false,
					0,
					source.size(),
					0
				};
			}
		}
		else {
			int nWideLen = WideCharToMultiByte(CP_ACP, 0, source.data(), source.size(), target.data(), target.size(), NULL, NULL);
			if (nWideLen > 0)
			{
				bool GoodString = true;
				std::size_t CharacterCount = 0;
				std::size_t SourceSpace = 0;
				std::size_t TargetSpace = 0;
				return EncodeInfo{
					true,
					0,
					source.size(),
					static_cast<std::size_t>(nWideLen)
				};
			}
			else {
				return EncodeInfo{
					false,
					0,
					source.size(),
					0
				};
			}
		}
	}
#endif
}