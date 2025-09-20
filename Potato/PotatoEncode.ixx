module;

#include <cassert>

export module PotatoEncode;

import std;
import PotatoMisc;
import PotatoTMP;

export namespace Potato::Encode
{

	struct EncodeInfo
	{
		bool good = true;
		std::size_t character_count = 0;
		std::size_t source_space = 0;
		std::size_t target_space = 0;
		explicit operator bool() const { return good; }
	};

	struct EncodeOption
	{
		bool untrusted = false;
		bool predict = false;
		std::size_t max_character = std::numeric_limits<std::size_t>::max();
	};

	template<typename Source>
	struct CharEncoder
	{
		constexpr std::optional<std::size_t> GetCharacterSize(std::span<Source const> source, bool untrusted = false);
	};

	template<>
	struct CharEncoder<char8_t>
	{
		constexpr std::optional<std::size_t> GetCharacterSize(std::span<char8_t const> source, bool untrusted = false)
		{
			if (!source.empty())
			{
				auto cur = source[0];
				if (!untrusted)
				{
					if ((cur & 0x80) == 0) return 1;
					else if ((cur & 0xE0) == 0xC0) return 2;
					else if ((cur & 0xF0) == 0xE0) return 3;
					else return 4;
				}
				else {
					std::size_t count = 0;
					if ((cur & 0x80) == 0)
						count = 1;
					else if ((cur & 0xE0) == 0xC0)
						count = 2;
					else if ((cur & 0xF0) == 0xE0)
						count = 3;
					else if ((cur & 0xF8) == 0xF0)
						count = 4;
					if (count <= source.size() && count != 0)
					{
						bool Succeed = true;
						for (std::size_t index = 1; index < count; ++index)
						{
							auto Ite = source[index];
							if ((Ite & 0x80) != 0x80)
							{
								Succeed = false;
								break;
							}
						}
						if (Succeed)
							return count;
					}
					return {};
				}
			}
			return 0;
		}
		constexpr char32_t EncodeTo(std::size_t character_space, std::span<char8_t const> source)
		{
			switch (character_space)
			{
			case 1:
				return source[0];
			case 2:
				return (static_cast<char32_t>(source[0] & 0x1F) << 6) | static_cast<char32_t>(source[1] & 0x3F);
			case 3:
				return (static_cast<char32_t>(source[0] & 0x0F) << 12) | (static_cast<char32_t>(source[1] & 0x3F) << 6)
					| static_cast<char32_t>(source[2] & 0x3F);
			default:
				return (static_cast<char32_t>(source[0] & 0x07) << 18) | (static_cast<char32_t>(source[1] & 0x3F) << 12)
					| (static_cast<char32_t>(source[2] & 0x3F) << 6) | static_cast<char32_t>(source[3] & 0x3F);
				break;
			}
		}
		constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			if (source <= 0x7F)
				return 1;
			else if (source <= 0x7FF)
				return 2;
			else if (source <= 0xFFFF)
				return 3;
			else
				return 4;
		}
		constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char8_t> target)
		{
			if (character_size <= target.size())
			{
				switch (character_size)
				{
				case 1:
					target[0] = static_cast<char8_t>(source);
					break;
				case 2:
					target[0] = 0xC0 | static_cast<char8_t>((source & 0x07C0) >> 6);
					target[1] = 0x80 | static_cast<char8_t>((source & 0x3F));
					break;
				case 3:
					target[0] = 0xE0 | static_cast<char8_t>((source & 0xF000) >> 12);
					target[1] = 0x80 | static_cast<char8_t>((source & 0xFC0) >> 6);
					target[2] = 0x80 | static_cast<char8_t>((source & 0x3F));
					break;
				default:
					target[0] = 0xF0 | static_cast<char>((source & 0x1C0000) >> 18);
					target[1] = 0x80 | static_cast<char>((source & 0x3F000) >> 12);
					target[2] = 0x80 | static_cast<char>((source & 0xFC0) >> 6);
					target[3] = 0x80 | static_cast<char>((source & 0x3F));
					break;
				}
				return true;
			}
			return false;
		}
	};

	template<>
	struct CharEncoder<char16_t>
	{
		constexpr std::optional<std::size_t> GetCharacterSize(std::span<char16_t const> source, bool untrusted = false)
		{
			if (!source.empty())
			{
				if (!untrusted)
				{
					if (source[0] >= 0xD800 && source[0] < 0xE000)
						return 2;
					else
						return 1;
				}
				else {
					if (source[0] >= 0xD800 && source[0] < 0xE00)
					{
						if (source.size() >= 2 && source[1] >= 0xDC00 && source[1] < 0xE000)
						{
							return 2;
						}
					}
					else {
						return 1;
					}
				}
			}
			return std::nullopt;
		}
		constexpr char32_t EncodeTo(std::size_t character_space, std::span<char16_t const> source)
		{
			switch (character_space)
			{
			case 1:
				return source[0];
			default:
				return (static_cast<char32_t>(source[0] & 0x3FF) << 10) + static_cast<char32_t>(source[1] & 0x3FF) + 0x10000;
			}
		}
		
		constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			if (source <= 0x010000)
				return 1;
			else
				return 2;
		}

		constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char16_t> target)
		{
			if (character_size <= target.size())
			{
				switch (character_size)
				{
				case 1:
					target[0] = static_cast<char16_t>(source);
					break;
				default:
					source -= 0x10000;
					target[0] = static_cast<char16_t>(0xd800 | (source >> 10));
					target[1] = static_cast<char16_t>(0xdc00 | (source & 0x3FF));
					break;
				}
				return true;
			}
			return false;
		}
	};

	template<>
	struct CharEncoder<char32_t>
	{
		constexpr std::optional<std::size_t> GetCharacterSize(std::span<char32_t const> source, bool untrusted = false)
		{
			if (!source.empty())
			{
				if (!untrusted)
				{
					return 1;
				}
				else {
					if (source[0] <= 0x10FFFF)
					{
						return 1;
					}
				}
			}
			return std::nullopt;
		}
		constexpr char32_t EncodeTo(std::size_t character_space, std::span<char32_t const> source)
		{
			return source[0];
		}

		constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			return 1;
		}

		constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char32_t> target)
		{
			if (character_size <= target.size())
			{
				target[0] = source;
				return true;
			}
			return false;
		}
	};

	template<>
	struct CharEncoder<wchar_t> : 
		CharEncoder<
			std::conditional_t<
				sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t
			>
		>
	{
		static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));

		using WrapperType = std::conditional_t<
			sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t
		>;

		constexpr std::optional<std::size_t> GetCharacterSize(std::span<wchar_t const> source, bool untrusted = false)
		{
			return CharEncoder<WrapperType>::GetCharacterSize({reinterpret_cast<WrapperType const*>(source.data()), source.size()});
		}
		constexpr char32_t EncodeTo(std::size_t character_space, std::span<wchar_t const> source)
		{
			return CharEncoder<WrapperType>::EncodeTo(character_space, { reinterpret_cast<WrapperType const*>(source.data()), source.size() });
		}

		constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			return CharEncoder<WrapperType>::DecodeCharacterSize(source);
		}

		constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<wchar_t> target)
		{
			return CharEncoder<WrapperType>::DecodeFrom(source, character_size, { reinterpret_cast<WrapperType*>(target.data()), target.size() });
		}
	};

	template<>
	struct CharEncoder<char> : CharEncoder<char8_t>
	{
		constexpr std::optional<std::size_t> GetCharacterSize(std::span<char const> source, bool untrusted = false)
		{
			return CharEncoder<char8_t>::GetCharacterSize({ reinterpret_cast<char8_t const*>(source.data()), source.size() });
		}
		constexpr char32_t EncodeTo(std::size_t character_space, std::span<char const> source)
		{
			return CharEncoder<char8_t>::EncodeTo(character_space, { reinterpret_cast<char8_t const*>(source.data()), source.size() });
		}

		constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			return CharEncoder<char8_t>::DecodeCharacterSize(source);
		}

		constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char> target)
		{
			return CharEncoder<char8_t>::DecodeFrom(source, character_size, { reinterpret_cast<char8_t*>(target.data()), target.size() });
		}
	};

	template<typename Source, typename Target>
	struct StrEncoder
	{
		template<std::size_t S1>
		constexpr EncodeInfo Encode(std::span<Source const, S1> source, std::span<Target> target, EncodeOption option = {})
		{
			if constexpr (sizeof(Source) != sizeof(Target))
			{
				CharEncoder<Source> encoder_source;
				CharEncoder<Target> encoder_target;
				EncodeInfo info;
				while (
					option.max_character != 0
					&& (option.predict || info.target_space < target.size())
					&& info.source_space < source.size()
					)
				{
					auto ite_span = source.subspan(info.source_space);
					auto count = encoder_source.GetCharacterSize(ite_span, option.untrusted);
					if (count.has_value())
					{
						char32_t temp = encoder_source.EncodeTo(*count, ite_span);
						auto target_count = encoder_target.DecodeCharacterSize(temp);

						if (!option.predict)
						{
							if (!encoder_target.DecodeFrom(temp, target_count, target.subspan(info.target_space)))
							{
								break;
							}
						}
						info.target_space += target_count;
						info.source_space += *count;
						++info.character_count;
						--option.max_character;
					}
					else {
						info.good = false;
						break;
					}
				}
				return info;
			}
			else {
				EncodeInfo info;
				CharEncoder<Source> encoder;
				while (
					option.max_character != 0
					&& (option.predict || info.target_space < target.size())
					&& info.source_space < source.size()
					)
				{
					auto count = encoder.GetCharacterSize(source.subspan(info.source_space), option.untrusted);
					if (count.has_value())
					{
						if (option.predict || info.target_space + *count <= target.size())
						{
							info.character_count += 1;
							info.source_space += *count;
							info.target_space += *count;
							--option.max_character;
						}
						else {
							break;
						}
					}
					else {
						info.good = false;
						break;
					}
				}
				if (!option.predict && info.target_space <= target.size())
				{
					std::copy_n(source.data(), info.target_space, reinterpret_cast<Source*>(target.data()));
				}
				return info;
			}
		}
		constexpr EncodeInfo Encode(std::basic_string_view<Source> source, std::span<Target> target, EncodeOption option = {})
		{
			return this->Encode(std::span<Source const>(source.data(), source.size()), target, option);
		}
		template<std::size_t S1>
		constexpr EncodeInfo Encode(std::span<Source, S1> source, std::span<Target> target, EncodeOption option = {})
		{
			return this->Encode(std::span<Source const, S1>(source), target, option);
		}
	};
}
