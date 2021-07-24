#include "../Public/StrFormat.h"
#include "../Public/Lexical.h"
#include "../Public/StrEncode.h"
namespace Potato
{
	LexicalTable const& Analyzer()
	{
		static LexicalRegexInitTuple PatternRex[] = {
			{UR"([^\{\}]+)"},
			{UR"(\{[^\{\}]*?\})"},
			{UR"(\}\})"},
			{UR"(\{\{)"},
		};
		static LexicalTable instance = CreateLexicalFromRegexsReverse(PatternRex, std::size(PatternRex));
		return instance;
	}

	
	FormatterPattern CreateFormatterPattern(std::u32string_view Ref)
	{
		try {
			auto Result = Analyzer().Process(Ref);
			std::vector<FormatterPattern::Element> patterns;
			size_t used_count = 0;
			for (auto& ite : Result)
			{
				switch (ite.acception)
				{
					case 0:{patterns.push_back({ FormatterPattern::Type::NormalString, ite.capture }); } break;
					case 2:
					case 3:{patterns.push_back({ FormatterPattern::Type::NormalString, {ite.capture.data(), ite.capture.size() - 1 }}); } break;
					case 1:{patterns.push_back({ FormatterPattern::Type::Parameter,  {ite.capture.data() + 1, ite.capture.size() - 2} }); } break;
					default: assert(false);
				}
			}
			return { Ref, std::move(patterns) };
		}
		catch (Exception::LexicalUnaccaptableItem const& str)
		{
			throw Exception::MakeExceptionTuple(Exception::FormatterUnsupportPatternString{ std::u32string(Ref), std::u32string(str.possible_token) });
		}
	}
	
	size_t FormatterWrapper<>::Preformat(std::span<FormatterPattern::Element const> pars)
	{
		size_t result = 0;
		while (!pars.empty())
		{
			auto& ref = *pars.begin();
			if (ref.type == FormatterPattern::Type::NormalString)
				result += PreformatNormalString(ref);
			else {
				throw Exception::MakeExceptionTuple(Exception::FormatterLackOfFormatParas{});
			}
			pars = pars.subspan(1);
		}
		return result;
	}

	void FormatterWrapper<>::Format(std::span<FormatterPattern::Element const> pars, std::span<char32_t>& output_buffer)
	{
		while (!pars.empty())
		{
			auto& ref = *pars.begin();
			if (ref.type == FormatterPattern::Type::NormalString)
			{
				FormatNormalString(ref, output_buffer);
			}
			else {
				throw Exception::MakeExceptionTuple(Exception::FormatterLackOfFormatParas{});
			}
			pars = pars.subspan(1);
		}
	}

	void FormatterWrapper<>::FormatNormalString(FormatterPattern::Element const& pars, std::span<char32_t>& output_buffer)
	{
		if (output_buffer.size() >= pars.string.size())
		{
			std::memcpy(output_buffer.data(), pars.string.data(), sizeof(char32_t) * pars.string.size());
			output_buffer = output_buffer.subspan(pars.string.size());
		}
		else
			throw Exception::MakeExceptionTuple(Exception::FormatterRequireMoreSpace{});
	}

}
namespace Potato
{

	static LexicalRegexInitTuple FormatRex[] = {
		{UR"(\s)"},
		{UR"(-hex)"},
		//UR"(-e)"
	};

	static LexicalTable FormatTable = CreateLexicalFromRegexsReverse(FormatRex, std::size(FormatRex));

	struct Paras
	{
		bool IsHex = false;
	};

	Paras ParasTranslate(std::u32string_view Code)
	{
		Paras Result;
		//std::optional<std::string> size_require;
		auto Re = FormatTable.Process(Code);
		for (auto& ite : Re)
		{
			switch (ite.acception)
			{
			case 0: {} break;
			case 1: {Result.IsHex = true; } break;
			//case 2: {IsE = true; } break;
			//case 3: {size_require = Encoding::EncodingWrapper<char32_t>({ ite.capture.data() + 1, ite.capture.size() - 1 }).To<char>();} break;
			default:
				break;
			}
		}
		return Result;
	}

	size_t Formatter<char32_t*>::Preformat(std::u32string_view par, char32_t const* Input)
	{
		return std::u32string_view(Input).size();
	}

	std::optional<size_t> Formatter<char32_t*>::Format(std::span<char32_t> output, std::u32string_view par, char32_t const* Input)
	{
		auto size = Preformat(par, Input);
		if (output.size() >= size)
		{
			std::memcpy(output.data(), Input, size * sizeof(char32_t));
			return size;
		}
		return {};
	}

	size_t Formatter<std::u32string_view>::Preformat(std::u32string_view par, std::u32string_view Input)
	{
		return Input.size();
	}

	std::optional<size_t> Formatter<std::u32string_view>::Format(std::span<char32_t> output, std::u32string_view par, std::u32string_view Input)
	{
		if (output.size() >= Input.size())
		{
			std::memcpy(output.data(), Input.data(), Input.size() * sizeof(char32_t));
			return Input.size();
		}
		return {};
	}

	size_t Formatter<std::u32string>::Preformat(std::u32string_view par, std::u32string const& Input)
	{
		return Input.size();
	}

	std::optional<size_t> Formatter<std::u32string>::Format(std::span<char32_t> output, std::u32string_view par, std::u32string const& Input)
	{
		if (output.size() >= Input.size())
		{
			std::memcpy(output.data(), Input.data(), Input.size() * sizeof(char32_t));
			return Input.size();
		}
		return {};
	}

	size_t Formatter<char32_t>::Preformat(std::u32string_view par, char32_t Input)
	{
		return 1;
	}

	std::optional<size_t> Formatter<char32_t>::Format(std::span<char32_t> output, std::u32string_view par, char32_t Input)
	{
		if (output.size() >= 1)
		{
			output[0] = Input;
			return 1;
		}
		return {};
	}

	size_t Formatter<double>::Preformat(std::u32string_view par, double Input)
	{
		char data[50];
		return sprintf_s(data, "%lf", Input);
	}

	std::optional<size_t> Formatter<double>::Format(std::span<char32_t> output, std::u32string_view par, double Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data, "%lf", Input);
		if (used_size <= output.size())
		{
			AsStrWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(output);
			return used_size;
		}
		return {};
	}

	size_t Formatter<float>::Preformat(std::u32string_view par, float Input)
	{
		char data[50];
		return sprintf_s(data, "%f", Input);
	}

	std::optional<size_t> Formatter<float>::Format(std::span<char32_t> output, std::u32string_view par, float Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data,"%f", Input);
		if (used_size <= output.size())
		{
			AsStrWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(output);
			return used_size;
		}
		return {};
	}

	size_t Formatter<uint32_t>::Preformat(std::u32string_view par, uint32_t Input)
	{
		Paras state = ParasTranslate(par);
		std::string_view format;
		if (state.IsHex)
			format = "%I32x";
		else
			format = "%I32u";
		char data[50];
		return sprintf_s(data, format.data(), Input);
	}

	std::optional<size_t> Formatter<uint32_t>::Format(std::span<char32_t> output, std::u32string_view par, uint32_t Input)
	{
		Paras state = ParasTranslate(par);
		std::string_view format;
		if (state.IsHex)
			format = "%I32x";
		else
			format = "%I32u";
		char data[50];
		size_t used_size = sprintf_s(data, format.data(), Input);
		if (used_size <= output.size())
		{
			AsStrWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(output);
			return used_size;
		}
		return {};
	}

	size_t Formatter<int32_t>::Preformat(std::u32string_view par, int32_t Input)
	{
		char data[50];
		return sprintf_s(data, "%I32d", Input);
	}

	std::optional<size_t> Formatter<int32_t>::Format(std::span<char32_t> output, std::u32string_view par, int32_t Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data, 50, "%I32d", Input);
		if (used_size <= output.size())
		{
			AsStrWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(output);
			return used_size;
		}
		return {};
	}

	size_t Formatter<uint64_t>::Preformat(std::u32string_view par, uint64_t Input)
	{
		Paras state = ParasTranslate(par);
		std::string_view format;
		if (state.IsHex)
			format = "%I64x";
		else
			format = "%I64u";
		char data[50];
		return sprintf_s(data, format.data(), Input);
	}

	std::optional<size_t> Formatter<uint64_t>::Format(std::span<char32_t> output, std::u32string_view par, uint64_t Input)
	{
		Paras state = ParasTranslate(par);
		std::string_view format;
		if (state.IsHex)
			format = "%I64x";
		else
			format = "%I64u";
		char data[50];
		size_t used_size = sprintf_s(data, 50, format.data(), Input);
		if (used_size <= output.size())
		{
			AsStrWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(output);
			return used_size;
		}
		return {};
	}
	
	size_t Formatter<int64_t>::Preformat(std::u32string_view par, int64_t Input)
	{
		char data[50];
		return sprintf_s(data, "%I64d", Input);
	}

	std::optional<size_t> Formatter<int64_t>::Format(std::span<char32_t> output, std::u32string_view par, int64_t Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data, 50, "%I64d", Input);
		if (used_size <= output.size())
		{
			AsStrWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(output);
			return used_size;
		}
		return {};
	}
	
}