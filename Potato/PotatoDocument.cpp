module;

#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#undef max
#undef min
#endif

module PotatoDocument;

namespace Potato::Document
{
	constexpr unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
	constexpr unsigned char utf16_le_bom[] = { 0xFF, 0xFE };
	constexpr unsigned char utf16_be_bom[] = { 0xFE, 0xFF };
	constexpr unsigned char utf32_le_bom[] = { 0x00, 0x00, 0xFE, 0xFF };
	constexpr unsigned char utf32_be_bom[] = { 0xFF, 0xFe, 0x00, 0x00 };

	std::size_t BinaryStreamReader::Read(std::span<std::byte> output)
	{
#ifdef _WIN32
		DWORD readed = 0;
		if(ReadFile(
			file, output.data(), output.size() * sizeof(std::byte), &readed, nullptr
		))
		{
			std::size_t total_count = readed;
			return total_count;
		}
#endif
		return 0;
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
			FILE_ATTRIBUTE_READONLY,
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

	std::size_t BinaryStreamReader::GetStreamSize() const
	{
#ifdef _WIN32
		std::ptrdiff_t result = 0;
		::GetFileSizeEx(file, reinterpret_cast<PLARGE_INTEGER>(&result));
		return static_cast<std::size_t>(result);
#endif
	}

	std::optional<std::ptrdiff_t> BinaryStreamReader::SetPointerOffsetFromBegin(std::ptrdiff_t offset)
	{
#ifdef _WIN32
		std::ptrdiff_t new_position = 0;
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

	std::optional<std::ptrdiff_t> BinaryStreamReader::SetPointerOffsetFromEnd(std::ptrdiff_t offset)
	{
#ifdef _WIN32
		std::ptrdiff_t new_position = 0;
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

	std::optional<std::ptrdiff_t> BinaryStreamReader::SetPointerOffsetFromCurrent(std::ptrdiff_t offset)
	{
#ifdef _WIN32
		std::ptrdiff_t new_position = 0;
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

	std::optional<std::size_t> BinaryStreamReader::ReadToBuffer(std::filesystem::path const& path, TMP::FunctionRef<std::span<std::byte>(std::size_t)> allocate_func)
	{
		if (allocate_func)
		{
			BinaryStreamReader reader{ path };
			if (reader)
			{
				auto size = reader.GetStreamSize();
				auto span = allocate_func(size);
				if (span.size() >= size)
				{
					return reader.Read(span);
				}
			}
		}
		return std::nullopt;
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

	DocumentReader::DocumentReader(std::span<std::byte const> in_stream, std::optional<BomT> force_bom)
		:stream(in_stream)
	{
		if (force_bom.has_value())
		{
			bom = *force_bom;
		}
		else {
			if (stream.size() >= std::size(utf8_bom) && std::memcmp(stream.data(), utf8_bom, std::size(utf8_bom)) == 0)
			{
				bom = BomT::UTF8;
				stream = stream.subspan(std::size(utf8_bom));
			}
			else if (stream.size() >= std::size(utf16_le_bom) && std::memcmp(stream.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
			{
				bom = BomT::UTF16LE;
				stream = stream.subspan(std::size(utf16_le_bom));
			}
			else if (stream.size() >= std::size(utf32_le_bom) && std::memcmp(stream.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
			{
				bom = BomT::UTF32LE;
				stream = stream.subspan(std::size(utf32_le_bom));
			}
			else if (stream.size() >= std::size(utf16_be_bom) && std::memcmp(stream.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
			{
				bom = BomT::UTF16BE;
				stream = stream.subspan(std::size(utf16_be_bom));
			}
			else if (stream.size() >= std::size(utf32_be_bom) && std::memcmp(stream.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
			{
				bom = BomT::UTF32BE;
				stream = stream.subspan(std::size(utf32_be_bom));
			}
			else {
				bom = BomT::NoBom;
			}
		}
	}

	std::optional<Encode::EncodeInfo> DocumentReader::ReadSome(std::span<char8_t> output, std::size_t max_character)
	{
		if (!stream.empty())
		{
			switch (bom)
			{
			case BomT::NoBom:
			case BomT::UTF8:
			{
				Encode::StrEncoder<char8_t, char8_t> encoder;
				Encode::EncodeOption option;
				option.max_character = max_character;
				option.predict = false;
				option.untrusted = true;
				auto info = encoder.Encode(std::u8string_view{ reinterpret_cast<char8_t const*>(stream.data()), stream.size() / sizeof(char8_t) }, output, option);
				stream = stream.subspan(info.source_space);
				return info;
			}
			default:
			break;
			}
		}
		return std::nullopt;
	}

	DocumentWriter::DocumentWriter(BomT force_bom, bool need_bom, std::pmr::memory_resource* resource)
		: cache_buffer(resource), bom(force_bom)
	{
		if (need_bom)
		{
			switch (bom)
			{
			case BomT::UTF8:
			{
				 auto bom_span = std::span<std::byte const>{reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom) * sizeof(unsigned char)};
				 cache_buffer.append_range(bom_span);
				 break;
			}
			default:
				break;
			}
		}
	}

	std::size_t DocumentWriter::Write(std::u8string_view str)
	{
		switch (bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			cache_buffer.append_range(std::span<std::byte const>{
				reinterpret_cast<std::byte const*>(str.data()), 
				str.size() * sizeof(char)
			}
			);
			return str.size();
		}
		default:
			break;
		}
		return 0;
	}

	bool DocumentWriter::FlushTo(BinaryStreamWriter& writer)
	{
		return writer.Write(
			cache_buffer.data(),
			cache_buffer.size()
		);
	}
}