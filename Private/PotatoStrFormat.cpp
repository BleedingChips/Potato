#include <sstream>
#include "../Public/PotatoStrFormat.h"
#include "../Public/PotatoStrEncode.h"

using namespace Potato::StrEncode;

namespace Potato::StrFormat
{
	
	Reg::TableWrapper FormatPatternWrapper() {
		static std::vector<Reg::TableWrapper::StorageT> Datas = Reg::TableWrapper::Create(std::u32string_view{UR"(\{([^\{\}]*?)\})"});
		return Reg::TableWrapper(Datas);
	}

	template<typename UnicodeT>
	std::vector<PatternElement<UnicodeT>> CreateImp(std::basic_string_view<UnicodeT> Str)
	{
		std::vector<PatternElement<UnicodeT>> Elements;
		auto Wrapper = FormatPatternWrapper();
		while (true)
		{
			auto Re = Reg::ProcessSearch(Wrapper, Str);
			if (Re.has_value())
			{
				if (Re->MainCapture.Begin() != 0)
					Elements.push_back({ PatternElementType::NormalString, Str.substr(0, Re->MainCapture.Begin())});
				auto Wrapper = Re->GetCaptureWrapper().GetTopSubCapture().GetCapture();
				Elements.push_back({ PatternElementType::Parameter, Str.substr(Wrapper.Begin(), Wrapper.Count())});
				if (Re->IsEndOfFile)
					break;
				Str = Str.substr(Re->MainCapture.End());
			}
			else {
				Elements.push_back({ PatternElementType::NormalString, Str });
				break;
			}
		}
		return Elements;
	}

	template<typename UnicodeT>
	std::size_t FlushSizeImp(std::size_t Count, std::span<PatternElement<UnicodeT> const>& Source)
	{
		while (!Source.empty())
		{
			auto Top = Source[0];
			switch (Top.Type)
			{
			case PatternElementType::NormalString:
				Count += Top.String.size();
				break;
			case PatternElementType::Parameter:
				return Count;
			}
			Source = Source.subspan(1);
		}
		return Count;
	}

	template<typename UnicodeT>
	void FlushBufferImp(std::span<PatternElement<UnicodeT> const>& Source, std::span<UnicodeT>& Output)
	{
		while (!Source.empty())
		{
			auto Top = Source[0];
			switch (Top.Type)
			{
			case PatternElementType::NormalString:
			{
				std::memcpy(Output.data(), Top.String.data(), Top.String.size() * sizeof(UnicodeT));
				Output = Output.subspan(Top.String.size());
				break;
			}
			case PatternElementType::Parameter:
				return;
			}
			Source = Source.subspan(1);
		}
	}

	std::vector<PatternElement<char32_t>> PatternElement<char32_t>::Create(std::u32string_view Str) { return CreateImp(Str); }
	std::size_t PatternElement<char32_t>::FlushSize(std::size_t Count, std::span<PatternElement<char32_t> const>& Source) { return FlushSizeImp(Count, Source); }
	void PatternElement<char32_t>::FlushBuffer(std::span<PatternElement<char32_t> const>& Source, std::span<char32_t>& Output) { return FlushBufferImp(Source, Output); }

	std::vector<PatternElement<char16_t>> PatternElement<char16_t>::Create(std::u16string_view Str) { return CreateImp(Str); }
	std::size_t PatternElement<char16_t>::FlushSize(std::size_t Count, std::span<PatternElement<char16_t> const>& Source) { return FlushSizeImp(Count, Source); }
	void PatternElement<char16_t>::FlushBuffer(std::span<PatternElement<char16_t> const>& Source, std::span<char16_t>& Output) { return FlushBufferImp(Source, Output); }

	std::vector<PatternElement<char8_t>> PatternElement<char8_t>::Create(std::u8string_view Str) { return CreateImp(Str); }
	std::size_t PatternElement<char8_t>::FlushSize(std::size_t Count, std::span<PatternElement<char8_t> const>& Source) { return FlushSizeImp(Count, Source); }
	void PatternElement<char8_t>::FlushBuffer(std::span<PatternElement<char8_t> const>& Source, std::span<char8_t>& Output) { return FlushBufferImp(Source, Output); }

	std::vector<PatternElement<wchar_t>> PatternElement<wchar_t>::Create(std::wstring_view Str) { return CreateImp(Str); }
	std::size_t PatternElement<wchar_t>::FlushSize(std::size_t Count, std::span<PatternElement<wchar_t> const>& Source) { return FlushSizeImp(Count, Source); }
	void PatternElement<wchar_t>::FlushBuffer(std::span<PatternElement<wchar_t> const>& Source, std::span<wchar_t>& Output) { return FlushBufferImp(Source, Output); }
}
namespace Potato::StrFormat
{

}