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

	std::size_t DocumentReader::Read(std::span<std::byte> output)
	{
#ifdef _WIN32
		std::size_t total_size = 0;
		while (total_size < output.size() * sizeof(std::byte))
		{
			DWORD readed = 0;
			if (ReadFile(
				file, output.data() + total_size, 
				std::min(
					output.size() * sizeof(std::byte) - total_size, 
					std::size_t{ std::numeric_limits<DWORD>::max() }
				),
				&readed, nullptr
			))
			{
				total_size += readed;
				if (readed == 0)
					return total_size;
			}
			else {
				return total_size;
			}
		}
		if (total_size == 0)
		{
			state = Streamer::StreamState::OK;
		}
		return total_size;
#else
		return 0;
#endif
	}

	std::size_t DocumentReader::StreamRead(std::span<std::byte> out_byte)
	{
		if (*this)
		{
			auto readed_byte = Read(out_byte);
			if (readed_byte == 0)
			{
				state = Streamer::StreamState::Depletion;
			}
			return readed_byte;
		}
		return 0;
	}

	DocumentReader::operator bool() const
	{
#ifdef _WIN32
		return file != INVALID_HANDLE_VALUE;
#endif
	}

	bool DocumentReader::Open(std::filesystem::path const& path)
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
		if (file != INVALID_HANDLE_VALUE)
		{
			state = Streamer::StreamState::OK;
		}
		return file != INVALID_HANDLE_VALUE;
#endif
	}

	DocumentReader::~DocumentReader()
	{
		Close();
	}

	void DocumentReader::Close()
	{
#ifdef _WIN32
		::CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
		state = Streamer::StreamState::NoExist;
#endif
	}

	DocumentReader::DocumentReader(DocumentReader&& reader)
	{
#ifdef _WIN32
		file = reader.file;
		state = reader.state;
		reader.file = INVALID_HANDLE_VALUE;
		reader.state = Streamer::StreamState::NoExist;
#endif
	}

	std::size_t DocumentReader::GetStreamSize() const
	{
#ifdef _WIN32
		std::ptrdiff_t result = 0;
		::GetFileSizeEx(file, reinterpret_cast<PLARGE_INTEGER>(&result));
		return static_cast<std::size_t>(result);
#endif
	}

	std::optional<std::ptrdiff_t> DocumentReader::StreamSeek(std::ptrdiff_t offset, Streamer::SeekAnchor anchor)
	{
#ifdef _WIN32
		std::ptrdiff_t new_position = 0;
		if (::SetFilePointerEx(
			file,
			*reinterpret_cast<LARGE_INTEGER*>(&offset),
			reinterpret_cast<LARGE_INTEGER*>(&new_position),
			anchor == Streamer::SeekAnchor::Current ? FILE_CURRENT :
			(
				anchor == Streamer::SeekAnchor::Start ? FILE_BEGIN : FILE_END
				)
		))
		{
			state = Streamer::StreamState::OK;
			return new_position;
		}
#endif
		return std::nullopt;
	}

	/*
	std::optional<std::size_t> DocumentReader::ReadToBuffer(std::filesystem::path const& path, TMP::FunctionRef<std::span<std::byte>(std::size_t)> allocate_func)
	{
		if (allocate_func)
		{
			DocumentReader reader{ path };
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
	*/

	DocumentWriter::DocumentWriter(DocumentWriter&& reader)
	{
#ifdef _WIN32
		::CloseHandle(file);
		file = reader.file;
		reader.file = INVALID_HANDLE_VALUE;
#endif
	}

	DocumentWriter::operator bool() const
	{
#ifdef _WIN32
		return file != INVALID_HANDLE_VALUE;
#endif
	}

	bool DocumentWriter::Open(std::filesystem::path const& path, OpenMode mode)
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
		


	void DocumentWriter::Close()
	{
#ifdef _WIN32
		::CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
#endif
	}

	std::size_t DocumentWriter::Write(std::byte const* buffer, std::size_t size)
	{
#ifdef _WIN32
		std::size_t total_size = 0;
		while (total_size < size)
		{
			DWORD writed = 0;
			if (WriteFile(
					file,
					reinterpret_cast<LPCVOID>(buffer + total_size),
					std::min(
						size * sizeof(std::byte) - total_size,
						std::size_t{ std::numeric_limits<DWORD>::max() }
					),
					&writed,
					nullptr
				)
			)
			{
				total_size += writed;
				if (writed == 0)
					return total_size;
			}
			else {
				return total_size;
			}
		}
		return total_size;
#endif
	}

	DocumentWriter::~DocumentWriter()
	{
		Close();
	}

	std::span<std::byte const> PlainTextReader::GetBomByteSpan(BomT bom)
	{
		switch (bom)
		{
		case BomT::UTF8:
			return std::span(reinterpret_cast<std::byte const*>(utf8_bom), std::size(utf8_bom));
		case BomT::UTF16LE:
			return std::span(reinterpret_cast<std::byte const*>(utf16_le_bom), std::size(utf16_le_bom));
		case BomT::UTF16BE:
			return std::span(reinterpret_cast<std::byte const*>(utf16_be_bom), std::size(utf16_be_bom));
		case BomT::UTF32LE:
			return std::span(reinterpret_cast<std::byte const*>(utf32_le_bom), std::size(utf32_le_bom));
		case BomT::UTF32BE:
			return std::span(reinterpret_cast<std::byte const*>(utf32_be_bom), std::size(utf32_be_bom));
		default:
			return {};
		}
	}

	std::tuple<BomT, std::size_t> PlainTextReader::DetectBom(std::span<std::byte const> byte)
	{
		if (byte.size() >= std::size(utf8_bom) && std::memcmp(byte.data(), utf8_bom, std::size(utf8_bom)) == 0)
		{
			return {BomT::UTF8, std::size(utf8_bom)};
		}
		else if (byte.size() >= std::size(utf16_le_bom) && std::memcmp(byte.data(), utf16_le_bom, std::size(utf16_le_bom)) == 0)
		{
			return { BomT::UTF16LE, std::size(utf16_le_bom) };
		}
		else if (byte.size() >= std::size(utf32_le_bom) && std::memcmp(byte.data(), utf32_le_bom, std::size(utf32_le_bom)) == 0)
		{
			return { BomT::UTF32LE, std::size(utf32_le_bom) };
		}
		else if (byte.size() >= std::size(utf16_be_bom) && std::memcmp(byte.data(), utf16_be_bom, std::size(utf16_be_bom)) == 0)
		{
			return { BomT::UTF16BE, std::size(utf16_be_bom) };
		}
		else if (byte.size() >= std::size(utf32_be_bom) && std::memcmp(byte.data(), utf32_be_bom, std::size(utf32_be_bom)) == 0)
		{
			return { BomT::UTF32BE, std::size(utf32_be_bom) };
		}
		else {
			return { BomT::NoBom, 0 };
		}
	}

	std::optional<BomT> PlainTextReader::UpdateBom()
	{
		if (buffer_index.End() < cache_buffer.size())
		{
			auto readed = reader_reference.StreamRead(
				std::span(cache_buffer.data(), cache_buffer.size()).subspan(buffer_index.End())
			);
			buffer_index = { buffer_index.Begin(), buffer_index.End() + readed };
		}

		auto stream = buffer_index.Slice(std::span(cache_buffer.data(), cache_buffer.size()));
		auto [detected_bom, bom_size] = DetectBom(stream);
		bom = detected_bom;
		buffer_index = buffer_index.SubIndex(bom_size);
		return bom;
	}

	void PlainTextReader::FillBuffer()
	{
		if (buffer_index.Size() == 0)
		{
			buffer_index = {};
		}
		if (buffer_index.Begin() > 0)
		{
			cache_buffer.erase(
				cache_buffer.begin(),
				cache_buffer.begin() + buffer_index.Begin()
			);
			buffer_index = { 0, buffer_index.Size()};
			cache_buffer.resize(cache_buffer_size);
		}

		if (buffer_index.End() < cache_buffer.size())
		{
			auto readed = reader_reference.StreamRead(
				std::span(cache_buffer.data(), cache_buffer.size()).subspan(buffer_index.End())
			);
			buffer_index = { buffer_index.Begin(), buffer_index.End() + readed };
		}
	}

	std::optional<Encode::EncodeInfo> PlainTextReader::ReadPlainText(std::span<Encode::Unicode::CodePointT> output, CutoffSetting cutoff)
	{
		
		switch (bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			EncodeInfo info;
			while (!output.empty() && info.is_good_string && !info.reach_cutoff_character && info.character_count < cutoff.max_character_count)
			{
				if (buffer_index.Size() < Encode::Unicode::UTF8::max_storage_size)
				{
					FillBuffer();
					if (buffer_index.Size() == 0)
					{
						if (info.character_count == 0)
						{
							return std::nullopt;
						}
						else {
							return info;
						}
					}
				}
				auto buffer_span = buffer_index.Slice(std::span(cache_buffer.data(), cache_buffer.size()));
				auto cur_info = Encode::UnicodeEncoder<char8_t, Encode::Unicode::CodePointT>::EncodeTo(
					std::span(reinterpret_cast<char8_t const*>(buffer_span.data()), buffer_span.size()),
					output,
					cutoff
				);
				info.character_count += cur_info.character_count;
				info.is_good_string = cur_info.is_good_string;
				info.reach_cutoff_character = cur_info.reach_cutoff_character;
				info.target_space += cur_info.target_space;
				info.source_space += cur_info.source_space;
				buffer_index = buffer_index.SubIndex(cur_info.source_space * sizeof(char8_t));
				output = output.subspan(cur_info.target_space);
				if(!cur_info)
					return info;
			}
			return info;
		}
			
			break;
		}
		return std::nullopt;
	}

	std::optional<Encode::EncodeInfo> PlainTextReader::ReadPlainText(std::span<char8_t> output, CutoffSetting cutoff)
	{
		switch (bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
		{
			EncodeInfo info;
			while (!output.empty() && info.is_good_string && !info.reach_cutoff_character && info.character_count < cutoff.max_character_count)
			{
				if (buffer_index.Size() < Encode::Unicode::UTF8::max_storage_size)
				{
					FillBuffer();
					if (buffer_index.Size() == 0)
					{
						if (info.character_count == 0)
						{
							return std::nullopt;
						}
						else {
							return info;
						}
					}
				}
				auto buffer_span = buffer_index.Slice(std::span(cache_buffer.data(), cache_buffer.size()));
				auto cur_info = Encode::UnicodeEncoder<char8_t, char8_t>::EncodeTo(
					std::span(reinterpret_cast<char8_t const*>(buffer_span.data()), buffer_span.size()),
					output,
					cutoff
				);
				info.character_count += cur_info.character_count;
				info.is_good_string = cur_info.is_good_string;
				info.reach_cutoff_character = cur_info.reach_cutoff_character;
				info.target_space += cur_info.target_space;
				info.source_space += cur_info.source_space;
				buffer_index = buffer_index.SubIndex(cur_info.source_space * sizeof(char8_t));
				output = output.subspan(cur_info.target_space);
				if (!cur_info)
					return info;
			}
			return info;
		}

		break;
		}
		return std::nullopt;
	}

	PlainTextWritter::PlainTextWritter(DocumentWriter& writer, Config config)
		: writer(writer), bom(config.bom), cache_buffer_size(std::max(config.cache_buffer_size, std::size_t{ 1024 })), cache_buffer(config.resource)
	{
		cache_buffer.resize(cache_buffer_size);
		if (config.need_bom)
		{
			auto span = PlainTextReader::GetBomByteSpan(bom);
			std::memcpy(cache_buffer.data(), span.data(), span.size());
			cache_buffer_used += span.size();
		}
	}

	std::size_t PlainTextWritter::Flush()
	{
		writer.Write(
			std::span(cache_buffer.data(), cache_buffer_used)
		);
		auto flushed = cache_buffer_used;
		cache_buffer_used = 0;
		return flushed;
	}

	PlainTextWritter::~PlainTextWritter()
	{
		Flush();
	}

	std::size_t PlainTextWritter::Write(std::u8string_view str)
	{
		switch (bom)
		{
		case BomT::NoBom:
		case BomT::UTF8:
			while (!str.empty())
			{
				auto size = std::min(str.size(), cache_buffer_size - cache_buffer_used);
				std::memcpy(
					cache_buffer.data() + cache_buffer_used, str.data(), size
				);
				cache_buffer_used += size;
				str = str.substr(size);
				if (cache_buffer_used == cache_buffer_size)
				{
					Flush();
				}
			}
			break;
		}
		return 0;
	}
}