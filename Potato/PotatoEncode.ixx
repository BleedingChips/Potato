module;

#include <cassert>

export module PotatoEncode;

import std;

export namespace Potato::Encode
{

	struct UnicodeCode
	{
		using CodePointT = std::uint32_t;

		struct UTF8
		{
			using StorageT = std::uint8_t;
			static constexpr StorageT placemenet_character = std::numeric_limits<StorageT>::max();

			static constexpr std::size_t fast_detect_storage_size = 1;
			static constexpr std::size_t detect_storage_size = 4;
			static constexpr std::size_t max_storage_size = 4;

			static consteval std::optional<std::size_t> DetectStorageSizeFast(StorageT char_1)
			{
				if ((char_1 & 0x80) == 0) return 1;
				else if ((char_1 & 0xE0) == 0xC0) return 2;
				else if ((char_1 & 0xF0) == 0xE0) return 3;
				else if ((char_1 & 0xF8) == 0xF0) return 4;
				return std::nullopt;
			}
			static consteval std::optional<std::size_t> DetectStorageSize(StorageT char_1, StorageT char_2 = placemenet_character, StorageT char_3 = placemenet_character, StorageT char_4 = placemenet_character)
			{
				if ((char_1 & 0x80) == 0)
					return 1;
				else if ((char_1 & 0xE0) == 0xC0)
				{
					if ((char_2 & 0x80) != 0x80)
					{
						return 2;
					}
				}
				else if ((char_1 & 0xF0) == 0xE0)
				{
					if ((char_2 & 0x80) != 0x80 && (char_3 & 0x80) != 0x80)
						return 3;
				}
				else if ((char_1 & 0xF8) == 0xF0)
				{
					if ((char_2 & 0x80) != 0x80 && (char_3 & 0x80) != 0x80 && (char_4 & 0x80) != 0x80)
						return 4;
				}
				return std::nullopt;
			}

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1)
			{
				return static_cast<CodePointT>(char_1);
			}

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1, StorageT char_2)
			{
				return (static_cast<CodePointT>(char_1 & 0x1F) << 6) 
						| static_cast<CodePointT>(char_2 & 0x3F);
			}

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1, StorageT char_2, StorageT char_3)
			{
				return
					(static_cast<CodePointT>(char_1 & 0x0F) << 12)
					| (static_cast<CodePointT>(char_2 & 0x3F) << 6)
					| static_cast<CodePointT>(char_3 & 0x3F);
			}

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1, StorageT char_2, StorageT char_3, StorageT char_4)
			{
				return
					(static_cast<CodePointT>(char_1 & 0x07) << 18)
					| (static_cast<CodePointT>(char_2 & 0x3F) << 12)
					| (static_cast<CodePointT>(char_3 & 0x3F) << 6)
					| static_cast<CodePointT>(char_4 & 0x3F);
			}

			static constexpr std::size_t DetectStorageSizeFromCodePoint(CodePointT point)
			{
				if (point <= 0x7F)
					return 1;
				else if (point <= 0x7FF)
					return 2;
				else if (point <= 0xFFFF)
					return 3;
				else
					return 4;
			}

			static constexpr std::tuple<std::size_t, StorageT, StorageT, StorageT, StorageT> EncodeFromUnicodePoint(CodePointT point)
			{
				std::size_t count = DetectStorageSizeFromCodePoint(point);
				switch (count)
				{
				case 1:
					return std::make_tuple(
						count, 
						static_cast<StorageT>(point), 
						StorageT{ 0 }, 
						StorageT{ 0 }, 
						StorageT{ 0 }
					);
				case 2:
					return std::make_tuple(
						count, 
						0xC0 | static_cast<StorageT>((point & 0x07C0) >> 6),
						0x80 | static_cast<StorageT>((point & 0x3F)),
						StorageT{ 0 }, 
						StorageT{ 0 }
					);
				case 3:
					return std::make_tuple(
						count,
						0xE0 | static_cast<StorageT>((point & 0xF000) >> 12),
						0x80 | static_cast<StorageT>((point & 0xFC0) >> 6),
						0x80 | static_cast<StorageT>((point & 0x3F)),
						StorageT{ 0 }
					);
				default:
					return std::make_tuple(
						count,
						0xF0 | static_cast<StorageT>((point & 0x1C0000) >> 18),
						0x80 | static_cast<StorageT>((point & 0x3F000) >> 12),
						0x80 | static_cast<StorageT>((point & 0xFC0) >> 6),
						0x80 | static_cast<StorageT>((point & 0x3F))
					);
				}
			}
		};

		struct UTF16
		{
			using StorageT = std::uint16_t;
			static constexpr StorageT placemenet_character = std::numeric_limits<StorageT>::max();

			static constexpr std::size_t fast_detect_storage_size = 2;
			static constexpr std::size_t detect_storage_size = 2;
			static constexpr std::size_t max_storage_size = 2;

			static constexpr std::optional<std::size_t> DetectStorageSizeFast(StorageT char_1, StorageT char_2 = placemenet_character)
			{
				if (char_1 >= 0xD800 && char_1 < 0xE00)
				{
					if (char_2 >= 0xDC00 && char_2 < 0xE000)
					{
						return 2;
					}
					return 1;
				}
				return std::nullopt;
			}

			static constexpr std::optional<std::size_t> DetectStorageSize(StorageT char_1, StorageT char_2 = placemenet_character) { return DetectStorageSizeFast(char_1, char_2); }

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1)
			{
				return static_cast<CodePointT>(char_1);
			}

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1, StorageT char_2)
			{
				return (static_cast<CodePointT>(char_1 & 0x3FF) << 10)
					+ static_cast<CodePointT>(char_2 & 0x3FF) + 0x10000;
			}

			static constexpr std::size_t DetectStorageSizeFromCodePoint(CodePointT point)
			{
				if (point <= 0x010000)
					return 1;
				else
					return 2;
			}

			static constexpr std::tuple<std::size_t, StorageT, StorageT> EncodeFromUnicodePoint(CodePointT point)
			{
				std::size_t count = DetectStorageSizeFromCodePoint(point);
				switch (count)
				{
				case 1:
					return std::make_tuple(
						count,
						static_cast<StorageT>(point),
						StorageT{ 0 }
					);
				default:
					point -= 0x10000;
					return std::make_tuple(
						count,
						static_cast<StorageT>(0xd800 | (point >> 10)),
						static_cast<StorageT>(0xdc00 | (point & 0x3FF))
					);
				}
			}

		};

		struct UTF32
		{
			using StorageT = std::uint32_t;
			static constexpr StorageT placemenet_character = std::numeric_limits<StorageT>::max();

			static constexpr std::size_t fast_detect_storage_size = 1;
			static constexpr std::size_t detect_storage_size = 1;
			static constexpr std::size_t max_storage_size = 1;

			static constexpr std::optional<std::size_t> DetectStorageSizeFast(StorageT char_1)
			{
				if (char_1 < 0x10FFFF)
					return 1;
				return std::nullopt;
			}

			static constexpr std::optional<std::size_t> DetectStorageSize(StorageT char_1) { return DetectStorageSizeFast(char_1); }

			static constexpr CodePointT DecodeToUnicodePoint(StorageT char_1)
			{
				return static_cast<CodePointT>(char_1);
			}

			static constexpr std::size_t DetectStorageSizeFromCodePoint(CodePointT point)
			{
				return 1;
			}

			static constexpr std::tuple<std::size_t, StorageT> EncodeFromUnicodePoint(CodePointT point)
			{
				return std::make_tuple(1, static_cast<StorageT>(point));
			}
		};
	};

	struct EncodeInfo
	{
		std::size_t character_count = 0;
		std::size_t source_space = 0;
		std::size_t target_space = 0;
		bool is_good_string = true;
		operator bool() const { return is_good_string; }
	};

	template<typename RawStorageT>
	struct UnicodeWrapper;

	template<>
	struct UnicodeWrapper<char>
	{
		using NativeStorageT = char;
		using WrapperT = UnicodeCode::UTF8;
	};

	template<>
	struct UnicodeWrapper<wchar_t>
	{
		using NativeStorageT = wchar_t;
		using WrapperT = std::conditional_t<
			sizeof(wchar_t) == sizeof(char16_t),
			UnicodeCode::UTF16,
			std::conditional_t<
				sizeof(wchar_t) == sizeof(char16_t),
				UnicodeCode::UTF32,
				void
			>
		>;
	};

	template<>
	struct UnicodeWrapper<char16_t>
	{
		using NativeStorageT = char16_t;
		using WrapperT = UnicodeCode::UTF16;
	};

	template<>
	struct UnicodeWrapper<char32_t>
	{
		using NativeStorageT = char32_t;
		using WrapperT = UnicodeCode::UTF32;
	};

	template<typename FromCharT, typename ToCharT = void>
	struct UnicodeEncoder;

	template<std::size_t count, typename WrapperT>
	struct UnicodeEncoderHelper
	{
		template<typename NativeT>
		static UnicodeCode::CodePointT DecodeToUnicodePoint(std::size_t storage_count, std::span<NativeT const> source)
		{
			if (count == storage_count)
			{
				return[]<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeT const> source) {
					return WrapperT::DecodeToUnicodePoint(
						static_cast<WrapperT::StorageT>(source[i])...
					);
				}(std::make_index_sequence<count>());
			}
			else {
				return UnicodeEncoderHelper<count - 1, WrapperT>::DecodeToUnicodePoint(storage_count, source);
			}
		}
	};

	template<typename WrapperT>
	struct UnicodeEncoderHelper<1, WrapperT>
	{
		template<typename NativeT>
		UnicodeCode::CodePointT DecodeToUnicodePoint(std::size_t storage_count, std::span<NativeT const> source)
		{
			return WrapperT::DecodeToUnicodePoint(
				static_cast<WrapperT::StorageT>(source[0])
			);
		}
	};

	template<>
	struct UnicodeEncoder<wchar_t, void>
	{
		using NativeStorageT = UnicodeWrapper<wchar_t>::NativeStorageT;
		using WrapperT = UnicodeWrapper<wchar_t>::WrapperT;

		static constexpr EncodeInfo StatisticsFast(std::span<NativeStorageT const> source, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			auto iterator_string = source;
			while (!iterator_string.empty())
			{
				if (info.character_count >= max_unicode_point)
					return info;

				std::optional<std::size_t> storage_size = []<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeStorageT const> source) {
					return WrapperT::DetectStorageSizeFast(
						(((i == 0) || source.size() > i) ? static_cast<WrapperT::StorageT>(source[i]) : WrapperT::placemenet_character)...
					);
				}(std::make_index_sequence<WrapperT::fast_detect_storage_size>(), iterator_string);

				if (storage_size.has_value())
				{
					info.source_space += *storage_size;
					info.character_count += 1;
					info.target_space += 1;
					iterator_string = iterator_string.subspan(*storage_size);
				}
				else {
					info.is_good_string = false;
					return info;
				}
			}
			return info;
		}
		static constexpr EncodeInfo Statistics(std::span<NativeStorageT const> source, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			auto iterator_string = source;
			while (!iterator_string.empty())
			{
				if (info.character_count >= max_unicode_point)
					return info;

				std::optional<std::size_t> storage_size = []<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeStorageT const> source) {
					return WrapperT::DetectStorageSize(
						(((i == 0) || source.size() > i) ? static_cast<WrapperT::StorageT>(source[i]) : WrapperT::placemenet_character)...
					);
				}(std::make_index_sequence<WrapperT::detect_storage_size>(), iterator_string);

				if (storage_size.has_value())
				{
					info.source_space += *storage_size;
					info.character_count += 1;
					info.target_space += 1;
					iterator_string = iterator_string.subspan(*storage_size);
				}
				else {
					info.is_good_string = false;
					return info;
				}
			}
			return info;
		}
		
		static constexpr std::tuple<EncodeInfo, UnicodeCode::CodePointT> EncodeOnceFast(std::span<NativeStorageT const> source)
		{
			auto info = StatisticsFast(source, 1);
			if (info)
			{
				return UnicodeEncoderHelper<WrapperT::max_storage_size, WrapperT>::DecodeToUnicodePoint(info.source_space, source);
			}
			return {info, std::numeric_limits<UnicodeCode::CodePointT>::max()};
		}
	};

	template<typename CharT>
	struct CharEncoder
	{
		

		static constexpr std::optional<std::size_t> StaticsString


		static constexpr std::optional<std::size_t> GetCount(std::span<NativeStorageT const> string);


		static constexpr EncodeInfo GetEncodeInfo(std::span<NativeStorageT const> string)
		{
			
		}
	};



	/*
	template<typename Source>
	struct CharEncoder
	{
		static constexpr std::optional<std::size_t> GetCharacterSize(std::span<Source const> source, bool untrusted = false);
	};

	template<>
	struct CharEncoder<char8_t>
	{

		static constexpr std::optional<std::size_t> GetCharacterSize(std::span<char8_t const> source, bool untrusted = false)
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
		static constexpr char32_t EncodeTo(std::size_t character_space, std::span<char8_t const> source)
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

		static constexpr std::size_t DecodeCharacterSize(char32_t source)
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

		static constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char8_t> target)
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
		static constexpr std::optional<std::size_t> GetCharacterSize(std::span<char16_t const> source, bool untrusted = false)
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
		static constexpr char32_t EncodeTo(std::size_t character_space, std::span<char16_t const> source)
		{
			switch (character_space)
			{
			case 1:
				return source[0];
			default:
				return (static_cast<char32_t>(source[0] & 0x3FF) << 10) + static_cast<char32_t>(source[1] & 0x3FF) + 0x10000;
			}
		}
		
		static constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			if (source <= 0x010000)
				return 1;
			else
				return 2;
		}

		static constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char16_t> target)
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
		static constexpr std::optional<std::size_t> GetCharacterSize(std::span<char32_t const> source, bool untrusted = false)
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
		static constexpr char32_t EncodeTo(std::size_t character_space, std::span<char32_t const> source)
		{
			return source[0];
		}

		static constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			return 1;
		}

		static constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char32_t> target)
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

		static constexpr std::optional<std::size_t> GetCharacterSize(std::span<wchar_t const> source, bool untrusted = false)
		{
			return CharEncoder<WrapperType>::GetCharacterSize({reinterpret_cast<WrapperType const*>(source.data()), source.size()});
		}
		static constexpr char32_t EncodeTo(std::size_t character_space, std::span<wchar_t const> source)
		{
			return CharEncoder<WrapperType>::EncodeTo(character_space, { reinterpret_cast<WrapperType const*>(source.data()), source.size() });
		}

		static constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			return CharEncoder<WrapperType>::DecodeCharacterSize(source);
		}

		static constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<wchar_t> target)
		{
			return CharEncoder<WrapperType>::DecodeFrom(source, character_size, { reinterpret_cast<WrapperType*>(target.data()), target.size() });
		}
	};

	template<>
	struct CharEncoder<char> : CharEncoder<char8_t>
	{
		static constexpr std::optional<std::size_t> GetCharacterSize(std::span<char const> source, bool untrusted = false)
		{
			return CharEncoder<char8_t>::GetCharacterSize({ reinterpret_cast<char8_t const*>(source.data()), source.size() });
		}
		static constexpr char32_t EncodeTo(std::size_t character_space, std::span<char const> source)
		{
			return CharEncoder<char8_t>::EncodeTo(character_space, { reinterpret_cast<char8_t const*>(source.data()), source.size() });
		}

		static constexpr std::size_t DecodeCharacterSize(char32_t source)
		{
			return CharEncoder<char8_t>::DecodeCharacterSize(source);
		}

		static constexpr bool DecodeFrom(char32_t source, std::size_t character_size, std::span<char> target)
		{
			return CharEncoder<char8_t>::DecodeFrom(source, character_size, { reinterpret_cast<char8_t*>(target.data()), target.size() });
		}
	};

	template<typename Source, typename Target>
	struct StrEncoder
	{
		template<std::size_t S1>
		static constexpr EncodeInfo Encode(std::span<Source const, S1> source, std::span<Target> target, EncodeOption option = {})
		{
			if constexpr (sizeof(Source) != sizeof(Target))
			{
				typedef CharEncoder<Source> encoder_source;
				typedef CharEncoder<Target> encoder_target;
				EncodeInfo info;
				while (
					option.max_character != 0
					&& (option.predict || info.target_space < target.size())
					&& info.source_space < source.size()
					)
				{
					auto ite_span = source.subspan(info.source_space);
					auto count = encoder_source::GetCharacterSize(ite_span, option.untrusted);
					if (count.has_value())
					{
						char32_t temp = encoder_source::EncodeTo(*count, ite_span);
						auto target_count = encoder_target::DecodeCharacterSize(temp);

						if (!option.predict)
						{
							if (!encoder_target::DecodeFrom(temp, target_count, target.subspan(info.target_space)))
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
				typedef CharEncoder<Source> encoder;
				while (
					option.max_character != 0
					&& (option.predict || info.target_space < target.size())
					&& info.source_space < source.size()
					)
				{
					auto count = encoder::GetCharacterSize(source.subspan(info.source_space), option.untrusted);
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

		static constexpr EncodeInfo Encode(std::basic_string_view<Source> source, std::span<Target> target, EncodeOption option = {})
		{
			return Encode(std::span<Source const>(source.data(), source.size()), target, option);
		}
		template<std::size_t S1>
		static constexpr EncodeInfo Encode(std::span<Source, S1> source, std::span<Target> target, EncodeOption option = {})
		{
			return Encode(std::span<Source const, S1>(source), target, option);
		}
	};

	template<typename TargetT, typename CharT, std::size_t index>
	constexpr auto EncodeTypeString(TMP::TypeString<CharT, index> const& str)
	{
		constexpr auto encode_info = StrEncoder<CharT, TargetT>::Encode(str.GetSpan(), {}, {false, true});
		TargetT Temp[encode_info.target_space];
		StrEncoder<CharT, TargetT>{}.Encode(str.GetSpan(), std::span(Temp), {false, false});
		return TMP::TypeString(Temp);
	}
	*/
}
