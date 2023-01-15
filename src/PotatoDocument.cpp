#include "Potato/PotatoDocument.h"

namespace Potato::Document
{
	const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
	const unsigned char utf16_le_bom[] = { 0xFF, 0xFE };
	const unsigned char utf16_be_bom[] = { 0xFE, 0xFF };
	const unsigned char utf32_le_bom[] = { 0x00, 0x00, 0xFE, 0xFF };
	const unsigned char utf32_be_bom[] = { 0xFF, 0xFe, 0x00, 0x00 };

	BomT DetectBom(std::span<std::byte const> bom) noexcept {
		if (bom.size() >= std::size(utf8_bom) && std::memcmp(bom.data(), utf8_bom, std::size(utf8_bom)) == 0)
			return BomT::UTF8;
		if (bom.size() >= std::size(utf16_le_bom) && std::memcmp(bom.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
			return BomT::UTF16LE;
		if (bom.size() >= std::size(utf32_le_bom) && std::memcmp(bom.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
			return BomT::UTF32LE;
		if (bom.size() >= std::size(utf16_be_bom) && std::memcmp(bom.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
			return BomT::UTF16BE;
		if (bom.size() >= std::size(utf32_be_bom) && std::memcmp(bom.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
			return BomT::UTF32BE;
		return BomT::NoBom;
	}

	std::span<std::byte const> ToBinary(BomT type) noexcept
	{
		switch (type)
		{
		case BomT::UTF8: return { reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom) };
		case BomT::UTF16LE: return { reinterpret_cast<std::byte const*>(utf16_le_bom), std::size(utf16_le_bom) };
		case BomT::UTF32LE: return { reinterpret_cast<std::byte const*>(utf32_le_bom), std::size(utf32_le_bom) };
		case BomT::UTF16BE: return { reinterpret_cast<std::byte const*>(utf16_be_bom), std::size(utf16_be_bom) };
		case BomT::UTF32BE: return { reinterpret_cast<std::byte const*>(utf32_be_bom), std::size(utf32_be_bom) };
		default: return {};
		}
	}

	std::optional<std::size_t> ReadUtf8(std::ifstream& File, BomT TargetBom, std::span<std::byte> Output)
	{
		std::array<char8_t, StrEncode::CharEncoder<char8_t, char32_t>::MaxSourceBufferLength> Head;
		auto OldPos = File.tellg();
		std::size_t ReadCount = File.read(reinterpret_cast<char*>(Head.data()), Head.size() * sizeof(char8_t)).gcount();
		auto Span = std::span(Head).subspan(0, ReadCount);
		if (ReadCount == 0)
		{
			return 0;
		}
		switch (TargetBom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			auto EncodeInfo = StrEncode::CharEncoder<char8_t, char8_t>::RequireSpaceOnce(Span);
			if (EncodeInfo.SourceSpace != 0 && Output.size() / sizeof(char8_t) >= EncodeInfo.TargetSpace)
			{
				StrEncode::CharEncoder<char8_t, char8_t>::EncodeOnceUnSafe(Head,
					{ reinterpret_cast<char8_t*>(Output.data()), Output.size() / sizeof(char8_t) }
				);
				File.seekg(std::size_t(OldPos) + EncodeInfo.SourceSpace, File.beg);
				return EncodeInfo.TargetSpace;
			}
			else {
				File.seekg(std::size_t(OldPos), File.beg);
				return {};
			}
		}
		case BomT::UTF16BE:
		case BomT::UTF16LE:
		{
			auto EncodeInfo = StrEncode::CharEncoder<char8_t, char16_t>::RequireSpaceOnce(Span);
			if (EncodeInfo.SourceSpace != 0 && Output.size() / sizeof(char16_t) >= EncodeInfo.TargetSpace)
			{
				StrEncode::CharEncoder<char8_t, char16_t>::EncodeOnceUnSafe(Head,
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
		case BomT::UTF32BE:
		case BomT::UTF32LE:
		{
			auto EncodeInfo = StrEncode::CharEncoder<char8_t, char32_t>::RequireSpaceOnce(Span);
			if (EncodeInfo.SourceSpace != 0 && Output.size() / sizeof(char32_t) >= EncodeInfo.TargetSpace)
			{
				StrEncode::CharEncoder<char8_t, char32_t>::EncodeOnceUnSafe(Head,
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

	Reader::Reader(std::filesystem::path path)
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

	void Reader::ResetIterator()
	{
		assert(*this);
		File.seekg(TextOffset, File.beg);
	}

	std::size_t Reader::RecalculateLastSize()
	{
		auto Cur = File.tellg();
		File.seekg(0, File.end);
		auto End = File.tellg();
		File.seekg(Cur, File.beg);
		return static_cast<std::size_t>(End - Cur);
	}

	auto Reader::Flush(ReaderBuffer& Reader) ->FlushResult
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
				case BomT::UTF16LE:
				case BomT::UTF16BE:
				{
					if (NeedReverso)
						ReversoByte<2>(ReadedSpan);
					std::span<char16_t> Buffer{ reinterpret_cast<char16_t*>(ReadedSpan.data()), ReadedSpan.size() / sizeof(char16_t) };
					EncodeInfo Info = StrEncode::StrEncoder<char16_t, char16_t>::RequireSpace(Buffer);
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
				case BomT::UTF32LE:
				case BomT::UTF32BE:
				{
					if (NeedReverso)
						ReversoByte<4>(ReadedSpan);
					std::span<char32_t> Buffer{ reinterpret_cast<char32_t*>(ReadedSpan.data()), ReadedSpan.size() / sizeof(char32_t) };
					EncodeInfo Info = StrEncode::StrEncoder<char32_t, char32_t>::RequireSpace(Buffer);
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
					EncodeInfo Info = StrEncode::StrEncoder<char8_t, char8_t>::RequireSpace(Buffer);
					Reader.Available.Length += Info.SourceSpace * sizeof(char8_t);
					Reader.TotalCharacter += Info.CharacterCount;
					if (!Info)
					{
						OldPos += Info.SourceSpace;
						File.seekg(OldPos, File.beg);
						if (Info.SourceSpace == 0)
							return FlushResult::BadString;
					}
					return FlushResult::Finish;
				}
				}
			}
			else
				return FlushResult::EndOfFile;
		}
		else {
			return FlushResult::UnSupport;
		}
	}

	template<typename UnocideT>
	EncodeInfo DocumentRequireSpaceUnsafe(std::basic_string_view<UnocideT> Str, BomT Bom, bool WriteBom)
	{
		switch (Bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			auto Info = StrEncode::StrEncoder<UnocideT, char8_t>::RequireSpaceUnSafe(Str);
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
	EncodeInfo DocumentEncodeUnsafe(std::span<std::byte> OutputBuffer, std::basic_string_view<UnocideT> Str, BomT Bom, bool WriteBom = false)
	{
		if (WriteBom)
		{
			auto BomSpan = ToBinary(Bom);
			std::memcpy(OutputBuffer.data(), BomSpan.data(), BomSpan.size());
			OutputBuffer = OutputBuffer.subspan(BomSpan.size());
		}
		switch (Bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			std::span<char8_t> Buffer = { reinterpret_cast<char8_t*>(OutputBuffer.data()), OutputBuffer.size() / sizeof(char8_t) };
			auto Info = StrEncode::StrEncoder<UnocideT, char8_t>::EncodeUnSafe(Str, Buffer);
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

	ImmediateReader::ImmediateReader(std::filesystem::path Path) {
		Reader Reader(Path);
		if (Reader)
		{
			std::vector<std::byte> TempBuffer;
			TempBuffer.resize(Reader.RecalculateLastSize());
			auto Wrapper = Reader.CreateWrapper(std::span(TempBuffer));
			auto Re = Reader.Flush(Wrapper);
			if (Re == Reader::FlushResult::Finish)
			{
				Buffer = std::move(TempBuffer);
				Bom = Wrapper.GetBom();
			}
		}
	}

	std::optional<std::u8string_view> ImmediateReader::TryCastU8() const
	{
		if (Buffer.has_value() && (Bom == BomT::NoBom || Bom == BomT::UTF8))
		{
			std::u8string_view Re = {reinterpret_cast<char8_t const*>(Buffer->data()), Buffer->size()};
			return Re;
		}
		return {};
	}


	EncodeInfo Encoder::RequireSpaceUnsafe(std::u32string_view Str, BomT Bom, bool WriteBom) { return DocumentRequireSpaceUnsafe(Str, Bom, WriteBom); }
	EncodeInfo Encoder::RequireSpaceUnsafe(std::u16string_view Str, BomT Bom, bool WriteBom) { return DocumentRequireSpaceUnsafe(Str, Bom, WriteBom); }
	EncodeInfo Encoder::RequireSpaceUnsafe(std::u8string_view Str, BomT Bom, bool WriteBom) { return DocumentRequireSpaceUnsafe(Str, Bom, WriteBom); }
	EncodeInfo Encoder::RequireSpaceUnsafe(std::wstring_view Str, BomT Bom, bool WriteBom) { return DocumentRequireSpaceUnsafe(Str, Bom, WriteBom); }

	EncodeInfo Encoder::EncodeUnsafe(std::span<std::byte> Span, std::u32string_view Str, BomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }
	EncodeInfo Encoder::EncodeUnsafe(std::span<std::byte> Span, std::u16string_view Str, BomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }
	EncodeInfo Encoder::EncodeUnsafe(std::span<std::byte> Span, std::u8string_view Str, BomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }
	EncodeInfo Encoder::EncodeUnsafe(std::span<std::byte> Span, std::wstring_view Str, BomT Bom, bool WriteBom) { return DocumentEncodeUnsafe(Span, Str, Bom, WriteBom); }

	template<typename UnicodeT>
	EncodeInfo WriteFile(std::ofstream& File, BomT Bom, std::vector<std::byte>& Buffer, std::basic_string_view<UnicodeT> Re)
	{
		auto Info = Encoder::RequireSpaceUnsafe(Re, Bom, false);
		Buffer.resize(Info.TargetSpace);
		auto Info2 = Encoder::EncodeUnsafe(Buffer, Re, Bom, false);
		File.write(reinterpret_cast<char const*>(Buffer.data()), Info2.TargetSpace / sizeof(char));
		Buffer.clear();
		return Info;
	}

	Writer::Writer(std::filesystem::path Path, BomT BomType)
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

	EncodeInfo Writer::Write(std::u32string_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
	EncodeInfo Writer::Write(std::u16string_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
	EncodeInfo Writer::Write(std::u8string_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }
	EncodeInfo Writer::Write(std::wstring_view Str) { return WriteFile(File, BomType, TemporaryBuffer, Str); }


	auto LineSperater::Consume(bool KeepLineSymbol) ->Result
	{
		auto Index = IteStr.find(u8'\n');
		if (Index != std::u8string_view::npos)
		{
			Result Re;
			Re.Str = IteStr.substr(0, Index + 1);
			IteStr = IteStr.substr(Index + 1);
			std::size_t CutSize = 1;
			if (Re.Str.size() >= 2 && Re.Str[Re.Str.size() - 2] == u8'\r')
			{
				CutSize = 2;
				Re.Mode = LineMode::RN;
			}
			else {
				Re.Mode = LineMode::N;
			}
			if(!KeepLineSymbol)
				Re.Str = Re.Str.substr(0, Re.Str.size() - CutSize);
			return Re;
		}
		else {
			Result Re;
			Re.Str = IteStr;
			IteStr = {};
			return Re;
		}
	}
}