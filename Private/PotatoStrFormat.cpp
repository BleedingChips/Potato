#include <sstream>
#include "../Public/PotatoStrFormat.h"
#include "../Public/PotatoStrEncode.h"

using namespace Potato::StrEncode;

namespace Potato::StrFormat
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
				Pro.Capture.push_back(Wrp.GetCapture().Slice(Str));
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


	Reg::TableWrapper FormatPatternWrapper() {
		static std::vector<Reg::StandardT> TableBuffer = [](){
			Reg::MulityRegCreater Crea;
			Crea.LowPriorityLink(u8R"([^\{\}]+)", false, {1});
			Crea.LowPriorityLink(u8R"({{)", true, {2});
			Crea.LowPriorityLink(u8R"(}})", true, { 3 });
			Crea.LowPriorityLink(u8R"(\{([^\{\}]*?)\})", false, { 4 });
			return *Crea.GenerateTableBuffer();
		}();
		return Reg::TableWrapper{ TableBuffer };
	}

	template<typename UnicodeT>
	std::optional<FormatPattern<UnicodeT>> CreateFormatPatternsImp(std::basic_string_view<UnicodeT> Str)
	{
		FormatPattern<UnicodeT> Patterns;
		auto Wrapper = FormatPatternWrapper();
		Reg::HeadMatchProcessor Pro{ FormatPatternWrapper(), false};
		while (!Str.empty())
		{
			Pro.Clear();
			Reg::MatchResult<Reg::HeadMatchProcessor::Result> Re = Reg::HeadMatch(Pro, Str);
			if (Re)
			{
				if (Re->AcceptData.Mask == 1)
				{
					Patterns.Elements.push_back({ Implement::PatternT::NormalString, Re->MainCapture.Slice(Str)});
				}
				else if (Re->AcceptData.Mask == 2)
				{
					Patterns.Elements.push_back({ Implement::PatternT::NormalString, u8"{"});
				}
				else if (Re->AcceptData.Mask == 3)
				{
					Patterns.Elements.push_back({ Implement::PatternT::NormalString, u8"}" });
				}
				else if (Re->AcceptData.Mask == 4)
				{
					auto Wrapper = Re->GetCaptureWrapper().GetTopSubCapture().GetCapture();
					Patterns.Elements.push_back({ Implement::PatternT::Parameter, Wrapper.Slice(Str)});
				}
				Str = Str.substr(Re->MainCapture.End());
			}
			else {
				return {};
			}
		}
		return Patterns;
	}


	std::optional<FormatPattern<char8_t>> CreateFormatPattern(std::u8string_view str) {
		return CreateFormatPatternsImp(str);
	}
}
namespace Potato::StrFormat
{

}