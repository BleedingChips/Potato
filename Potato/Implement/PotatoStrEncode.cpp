#include "../Include/PotatoStrEncode.h"
#include <array>
namespace Potato::StrEncode
{

	/*
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
		auto path_str = AsStrWrapper(Path).ToString<wchar_t>();
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
			case BomType::UTF8_NoBom:
			case BomType::UTF8:
				return Read<char8_t, 8>(fstream, CharWrapper<char8_t>{});
			case BomType::UTF16LE:
			case BomType::UTF16BE:
			{
				auto P = DefaultBom<char16_t>();
				if (bom_type == P)
				{
					return Read<char16_t, 2>(fstream, CharWrapper<char16_t>{});
				}
				else {
					return Read<char16_t, 2>(fstream, CharWrapper<ReverseEndianness<char16_t>>{});
				}
			}
			case BomType::UTF32LE:
			case BomType::UTF32BE:
			{
				auto P = DefaultBom<char32_t>();
				if (bom_type == P)
				{
					return Read<char32_t, 1>(fstream, CharWrapper<char32_t>{});
				}
				else {
					return Read<char32_t, 1>(fstream, CharWrapper<ReverseEndianness<char32_t>>{});
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
	*/

	StrInfo CoreEncoder<char32_t, char16_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		StrInfo Info;
		assert(!Source.empty());
		if (Source[0] <= 0x1000)
		{
			Info.TargetSpace += 1;
		}else
			Info.TargetSpace += 2;
		Info.SourceSpace += 1;
		return Info;
	}

	StrInfo CoreEncoder<char32_t, char16_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		StrInfo Info;
		if (!Source.empty())
		{
			auto Ite = Source[0];
			if (Ite <= 0x01000)
				Info.TargetSpace = 1;
			else if (Ite < 0x110000)
				Info.TargetSpace = 2;
			else
				return Info;
			Info.SourceSpace = 1;
		}
		return Info;
	}

	StrInfo CoreEncoder<char32_t, char16_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		auto Info = RequireSpaceOnceUnSafe(Source);
		switch (Info.TargetSpace)
		{
			case 1:
				assert(Target.size() >= 1);
				Target[0] = static_cast<char16_t>(Source[0]);
			break;
			case 2:
				assert(Target.size() >= 2);
				Target[0] = (0xd800 & (Source[0] >> 10));
				Target[1] = (0xdc00 & (Source[0] >> 10));
			break;
			default:
				assert(false);
			break;
		}
		return Info;
	}

	StrInfo CoreEncoder<char32_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		StrInfo Info;
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
		return Info;
	}

	StrInfo CoreEncoder<char32_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		StrInfo Info;
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
				return Info;
			Info.SourceSpace = 1;
		}
		return Info;
	}

	StrInfo CoreEncoder<char32_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if (!Source.empty())
		{
			if(Source[0] < 0x110000)
				return {1, 1};
		}
		return {0, 0};
	}

	StrInfo CoreEncoder<char32_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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
			Target[0] = 0x1E | static_cast<char>((Cur & 0x1C0000) >> 18);
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

	StrInfo CoreEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		assert(!Source.empty());
		StrInfo Result;
		assert(Source.size() >= 1);
		auto Cur = Source[0];
		if ((Cur & 0xd800) == 0xd800 && Source.size() >= 2 && (Source[1] & 0xdc00) == 0xdc00)
		{
			return {2, 1};
		}else
			return {1, 1};
		return Result;
	}

	StrInfo CoreEncoder<char16_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if(!Source.empty())
			return RequireSpaceOnceUnSafe(Source);
		else
			return {};
	}

	StrInfo CoreEncoder<char16_t, char32_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		assert(!Source.empty());
		assert(Target.size() >= 1);
		auto Cur = Source[0];
		if ((Cur & 0xd800) == 0xd800 && Source.size() >= 2 && (Source[1] & 0xdc00) == 0xdc00)
		{
			Target[0] = (static_cast<char32_t>(Cur | 0x3FF) << 10) + (Source[1] | 0x3FF) + 0x10000;
			return {2, 1};
		}
		else {
			Target[0] = Cur;
			return { 1, 1 };
		}
	}

	StrInfo CoreEncoder<char8_t, char32_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		assert(!Source.empty());
		auto Cur = Source[0];
		if ((Cur & 0x80) == 0)
			return {1, 1};
		else if((Cur & 0xE0) == 0xC0)
			return {2, 1};
		else if((Cur & 0xF0) == 0xE0)
			return {3, 1};
		else
			return {4, 1};
	}

	StrInfo CoreEncoder<char8_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
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
			if (Count < Source.size() && Count != 0)
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
					return {Count, 1};
			}
		}
		return {0, 0};
	}

	StrInfo CoreEncoder<char8_t, char32_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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

	StrInfo CoreEncoder<char8_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		assert(!Source.empty());
		auto Cur = Source[0];
		if ((Cur & 0x80) == 0)
			return { 1, 1 };
		else if ((Cur & 0xE0) == 0xC0)
			return { 2, 2 };
		else if ((Cur & 0xF0) == 0xE0)
			return { 3, 3 };
		else
			return { 4, 4 };
	}

	StrInfo CoreEncoder<char8_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
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
			else if ((Cur & 0xF8) == 0xF0)
				Count = 4;
			if (Count < Source.size() && Count != 0)
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
				if (Succeed)
					return { Count, Count };
			}
		}
		return { 0, 0 };
	}

	StrInfo CoreEncoder<char8_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		auto Info = RequireSpaceOnceUnSafe(Source);
		std::memcpy(Target.data(), Source.data(), Info.SourceSpace * sizeof(SourceT));
		return Info;
	}


	const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
	const unsigned char utf16_le_bom[] = { 0xFF, 0xFE };
	const unsigned char utf16_be_bom[] = { 0xFE, 0xFF };
	const unsigned char utf32_le_bom[] = { 0x00, 0x00, 0xFE, 0xFF };
	const unsigned char utf32_be_bom[] = { 0xFF, 0xFe, 0x00, 0x00 };

	DocumenetBomT DetectBom(std::span<std::byte const> bom) noexcept {
		if (bom.size() >= std::size(utf8_bom) && std::memcmp(bom.data(), utf8_bom, std::size(utf8_bom)) == 0)
			return DocumenetBomT::UTF8;
		if (bom.size() >= std::size(utf16_le_bom) && std::memcmp(bom.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
			return DocumenetBomT::UTF16LE;
		if (bom.size() >= std::size(utf32_le_bom) && std::memcmp(bom.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
			return DocumenetBomT::UTF32LE;
		if (bom.size() >= std::size(utf16_be_bom) && std::memcmp(bom.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
			return DocumenetBomT::UTF16BE;
		if (bom.size() >= std::size(utf32_be_bom) && std::memcmp(bom.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
			return DocumenetBomT::UTF32BE;
		return DocumenetBomT::NoBom;
	}

	std::span<std::byte const> ToBinary(DocumenetBomT type) noexcept 
	{
		switch (type)
		{
		case DocumenetBomT::UTF8: return { reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom) };
		case DocumenetBomT::UTF16LE: return { reinterpret_cast<std::byte const*>(utf16_le_bom), std::size(utf16_le_bom) };
		case DocumenetBomT::UTF32LE: return { reinterpret_cast<std::byte const*>(utf32_le_bom), std::size(utf32_le_bom) };
		case DocumenetBomT::UTF16BE: return { reinterpret_cast<std::byte const*>(utf16_be_bom), std::size(utf16_be_bom) };
		case DocumenetBomT::UTF32BE: return { reinterpret_cast<std::byte const*>(utf32_be_bom), std::size(utf32_be_bom) };
		default: return {};
		}
	}

	DocumentReaderWrapper::DocumentReaderWrapper(std::filesystem::path path)
		: File(path, std::ios::binary)
	{
		if (File.is_open())
		{
			File.seekg(0, File.end);
			std::size_t FileSize = File.tellg();
			auto ReadBuffer = std::min(FileSize, std::size_t(4));
			std::array<std::byte, 4> Buffer;
			File.read(reinterpret_cast<char*>(Buffer.size()), FileSize);
			File.seekg(0, File.beg);
			Type = DetectBom(std::span(Buffer));
			auto Bom = ToBinary(Type);
			TextOffset = Bom.size();
			File.seekg(TextOffset, File.beg);
		}
	}

	std::size_t DocumentReaderWrapper::ReadSingle(std::span<std::byte> Output)
	{
		switch (Type)
		{
		case Potato::StrEncode::DocumenetBomT::NoBom:
		case Potato::StrEncode::DocumenetBomT::UTF8:
		{
			assert(Output.size() >= 6 * sizeof(char8_t));
			auto OldIte = File.tellg();
			File.read(reinterpret_cast<char*>(Output.data()), 6 * sizeof(char8_t));
			std::span<char8_t const> Tem(reinterpret_cast<char8_t*>(Output.data()), 6);
			auto Index = CoreEncoder<char8_t, char8_t>::RequireSpaceOnce(Tem.subspan(0, 6 * sizeof(char8_t)));
			break;
		}
		case Potato::StrEncode::DocumenetBomT::UTF16LE:
			break;
		case Potato::StrEncode::DocumenetBomT::UTF16BE:
			break;
		case Potato::StrEncode::DocumenetBomT::UTF32LE:
			break;
		case Potato::StrEncode::DocumenetBomT::UTF32BE:
			break;
		default:
			break;
		}
		return 0;
	}


	DocumenetBinaryWrapper::DocumenetBinaryWrapper(std::span<std::byte const> Documenet)
		: TotalBinary(Documenet)
	{
		Type = DetectBom(Documenet);
		Text = TotalBinary.subspan(ToBinary(Type).size());
	}

	auto DocumenetBinaryWrapper::PreCalculateSpaceUnSafe(DocumenetBomT SourceBomT, DocumenetBomT TargetBom, std::span<std::byte const> Source, std::size_t CharacterCount) ->StrEncodeInfo
	{
		// todo
		if (IsNativeEndian(SourceBomT) && IsNativeEndian(TargetBom))
		{
			switch (SourceBomT)
			{
			case DocumenetBomT::UTF32LE:
			case DocumenetBomT::UTF32BE:
			{
				assert(Source.size() % sizeof(char32_t) == 0);
				std::span<char32_t const> Buffer{reinterpret_cast<char32_t const*>(Source.data()), Source.size() / sizeof(char32_t)};
				switch (TargetBom)
				{
				case DocumenetBomT::UTF32LE:
				case DocumenetBomT::UTF32BE:
				{
					std::size_t Min = std::min(Buffer.size(), CharacterCount) * sizeof(char32_t);
					return {Min, Min, Min};
				}
				case DocumenetBomT::UTF8:
				case DocumenetBomT::NoBom:
				{
					auto Result = StrCodeEncoder<char32_t, char8_t>::RequireSpaceUnSafe(Buffer, CharacterCount);
					Result.SourceSpace *= sizeof(char32_t);
					Result.TargetSpace *= sizeof(char8_t);
					return Result;
				}
				};
				break;
			}
			case DocumenetBomT::NoBom:
			case DocumenetBomT::UTF8:
			{
				assert(Source.size() % sizeof(char8_t) == 0);
				std::span<char8_t const> Buffer{ reinterpret_cast<char8_t const*>(Source.data()), Source.size() / sizeof(char8_t) };
				switch (TargetBom)
				{
				case DocumenetBomT::UTF32LE:
				case DocumenetBomT::UTF32BE:
				{
					auto Result = StrCodeEncoder<char8_t, char32_t>::RequireSpaceUnSafe(Buffer, CharacterCount);
					Result.TargetSpace *= sizeof(char32_t);
					Result.SourceSpace *= sizeof(char8_t);
					return Result;
				}
				}
				break;
			}
			default:
			{
				break;
			}
			}
		};
		return {};
	}

	StrEncodeInfo DocumenetBinaryWrapper::TranslateUnSafe(DocumenetBomT SourceBomT, std::span<std::byte const> Source, DocumenetBomT TargetBom, std::span<std::byte> Target, std::size_t MaxCharacter)
	{
		// todo
		if (IsNativeEndian(SourceBomT) && IsNativeEndian(TargetBom))
		{
			switch (SourceBomT)
			{
			case DocumenetBomT::UTF32LE:
			case DocumenetBomT::UTF32BE:
			{
				assert(Source.size() % sizeof(char32_t) == 0);
				std::span<char32_t const> Buffer{ reinterpret_cast<char32_t const*>(Source.data()), Source.size() / sizeof(char32_t) };
				switch (TargetBom)
				{
				case DocumenetBomT::UTF32LE:
				case DocumenetBomT::UTF32BE:
				{
					std::size_t Min = std::min(MaxCharacter, Buffer.size()) * sizeof(char32_t);
					std::memcpy(Target.data(), Source.data(), Min);
					return {Min, Min, Min};
				}
				case DocumenetBomT::UTF8:
				case DocumenetBomT::NoBom:
				{
					std::span<char8_t> TemBuffer{ reinterpret_cast<char8_t*>(Target.data()), Target.size() / sizeof(char8_t) };
					auto Result = StrCodeEncoder<char32_t, char8_t>::EncodeUnSafe(Buffer, TemBuffer, MaxCharacter);
					Result.TargetSpace *= sizeof(char32_t);
					Result.SourceSpace *= sizeof(char8_t);
					return Result;
				}
				};
			}
			case DocumenetBomT::NoBom:
			case DocumenetBomT::UTF8:
			{
				assert(Source.size() % sizeof(char8_t) == 0);
				std::span<char8_t const> Buffer{ reinterpret_cast<char8_t const*>(Source.data()), Source.size() / sizeof(char8_t) };
				switch (TargetBom)
				{
				case DocumenetBomT::UTF32LE:
				case DocumenetBomT::UTF32BE:
				{
					std::span<char32_t> TemBuffer{ reinterpret_cast<char32_t*>(Target.data()), Target.size() / sizeof(char8_t) };
					auto Result = StrCodeEncoder<char8_t, char32_t>::EncodeUnSafe(Buffer, TemBuffer, MaxCharacter);
					Result.TargetSpace *= sizeof(char8_t);
					Result.SourceSpace *= sizeof(char32_t);
					return Result;
				}
				}
			}
			default:
				break;
			}
		};
		return {};
	}
}