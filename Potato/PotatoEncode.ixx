module;

#include <cassert>

export module PotatoEncode;

import std;

export namespace Potato::Encode
{

	struct Unicode
	{
		using CodePointT = std::uint32_t;

		struct UTF8
		{
			using StorageT = std::uint8_t;
			static constexpr StorageT placemenet_character = std::numeric_limits<StorageT>::max();

			static constexpr std::size_t detect_storage_size = 4;
			static constexpr std::size_t max_storage_size = 4;

			static constexpr std::optional<std::size_t> DetectStorageSize(StorageT char_1, StorageT char_2 = placemenet_character, StorageT char_3 = placemenet_character, StorageT char_4 = placemenet_character)
			{
				if ((char_1 & 0x80) == 0)
					return 1;
				else if ((char_1 & 0xE0) == 0xC0)
				{
					if ((char_2 & 0x80) == 0x80)
					{
						return 2;
					}
				}
				else if ((char_1 & 0xF0) == 0xE0)
				{
					if ((char_2 & 0x80) == 0x80 && (char_3 & 0x80) == 0x80)
						return 3;
				}
				else if ((char_1 & 0xF8) == 0xF0)
				{
					if ((char_2 & 0x80) == 0x80 && (char_3 & 0x80) == 0x80 && (char_4 & 0x80) == 0x80)
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
				auto storage_size = DetectStorageSizeFromCodePoint(point);
				switch (storage_size)
				{
				case 1:
					return std::make_tuple(
						storage_size,
						static_cast<StorageT>(point), 
						placemenet_character, 
						placemenet_character,
						placemenet_character
					);
				case 2:
					return std::make_tuple(
						storage_size,
						0xC0 | static_cast<StorageT>((point & 0x07C0) >> 6),
						0x80 | static_cast<StorageT>((point & 0x3F)),
						placemenet_character,
						placemenet_character
					);
				case 3:
					return std::make_tuple(
						storage_size,
						0xE0 | static_cast<StorageT>((point & 0xF000) >> 12),
						0x80 | static_cast<StorageT>((point & 0xFC0) >> 6),
						0x80 | static_cast<StorageT>((point & 0x3F)),
						placemenet_character
					);
				default:
					return std::make_tuple(
						storage_size,
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

			static constexpr std::size_t detect_storage_size = 2;
			static constexpr std::size_t max_storage_size = 2;

			static constexpr std::optional<std::size_t> DetectStorageSize(StorageT char_1, StorageT char_2 = placemenet_character)
			{
				if (char_1 < 0x10000)
				{
					return 1;
				}else if (char_1 >= 0xD800 && char_1 < 0xE00 && char_2 >= 0xDC00 && char_2 < 0xE000)
				{
					return 2;
				}
				return std::nullopt;
			}

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
				if (point < 0x010000)
					return 1;
				else
					return 2;
			}

			static constexpr std::tuple<std::size_t, StorageT, StorageT> EncodeFromUnicodePoint(CodePointT point)
			{
				auto storage_size = DetectStorageSizeFromCodePoint(point);
				switch (storage_size)
				{
				case 1:
					return std::make_tuple(
						storage_size,
						static_cast<StorageT>(point),
						placemenet_character
					);
				default:
					point -= 0x10000;
					return std::make_tuple(
						storage_size,
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

			static constexpr std::size_t detect_storage_size = 1;
			static constexpr std::size_t max_storage_size = 1;

			static constexpr std::optional<std::size_t> DetectStorageSize(StorageT char_1)
			{
				if (char_1 < 0x10FFFF)
					return 1;
				return std::nullopt;
			}

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
				return std::make_tuple(DetectStorageSizeFromCodePoint(point), static_cast<StorageT>(point));
			}
		};
	};

	struct EncodeInfo
	{
		std::size_t character_count = 0;
		std::size_t source_space = 0;
		std::size_t target_space = 0;
		bool is_good_string = true;
		explicit constexpr operator bool() const { return is_good_string; }
	};

	template<typename RawStorageT>
	struct UnicodeWrapper;

	template<>
	struct UnicodeWrapper<char>
	{
		using NativeStorageT = char;
		using WrapperT = Unicode::UTF8;
	};

	template<>
	struct UnicodeWrapper<char8_t>
	{
		using NativeStorageT = char8_t;
		using WrapperT = Unicode::UTF8;
	};

	template<>
	struct UnicodeWrapper<wchar_t>
	{
		using NativeStorageT = wchar_t;
		using WrapperT = std::conditional_t<
			sizeof(wchar_t) == sizeof(char16_t),
			Unicode::UTF16,
			std::conditional_t<
				sizeof(wchar_t) == sizeof(char32_t),
				Unicode::UTF32,
				void
			>
		>;
	};

	template<>
	struct UnicodeWrapper<char16_t>
	{
		using NativeStorageT = char16_t;
		using WrapperT = Unicode::UTF16;
	};

	template<>
	struct UnicodeWrapper<char32_t>
	{
		using NativeStorageT = char32_t;
		using WrapperT = Unicode::UTF32;
	};

	template<std::size_t count, typename WrapperT>
	struct UnicodeEncoderHelper
	{
		template<typename NativeT>
		static constexpr std::optional<std::size_t> DetectStorageSize(std::span<NativeT const> source)
		{
			return[]<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeT const> source)
			{
				if (source.size() >= count)
				{
					return WrapperT::DetectStorageSize(
						(( i < count) ? static_cast<WrapperT::StorageT>(source[i]) : WrapperT::placemenet_character)...
					);
				}
				else {
					return UnicodeEncoderHelper<count - 1, WrapperT>::DetectStorageSize(source);
				}
			}(std::make_index_sequence<WrapperT::detect_storage_size>(), source);
		}

		template<typename NativeT>
		static constexpr Unicode::CodePointT DecodeToUnicodePoint(std::size_t storage_count, std::span<NativeT const> source)
		{
			if (count == storage_count)
			{
				return[]<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeT const> source) {
					return WrapperT::DecodeToUnicodePoint(
						static_cast<WrapperT::StorageT>(source[i])...
					);
				}(std::make_index_sequence<count>(), source);
			}
			else {
				return UnicodeEncoderHelper<count - 1, WrapperT>::DecodeToUnicodePoint(storage_count, source);
			}
		}

		template<typename NativeT, typename ...AT>
		static constexpr std::optional<std::size_t> EncodeFromUnicodePoint(std::tuple<AT...> result, std::span<NativeT> target)
		{
			return []<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeT> target, std::tuple<AT...> result) -> std::optional<std::size_t> {
				if (std::get<0>(result) == count)
				{
					if (target.size() >= std::get<0>(result))
					{
						[](auto ...) {}(target[i] = static_cast<NativeT>(std::get<i + 1>(result))...);
						return std::get<0>(result);
					}
					return std::nullopt;
				}
				return UnicodeEncoderHelper<count - 1, WrapperT>::EncodeFromUnicodePoint(result, target);
			}(std::make_index_sequence<count>(), target, result);
		}
	};

	template<typename WrapperT>
	struct UnicodeEncoderHelper<1, WrapperT>
	{
		template<typename NativeT>
		static constexpr std::optional<std::size_t> DetectStorageSize(std::span<NativeT const> source)
		{
			return[]<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeT const> source) -> std::optional<std::size_t>
			{
				if (source.size() >= 1)
				{
					return WrapperT::DetectStorageSize(
						((i < 1) ? static_cast<WrapperT::StorageT>(source[i]) : WrapperT::placemenet_character)...
						);
				}
				else {
					return std::nullopt;
				}
			}(std::make_index_sequence<WrapperT::detect_storage_size>(), source);
		}

		template<typename NativeT>
		static constexpr Unicode::CodePointT DecodeToUnicodePoint(std::size_t storage_count, std::span<NativeT const> source)
		{
			return WrapperT::DecodeToUnicodePoint(
				static_cast<WrapperT::StorageT>(source[0])
			);
		}
		template<typename NativeT, typename ...AT>
		static constexpr std::optional<std::size_t> EncodeFromUnicodePoint(std::tuple<AT...> result, std::span<NativeT> target)
		{
			return []<std::size_t ...i>(std::index_sequence<i...>, std::span<NativeT> target, std::tuple<AT...> result) -> std::optional<std::size_t> {
				if (std::get<0>(result) == 1 && target.size() >= std::get<0>(result))
				{
					target[0] = static_cast<NativeT>(std::get<1>(result));
					return 1;
				}
				return std::nullopt;
			}(std::make_index_sequence<WrapperT::max_storage_size>(), target, result);
		}
	};

	template<typename FromCharT, typename ToCharT>
	struct UnicodeEncoder;

	template<typename CharT>
	struct UnicodeEncoder<CharT, Unicode::CodePointT>
	{
		using NativeStorageT = UnicodeWrapper<CharT>::NativeStorageT;
		using WrapperT = UnicodeWrapper<CharT>::WrapperT;

		static constexpr EncodeInfo Statistics(std::span<NativeStorageT const> source, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			auto iterator_string = source;
			while (!iterator_string.empty())
			{
				if (info.character_count >= max_unicode_point)
					return info;

				auto detect_size = UnicodeEncoderHelper<WrapperT::detect_storage_size, WrapperT>::DetectStorageSize(iterator_string);

				if (detect_size.has_value())
				{
					info.source_space += *detect_size;
					info.character_count += 1;
					info.target_space += 1;
					iterator_string = iterator_string.subspan(*detect_size);
				}
				else {
					info.is_good_string = false;
					return info;
				}
			}
			return info;
		}

		static constexpr EncodeInfo EncodeTo(std::span<NativeStorageT const> source, std::span<Unicode::CodePointT> target, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			auto iterator_string = source;
			auto iterator_out = target;
			while (!iterator_string.empty() && !iterator_out.empty())
			{
				if (info.character_count >= max_unicode_point)
					return info;

				auto detect_size = UnicodeEncoderHelper<WrapperT::detect_storage_size, WrapperT>::DetectStorageSize(iterator_string);

				if (detect_size.has_value())
				{
					auto code_point = UnicodeEncoderHelper<WrapperT::max_storage_size, WrapperT>::DecodeToUnicodePoint(*detect_size, iterator_string);
					iterator_out[0] = code_point;
					info.character_count += 1;
					info.source_space += *detect_size;
					info.target_space += 1;
					iterator_string = iterator_string.subspan(*detect_size);
					iterator_out = iterator_out.subspan(1);
				}
				else {
					info.is_good_string = false;
					return info;
				}
			}
			return info;
		}
		
		template<typename OutIterator>
			requires std::output_iterator<OutIterator, Unicode::CodePointT>
		static constexpr EncodeInfo EncodeTo(std::span<NativeStorageT const> source, OutIterator iterator, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			std::array<Unicode::CodePointT, 16> temporary_buffer;
			EncodeInfo info;
			auto iterator_string = source;
			while (info.is_good_string && !iterator_string.empty() && max_unicode_point > 0)
			{
				auto curret_info = EncodeTo(iterator_string, std::span(temporary_buffer), max_unicode_point);
				info.character_count += curret_info.character_count;
				info.source_space += curret_info.source_space;
				info.target_space += curret_info.target_space;
				info.is_good_string = curret_info.is_good_string;
				info.character_count += curret_info.character_count;
				max_unicode_point -= curret_info.character_count;
				iterator_string = iterator_string.subspan(curret_info.target_space);
				for (std::size_t i = 0; i < curret_info.target_space; ++i)
				{
					(*iterator++) = temporary_buffer[i];
				}
			}
			return info;
		}
	};

	template<typename CharT>
	struct UnicodeEncoder<Unicode::CodePointT, CharT>
	{
		using NativeStorageT = UnicodeWrapper<CharT>::NativeStorageT;
		using WrapperT = UnicodeWrapper<CharT>::WrapperT;

		static constexpr EncodeInfo Statistics(std::span<Unicode::CodePointT const> source, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			auto iterator_string = source;
			while (!iterator_string.empty())
			{
				if (info.character_count >= max_unicode_point)
					return info;

				auto size = WrapperT::DetectStorageSizeFromCodePoint(iterator_string[0]);

				info.character_count += 1;
				info.source_space += 1;
				info.target_space += size;

				iterator_string = iterator_string.subspan(1);
			}
			return info;
		}

		static constexpr EncodeInfo EncodeTo(std::span<Unicode::CodePointT const> source, std::span<CharT> target, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			auto iterator_string = source;
			auto iterator_out = target;
			while (!iterator_string.empty() && !iterator_out.empty())
			{
				if (info.character_count >= max_unicode_point)
					return info;

				auto result = WrapperT::EncodeFromUnicodePoint(iterator_string[0]);
				auto target_size = UnicodeEncoderHelper<WrapperT::max_storage_size, WrapperT>::EncodeFromUnicodePoint(result, iterator_out);
				if (target_size.has_value())
				{
					info.character_count += 1;
					info.source_space += 1;
					info.target_space += *target_size;
					iterator_string = iterator_string.subspan(1);
					iterator_out = iterator_out.subspan(*target_size);
				}
				else {
					info.is_good_string = false;
					return info;
				}
			}
			return info;
		}
	
		template<typename OutIterator>
			requires std::output_iterator<OutIterator, CharT>
		static constexpr EncodeInfo EncodeTo(std::span<NativeStorageT const> source, OutIterator iterator, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			std::array<CharT, 16 * WrapperT::max_storage_size> temporary_buffer;
			EncodeInfo info;
			auto iterator_string = source;
			while (info.is_good_string && !iterator_string.empty() && max_unicode_point > 0)
			{
				auto curret_info = EncodeTo(iterator_string, std::span(temporary_buffer), max_unicode_point);
				info.character_count += curret_info.character_count;
				info.source_space += curret_info.source_space;
				info.target_space += curret_info.target_space;
				info.is_good_string = curret_info.is_good_string;
				info.character_count += curret_info.character_count;
				max_unicode_point -= curret_info.character_count;
				iterator_string = iterator_string.subspan(curret_info.target_space);
				for (std::size_t i = 0; i < curret_info.target_space; ++i)
				{
					(*iterator++) = temporary_buffer[i];
				}
			}
			return info;
		}
	};

	
	template<typename FromCharT, typename ToCharT>
	struct UnicodeEncoder
	{

		static constexpr EncodeInfo Statistics(std::span<FromCharT const> source, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			std::array<Unicode::CodePointT, 16> temporary_buffer;
			auto iterator_string = source;
			while (max_unicode_point != 0 && !iterator_string.empty() && info.is_good_string)
			{
				auto decode_info = UnicodeEncoder<FromCharT, Unicode::CodePointT>::EncodeTo(iterator_string, std::span(temporary_buffer), max_unicode_point);
				auto encode_info = UnicodeEncoder<Unicode::CodePointT, ToCharT>::Statistics(std::span(temporary_buffer).subspan(0, decode_info.target_space));
				info.character_count += decode_info.character_count;
				info.source_space += decode_info.source_space;
				info.target_space += encode_info.target_space;
				info.is_good_string = decode_info.is_good_string && encode_info.is_good_string;
				max_unicode_point -= decode_info.character_count;
				iterator_string = iterator_string.subspan(decode_info.source_space);
			}
			return info;
		}

		static constexpr EncodeInfo Statistics(std::basic_string_view<FromCharT> source, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			return Statistics(std::span(source.data(), source.size()), max_unicode_point);
		}

		static constexpr EncodeInfo EncodeTo(std::span<FromCharT const> source, std::span<ToCharT> target, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			EncodeInfo info;
			std::array<Unicode::CodePointT, 16> temporary_buffer;
			auto iterator_string = source;
			auto iterator_out = target;
			while (info.is_good_string && max_unicode_point != 0 && !iterator_string.empty() && !iterator_out.empty())
			{
				auto decode_info = UnicodeEncoder<FromCharT, Unicode::CodePointT>::EncodeTo(iterator_string, std::span(temporary_buffer), max_unicode_point);
				auto encode_info = UnicodeEncoder<Unicode::CodePointT, ToCharT>::EncodeTo(std::span(temporary_buffer).subspan(0, decode_info.target_space), iterator_out);

				if (decode_info.character_count == encode_info.character_count)
				{
					info.character_count += decode_info.character_count;
					info.source_space += decode_info.source_space;
					info.target_space += encode_info.target_space;
					info.is_good_string = decode_info.is_good_string && encode_info.is_good_string;
					max_unicode_point -= decode_info.character_count;
					iterator_string = iterator_string.subspan(decode_info.source_space);
					iterator_out = iterator_out.subspan(encode_info.target_space);
				}
				else {
					auto redecode_info = UnicodeEncoder<FromCharT, Unicode::CodePointT>::Statistics(iterator_string, encode_info.character_count);
					info.character_count += redecode_info.character_count;
					info.source_space += redecode_info.source_space;
					info.target_space += encode_info.target_space;
					info.is_good_string = redecode_info.is_good_string && redecode_info.is_good_string;
					max_unicode_point -= redecode_info.character_count;
					iterator_string = iterator_string.subspan(redecode_info.source_space);
					iterator_out = iterator_out.subspan(encode_info.target_space);
					return info;
				}
			}
			return info;
		}
		static constexpr EncodeInfo EncodeTo(std::basic_string_view<FromCharT> source, std::span<ToCharT> target, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			return EncodeTo(std::span(source.data(), source.size()), target, max_unicode_point);
		}

		template<typename OutIterator>
			requires std::output_iterator<OutIterator, ToCharT>
		static constexpr EncodeInfo EncodeTo(std::span<FromCharT const> source, OutIterator iterator, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			std::array<ToCharT, 16 * UnicodeWrapper<ToCharT>::WrapperT::max_storage_size> temporary_buffer;
			EncodeInfo info;
			auto iterator_string = source;
			while (info.is_good_string && max_unicode_point > 0 && !iterator_string.empty())
			{
				auto curret_info = EncodeTo(iterator_string, std::span(temporary_buffer), max_unicode_point);
				info.character_count += curret_info.character_count;
				info.source_space += curret_info.source_space;
				info.target_space += curret_info.target_space;
				info.is_good_string = curret_info.is_good_string;
				info.character_count += curret_info.character_count;
				max_unicode_point -= curret_info.character_count;
				iterator_string = iterator_string.subspan(curret_info.source_space);
				for (std::size_t i = 0; i < curret_info.target_space; ++i)
				{
					(*iterator++) = temporary_buffer[i];
				}
			}
			return info;
		}

		template<typename OutIterator>
			requires std::output_iterator<OutIterator, ToCharT>
		static constexpr EncodeInfo EncodeTo(std::basic_string_view<FromCharT const> source, OutIterator iterator, std::size_t max_unicode_point = std::numeric_limits<std::size_t>::max())
		{
			return EncodeTo(std::span(source.data(), source.size()), std::move(iterator), max_unicode_point);
		}
	};
}
