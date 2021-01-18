#pragma once
#include <optional>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace Potato::StrEncode
{

	template<typename Type>
	struct ReverseEndianness{};

	template<typename From>
	struct RemoveReverseEndianness{ using Type = From;};

	template<typename From>
	struct RemoveReverseEndianness<ReverseEndianness<From>> { using Type = From; };

	template<typename From>
	using RemoveReverseEndianness_t = typename RemoveReverseEndianness<From>::Type;

	struct DecodeResult
	{
		size_t used_length = 0;
		char32_t temporary = 0;
	};
	
	template<typename Type>
	struct CharWrapper
	{
		/*
		size_t DetectOne(Type const* input, size_t input_length) = delete;
		DecodeResult DecodeOne(Type const* input, size_t input_length) = delete;
		size_t EncodeRequest(char32_t temporary) = delete;
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length) = delete;
		*/
	};

	template<>
	struct CharWrapper<char8_t>
	{
		using Type = char8_t;
		size_t DetectOne(Type const* input, size_t input_length);
		DecodeResult DecodeOne(Type const* input, size_t input_length);
		size_t EncodeRequest(char32_t temporary);
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length);
	};

	template<>
	struct CharWrapper<ReverseEndianness<char8_t>>
	{
		using Type = char8_t;
		CharWrapper<char8_t> temporary;
		size_t DetectOne(Type const* input, size_t input_length){ return temporary.DetectOne(input, input_length); }
		DecodeResult DecodeOne(Type const* input, size_t input_length){ return temporary.DecodeOne(input, input_length); }
		size_t EncodeRequest(char32_t input_temporary){ return temporary.EncodeRequest(input_temporary); }
		size_t EncodeOne(char32_t input_temporary, Type* input, size_t input_length){ return temporary.EncodeOne(input_temporary, input, input_length); }
	};

	template<>
	struct CharWrapper<char16_t>
	{
		using Type = char16_t;
		size_t DetectOne(Type const* input, size_t input_length);
		DecodeResult DecodeOne(Type const* input, size_t input_length);
		size_t EncodeRequest(char32_t temporary);
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length);
	};

	template<>
	struct CharWrapper<ReverseEndianness<char16_t>>
	{
		using Type = char16_t;
		CharWrapper<char16_t> wrapper;
		size_t DetectOne(Type const* input, size_t input_length);
		DecodeResult DecodeOne(Type const* input, size_t input_length);
		size_t EncodeRequest(char32_t temporary){ return wrapper.EncodeRequest(temporary);}
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length);
	};

	template<>
	struct CharWrapper<char32_t>
	{
		using Type = char32_t;
		size_t DetectOne(Type const* input, size_t input_length){ return (input != nullptr && input_length > 0 && input[0] < 0x110000) ? 1 : 0; }
		DecodeResult DecodeOne(Type const* input, size_t input_length){ return (input != nullptr && input_length > 0 && input[0] < 0x110000) ? DecodeResult{1, input[0]} : DecodeResult{ 0, {} };  }
		size_t EncodeRequest(char32_t temporary){ return temporary < 0x110000 ? 1 : 0; }
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length){ if(input != nullptr && input_length > 0 && temporary < 0x110000) {input[0] = temporary; return 1;} else return 0; }
	};

	template<>
	struct CharWrapper<ReverseEndianness<char32_t>>
	{
		using Type = char32_t;
		CharWrapper<char32_t> wrapper;
		size_t DetectOne(Type const* input, size_t input_length);
		DecodeResult DecodeOne(Type const* input, size_t input_length);
		size_t EncodeRequest(char32_t temporary){ return wrapper.EncodeRequest(temporary); }
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length);
	};

	template<>
	struct CharWrapper<wchar_t>
	{
		using RealType = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
		using Type = wchar_t;
		CharWrapper<RealType> wrapper;
		size_t DetectOne(Type const* input, size_t input_length){ return wrapper.DetectOne(reinterpret_cast<RealType const*>(input), input_length); }
		DecodeResult DecodeOne(Type const* input, size_t input_length){ return wrapper.DecodeOne(reinterpret_cast<RealType const*>(input), input_length); }
		size_t EncodeRequest(char32_t temporary) { return wrapper.EncodeRequest(temporary); }
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length){ return wrapper.EncodeOne(temporary, reinterpret_cast<RealType*>(input), input_length); }
	};

	template<>
	struct CharWrapper<ReverseEndianness<wchar_t>>
	{
		using RealType = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
		using Type = wchar_t;
		CharWrapper<ReverseEndianness<RealType>> wrapper;
		size_t DetectOne(Type const* input, size_t input_length) { return wrapper.DetectOne(reinterpret_cast<RealType const*>(input), input_length); }
		DecodeResult DecodeOne(Type const* input, size_t input_length) { return wrapper.DecodeOne(reinterpret_cast<RealType const*>(input), input_length); }
		size_t EncodeRequest(char32_t temporary) { return wrapper.EncodeRequest(temporary); }
		size_t EncodeOne(char32_t temporary, Type* input, size_t input_length) { return wrapper.EncodeOne(temporary, reinterpret_cast<RealType*>(input), input_length); }
	};

	struct RequestResult
	{
		size_t characters = 0;
		size_t require_length = 0;
	};

	struct EncodeResult
	{
		size_t used_source_length = 0;
		size_t used_target_length = 0;
		size_t encode_character_count = 0;
	};

	template<typename From, typename To>
	struct Encode
	{
		RequestResult Request(RemoveReverseEndianness_t<From> const* from, size_t from_length);
		EncodeResult Decode(RemoveReverseEndianness_t<From> const* from, size_t from_length, RemoveReverseEndianness_t<To>* to, size_t to_length);
	};

	template<typename From, typename To>
	RequestResult Encode<From, To>::Request(RemoveReverseEndianness_t<From> const* from, size_t from_length)
	{
		if(from != nullptr)
		{
			RequestResult result;
			CharWrapper<From> from_wrapper;
			CharWrapper<To> to_wrapper;
			while (from_length > 0)
			{
				DecodeResult decode_result = from_wrapper.DecodeOne(from, from_length);
				if (decode_result.used_length != 0)
				{
					from += decode_result.used_length;
					from_length -= decode_result.used_length;
					size_t require = to_wrapper.EncodeRequest(decode_result.temporary);
					result.characters += 1;
					result.require_length += require;
				}
				else
					break;
			}
			return result;
		}
		return {};
	}

	template<typename From, typename To>
	EncodeResult Encode<From, To>::Decode(RemoveReverseEndianness_t<From> const* from, size_t from_length, RemoveReverseEndianness_t<To>* to, size_t to_length)
	{
		
		if(from != nullptr && to != nullptr)
		{
			EncodeResult result;
			CharWrapper<From> from_wrapper;
			CharWrapper<To> to_wrapper;
			while (from_length > 0 && to_length > 0)
			{
				DecodeResult decode_result = from_wrapper.DecodeOne(from, from_length);
				if (decode_result.used_length != 0)
				{
					std::optional<size_t> re = to_wrapper.EncodeOne(decode_result.temporary, to, to_length);
					if(re && *re != 0)
					{
						result.encode_character_count +=1;
						result.used_source_length += decode_result.used_length;
						result.used_target_length += *re;
						from += decode_result.used_length;
						from_length -= decode_result.used_length;
						to += *re;
						to_length -= *re;
					}
				}
				else
					break;
			}
			return result;
		}
		return {};
	}

	template<typename SameType>
	struct Encode<SameType, SameType>
	{
		RequestResult Request(RemoveReverseEndianness_t<SameType> const* from, size_t from_length);
		EncodeResult Decode(RemoveReverseEndianness_t<SameType> const* from, size_t from_length, RemoveReverseEndianness_t<SameType>* to, size_t to_length);
	};

	template<typename SameType>
	RequestResult Encode<SameType, SameType>::Request(RemoveReverseEndianness_t<SameType> const* from, size_t from_length)
	{
		RequestResult result;
		CharWrapper<SameType> wrapper;
		while (from_length > 0)
		{
			DecodeResult decode_result = wrapper.DecodeOne(from, from_length);
			if (decode_result.used_length != 0)
			{
				from += decode_result.used_length;
				from_length -= decode_result.used_length;
				result.require_length += decode_result.used_length;
				result.characters += 1;
			}
			else
				break;
		}
		return result;
	}

	template<typename SameType>
	EncodeResult Encode<SameType, SameType>::Decode(RemoveReverseEndianness_t<SameType> const* from, size_t from_length, RemoveReverseEndianness_t<SameType>* to, size_t to_length)
	{
		if(to != nullptr)
		{
			RequestResult result;
			CharWrapper<SameType> wrapper;
			auto ite_from = from;
			while (from_length > 0)
			{
				DecodeResult decode_result = wrapper.DecodeOne(ite_from, from_length);
				if (decode_result.used_length != 0 && result.require_length + decode_result.used_length <= to_length)
				{
					ite_from += decode_result.used_length;
					from_length -= decode_result.used_length;
					result.require_length += decode_result.used_length;
					result.characters += 1;
				}
				else
					break;
			}
			if(result.require_length != 0)
				std::memcpy(to, from, sizeof(RemoveReverseEndianness_t<SameType>) * result.require_length);
			return {result.require_length, result.require_length, result.characters};
		}
		return {0, 0, 0};
	}

	template<typename From>
	struct StrWrapper
	{
		std::basic_string_view<RemoveReverseEndianness_t<From>> str;
		template<typename ToType>
		std::basic_string<RemoveReverseEndianness_t<ToType>> ToString() const
		{
			RequestResult request = Encode<From, ToType>{}.Request(str.data(), str.size());
			std::basic_string<RemoveReverseEndianness_t<ToType>> result(request.require_length, 0);
			To<ToType>(result.data(), result.size());
			return std::move(result);
		}
		template<typename ToType>
		EncodeResult To(RemoveReverseEndianness_t<ToType>* output, size_t output_length) const
		{
			return Encode<From, ToType>{}.Decode(str.data(), str.size(), output, output_length);
		}
	};

	template<typename Type>
	auto AsWrapper(std::basic_string_view<Type> in) { return StrWrapper<Type>{in};}

	template<typename Type>
	auto AsWrapper(Type const* in, size_t length) { return StrWrapper<Type>{std::basic_string_view<Type>{in, length}}; }

	template<typename Type>
	auto AsWrapperReverse(std::basic_string_view<Type> in) { return StrWrapper<ReverseEndianness<Type>>{in}; }

	template<typename Type>
	auto AsWrapperReverse(Type const* in, size_t length) { return StrWrapper<ReverseEndianness<Type>>{std::basic_string_view<Type>(in, length)}; }

	enum class BomType
	{
		UTF8_NoBom,
		UTF8,
		UTF16LE,
		UTF16BE,
		UTF32LE,
		UTF32BE
	};

	constexpr size_t ToSize(BomType bom)
	{
		switch (bom)
		{
		case BomType::UTF8: return 3;
		case BomType::UTF16BE:
		case BomType::UTF16LE:
			return 2;
		case BomType::UTF32BE:
		case BomType::UTF32LE:
			return 4;
		case BomType::UTF8_NoBom:
		default:
			return 0;
		}
	}

	enum class Endian : uint8_t {
		Less,
		Big
	};

	Endian DetectEndian();
	BomType DetectBom(std::byte const* bom,size_t bom_length) noexcept;
	std::byte const* ToBinary(BomType type) noexcept;
	
	template<typename StorageType>
	BomType DefaultBom() noexcept
	{
		if constexpr(std::is_same_v<StorageType, char8_t> || std::is_same_v<StorageType, ReverseEndianness<char8_t>>)
			return BomType::UTF8;
		else if constexpr (std::is_same_v<StorageType, char16_t>)
			return DetectEndian() == Endian::Less ? BomType::UTF16LE : BomType::UTF16BE;
		else if constexpr (std::is_same_v<StorageType, ReverseEndianness<char16_t>>)
			return DetectEndian() == Endian::Less ? BomType::UTF16BE : BomType::UTF16LE;
		else if constexpr (std::is_same_v<StorageType, char32_t>)
			return DetectEndian() == Endian::Less ? BomType::UTF32LE : BomType::UTF32BE;
		else if constexpr (std::is_same_v<StorageType, ReverseEndianness<char32_t>>)
			return DetectEndian() == Endian::Less ? BomType::UTF32BE : BomType::UTF32LE;
		else
			static_assert(false, "unsupport type");
	}

	
	struct DocumentWrapper
	{
		DocumentWrapper(std::byte const* code, size_t length);
		template<typename Type>
		std::basic_string<Type> ToString() const;
		operator bool() const noexcept{ return documenet != nullptr;}
		template<typename Type>
		static std::vector<std::byte> EncodeToDocument(RemoveReverseEndianness_t<Type> const* input, size_t length, BomType bom = BomType::UTF8_NoBom);
	private:
		std::byte const* documenet = nullptr;
		size_t documenet_length = 0;
		std::byte const* main_body = nullptr;
		size_t main_body_length = 0;
		BomType type = BomType::UTF8_NoBom;
	};

	template<typename Type>
	std::basic_string<Type> DocumentWrapper::ToString() const
	{
		switch (type)
		{
		case BomType::UTF8:
		case BomType::UTF8_NoBom:
			return AsWrapper(reinterpret_cast<char8_t const*>(main_body), main_body_length).ToString<Type>();
		case BomType::UTF16LE:
			if (DetectEndian() == Endian::Less)
				return AsWrapper(reinterpret_cast<char16_t const*>(main_body), main_body_length / sizeof(char16_t)).ToString<Type>();
			else
				return AsWrapperReverse(reinterpret_cast<char16_t const*>(main_body), main_body_length / sizeof(char16_t)).ToString<Type>();
		case BomType::UTF16BE:
			if (DetectEndian() == Endian::Big)
				return AsWrapper(reinterpret_cast<char16_t const*>(main_body), main_body_length / sizeof(char16_t)).ToString<Type>();
			else
				return AsWrapperReverse(reinterpret_cast<char16_t const*>(main_body), main_body_length / sizeof(char16_t)).ToString<Type>();
		case BomType::UTF32LE:
			if (DetectEndian() == Endian::Less)
				return AsWrapper(reinterpret_cast<char32_t const*>(main_body), main_body_length / sizeof(char32_t)).ToString<Type>();
			else
				return AsWrapperReverse(reinterpret_cast<char32_t const*>(main_body), main_body_length / sizeof(char32_t)).ToString<Type>();
		case BomType::UTF32BE:
			if (DetectEndian() == Endian::Big)
				return AsWrapper(reinterpret_cast<char32_t const*>(main_body), main_body_length / sizeof(char32_t)).ToString<Type>();
			else
				return AsWrapperReverse(reinterpret_cast<char32_t const*>(main_body), main_body_length / sizeof(char32_t)).ToString<Type>();
		default:
			return std::basic_string<Type>{};
		};
	}

	template<typename Type>
	std::vector<std::byte> DocumentWrapper::EncodeToDocument(RemoveReverseEndianness_t<Type> const* input, size_t length, BomType bom)
	{
		size_t bom_size = ToSize(bom);
		std::vector<std::byte> result;
		switch (bom)
		{
		case BomType::UTF8:
		case BomType::UTF8_NoBom:
		{
			Encode<Type, char8_t> Wrapper;
			RequestResult re = Wrapper.Request(input, length);
			result.resize(re.require_length * sizeof(char8_t) + bom_size);
			std::memcpy(result.data(), ToBinary(bom), bom_size);
			Wrapper.Decode(input, length, reinterpret_cast<char8_t *>(result.data() + bom_size), (result.size() - bom_size) / sizeof(char8_t));
			return std::move(result);
		}
		break;
		case BomType::UTF16LE:
		case BomType::UTF16BE:
		{
			if(DetectEndian() == Endian::Less && bom == BomType::UTF16LE || DetectEndian() == Endian::Big && bom == BomType::UTF16BE)
			{
				Encode<Type, char16_t> Wrapper;
				RequestResult re = Wrapper.Request(input, length);
				std::vector<std::byte> result;
				result.resize(bom_size + re.require_length * sizeof(char16_t));
				std::memcpy(result.data(), ToBinary(bom), bom_size);
				Wrapper.Decode(input, length, reinterpret_cast<char16_t*>(result.data() + bom_size), (result.size() - bom_size) / sizeof(char16_t));
				return std::move(result);
			}else
			{
				Encode<Type, ReverseEndianness<char16_t>> Wrapper;
				RequestResult re = Wrapper.Request(input, length);
				std::vector<std::byte> result;
				result.resize(bom_size + re.require_length * sizeof(char16_t));
				std::memcpy(result.data(), ToBinary(bom), bom_size);
				Wrapper.Decode(input, length, reinterpret_cast<char16_t*>(result.data() + bom_size), (result.size() - bom_size) / sizeof(char16_t));
				return std::move(result);
			}
		}
		break;
		case BomType::UTF32BE:
		case BomType::UTF32LE:
		{
			if (DetectEndian() == Endian::Less && bom == BomType::UTF32LE || DetectEndian() == Endian::Big && bom == BomType::UTF32BE)
			{
				Encode<Type, char32_t> Wrapper;
				RequestResult re = Wrapper.Request(input, length);
				std::vector<std::byte> result;
				result.resize(bom_size + re.require_length * sizeof(char32_t));
				std::memcpy(result.data(), ToBinary(bom), bom_size);
				Wrapper.Decode(input, length, reinterpret_cast<char32_t*>(result.data() + bom_size), (result.size() - bom_size) / sizeof(char32_t));
				return std::move(result);
			}
			else
			{
				Encode<Type, ReverseEndianness<char32_t>> Wrapper;
				RequestResult re = Wrapper.Request(input, length);
				std::vector<std::byte> result;
				result.resize(bom_size + re.require_length * sizeof(char32_t));
				std::memcpy(result.data(), ToBinary(bom), bom_size);
				Wrapper.Decode(input, length, reinterpret_cast<char32_t*>(result.data() + bom_size), (result.size() - bom_size) / sizeof(char32_t));
				return std::move(result);
			}
		}
		break;
		default: return {};
		};
	}
}
