#include "../Public/PotatoStrEncode.h"
#include <array>
namespace Potato::StrEncode
{

	EncodeInfo CoreEncoder<char32_t, char16_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		EncodeInfo Info;
		assert(!Source.empty());
		if (Source[0] <= 0x1000)
		{
			Info.TargetSpace += 1;
		}else
			Info.TargetSpace += 2;
		Info.SourceSpace += 1;
		return Info;
	}

	EncodeInfo CoreEncoder<char32_t, char16_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		EncodeInfo Info;
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

	EncodeInfo CoreEncoder<char32_t, char16_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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

	EncodeInfo CoreEncoder<char32_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
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
		return Info;
	}

	EncodeInfo CoreEncoder<char32_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
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
				return Info;
			Info.SourceSpace = 1;
		}
		return Info;
	}

	EncodeInfo CoreEncoder<char32_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if (!Source.empty())
		{
			if(Source[0] < 0x110000)
				return {1, 1};
		}
		return {0, 0};
	}

	EncodeInfo CoreEncoder<char32_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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

	EncodeInfo CoreEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		assert(!Source.empty());
		EncodeInfo Result;
		assert(Source.size() >= 1);
		auto Cur = Source[0];
		if ((Cur & 0xd800) == 0xd800 && Source.size() >= 2 && (Source[1] & 0xdc00) == 0xdc00)
		{
			return {2, 1};
		}else
			return {1, 1};
		return Result;
	}

	EncodeInfo CoreEncoder<char16_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		if(!Source.empty())
			return RequireSpaceOnceUnSafe(Source);
		else
			return {};
	}

	EncodeInfo CoreEncoder<char16_t, char32_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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

	EncodeInfo CoreEncoder<char16_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		auto Re = CoreEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(Source);
		assert(Re.SourceSpace != 0);
		if(Re.SourceSpace == 2)
			return {2, 4};
		else {
			auto Cur = Source[0];
			if(Cur <= 0x7f) return {1, 1};
			else if(Cur <= 0x7ff) return {1, 2};
			else if(Cur <= 0xfff) return {1, 3};
			else {
				assert(false);
				return {};
			}
		}
	}

	EncodeInfo CoreEncoder<char16_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		auto Re = CoreEncoder<char16_t, char32_t>::RequireSpaceOnceUnSafe(Source);
		if (Re.SourceSpace != 0)
		{
			if (Re.SourceSpace == 2)
				return { 2, 4 };
			else {
				auto Cur = Source[0];
				if (Cur <= 0x7f) return { 1, 1 };
				else if (Cur <= 0x7ff) return { 1, 2 };
				else if (Cur <= 0xfff) return { 1, 3 };
				else {
					return {};
				}
			}
		}
		return {};
	}

	EncodeInfo CoreEncoder<char16_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		std::array<char32_t, 1> Tem;
		auto Re1 = CoreEncoder<char16_t, char32_t>::EncodeOnceUnSafe(Source, Tem);
		auto Re2 = CoreEncoder<char32_t, char8_t>::EncodeOnceUnSafe(Tem, Target);
		return {Re1.SourceSpace, Re2.TargetSpace};
	}

	EncodeInfo CoreEncoder<char8_t, char16_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
	{
		auto Re = CoreEncoder<char8_t, char32_t>::RequireSpaceOnceUnSafe(Source);
		if(Re.SourceSpace == 4)
			Re.TargetSpace = 2;
		return Re;
	}

	EncodeInfo CoreEncoder<char8_t, char16_t>::RequireSpaceOnce(std::span<SourceT const> Source)
	{
		auto Re = CoreEncoder<char8_t, char32_t>::RequireSpaceOnce(Source);
		if(Re.SourceSpace == 4)
			Re.TargetSpace = 2;
		return Re;
	}

	EncodeInfo CoreEncoder<char8_t, char16_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
	{
		std::array<char32_t, 1> Tem;
		auto Re1 = CoreEncoder<char8_t, char32_t>::EncodeOnceUnSafe(Source, Tem);
		auto Re2 = CoreEncoder<char32_t, char16_t>::EncodeOnceUnSafe(Tem, Target);
		return { Re1.SourceSpace, Re2.TargetSpace };
	}

	EncodeInfo CoreEncoder<char8_t, char32_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
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

	EncodeInfo CoreEncoder<char8_t, char32_t>::RequireSpaceOnce(std::span<SourceT const> Source)
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
					return {Count, 1};
			}
		}
		return {0, 0};
	}

	EncodeInfo CoreEncoder<char8_t, char32_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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

	EncodeInfo CoreEncoder<char8_t, char8_t>::RequireSpaceOnceUnSafe(std::span<SourceT const> Source)
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

	EncodeInfo CoreEncoder<char8_t, char8_t>::RequireSpaceOnce(std::span<SourceT const> Source)
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
				if (Succeed)
					return { Count, Count };
			}
		}
		return { 0, 0 };
	}

	EncodeInfo CoreEncoder<char8_t, char8_t>::EncodeOnceUnSafe(std::span<SourceT const> Source, std::span<TargetT> Target)
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

	std::optional<std::size_t> ReadUtf8(std::ifstream& File, DocumenetBomT TargetBom, std::span<std::byte> Output)
	{
		std::array<char8_t, CoreEncoder<char8_t, char32_t>::MaxSourceBufferLength> Head;
		auto OldPos = File.tellg();
		std::size_t ReadCount = File.read(reinterpret_cast<char*>(Head.data()), Head.size() * sizeof(char8_t)).gcount();
		auto Span = std::span(Head).subspan(0, ReadCount);
		if (ReadCount == 0)
		{
			return 0;
		}
		switch (TargetBom)
		{
		case DocumenetBomT::NoBom:
		case DocumenetBomT::UTF8:
		{
			auto EncodeInfo = CoreEncoder<char8_t, char8_t>::RequireSpaceOnce(Span);
			if (EncodeInfo.SourceSpace != 0 && Output.size() / sizeof(char8_t) >= EncodeInfo.TargetSpace)
			{
				CoreEncoder<char8_t, char8_t>::EncodeOnceUnSafe(Head, 
					{reinterpret_cast<char8_t*>(Output.data()), Output.size() / sizeof(char8_t)}
				);
				File.seekg(std::size_t(OldPos) + EncodeInfo.SourceSpace, File.beg);
				return EncodeInfo.TargetSpace;
			}
			else {
				File.seekg(std::size_t(OldPos), File.beg);
				return {};
			}
		}
		case DocumenetBomT::UTF16BE:
		case DocumenetBomT::UTF16LE:
		{
			auto EncodeInfo = CoreEncoder<char8_t, char16_t>::RequireSpaceOnce(Span);
			if (EncodeInfo.SourceSpace != 0 && Output.size() / sizeof(char16_t) >= EncodeInfo.TargetSpace)
			{
				CoreEncoder<char8_t, char16_t>::EncodeOnceUnSafe(Head,
					{ reinterpret_cast<char16_t*>(Output.data()), Output.size() / sizeof(char16_t) }
				);
				File.seekg(std::size_t(OldPos) + EncodeInfo.SourceSpace, File.beg);
				return EncodeInfo.TargetSpace;
			}
			else {
				File.seekg(std::size_t(OldPos), File.beg);
				return {};
			}
		}
		case DocumenetBomT::UTF32BE:
		case DocumenetBomT::UTF32LE:
		{
			auto EncodeInfo = CoreEncoder<char8_t, char32_t>::RequireSpaceOnce(Span);
			if (EncodeInfo.SourceSpace != 0 && Output.size() / sizeof(char32_t) >= EncodeInfo.TargetSpace)
			{
				CoreEncoder<char8_t, char32_t>::EncodeOnceUnSafe(Head,
					{ reinterpret_cast<char32_t*>(Output.data()), Output.size() / sizeof(char32_t) }
				);
				File.seekg(std::size_t(OldPos) + EncodeInfo.SourceSpace, File.beg);
				return EncodeInfo.TargetSpace;
			}
			else {
				File.seekg(std::size_t(OldPos), File.beg);
				return {};
			}
		}
		default:
			File.seekg(std::size_t(OldPos), File.beg);
			return {};
		}
	}

	DocumentReader::DocumentReader(std::filesystem::path path)
		: File(path, std::ios::binary | std::ios::in)
	{
		bool R = File.good();
		if (File.is_open())
		{
			std::array<std::byte, 4> Buffer;
			File.seekg(0, File.beg);
			File.read(reinterpret_cast<char*>(Buffer.data()), Buffer.size());
			Bom = DetectBom(std::span(Buffer));
			auto BomSpan = ToBinary(Bom);
			TextOffset = BomSpan.size();
			File.seekg(TextOffset, File.beg);
		}
	}

	template<std::size_t Size>
	void ReversoByte(std::span<std::byte> Buffer)
	{
		while (Buffer.size() >= Size)
		{
			constexpr std::size_t MiddleIndex = Size / 2;
			for (std::size_t Index = 0; Index < MiddleIndex; ++Index)
			{
				std::swap(Buffer[Index], Buffer[Size - 1 - Index]);
			}
			Buffer = Buffer.subspan(Size);
		}
	}

	void DocumentReader::ResetIterator()
	{
		assert(*this);
		File.seekg(TextOffset, File.beg);
	}

	std::size_t DocumentReader::RecalculateLastSize()
	{
		auto Cur = File.tellg();
		File.seekg(0, File.end);
		auto End = File.tellg();
		File.seekg(Cur, File.beg);
		return static_cast<std::size_t>(End - Cur);
	}

	auto DocumentReader::Flush(DocumenetReaderWrapper& Reader) ->FlushResult
	{
		if (Reader.GetBom() == GetBom())
		{
			if (Reader.Available.Begin() != 0 && Reader.Available.Count() != 0 && Reader.Available.Count() <= Reader.Available.Begin())
			{
				std::memcpy(Reader.DocumentSpan.data(), Reader.DocumentSpan.data() + Reader.Available.Begin(), Reader.Available.Count());
				Reader.Available.Offset = 0;
			}

			auto OldPos = File.tellg();
			auto Readed = File.read(reinterpret_cast<char*>(Reader.DocumentSpan.data() + Reader.Available.End()), Reader.DocumentSpan.size() - Reader.Available.End()).gcount();

			std::span<std::byte> ReadedSpan = std::span(Reader.DocumentSpan).subspan(Reader.Available.End(), Readed);

			if (Readed != 0)
			{
				bool NeedReverso = IsNativeEndian(GetBom());
				switch (GetBom())
				{
				case DocumenetBomT::UTF16LE:
				case DocumenetBomT::UTF16BE:
				{
					if(NeedReverso)
						ReversoByte<2>(ReadedSpan);
					std::span<char16_t> Buffer{reinterpret_cast<char16_t*>(ReadedSpan.data()), ReadedSpan .size() / sizeof(char16_t)};
					EncodeStrInfo Info = StrCodeEncoder<char16_t, char16_t>::RequireSpace(Buffer);
					Reader.Available.Length += Info.SourceSpace * sizeof(char16_t);
					Reader.TotalCharacter += Info.CharacterCount;
					if (!Info)
					{
						OldPos += Info.SourceSpace;
						File.seekg(OldPos, File.beg);
						return FlushResult::BadString;
					}	
					else
						return FlushResult::Finish;
				}
				case DocumenetBomT::UTF32LE:
				case DocumenetBomT::UTF32BE:
				{
					if (NeedReverso)
						ReversoByte<4>(ReadedSpan);
					std::span<char32_t> Buffer{ reinterpret_cast<char32_t*>(ReadedSpan.data()), ReadedSpan.size() / sizeof(char32_t) };
					EncodeStrInfo Info = StrCodeEncoder<char32_t, char32_t>::RequireSpace(Buffer);
					Reader.Available.Length += Info.SourceSpace * sizeof(char32_t);
					Reader.TotalCharacter += Info.CharacterCount;
					if (!Info)
					{
						OldPos += Info.SourceSpace;
						File.seekg(OldPos, File.beg);
						return FlushResult::BadString;
					}	
					else
						return FlushResult::Finish;
				}
				default:
				{
					std::span<char8_t> Buffer{ reinterpret_cast<char8_t*>(ReadedSpan.data()), ReadedSpan.size() / sizeof(char8_t) };
					EncodeStrInfo Info = StrCodeEncoder<char8_t, char8_t>::RequireSpace(Buffer);
					Reader.Available.Length += Info.SourceSpace * sizeof(char8_t);
					Reader.TotalCharacter += Info.CharacterCount;
					if (!Info)
					{
						OldPos += Info.SourceSpace;
						File.seekg(OldPos, File.beg);
						if(Info.SourceSpace == 0)
							return FlushResult::BadString;
					}
					return FlushResult::Finish;
				}
				}
			}else
				return FlushResult::EndOfFile;
		}
		else {
			return FlushResult::UnSupport;
		}
	}

	template<typename UnocideT>
	EncodeStrInfo DecumentRequireSpaceUnsafe(std::basic_string_view<UnocideT> Str, DocumenetBomT Bom, bool WriteBom)
	{
		switch (Bom)
		{
		case DocumenetBomT::NoBom:
		case DocumenetBomT::UTF8:
		{
			auto Info = StrCodeEncoder<UnocideT, char8_t>::RequireSpaceUnSafe(Str);
			Info.TargetSpace = Info.TargetSpace * sizeof(char8_t);
			if (WriteBom)
			{
				Info.TargetSpace += ToBinary(Bom).size();
			}
			return Info;
		}
		}
		return {};
	}

	template<typename UnocideT>
	EncodeStrInfo DocumentEncodeUnsafe(std::span<std::byte> OutputBuffer, std::basic_string_view<UnocideT> Str, DocumenetBomT Bom, bool WriteBom = false)
	{
		if (WriteBom)
		{
			auto BomSpan = ToBinary(Bom);
			std::memcpy(OutputBuffer.data(), BomSpan.data(), BomSpan.size());
			OutputBuffer = OutputBuffer.subspan(BomSpan.size());
		}
		switch (Bom)
		{
		case DocumenetBomT::NoBom:
		case DocumenetBomT::UTF8:
		{
			std::span<char8_t> Buffer = {reinterpret_cast<char8_t*>(OutputBuffer.data()), OutputBuffer .size() / sizeof(char8_t)};
			auto Info = StrCodeEncoder<UnocideT, char8_t>::EncodeUnSafe(Str, Buffer);
			Info.TargetSpace = Info.TargetSpace * sizeof(char8_t);
			if (WriteBom)
			{
				Info.TargetSpace += ToBinary(Bom).size();
			}
			return Info;
		}
		}
		return {};
	}

	EncodeStrInfo DocumentEncoder::RequireSpaceUnsafe(std::u32string_view Str, DocumenetBomT Bom, bool WriteBom){ return DecumentRequireSpaceUnsafe(Str, Bom, WriteBom);}
	EncodeStrInfo DocumentEncoder::RequireSpaceUnsafe(std::u16string_view Str, DocumenetBomT Bom, bool WriteBom) { return DecumentRequireSpaceUnsafe(Str, Bom, WriteBom); }
	EncodeStrInfo DocumentEncoder::RequireSpaceUnsafe(std::u8string_view Str, DocumenetBomT Bom, bool WriteBom) { return DecumentRequireSpaceUnsafe(Str, Bom, WriteBom); }
	EncodeStrInfo DocumentEncoder::RequireSpaceUnsafe(std::wstring_view Str, DocumenetBomT Bom, bool WriteBom) { return DecumentRequireSpaceUnsafe(Str, Bom, WriteBom); }

	EncodeStrInfo DocumentEncoder::EncodeUnsafe(std::span<std::byte> Span, std::u32string_view Str, DocumenetBomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }
	EncodeStrInfo DocumentEncoder::EncodeUnsafe(std::span<std::byte> Span, std::u16string_view Str, DocumenetBomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }
	EncodeStrInfo DocumentEncoder::EncodeUnsafe(std::span<std::byte> Span, std::u8string_view Str, DocumenetBomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }
	EncodeStrInfo DocumentEncoder::EncodeUnsafe(std::span<std::byte> Span, std::wstring_view Str, DocumenetBomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }

	template<typename UnicodeT>
	EncodeStrInfo WriteFile(std::ofstream& File, DocumenetBomT Bom, std::vector<std::byte>& Buffer, std::basic_string_view<UnicodeT> Re)
	{
		auto Info = DocumentEncoder::RequireSpaceUnsafe(Re, Bom, false);
		Buffer.resize(Info.TargetSpace);
		auto Info2 = DocumentEncoder::EncodeUnsafe(Buffer, Re, Bom, false);
		File.write(reinterpret_cast<char const*>(Buffer.data()), Info2.TargetSpace / sizeof(char));
		Buffer.clear();
		return Info;
	}

	DocumenetWriter::DocumenetWriter(std::filesystem::path Path, DocumenetBomT BomType)
		: File(Path, std::ios::binary), BomType(BomType)
	{
		if (File.is_open())
		{
			auto BomSpan = ToBinary(BomType);
			if (!BomSpan.empty())
			{
				File.write(reinterpret_cast<char const*>(BomSpan.data()), BomSpan.size() / sizeof(char));
			}
		}
	}

	EncodeStrInfo DocumenetWriter::Write(std::u32string_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
	EncodeStrInfo DocumenetWriter::Write(std::u16string_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
	EncodeStrInfo DocumenetWriter::Write(std::u8string_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
	EncodeStrInfo DocumenetWriter::Write(std::wstring_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
}