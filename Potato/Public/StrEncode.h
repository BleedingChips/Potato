#pragma once
#include <optional>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <span>
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
		size_t DetectOne(std::basic_string_view<Type> input);
		DecodeResult DecodeOne(std::basic_string_view<Type> input);
		size_t EncodeRequest(char32_t temporary);
		size_t EncodeOne(char32_t temporary, std::span<Type> output);
	};

	template<>
	struct CharWrapper<ReverseEndianness<char8_t>> : public CharWrapper<char8_t>
	{
	};

	template<>
	struct CharWrapper<char16_t>
	{
		using Type = char16_t;
		size_t DetectOne(std::basic_string_view<Type> input);
		DecodeResult DecodeOne(std::basic_string_view<Type> input);
		size_t EncodeRequest(char32_t temporary);
		size_t EncodeOne(char32_t temporary, std::span<Type> output);
	};

	template<>
	struct CharWrapper<ReverseEndianness<char16_t>>
	{
		using Type = char16_t;
		CharWrapper<char16_t> wrapper;
		size_t DetectOne(std::basic_string_view<Type> input);
		DecodeResult DecodeOne(std::basic_string_view<Type> input);
		size_t EncodeRequest(char32_t temporary){ return wrapper.EncodeRequest(temporary);}
		size_t EncodeOne(char32_t temporary, std::span<Type> output);
	};

	template<>
	struct CharWrapper<char32_t>
	{
		using Type = char32_t;
		size_t DetectOne(std::basic_string_view<Type> input){ return (!input.empty() && input[0] < 0x110000) ? 1 : 0; }
		DecodeResult DecodeOne(std::basic_string_view<Type> input){ return (!input.empty() && input[0] < 0x110000) ? DecodeResult{1, input[0]} : DecodeResult{ 0, {} };  }
		size_t EncodeRequest(char32_t temporary){ return temporary < 0x110000 ? 1 : 0; }
		size_t EncodeOne(char32_t temporary, std::span<Type> output){ if(!output.empty() && temporary < 0x110000) { output[0] = temporary; return 1;} else return 0; }
	};

	template<>
	struct CharWrapper<ReverseEndianness<char32_t>>
	{
		using Type = char32_t;
		CharWrapper<char32_t> wrapper;
		size_t DetectOne(std::basic_string_view<Type> input);
		DecodeResult DecodeOne(std::basic_string_view<Type> input);
		size_t EncodeRequest(char32_t temporary){ return wrapper.EncodeRequest(temporary); }
		size_t EncodeOne(char32_t temporary, std::span<Type> output);
	};

	template<>
	struct CharWrapper<wchar_t>
	{
		using RealType = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
		using Type = wchar_t;
		CharWrapper<RealType> wrapper;
		size_t DetectOne(std::basic_string_view<Type> input){ return wrapper.DetectOne({reinterpret_cast<RealType const*>(input.data()), input.size()}); }
		DecodeResult DecodeOne(std::basic_string_view<Type> input){ return wrapper.DecodeOne({ reinterpret_cast<RealType const*>(input.data()), input.size() }); }
		size_t EncodeRequest(char32_t temporary) { return wrapper.EncodeRequest(temporary); }
		size_t EncodeOne(char32_t temporary, std::span<Type> output){ return wrapper.EncodeOne(temporary, { reinterpret_cast<RealType*>(output.data()), output.size() }); }
	};

	template<>
	struct CharWrapper<ReverseEndianness<wchar_t>>
	{
		using RealType = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
		using Type = wchar_t;
		CharWrapper<ReverseEndianness<RealType>> wrapper;
		size_t DetectOne(std::basic_string_view<Type> input) { return wrapper.DetectOne({ reinterpret_cast<RealType const*>(input.data()), input.size() }); }
		DecodeResult DecodeOne(std::basic_string_view<Type> input) { return wrapper.DecodeOne({ reinterpret_cast<RealType const*>(input.data()), input.size() }); }
		size_t EncodeRequest(char32_t temporary) { return wrapper.EncodeRequest(temporary); }
		size_t EncodeOne(char32_t temporary, std::span<Type> output) { return wrapper.EncodeOne(temporary, { reinterpret_cast<RealType*>(output.data()), output.size() }); }
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
		RequestResult Request(std::basic_string_view<RemoveReverseEndianness_t<From>> from);
		EncodeResult Decode(std::basic_string_view<RemoveReverseEndianness_t<From>> from, std::span<RemoveReverseEndianness_t<To>> to);
	};

	template<typename From, typename To>
	RequestResult Encode<From, To>::Request(std::basic_string_view<RemoveReverseEndianness_t<From>> from)
	{
		if(!from.empty())
		{
			RequestResult result;
			CharWrapper<From> from_wrapper;
			CharWrapper<To> to_wrapper;
			while (from.size() > 0)
			{
				DecodeResult decode_result = from_wrapper.DecodeOne(from);
				if (decode_result.used_length != 0)
				{
					from = from.substr(decode_result.used_length);
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
	EncodeResult Encode<From, To>::Decode(std::basic_string_view<RemoveReverseEndianness_t<From>> from, std::span<RemoveReverseEndianness_t<To>> to)
	{
		if(!from.empty())
		{
			EncodeResult result;
			CharWrapper<From> from_wrapper;
			CharWrapper<To> to_wrapper;
			while (!from.empty())
			{
				DecodeResult decode_result = from_wrapper.DecodeOne(from);
				if (decode_result.used_length != 0)
				{
					std::optional<size_t> re = to_wrapper.EncodeOne(decode_result.temporary, to);
					if(re && *re != 0)
					{
						result.encode_character_count +=1;
						result.used_source_length += decode_result.used_length;
						result.used_target_length += *re;
						from = from.substr(decode_result.used_length);
						to = to.subspan(*re);
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
		RequestResult Request(std::basic_string_view<RemoveReverseEndianness_t<SameType>> from);
		EncodeResult Decode(std::basic_string_view<RemoveReverseEndianness_t<SameType>> from, std::span<RemoveReverseEndianness_t<SameType>> to);
	};

	template<typename SameType>
	RequestResult Encode<SameType, SameType>::Request(std::basic_string_view<RemoveReverseEndianness_t<SameType>> from)
	{
		RequestResult result;
		CharWrapper<SameType> wrapper;
		while (!from.empty())
		{
			DecodeResult decode_result = wrapper.DecodeOne(from);
			if (decode_result.used_length != 0)
			{
				from = from.substr(decode_result.used_length);
				result.require_length += decode_result.used_length;
				result.characters += 1;
			}
			else
				break;
		}
		return result;
	}

	template<typename SameType>
	EncodeResult Encode<SameType, SameType>::Decode(std::basic_string_view<RemoveReverseEndianness_t<SameType>> from, std::span<RemoveReverseEndianness_t<SameType>> to)
	{
		if(!to.empty())
		{
			RequestResult result;
			CharWrapper<SameType> wrapper;
			auto ite_from = from;
			while (!from.empty())
			{
				DecodeResult decode_result = wrapper.DecodeOne(ite_from);
				if (decode_result.used_length != 0 && result.require_length + decode_result.used_length <= to.size())
				{
					ite_from.substr(decode_result.used_length);
					result.require_length += decode_result.used_length;
					result.characters += 1;
				}
				else
					break;
			}
			if(result.require_length != 0)
				std::memcpy(to.data(), from.data(), sizeof(RemoveReverseEndianness_t<SameType>) * result.require_length);
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
			RequestResult request = Encode<From, ToType>{}.Request(str);
			std::basic_string<RemoveReverseEndianness_t<ToType>> result(request.require_length, 0);
			To<ToType>(result);
			return std::move(result);
		}
		template<typename ToType>
		EncodeResult To(std::span<RemoveReverseEndianness_t<ToType>> output) const
		{
			return Encode<From, ToType>{}.Decode(str, output);
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

	BomType DetectBom(std::span<std::byte const> bom) noexcept;
	std::span<std::byte const> ToBinary(BomType type) noexcept;
	
	template<typename StorageType>
	BomType DefaultBom() noexcept
	{
		if constexpr(std::is_same_v<StorageType, char8_t> || std::is_same_v<StorageType, ReverseEndianness<char8_t>>)
			return BomType::UTF8;
		else if constexpr (std::is_same_v<StorageType, char16_t>)
			return std::endian::native == std::endian::little ? BomType::UTF16LE : BomType::UTF16BE;
		else if constexpr (std::is_same_v<StorageType, ReverseEndianness<char16_t>>)
			return std::endian::native == std::endian::little ? BomType::UTF16BE : BomType::UTF16LE;
		else if constexpr (std::is_same_v<StorageType, char32_t>)
			return std::endian::native == std::endian::little ? BomType::UTF32LE : BomType::UTF32BE;
		else if constexpr (std::is_same_v<StorageType, ReverseEndianness<char32_t>>)
			return std::endian::native == std::endian::little ? BomType::UTF32BE : BomType::UTF32LE;
		else
			static_assert(false, "unsupport type");
	}

	
	struct DocumentWrapper
	{
		DocumentWrapper(std::span<std::byte const> input);
		template<typename Type>
		std::basic_string<Type> ToString() const;
		operator bool() const noexcept{ return !document.empty();}
		template<typename Type>
		static std::vector<std::byte> EncodeToDocument(std::basic_string_view<RemoveReverseEndianness_t<Type>> input, BomType bom = BomType::UTF8_NoBom);
	private:
		template<typename Type>
		std::basic_string_view<Type> AsViewer() const {
			return {reinterpret_cast<Type const*>(document_without_bom.data()), document_without_bom.size() / sizeof(Type) };
		}
		std::span<std::byte const> document;
		std::span<std::byte const> document_without_bom;
		BomType type = BomType::UTF8_NoBom;
	};

	template<typename Type>
	std::basic_string<Type> DocumentWrapper::ToString() const
	{
		switch (type)
		{
		case BomType::UTF8:
		case BomType::UTF8_NoBom:
			return AsWrapper(this->AsViewer<char8_t>()).ToString<Type>();
		case BomType::UTF16LE:
			if constexpr (std::endian::native == std::endian::little)
				return AsWrapper(this->AsViewer<char16_t>()).ToString<Type>();
			else
				return AsWrapperReverse(this->AsViewer<char16_t>()).ToString<Type>();
		case BomType::UTF16BE:
			if constexpr (std::endian::native == std::endian::big)
				return AsWrapper(this->AsViewer<char16_t>()).ToString<Type>();
			else
				return AsWrapperReverse(this->AsViewer<char16_t>()).ToString<Type>();
		case BomType::UTF32LE:
			if constexpr (std::endian::native == std::endian::little)
				return AsWrapper(this->AsViewer<char16_t>()).ToString<Type>();
			else
				return AsWrapperReverse(this->AsViewer<char16_t>()).ToString<Type>();
		case BomType::UTF32BE:
			if constexpr (std::endian::native == std::endian::big)
				return AsWrapper(this->AsViewer<char16_t>()).ToString<Type>();
			else
				return AsWrapperReverse(this->AsViewer<char16_t>()).ToString<Type>();
		default:
			return std::basic_string<Type>{};
		};
	}

	template<typename Type>
	std::vector<std::byte> DocumentWrapper::EncodeToDocument(std::basic_string_view<RemoveReverseEndianness_t<Type>> input, BomType bom)
	{
		auto Bom = ToBinary(bom);
		std::vector<std::byte> result = Bom;
		switch (bom)
		{
		case BomType::UTF8:
		case BomType::UTF8_NoBom:
		{
			Encode<Type, char8_t> Wrapper;
			RequestResult re = Wrapper.Request(input);
			result.resize(re.require_length * sizeof(char8_t) + Bom.size());
			auto output = std::span<char8_t>(reinterpret_cast<char8_t*>(result.data() + Bom.size()), re.require_length);
			Wrapper.Decode(input, output);
			return std::move(result);
		}
		break;
		case BomType::UTF16LE:
		case BomType::UTF16BE:
		{
			if(std::endian::native == std::endian::little && bom == BomType::UTF16LE || std::endian::native == std::endian::big && bom == BomType::UTF16BE)
			{
				Encode<Type, char16_t> Wrapper;
				RequestResult re = Wrapper.Request(input);
				result.resize(Bom.size() + re.require_length * sizeof(char16_t));
				auto output = std::span<char16_t>(reinterpret_cast<char16_t*>(result.data() + Bom.size()), re.require_length);
				Wrapper.Decode(input, output);
				return std::move(result);
			}else
			{
				Encode<Type, ReverseEndianness<char16_t>> Wrapper;
				RequestResult re = Wrapper.Request(input);
				std::vector<std::byte> result;
				result.resize(Bom.size() + re.require_length * sizeof(char16_t));
				auto output = std::span<char16_t>(reinterpret_cast<char16_t*>(result.data() + Bom.size()), re.require_length);
				Wrapper.Decode(input, output); 
				return std::move(result);
			}
		}
		break;
		case BomType::UTF32BE:
		case BomType::UTF32LE:
		{
			if (std::endian::native == std::endian::little && bom == BomType::UTF32LE || std::endian::native == std::endian::big && bom == BomType::UTF32BE)
			{
				Encode<Type, char32_t> Wrapper;
				RequestResult re = Wrapper.Request(input);
				result.resize(Bom.size() + re.require_length * sizeof(char32_t));
				auto output = std::span<char32_t>(reinterpret_cast<char32_t*>(result.data() + Bom.size()), re.require_length);
				Wrapper.Decode(input, output); 
				return std::move(result);
			}
			else
			{
				Encode<Type, ReverseEndianness<char32_t>> Wrapper;
				RequestResult re = Wrapper.Request(input);
				result.resize(Bom.size() + re.require_length * sizeof(char32_t));
				auto output = std::span<char32_t>(reinterpret_cast<char32_t*>(result.data() + Bom.size()), re.require_length);
				Wrapper.Decode(input, output); 
				return std::move(result);
			}
		}
		break;
		default: return {};
		};
	}
}
