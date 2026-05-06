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
		virtual std::size_t StreamRead(std::span<std::byte> out_byte) = 0;
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
		virtual std::optional<std::ptrdiff_t> StreamSeek(std::ptrdiff_t offset, SeekAnchor anchor = SeekAnchor::Current) = 0;
	};

}