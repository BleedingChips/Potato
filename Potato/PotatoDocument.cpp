module;

#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#endif

module PotatoDocument;

namespace Potato::Document
{
	constexpr unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
	constexpr unsigned char utf16_le_bom[] = { 0xFF, 0xFE };
	constexpr unsigned char utf16_be_bom[] = { 0xFE, 0xFF };
	constexpr unsigned char utf32_le_bom[] = { 0x00, 0x00, 0xFE, 0xFF };
	constexpr unsigned char utf32_be_bom[] = { 0xFF, 0xFe, 0x00, 0x00 };

	std::span<std::byte> BinaryStreamReader::Read(std::span<std::byte> output)
	{
#ifdef _WIN32
		DWORD readed = 0;
		if(ReadFile(
			file, output.data(), output.size() * sizeof(std::byte), &readed, nullptr
		))
		{
			std::size_t total_count = readed;
			return output.subspan(0, readed / sizeof(std::byte));
		}
#endif
		return {};
	}

	BinaryStreamReader::operator bool() const
	{
#ifdef _WIN32
		return file != INVALID_HANDLE_VALUE;
#endif
	}

	bool BinaryStreamReader::Open(std::filesystem::path const& path)
	{
#ifdef _WIN32
		Close();
		file = ::CreateFileW(
			path.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		return file != INVALID_HANDLE_VALUE;
#endif
	}

	BinaryStreamReader::~BinaryStreamReader()
	{
		Close();
	}

	void BinaryStreamReader::Close()
	{
#ifdef _WIN32
		::CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
#endif
	}

	BinaryStreamReader::BinaryStreamReader(BinaryStreamReader&& reader)
	{
#ifdef _WIN32
		file = reader.file;
		reader.file = INVALID_HANDLE_VALUE;
#endif
	}

	std::uint64_t BinaryStreamReader::GetStreamSize() const
	{
#ifdef _WIN32
		std::int64_t result = 0;
		::GetFileSizeEx(file, reinterpret_cast<PLARGE_INTEGER>(&result));
		return static_cast<std::uint64_t>(result);
#endif
	}

	std::optional<std::int64_t> BinaryStreamReader::SetPointerOffsetFromBegin(std::int64_t offset)
	{
#ifdef _WIN32
		std::int64_t new_position = 0;
		if(offset >= 0)
		{
			if(::SetFilePointerEx(
				file,
				*reinterpret_cast<LARGE_INTEGER*>(&offset),
				reinterpret_cast<LARGE_INTEGER*>(&new_position),
				FILE_BEGIN
			))
			{
				return new_position;
			}
		}
		return std::nullopt;
#endif
	}

	std::optional<std::int64_t> BinaryStreamReader::SetPointerOffsetFromEnd(std::int64_t offset)
	{
#ifdef _WIN32
		std::int64_t new_position = 0;
		if(::SetFilePointerEx(
			file,
			*reinterpret_cast<LARGE_INTEGER*>(&offset),
			reinterpret_cast<LARGE_INTEGER*>(&new_position),
			FILE_END
		))
		{
			return new_position;
		}
		return std::nullopt;
#endif
	}

	std::optional<std::int64_t> BinaryStreamReader::SetPointerOffsetFromCurrent(std::int64_t offset)
	{
#ifdef _WIN32
		std::int64_t new_position = 0;
		if(::SetFilePointerEx(
			file,
			*reinterpret_cast<LARGE_INTEGER*>(&offset),
			reinterpret_cast<LARGE_INTEGER*>(&new_position),
			FILE_CURRENT
		))
		{
			return new_position;
		}
		return std::nullopt;
#endif
	}

	BinaryStreamWriter::BinaryStreamWriter(BinaryStreamWriter&& reader)
	{
#ifdef _WIN32
		::CloseHandle(file);
		file = reader.file;
		reader.file = INVALID_HANDLE_VALUE;
#endif
	}

	BinaryStreamWriter::operator bool() const
	{
#ifdef _WIN32
		return file != INVALID_HANDLE_VALUE;
#endif
	}

	bool BinaryStreamWriter::Open(std::filesystem::path const& path, OpenMode mode)
	{
#ifdef _WIN32
		Close();
		DWORD Mode = 0;
		switch(mode)
		{
		case OpenMode::APPEND:
			Mode = OPEN_EXISTING;
			break;
		case OpenMode::CREATE:
			Mode = CREATE_NEW;
			break;
		case OpenMode::APPEND_OR_CREATE:
			Mode = OPEN_ALWAYS;
			break;
		case OpenMode::CREATE_OR_EMPTY:
			Mode = CREATE_ALWAYS;
			break;
		case OpenMode::EMPTY:
			Mode = TRUNCATE_EXISTING;
			break;
		}
		file = ::CreateFileW(
			path.c_str(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			Mode,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		return file != INVALID_HANDLE_VALUE;
#endif
	}
		


	void BinaryStreamWriter::Close()
	{
#ifdef _WIN32
		::CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
#endif
	}

	bool BinaryStreamWriter::Write(std::byte const* buffer, std::size_t size)
	{
#ifdef _WIN32
		DWORD writed;
		return WriteFile(
			file,
			reinterpret_cast<LPCVOID>(buffer),
			size * sizeof(std::byte),
			&writed,
			nullptr
		);
#endif
	}

	BinaryStreamWriter::~BinaryStreamWriter()
	{
		Close();
	}

	std::span<std::byte const> StringSerializer::ConsumeBom(std::span<std::byte const> input)
	{
		if (input.size() >= std::size(utf8_bom) && std::memcmp(input.data(), utf8_bom, std::size(utf8_bom)) == 0)
		{
			bom = BomT::UTF8;
			return input.subspan(std::size(utf8_bom));
		}
		if (input.size() >= std::size(utf16_le_bom) && std::memcmp(input.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
		{
			bom = BomT::UTF16LE;
			return input.subspan(std::size(utf16_le_bom));
		}
		if (input.size() >= std::size(utf32_le_bom) && std::memcmp(input.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
		{
			bom = BomT::UTF32LE;
			return input.subspan(std::size(utf32_le_bom));
		}
		if (input.size() >= std::size(utf16_be_bom) && std::memcmp(input.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
		{
			bom = BomT::UTF16BE;
			return input.subspan(std::size(utf16_be_bom));
		}if (input.size() >= std::size(utf32_be_bom) && std::memcmp(input.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
		{
			bom = BomT::UTF32BE;
			return input.subspan(std::size(utf32_be_bom));
		}
		bom = BomT::NoBom;
		return input;
	}

	std::span<std::byte const> StringSerializer::GetBinaryBom() const
	{
		switch (bom)
		{
		case BomT::UTF8: return { reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom) };
		case BomT::UTF16LE: return { reinterpret_cast<std::byte const*>(utf16_le_bom), std::size(utf16_le_bom) };
		case BomT::UTF32LE: return { reinterpret_cast<std::byte const*>(utf32_le_bom), std::size(utf32_le_bom) };
		case BomT::UTF16BE: return { reinterpret_cast<std::byte const*>(utf16_be_bom), std::size(utf16_be_bom) };
		case BomT::UTF32BE: return { reinterpret_cast<std::byte const*>(utf32_be_bom), std::size(utf32_be_bom) };
		default: return {};
		}
	}

	EncodeInfo StringSerializer::RequireSpace(Holder<char8_t> holder, std::span<std::byte const> input, std::size_t max_character) const
	{
		switch(bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
			{
				auto re = Encode::StrEncoder<char8_t, char8_t>::RequireSpace(
					{
						reinterpret_cast<char8_t const*>(input.data()),
						input.size() / sizeof(char8_t)
					},
					max_character);
				return re;
			}
		case UnicodeTypeToBom<char16_t>::Value:
			{
				auto re = Encode::StrEncoder<char16_t, char8_t>::RequireSpace(
					{
						reinterpret_cast<char16_t const*>(input.data()),
						input.size() / sizeof(char16_t)
					},
					max_character);
				return re;
			}
		case UnicodeTypeToBom<char32_t>::Value:
			{
				auto re = Encode::StrEncoder<char32_t, char8_t>::RequireSpace(
						{
							reinterpret_cast<char32_t const*>(input.data()),
							input.size() / sizeof(char32_t)
						},
						max_character);
				return re;
			}
		default:
			return {false};
		}
	}

	EncodeInfo StringSerializer::SerializerToUnsafe(Holder<char8_t> holder, std::span<char8_t> output, std::span<std::byte const> input, std::size_t max_character) const
	{
		switch(bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
			{
				auto re = Encode::StrEncoder<char8_t, char8_t>::EncodeUnSafe(
					{
						reinterpret_cast<char8_t const*>(input.data()),
						input.size() / sizeof(char8_t)
					},
					output,
					max_character);
				return re;
			}
		case UnicodeTypeToBom<char16_t>::Value:
			{
				auto re = Encode::StrEncoder<char16_t, char8_t>::EncodeUnSafe(
					{
						reinterpret_cast<char16_t const*>(input.data()),
						input.size() / sizeof(char16_t)
					},
					output,
					max_character);
				return re;
			}
		case UnicodeTypeToBom<char32_t>::Value:
			{
				auto re = Encode::StrEncoder<char32_t, char8_t>::EncodeUnSafe(
						{
							reinterpret_cast<char32_t const*>(input.data()),
							input.size() / sizeof(char32_t)
						},
					output,
					max_character);
				return re;
			}
		default:
			return {false};
		}
	}

	bool StringSerializer::SerializerTo(BinaryStreamWriter& writer, std::span<char8_t const> input) const
	{
		switch(bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
			return writer.Write(
				{
					reinterpret_cast<std::byte const*>(input.data()),
					input.size() * sizeof(char8_t) / sizeof(std::byte)
				}
			);
		default:
			return false;
		}
	}

	std::optional<std::basic_string_view<char8_t>> StringSerializer::TryDirectCastToStringView(Holder<char8_t> holder, std::span<std::byte const> input) const
	{
		switch(bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
			return
				std::basic_string_view<char8_t>{
					reinterpret_cast<char8_t const*>(input.data()),
					input.size() * sizeof(std::byte) / sizeof(char8_t)
				};
		default:
			return std::nullopt;
		}
	}

	template<typename CharT, CharT R, CharT N>
	StringSplitter::Line SplitLineImp(std::basic_string_view<CharT> str, std::size_t offset)
	{
		auto index = str.find_first_of(N, offset);
		if(index >= str.size())
		{
			return {
				StringSplitter::Line::Mode::None,
				{offset, str.size()},
				{offset, str.size()}
			};
		}else if(index == offset || str[index - 1] != R)
		{
			return {
				StringSplitter::Line::Mode::N,
				{offset, index},
				{offset, index + 1}
			};
		}else
		{
			return {
				StringSplitter::Line::Mode::RN,
				{offset, index - 1},
				{offset, index + 1}
			};
		}
	}


	auto StringSplitter::SplitLine(std::u8string_view str, std::size_t offset)
		-> Line
	{
		return SplitLineImp<char8_t, u'\r', u'\n'>(str, offset);
	}
}