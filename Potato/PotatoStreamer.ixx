export module PotatoStreamer;
import std;
import PotatoTMP;
import PotatoMemLayout;


export namespace Potato::Streamer
{
	enum class StreamState
	{
		OK,
		NoExist,
		Depletion
	};


	struct BinaryStreamer
	{
		struct Result
		{
			StreamState state = StreamState::OK;
			std::size_t readed_size = 0;
		};

		virtual Result StreamRead(std::span<std::byte> out_byte) = 0;
	};

	enum StreamerSeekType
	{
		Start,
		End
	};

	struct BinaryRandomStreamer : public BinaryStreamer
	{
		virtual bool StreamSeek(StreamerSeekType type, std::ptrdiff_t offset) = 0;
	};
}