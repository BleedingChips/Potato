module;

module Potato.Format;

namespace Potato::Format
{
	
	template<typename UnicodeT>
	std::optional<CaptureScanProcessor<UnicodeT>> Translate(Reg::CaptureWrapper Wrp, std::basic_string_view<UnicodeT> Str)
	{
		if (Wrp.HasSubCapture())
		{
			CaptureScanProcessor<UnicodeT> Pro;
			Wrp = Wrp.GetTopSubCapture();
			while (Wrp.HasCapture())
			{
				Pro.Capture.push_back(Wrp.Slice(Str));
				Wrp = Wrp.GetNextCapture();
			}
			return Pro;
		}
		return {};
	}


	std::optional<CaptureScanProcessor<char8_t>> HeadMatchScanPatternProcessor::Scan(std::u8string_view Str)
	{
		Pro.Clear();
		auto Re = Reg::HeadMatch(Pro, Str);
		if (Re)
		{
			auto Wrp = Re->GetCaptureWrapper();
			return Translate(Wrp, Str);
		}
		return {};
	}

	std::optional<CaptureScanProcessor<char8_t>> MatchScanPatternProcessor::Scan(std::u8string_view Str) {
		Pro.Clear();
		auto Re = Reg::Match(Pro, Str);
		if (Re)
		{
			auto Wrp = Re->GetCaptureWrapper();
			return Translate(Wrp, Str);
		}
		return {};
	}

}
namespace Potato::StrFormat
{

}