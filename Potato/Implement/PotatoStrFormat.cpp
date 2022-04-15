#include <sstream>
#include "../Include/PotatoStrFormat.h"
#include "../Include/PotatoStrEncode.h"

using namespace Potato::StrEncode;

namespace Potato::StrFormat
{

	Reg::TableWrapper FormatPatternWrapper() {
		static std::vector<Reg::TableWrapper::StorageT> Datas = Reg::TableWrapper::Create(std::u32string_view{UR"(\{([^\{\}]*?)\})"});
		return Reg::TableWrapper(Datas);
	}

	CoreFormatPattern::CoreFormatPattern(Misc::IndexSpan<> Index, Reg::CodePoint(*F1)(std::size_t Index, void* Data), void* Data)
	{
		auto Wrapper = FormatPatternWrapper();
		std::size_t Start = Index.Begin();
		while (true)
		{
			auto Re = Reg::ProcessSearch(Wrapper, Start, F1, Data);
			if (Re.has_value())
			{
				if(Re->MainCapture.Begin() != Start)
					Elements.push_back({ TypeT::NormalString, {Start, Re->MainCapture.Begin() - Start}});
				assert(Re->Captures.size() == 2 && Re->Captures[0].IsBegin && !Re->Captures[1].IsBegin);
				Elements.push_back({ TypeT::Parameter, {Re->Captures[0].Index, Re->Captures[1].Index - Re->Captures[0].Index}});
				Start = Re->MainCapture.End();
				if (Re->IsEndOfFile)
					break;
			}
			else {
				if(Start != Index.End())
					Elements.push_back({ TypeT::NormalString, {Start, Index.End() - Start} });
				break;
			}
		}
	}

	/*
	FormatPattern::FormatPattern(std::u32string_view Str)
		: string(Str)
	{
		auto Wrapper = FormatPatternWrapper();
		while (!Str.empty())
		{
			auto Re = Reg::GreedyMarchProcessor::Process(Wrapper, Str);
			if (Re.has_value())
			{
				auto Cur = Str.substr(0, Re->MainCapture.End());
				Str = Str.substr(Re->MainCapture.End());
				volatile int i = 0;
			}
			else {
				volatile int i = 0;
			}
		}
		/*
		std::size_t State = 0;
		std::size_t LastIndex = 0;
		for (std::size_t Index = 0; Index < Str.size(); ++Index)
		{
			auto Cur = Str[Index];
			switch (State)
			{
			case 0:
				if (Cur == U'{')
				{
					State = 1;
				}
				else if (Cur == U'}')
				{
					State = 10;
				}
				break;
			case 1:
				if (Cur == U'{')
				{
					patterns.push_back({ Type::NormalString, Str.substr(LastIndex, Index - LastIndex)});
					LastIndex = Index + 1;
					State = 0;
				}
				else if (Cur == U'}')
				{
					if (LastIndex - Index != 1)
					{
						patterns.push_back({ Type::NormalString, Str.substr(LastIndex, Index - LastIndex - 1) });
					}
					patterns.push_back({Type::Parameter, {}});
					LastIndex = Index + 1;
					State = 0;
				}
				else {
					patterns.push_back({ Type::NormalString, Str.substr(LastIndex, Index - LastIndex - 1) });
					LastIndex = Index;
					State = 2;
				}
				break;
			case 2:
				if (Cur == U'}')
				{
					patterns.push_back({ Type::Parameter, Str.substr(LastIndex, Index - LastIndex)});
					LastIndex = Index + 1;
					State = 0;
				}
				else if (Cur == U'{')
				{
					throw Exception::UnsupportPatternString{ std::u32string(Str) };
				}
				break;
			case 10:
				if (Cur == U'}')
				{
					patterns.push_back({ Type::NormalString, Str.substr(LastIndex, Index - LastIndex) });
					LastIndex = Index + 1;
					State = 0;
				}
				else {
					throw Exception::UnsupportPatternString{std::u32string(Str)};
				}
				break;
			default:
				break;
			}
		}
		switch (State)
		{
		case 0:
			if (LastIndex < Str.size())
			{
				patterns.push_back({ Type::NormalString, Str.substr(LastIndex, Str.size() - LastIndex)});
			}
			break;
		default:
			throw Exception::UnsupportPatternString{ std::u32string(Str) };
			break;
		}
		*/
	//}

	/*
	bool FormatPattern::Dispatch(std::span<Element const> Elements, std::u32string& Output)
	{
		for (auto& Ite : Elements)
		{
			if(Ite.type == Type::NormalString)
				Output.insert(Output.end(), Ite.string.begin(), Ite.string.end());
			else
				return false;
		}
		return true;
	}
	*/
}
namespace Potato::StrFormat
{
	/*
	bool Scanner<uint64_t, char32_t>::Scan(std::basic_string_view<UnicodeType> Par, SourceType& Input)
	{
		std::wstringstream wss;
		while (!Par.empty())
		{
			wchar_t Buffer[2];
			auto Info = StrEncode::CoreEncoder<char32_t, wchar_t>::EncodeOnceUnSafe(Par, Buffer);
			Par = Par.substr(Info.SourceSpace);
			auto Str = std::span(Buffer).subspan(0, Info.RequireSpace);
			wss.write(Str.data(), Str.size());
		}
		wss >> Input;
		return true;
	}
	*/
	/*
	struct Paras
	{
		bool IsHex = false;
	};

	using namespace Potato::StrEncode;

	std::optional<std::size_t> Formatter<std::u32string_view>::FormatSize(std::u32string_view Par, std::u32string_view Input) const
	{
		return Input.size();
	}

	bool Formatter<std::u32string_view>::Format(std::u32string& Output, std::u32string_view Par, std::u32string_view Input)
	{
		Output.insert(Output.end(), Input.begin(), Input.end());
		return true;
	}

	bool Formatter<std::u32string>::Scan(std::u32string_view par, std::u32string& Output)
	{
		Output = par;
		return true;
	}


	bool Formatter<int64_t>::Scan(std::u32string_view par, int64_t& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
		return true;
	}

	std::optional<std::size_t> Formatter<int64_t>::FormatSize(std::u32string_view Par, int64_t Input) const
	{
		std::string_view format;
		bool IsHex = false;
		if (IsHex)
			format = "%I64x";
		else
			format = "%I64u";
		char data[50];
		return sprintf_s(data, format.data(), Input);
	}

	bool Formatter<int64_t>::Format(std::u32string& Output, std::u32string_view Par, int64_t Input)
	{
		std::stringstream ss;
		ss << Input;
		std::string out;
		ss >> out;
		auto str = AsStrWrapper(reinterpret_cast<char8_t const*>(out.data())).ToString<char32_t>();
		Output.insert(Output.end(), str.begin(), str.end());
		return true;
	}

	bool Formatter<uint64_t>::Scan(std::u32string_view par, uint64_t& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
		return true;
	}

	std::optional<std::size_t> Formatter<uint64_t>::FormatSize(std::u32string_view Par, uint64_t Input) const
	{
		std::string_view format;
		bool IsHex = false;
		if (IsHex)
			format = "%I64x";
		else
			format = "%I64u";
		char data[50];
		return sprintf_s(data, format.data(), Input);
	}

	bool Formatter<uint64_t>::Format(std::u32string& Output, std::u32string_view Par, uint64_t Input)
	{
		std::stringstream ss;
		ss << Input;
		std::string out;
		ss >> out;
		auto str = AsStrWrapper(reinterpret_cast<char8_t const*>(out.data())).ToString<char32_t>();
		Output.insert(Output.end(), str.begin(), str.end());
		return true;
	}

	bool Formatter<double>::Scan(std::u32string_view par, double& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
		return true;
	}

	std::optional<std::size_t> Formatter<double>::FormatSize(std::u32string_view Par, double Input) const
	{
		std::string_view format;
		bool IsHex = false;
		if (IsHex)
			format = "%I64x";
		else
			format = "%I64u";
		char data[50];
		return sprintf_s(data, format.data(), Input);
	}

	bool Formatter<double>::Format(std::u32string& Output, std::u32string_view Par, double Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(Par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
		return true;
	}
	*/

}