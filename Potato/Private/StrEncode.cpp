#include "../Public/StrEncode.h"
namespace Potato::StrEncode
{
	
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

	size_t CharWrapper<char8_t>::RequireSpace(Type first_char)
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
	
	size_t CharWrapper<char8_t>::DetectOne(std::basic_string_view<Type> input)
	{
		if(!input.empty())
		{
			auto rs = RequireSpace(input[0]);
			if(input.size() >= rs && CheckUTF8(input.data() + 1, rs - 1))
				return rs;
		}
		return 0;
	}

	DecodeResult CharWrapper<char8_t>::DecodeOne(std::basic_string_view<Type> input)
	{
		assert(!input.empty());
		auto rs = RequireSpace(input[0]);
		assert(input.size() >= rs);
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

	size_t CharWrapper<char8_t>::EncodeOne(char32_t temporary, std::span<Type> output)
	{
		assert(!output.empty());
		auto rr = EncodeRequest(temporary);
		assert(output.size() >= rr);
		switch (rr)
		{
		case 1:
		{
			output[0] = static_cast<char8_t>(temporary & 0x0000007F);
		}break;
		case 2:
		{
			output[0] = 0xC0 | static_cast<char>((temporary & 0x07C0) >> 6);
			output[1] = 0x80 | static_cast<char>((temporary & 0x3F));
		}break;
		case 3:
		{
			output[0] = 0xE0 | static_cast<char>((temporary & 0xF000) >> 12);
			output[1] = 0x80 | static_cast<char>((temporary & 0xFC0) >> 6);
			output[2] = 0x80 | static_cast<char>((temporary & 0x3F));
		}break;
		case 4:
		{
			output[0] = 0x1E | static_cast<char>((temporary & 0x1C0000) >> 18);
			output[1] = 0x80 | static_cast<char>((temporary & 0x3F000) >> 12);
			output[2] = 0x80 | static_cast<char>((temporary & 0xFC0) >> 6);
			output[3] = 0x80 | static_cast<char>((temporary & 0x3F));
		}break;
		default: break;
		}
		return rr;
	}

	size_t CharWrapper<char16_t>::RequireSpace(Type input)
	{
		if ((input & 0xD800) == 0xD800)
			return 2;
		return 1;
	}

	size_t CharWrapper<char16_t>::DetectOne(std::basic_string_view<Type> input)
	{
		if(!input.empty())
		{
			if((input[0] & 0xD800) == 0xD800)
			{
				if(input.size() >= 2 && (input[1] & 0xDC00) == 0xDC00)
				{
					return 2;
				}else
					return 0;
			}
			return 1;
		}
		return 0;
	}

	DecodeResult CharWrapper<char16_t>::DecodeOne(std::basic_string_view<Type> input)
	{
		assert(!input.empty());
		size_t rs  = 0;
		if ((input[0] & 0xD800) == 0xD800)
			rs = 2;
		else
			rs = 1;
		assert(input.size() >= rs);
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

	size_t CharWrapper<char16_t>::EncodeOne(char32_t temporary, std::span<Type> output)
	{
		assert(!output.empty());
		size_t rs = EncodeRequest(temporary);
		assert(output.size() >= rs);
		switch (rs)
		{
		case 1: output[0] = static_cast<char16_t>(temporary); return 1;
		case 2:
		{
			char32_t tar = temporary - 0x10000;
			output[0] = static_cast<char16_t>((tar & 0xFFC00) >> 10) + 0xD800;
			output[1] = static_cast<char16_t>((tar & 0x3FF)) + 0xDC00;
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

	size_t CharWrapper<ReverseEndianness<char16_t>>::RequireSpace(Type Input)
	{
		Reverser(&Input, sizeof(char16_t), 1);
		return wrapper.RequireSpace(Input);
	}

	size_t CharWrapper<ReverseEndianness<char16_t>>::DetectOne(std::basic_string_view<Type> input)
	{
		// todo
		if(!input.empty())
		{
			char16_t temporary[2];
			auto size = std::min(std::size(temporary), input.size());
			std::memcpy(temporary, input.data(), size * sizeof(char16_t));
			Reverser(temporary, sizeof(char16_t), size);
			return wrapper.DetectOne({temporary, size});
		}
		return 0;
	}

	DecodeResult CharWrapper<ReverseEndianness<char16_t>>::DecodeOne(std::basic_string_view<Type> input)
	{
		// todo
		char16_t temporary[2];
		auto size = std::min(std::size(temporary), input.size());
		std::memcpy(temporary, input.data(), size * sizeof(char16_t));
		Reverser(temporary, sizeof(char16_t), size);
		return wrapper.DecodeOne({temporary, size});
	}

	size_t CharWrapper<ReverseEndianness<char16_t>>::EncodeOne(char32_t temporary, std::span<Type> output)
	{
		// todo
		if(!output.empty())
		{
			char16_t temporary_buffer[2];
			auto size = std::min(std::size(temporary_buffer), output.size());
			auto re = wrapper.EncodeOne(temporary, {temporary_buffer, size});
			Reverser(temporary_buffer, sizeof(char16_t), re);
			std::memcpy(output.data(), temporary_buffer, sizeof(char16_t) * re);
			return re;
		}
		return 0;
	}

	size_t CharWrapper<ReverseEndianness<char32_t>>::DetectOne(std::basic_string_view<Type> input)
	{
		// todo
		if (!input.empty())
		{
			char32_t temporary;
			auto size = std::min(static_cast<size_t>(1), input.size());
			std::memcpy(&temporary, input.data(), size * sizeof(char32_t));
			Reverser(&temporary, sizeof(char32_t), size);
			return wrapper.DetectOne({&temporary, size});
		}
		return 0;
	}

	DecodeResult CharWrapper<ReverseEndianness<char32_t>>::DecodeOne(std::basic_string_view<Type> input)
	{
		// todo
		char32_t temporary;
		auto size = std::min(static_cast<size_t>(1), input.size());
		std::memcpy(&temporary, input.data(), size * sizeof(char32_t));
		Reverser(&temporary, sizeof(char32_t), size);
		return wrapper.DecodeOne({&temporary, size});
	}

	size_t CharWrapper<ReverseEndianness<char32_t>>::EncodeOne(char32_t temporary, std::span<Type> output)
	{
		// todo
		if (!output.empty())
		{
			char32_t temporary_buffer;
			auto size = std::min(static_cast<size_t>(1), output.size());
			auto re = wrapper.EncodeOne(temporary, {&temporary_buffer, size});
			Reverser(&temporary_buffer, sizeof(char32_t), re);
			std::memcpy(output.data(), &temporary_buffer, sizeof(char32_t) * re);
			return re;
		}
		return 0;
	}
	

	const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
	const unsigned char utf16_le_bom[] = { 0xFF, 0xFE };
	const unsigned char utf16_be_bom[] = { 0xFE, 0xFF };
	const unsigned char utf32_le_bom[] = { 0x00, 0x00, 0xFE, 0xFF };
	const unsigned char utf32_be_bom[] = { 0xFF, 0xFe, 0x00, 0x00 };

	BomType DetectBom(std::span<std::byte const> bom) noexcept
	{
		if(bom.size() >= std::size(utf8_bom) && std::memcmp(bom.data(), utf8_bom, std::size(utf8_bom)) == 0)
			return BomType::UTF8;
		if (bom.size() >= std::size(utf16_le_bom) && std::memcmp(bom.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
			return BomType::UTF16LE;
		if (bom.size() >= std::size(utf32_le_bom) && std::memcmp(bom.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
			return BomType::UTF32LE;
		if (bom.size() >= std::size(utf16_be_bom) && std::memcmp(bom.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
			return BomType::UTF16BE;
		if (bom.size() >= std::size(utf32_be_bom) && std::memcmp(bom.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
			return BomType::UTF32BE;
		return BomType::UTF8_NoBom;
	}

	std::span<std::byte const> ToBinary(BomType type) noexcept
	{
		switch (type)
		{
		case BomType::UTF8: return { reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom)};
		case BomType::UTF16LE: return { reinterpret_cast<std::byte const*>(utf16_le_bom), std::size(utf16_le_bom) };
		case BomType::UTF32LE: return { reinterpret_cast<std::byte const*>(utf32_le_bom), std::size(utf32_le_bom) };
		case BomType::UTF16BE: return { reinterpret_cast<std::byte const*>(utf16_be_bom), std::size(utf16_be_bom) };
		case BomType::UTF32BE: return { reinterpret_cast<std::byte const*>(utf32_be_bom), std::size(utf32_be_bom) };
		default: return {};
		}
	}

	std::vector<std::u32string_view> DocumentWrapper::SperateWithLineEnding(std::u32string_view source, bool KeepLineEnding)
	{
		std::vector<std::u32string_view> result;
		auto StartIte = source.begin();
		std::optional<decltype(source.begin())> LineEndIte;
		for (auto Ite = source.begin(); Ite != source.end(); ++Ite)
		{
			if (*Ite == U'\r')
			{
				LineEndIte = Ite;
			}
			else if (*Ite == U'\n')
			{
				if(!LineEndIte.has_value())
					LineEndIte = Ite;
				result.push_back({ StartIte, !KeepLineEnding ? *LineEndIte : Ite + 1});
				StartIte = Ite + 1;
				LineEndIte = {};
			}
			else {
				LineEndIte = {};
			}
		}
		if (StartIte != source.end())
			result.push_back({ StartIte, source.end() });
		return result;
	}

	DocumentWrapper::DocumentWrapper(std::span<std::byte const> input)
	{
		if(!input.empty())
		{
			type = DetectBom(input);
			document = input;
			document_without_bom = input.subspan(ToBinary(type).size());
		}
	}

	HugeDocumenetReader::HugeDocumenetReader(std::u32string_view Path)
	{
		auto path_str = AsWrapper(Path).ToString<wchar_t>();
		fstream.open(path_str,  std::ios::binary);
		if (fstream.is_open())
		{
			std::byte Bom[4];
			fstream.read(reinterpret_cast<char*>(Bom), sizeof(std::byte) * 4);
			bom_type = DetectBom(Bom);
			fstream.seekg(ToBinary(bom_type).size(), std::ios_base::beg);
		}
	}

	template<typename Storage, size_t size, typename Wrapper>
	std::optional<char32_t> Read(std::ifstream& fstream, Wrapper wrapper)
	{
		Storage Data[size];
		fstream.read(reinterpret_cast<char*>(Data), sizeof(Storage));
		auto re_size = wrapper.RequireSpace(Data[0]);
		if (re_size > 0)
		{
			assert(re_size <= size);
			fstream.read(reinterpret_cast<char*>(Data + 1), (re_size - 1) * sizeof(Storage));
			return wrapper.DecodeOne({ Data, re_size }).temporary;
		}
		return {};
	}

	std::optional<char32_t> HugeDocumenetReader::ReadOne()
	{
		if (fstream.good())
		{
			switch (bom_type)
			{
			case Potato::StrEncode::BomType::UTF8_NoBom:
			case Potato::StrEncode::BomType::UTF8:
				return Read<char8_t, 8>(fstream, StrEncode::CharWrapper<char8_t>{});
			case Potato::StrEncode::BomType::UTF16LE:
			case Potato::StrEncode::BomType::UTF16BE:
			{
				auto P = DefaultBom<char16_t>();
				if (bom_type == P)
				{
					return Read<char16_t, 2>(fstream, StrEncode::CharWrapper<char16_t>{});
				}
				else {
					return Read<char16_t, 2>(fstream, StrEncode::CharWrapper<StrEncode::ReverseEndianness<char16_t>>{});
				}
			}
			case Potato::StrEncode::BomType::UTF32LE:
			case Potato::StrEncode::BomType::UTF32BE:
			{
				auto P = DefaultBom<char32_t>();
				if (bom_type == P)
				{
					return Read<char32_t, 1>(fstream, StrEncode::CharWrapper<char32_t>{});
				}
				else {
					return Read<char32_t, 1>(fstream, StrEncode::CharWrapper<StrEncode::ReverseEndianness<char32_t>>{});
				}
			}
			default:
				break;
			}
		}
		return {};
	}

	std::optional<std::u32string> HugeDocumenetReader::ReadLine(bool KeepLine)
	{
		if (fstream.good())
		{
			std::u32string result;
			bool has_special = 0;
			while (true)
			{
				auto cur = ReadOne();
				if (cur)
				{
					if(*cur == U'\r' && KeepLine)
						result.push_back(*cur);
					else if (*cur == U'\n')
					{
						if(KeepLine)
							result.push_back(*cur);
						return result;
					}else
						result.push_back(*cur);
				}
				else
					break;
			}
			return result;
		}
		return {};
	}
}