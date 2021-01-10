#include "../Public/StrEncode.h"
namespace Potato::StrEncode
{

	size_t UTF8RequireSpace(char8_t first_char)
	{
		if ((first_char & 0xF8) == 0xF0)
			return 4;
		else if ((first_char & 0xF0) == 0xE0)
			return 3;
		else if ((first_char & 0xE0) == 0xC0)
			return 2;
		else if ((first_char & 0x80) == 0)
			return 1;
		else
			return 0;
	}
	
	bool CheckUTF8(char8_t const* input, size_t space)
	{
		if(space == 0)
			return true;
		else
		{
			for(size_t i = 0; i < space; ++i)
			{
				if((input[i] & 0xC0) != 0x80)
					return false;
			}
			return true;
		}
	}
	
	size_t CharWrapper<char8_t>::DetectOne(Type const* input, size_t input_length)
	{
		if(input != nullptr && input_length > 0)
		{
			auto rs = UTF8RequireSpace(input[0]);
			if(input_length >= rs && CheckUTF8(input + 1, rs - 1))
				return rs;
		}
		return 0;
	}

	DecodeResult CharWrapper<char8_t>::DecodeOne(Type const* input, size_t input_length)
	{
		assert(input != nullptr && input_length > 0);
		auto rs = UTF8RequireSpace(input[0]);
		assert(input_length >= rs);
		switch (rs)
		{
		case 1: return {1, input[0]};
		case 2: return {2, (static_cast<char32_t>(input[0] & 0x1F) << 6) | static_cast<char32_t>(input[1] & 0x3F) };
		case 3: return {3, (static_cast<char32_t>(input[0] & 0x0F) << 12) | (static_cast<char32_t>(input[1] & 0x3F) << 6)
			| static_cast<char32_t>(input[2] & 0x3F) };
		case 4: return {4, (static_cast<char32_t>(input[0] & 0x07) << 18) | (static_cast<char32_t>(input[1] & 0x3F) << 12)
			| (static_cast<char32_t>(input[2] & 0x3F) << 6) | static_cast<char32_t>(input[3] & 0x3F) };
		default: return { 0, 0 };
		}
	}

	size_t CharWrapper<char8_t>::EncodeRequest(char32_t temporary)
	{
		if (temporary <= 0x7F) return 1;
		else if (temporary <= 0x7FF) return 2;
		else if (temporary <= 0xFFFF) return 3;
		else if (temporary <= 0x10FFFF) return 4;
		else return 0;
	}

	size_t CharWrapper<char8_t>::EncodeOne(char32_t temporary, Type* input, size_t input_length)
	{
		assert(input != nullptr && input_length > 0);
		auto rr = EncodeRequest(temporary);
		assert(input_length >= rr);
		switch (rr)
		{
		case 1:
		{
			input[0] = static_cast<char8_t>(temporary & 0x0000007F);
		}break;
		case 2:
		{
			input[0] = 0xC0 | static_cast<char>((temporary & 0x07C0) >> 6);
			input[1] = 0x80 | static_cast<char>((temporary & 0x3F));
		}break;
		case 3:
		{
			input[0] = 0xE0 | static_cast<char>((temporary & 0xF000) >> 12);
			input[1] = 0x80 | static_cast<char>((temporary & 0xFC0) >> 6);
			input[2] = 0x80 | static_cast<char>((temporary & 0x3F));
		}break;
		case 4:
		{
			input[0] = 0x1E | static_cast<char>((temporary & 0x1C0000) >> 18);
			input[1] = 0x80 | static_cast<char>((temporary & 0x3F000) >> 12);
			input[2] = 0x80 | static_cast<char>((temporary & 0xFC0) >> 6);
			input[3] = 0x80 | static_cast<char>((temporary & 0x3F));
		}break;
		default: break;
		}
		return rr;
	}

	size_t CheckUTF16RequireSize(char16_t input)
	{
		if((input & 0xD800) == 0xD800)
			return 2;
		return 1;
	}

	size_t CharWrapper<char16_t>::DetectOne(Type const* input, size_t input_length)
	{
		if(input != nullptr && input_length > 0)
		{
			if((input[0] & 0xD800) == 0xD800)
			{
				if(input_length >= 2 && (input[1] & 0xDC00) == 0xDC00)
				{
					return 2;
				}else
					return 0;
			}
			return 1;
		}
		return 0;
	}

	DecodeResult CharWrapper<char16_t>::DecodeOne(Type const* input, size_t input_length)
	{
		assert(input != nullptr && input_length > 0);
		size_t rs  = 0;
		if ((input[0] & 0xD800) == 0xD800)
			rs = 2;
		else
			rs = 1;
		assert(input_length >= rs);
		switch (rs)
		{
		case 1: return { 1, input[0] };
		case 2: return { 2, ((static_cast<char32_t>(input[0] & 0x3FF) << 10) | (static_cast<char32_t>(input[1] & 0x3FF))) + 0x10000 };
		default: return { 0, 0 };
		}
	}

	size_t CharWrapper<char16_t>::EncodeRequest(char32_t temporary)
	{
		if(temporary < 0x10000)
			return 1;
		else if(temporary < 0x110000)
			return 2;
		else
			return 0;
	}

	size_t CharWrapper<char16_t>::EncodeOne(char32_t temporary, Type* input, size_t input_length)
	{
		assert(input != nullptr && input_length > 0);
		size_t rs = EncodeRequest(temporary);
		assert(input_length >= rs);
		switch (rs)
		{
		case 1: input[0] = static_cast<char16_t>(temporary); return 1;
		case 2:
		{
			char32_t tar = temporary - 0x10000;
			input[0] = static_cast<char16_t>((tar & 0xFFC00) >> 10) + 0xD800;
			input[1] = static_cast<char16_t>((tar & 0x3FF)) + 0xDC00;
			return 2;
		}
		default: return 0;
		}
	}

	void Reverser(void* input_element, size_t element_length, size_t element_count)
	{
		size_t tar_length = element_length / 2;
		std::byte* element = static_cast<std::byte*>(input_element);
		for(size_t i = 0; i < element_count; ++i)
		{
			for(size_t k =0; k < tar_length; ++k)
				std::swap(element[k], element[element_length - k - 1]);
			element += element_length;
		}
	}

	size_t CharWrapper<ReverseEndianness<char16_t>>::DetectOne(Type const* input, size_t input_length)
	{
		// todo
		if(input != nullptr && input_length > 0)
		{
			char16_t temporary[2];
			auto size = std::min(std::size(temporary), input_length);
			std::memcpy(temporary, input, size * sizeof(char16_t));
			Reverser(temporary, sizeof(char16_t), size);
			return wrapper.DetectOne(temporary, size);
		}
		return 0;
	}

	DecodeResult CharWrapper<ReverseEndianness<char16_t>>::DecodeOne(Type const* input, size_t input_length)
	{
		// todo
		char16_t temporary[2];
		auto size = std::min(std::size(temporary), input_length);
		std::memcpy(temporary, input, size * sizeof(char16_t));
		Reverser(temporary, sizeof(char16_t), size);
		return wrapper.DecodeOne(temporary, size);
	}

	size_t CharWrapper<ReverseEndianness<char16_t>>::EncodeOne(char32_t temporary, Type* input, size_t input_length)
	{
		// todo
		if(input > nullptr)
		{
			char16_t temporary_buffer[2];
			auto size = std::min(std::size(temporary_buffer), input_length);
			auto re = wrapper.EncodeOne(temporary, temporary_buffer, size);
			Reverser(temporary_buffer, sizeof(char16_t), re);
			std::memcpy(input, temporary_buffer, sizeof(char16_t) * re);
			return re;
		}
		return 0;
	}

	size_t CharWrapper<ReverseEndianness<char32_t>>::DetectOne(Type const* input, size_t input_length)
	{
		// todo
		if (input != nullptr && input_length > 0)
		{
			char32_t temporary;
			auto size = std::min(static_cast<size_t>(1), input_length);
			std::memcpy(&temporary, input, size * sizeof(char32_t));
			Reverser(&temporary, sizeof(char32_t), size);
			return wrapper.DetectOne(&temporary, size);
		}
		return 0;
	}

	DecodeResult CharWrapper<ReverseEndianness<char32_t>>::DecodeOne(Type const* input, size_t input_length)
	{
		// todo
		char32_t temporary;
		auto size = std::min(static_cast<size_t>(1), input_length);
		std::memcpy(&temporary, input, size * sizeof(char32_t));
		Reverser(&temporary, sizeof(char32_t), size);
		return wrapper.DecodeOne(&temporary, size);
	}

	size_t CharWrapper<ReverseEndianness<char32_t>>::EncodeOne(char32_t temporary, Type* input, size_t input_length)
	{
		// todo
		if (input > nullptr)
		{
			char32_t temporary_buffer;
			auto size = std::min(static_cast<size_t>(1), input_length);
			auto re = wrapper.EncodeOne(temporary, &temporary_buffer, size);
			Reverser(&temporary_buffer, sizeof(char32_t), re);
			std::memcpy(input, &temporary_buffer, sizeof(char32_t) * re);
			return re;
		}
		return 0;
	}
	Endian DetectEndian()
	{
		constexpr uint16_t Detect = 0x0001;
		static Endian result = (*reinterpret_cast<uint8_t const*>(&Detect) == 0x01) ? Endian::Less : Endian::Big;
		return result;
	}
	

	const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
	const unsigned char utf16_le_bom[] = { 0xFF, 0xFE };
	const unsigned char utf16_be_bom[] = { 0xFE, 0xFF };
	const unsigned char utf32_le_bom[] = { 0x00, 0x00, 0xFE, 0xFF };
	const unsigned char utf32_be_bom[] = { 0xFF, 0xFe, 0x00, 0x00 };

	BomType DetectBom(std::byte const* bom, size_t bom_length) noexcept
	{
		if(bom_length >= std::size(utf8_bom) && std::memcmp(bom, utf8_bom, std::size(utf8_bom)) == 0)
			return BomType::UTF8;
		if (bom_length >= std::size(utf16_le_bom) && std::memcmp(bom, utf16_le_bom, std::size(utf16_le_bom)) == 0)
			return BomType::UTF16LE;
		if (bom_length >= std::size(utf32_le_bom) && std::memcmp(bom, utf32_le_bom, std::size(utf32_le_bom)) == 0)
			return BomType::UTF32LE;
		if (bom_length >= std::size(utf16_be_bom) && std::memcmp(bom, utf16_be_bom, std::size(utf16_be_bom)) == 0)
			return BomType::UTF16BE;
		if (bom_length >= std::size(utf32_be_bom) && std::memcmp(bom, utf32_be_bom, std::size(utf32_be_bom)) == 0)
			return BomType::UTF32BE;
		return BomType::UTF8_NoBom;
	}

	std::byte const* ToBinary(BomType type) noexcept
	{
		switch (type)
		{
		case BomType::UTF8: return reinterpret_cast<std::byte const*>(utf8_bom);
		case BomType::UTF16LE: return reinterpret_cast<std::byte const*>(utf16_le_bom);
		case BomType::UTF32LE: return reinterpret_cast<std::byte const*>(utf32_le_bom);
		case BomType::UTF16BE: return reinterpret_cast<std::byte const*>(utf16_be_bom);
		case BomType::UTF32BE: return reinterpret_cast<std::byte const*>(utf32_be_bom);
		default: return nullptr;
		}
	}

	DocumentWrapper::DocumentWrapper(std::byte const* code, size_t length)
	{
		if(code != nullptr && length != 0)
		{
			type = DetectBom(code, length);
			documenet = code;
			documenet_length = length;
			main_body = code + ToSize(type);
			main_body_length = length - ToSize(type);
		}
	}
}