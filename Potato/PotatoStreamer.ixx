export module PotatoStreamer;
import std;
import PotatoTMP;
import PotatoMisc;


export namespace Potato::Streamer
{
	enum class StreamState
	{
		OK,
		NoExist,
		Depletion,
	};

	struct StreamReader
	{
		virtual std::size_t StreamRead(std::byte* out, std::size_t byte) = 0;
		virtual std::size_t StreamRead(std::span<std::byte> out_byte) { return StreamRead(out_byte.data(), out_byte.size()); }
		template<typename Type> requires(std::is_trivially_copyable_v<Type>)
		std::size_t StreamRead(Type* type, std::size_t array_count = 1) { return StreamRead(reinterpret_cast<std::byte*>(type), array_count * sizeof(Type)); }
		virtual StreamState GetStreamState() const = 0;
	};

	enum SeekAnchor
	{
		Start,
		Current,
		End
	};

	struct StreamRandomReader : public StreamReader
	{
		using StreamReader::StreamRead;
		virtual std::optional<std::ptrdiff_t> StreamSeek(std::ptrdiff_t offset, SeekAnchor anchor = SeekAnchor::Current) = 0;
	};

}