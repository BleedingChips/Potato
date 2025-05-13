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

	DocumentReader::DocumentReader(BinaryStreamReader& reader, std::optional<BomT> force_bom)
		:reader(reader)
	{
		if (reader)
		{
			if (force_bom.has_value())
			{
				bom = *force_bom;
			}
			else {
				std::array<unsigned char, 4> input = {0xCC, 0xCC, 0xCC, 0xCC};
				auto re = static_cast<std::int64_t>(reader.Read(std::span<std::byte>(reinterpret_cast<std::byte*>(input.data()), 4)));
				if (std::memcmp(input.data(), utf8_bom, std::size(utf8_bom)) == 0)
				{
					bom = BomT::UTF8;
					auto bom_offset = static_cast<std::int64_t>(std::size(utf8_bom));
					reader.SetPointerOffsetFromCurrent(bom_offset - re);
				}
				if (input.size() >= std::size(utf16_le_bom) && std::memcmp(input.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
				{
					bom = BomT::UTF16LE;
					auto bom_offset = static_cast<std::int64_t>(std::size(utf16_le_bom));
					reader.SetPointerOffsetFromCurrent(bom_offset - re);
				}
				if (input.size() >= std::size(utf32_le_bom) && std::memcmp(input.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
				{
					bom = BomT::UTF32LE;
					auto bom_offset = static_cast<std::int64_t>(std::size(utf32_le_bom));
					reader.SetPointerOffsetFromCurrent(bom_offset - re);
				}
				if (input.size() >= std::size(utf16_be_bom) && std::memcmp(input.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
				{
					bom = BomT::UTF16BE;
					auto bom_offset = static_cast<std::int64_t>(std::size(utf16_be_bom));
					reader.SetPointerOffsetFromCurrent(bom_offset - re);
				}if (input.size() >= std::size(utf32_be_bom) && std::memcmp(input.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
				{
					bom = BomT::UTF32BE;
					auto bom_offset = static_cast<std::int64_t>(std::size(utf32_be_bom));
					reader.SetPointerOffsetFromCurrent(bom_offset - re);
				}
				else {
					bom = BomT::NoBom;
					reader.SetPointerOffsetFromCurrent(-re);
				}
			}
		}
	}

	DocumentReader::~DocumentReader()
	{
		if (reader)
		{
			auto span = available_buffer_index.Slice(std::span(temporary_buffer));
			Encode::EncodeOption option;
			option.predict = true;
			
			switch (bom)
			{
			case Potato::Document::BomT::UTF8:
			case Potato::Document::BomT::NoBom:
			{
				auto info = Encode::StrEncoder<wchar_t, char8_t>{}.Encode(span, {}, option);
				reader.SetPointerOffsetFromCurrent(-info.target_space);
			}
				break;
			default:
				assert(false);
				break;
			}
		}
		
	}

	std::size_t DocumentReader::FlushBuffer()
	{
		if (available_buffer_index.Begin() > temporary_buffer.size() / 2)
		{
			std::memcpy(temporary_buffer.data(), temporary_buffer.data() + available_buffer_index.Begin(), available_buffer_index.Size() * sizeof(wchar_t));
			available_buffer_index.WholeForward(available_buffer_index.Begin());
		}
		switch (bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			std::array<char8_t, 512> tem_buffer;
			auto readed = reader.Read(std::span<std::byte>(reinterpret_cast<std::byte*>(tem_buffer.data()), tem_buffer.size()));
			Encode::EncodeOption option;
			option.untrusted = true;
			auto read_span = std::span(tem_buffer).subspan(0, readed);
			auto info = Encode::StrEncoder<char8_t, wchar_t>{}.Encode(read_span,
				Misc::IndexSpan<>{available_buffer_index.End(), temporary_buffer.size()}.Slice(std::span(temporary_buffer)),
				option
			);
			reader.SetPointerOffsetFromCurrent(info.source_space - readed);
			available_buffer_index.BackwardEnd(info.target_space);
			return info.character_count;
		}
		default:
			return 0;
		}
		
	}

	DocumentWriter::DocumentWriter(BinaryStreamWriter& writer, BomT force_bom, bool append_bom)
		: writer(writer), bom(force_bom)
	{
		if (append_bom && force_bom != BomT::NoBom && writer)
		{
			switch (force_bom)
			{
			case Potato::Document::BomT::UTF8:
				writer.Write(reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom) * sizeof(unsigned char));
				break;
			case Potato::Document::BomT::NoBom:
				break;
			default:
				assert(false);
				break;
			}
		}
	}

	void DocumentWriter::Write(std::wstring_view str)
	{
		while (!str.empty())
		{
			std::size_t readed_size = std::min( str.size(), cache_buffer.size() - buffer_space);
			std::copy(
				str.begin(),
				str.begin() + readed_size,
				cache_buffer.data() + buffer_space
			);
			buffer_space += readed_size;
			str = str.substr(readed_size);
			if (buffer_space >= cache_buffer.size())
			{
				TryFlush();
			}
		}
	}

	std::size_t DocumentWriter::TryFlush()
	{
		switch (bom)
		{
		case BomT::UTF8:
		case BomT::NoBom:
		{
			Encode::StrEncoder<wchar_t, char8_t> encoder;
			std::array<char8_t, 256> tem_buffer;
			std::size_t total = 0;
			std::wstring_view str{ cache_buffer.data(), buffer_space };
			while (!str.empty())
			{
				auto info = encoder.Encode(str, std::span(tem_buffer));
				writer.Write(
					reinterpret_cast<std::byte const*>(tem_buffer.data()),
					info.target_space * sizeof(char8_t)
				);
				str = str.substr(info.source_space);
				total += info.target_space;
				if (info.source_space == 0)
					break;
			}
			std::size_t readed = buffer_space - str.size();
			if (readed * 2 >= buffer_space)
			{
				std::copy(cache_buffer.begin() + readed, cache_buffer.begin() + buffer_space, cache_buffer.data());
				buffer_space = str.size();
			}
			break;
		}
		default:
			assert(false);
			break;
		}
		return 0;
	}
}