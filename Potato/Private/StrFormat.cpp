#include "../Public/StrFormat.h"
#include "../Public/Lexical.h"
#include "../Public/StrEncode.h"
namespace Potato::StrFormat
{
	Lexical::Table const& Analyzer()
	{
		static Lexical::RegexInitTuple PatternRex[] = {
			{UR"([^\{\}]+)"},
			{UR"(\{[^\{\}]*?\})"},
			{UR"(\}\})"},
			{UR"(\{\{)"},
		};
		static Lexical::Table instance = Lexical::CreateLexicalFromRegexsReverse(PatternRex, std::size(PatternRex));
		return instance;
	}

	char32_t* FormatWrapper::ConsumeBuffer(size_t require_length)
	{
		if(last_length >= require_length)
		{
			auto result = buffer + ite;
			last_length -= require_length;
			ite += require_length;
			return result;
		}else
			throw Error::LackOfBufferLength{ std::u32string(buffer, total_length),  require_length };
	}

	
	PatternRef CreatePatternRef(std::u32string_view Ref)
	{
		try {
			auto Result = Analyzer().Process(Ref);
			std::vector<PatternRef::Element> patterns;
			size_t used_count = 0;
			for (auto& ite : Result)
			{
				switch (ite.acception)
				{
					case 0:{patterns.push_back({ PatternType::NormalString, ite.capture }); } break;
					case 2:
					case 3:{patterns.push_back({ PatternType::NormalString, {ite.capture.data(), ite.capture.size() - 1 }}); } break;
					case 1:{patterns.push_back({ PatternType::Parameter,  {ite.capture.data() + 1, ite.capture.size() - 2} }); } break;
					default: assert(false);
				}
			}
			return { Ref, std::move(patterns) };
		}
		catch (Exception::Lexical::UnaccaptableItem const& str)
		{
			throw Error::UnsupportPatternString{ std::u32string(Ref), std::u32string(str.possible_token) };
		}
	}

	namespace Implement
	{

		size_t StringLengthImpLocateParmeters(size_t& total_size, PatternRef const& pattern, size_t pattern_index)
		{
			for (; pattern_index < pattern.patterns.size(); ++pattern_index)
			{
				auto& [Type, str] = pattern.patterns[pattern_index];
				switch (Type)
				{
				case PatternType::NormalString: {total_size += str.size(); } break;
				case PatternType::Parameter: {return pattern_index; } break;
				default:
					break;
				}
			}
			return pattern_index;
		}

		void StringLengthImp(size_t& total_size, PatternRef const& pattern, size_t pattern_index)
		{
			auto PIndex = StringLengthImpLocateParmeters(total_size, pattern, pattern_index);
			if (PIndex < pattern.patterns.size())
				throw Error::LackOfFormatParas{ std::u32string(pattern.string), PIndex };
		}

		size_t FormatImpLocateParmeters(FormatWrapper& Wrapper, PatternRef const& pattern, size_t pattern_index)
		{
			size_t used = 0;
			for (; pattern_index < pattern.patterns.size(); ++pattern_index)
			{
				auto& [Type, str] = pattern.patterns[pattern_index];
				switch (Type)
				{
				case PatternType::NormalString:
				{
					std::memcpy(Wrapper.ConsumeBuffer(str.size()), str.data(), str.size() * sizeof(std::u32string_view::value_type));
				} break;
				case PatternType::Parameter: {return pattern_index; } break;
				default:
					break;
				}
			}
			return pattern_index;
		}

		void FormatImp(FormatWrapper& Wrapper, PatternRef const& pattern, size_t pattern_index)
		{
			auto PIndex = FormatImpLocateParmeters(Wrapper, pattern, pattern_index);
			if(PIndex < pattern.patterns.size())
				throw Error::LackOfFormatParas{ std::u32string(pattern.string), PIndex };
		}
		
	}

		
}
namespace Potato::StrFormat
{

	static Lexical::RegexInitTuple FormatRex[] = {
		{UR"(\s)"},
		{UR"(-hex)"},
		//UR"(-e)"
	};

	static Lexical::Table FormatTable = Lexical::CreateLexicalFromRegexsReverse(FormatRex, std::size(FormatRex));

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

	size_t Formatter<char32_t*>::StringLength(std::u32string_view par, char32_t const* Input)
	{
		return std::u32string_view(Input).size();
	}

	void Formatter<char32_t*>::Format(FormatWrapper& wrapper, std::u32string_view par, char32_t const* Input)
	{
		auto size_t = StringLength(par, Input);
		std::memcpy(wrapper.ConsumeBuffer(size_t), Input, size_t * sizeof(char32_t));
	}

	size_t Formatter<std::u32string_view>::StringLength(std::u32string_view par, std::u32string_view Input)
	{
		return Input.size();
	}

	void Formatter<std::u32string_view>::Format(FormatWrapper& wrapper, std::u32string_view par, std::u32string_view Input)
	{
		std::memcpy(wrapper.ConsumeBuffer(Input.size()), Input.data(), Input.size() * sizeof(char32_t));
	}

	size_t Formatter<std::u32string>::StringLength(std::u32string_view par, std::u32string const& Input)
	{
		return Input.size();
	}

	void Formatter<std::u32string>::Format(FormatWrapper& wrapper, std::u32string_view par, std::u32string const& Input)
	{
		std::memcpy(wrapper.ConsumeBuffer(Input.size()), Input.data(), Input.size() * sizeof(char32_t));
	}

	size_t Formatter<char32_t>::StringLength(std::u32string_view par, char32_t Input)
	{
		return sizeof(char32_t);
	}

	void Formatter<char32_t>::Format(FormatWrapper& wrapper, std::u32string_view par, char32_t Input)
	{
		*wrapper.ConsumeBuffer(1) = Input;
	}

	size_t Formatter<double>::StringLength(std::u32string_view par, double Input)
	{
		char data[50];
		return sprintf_s(data, "%lf", Input);
	}

	void Formatter<double>::Format(FormatWrapper& wrapper, std::u32string_view par, double Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data, "%lf", Input);
		StrEncode::AsWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(wrapper.ConsumeBuffer(used_size), used_size);
	}

	size_t Formatter<float>::StringLength(std::u32string_view par, float Input)
	{
		char data[50];
		return sprintf_s(data, "%f", Input);
	}

	void Formatter<float>::Format(FormatWrapper& wrapper, std::u32string_view par, float Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data,"%f", Input);
		StrEncode::AsWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(wrapper.ConsumeBuffer(used_size), used_size);
	}

	size_t Formatter<uint32_t>::StringLength(std::u32string_view par, uint32_t Input)
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

	void Formatter<uint32_t>::Format(FormatWrapper& wrapper, std::u32string_view par, uint32_t Input)
	{
		Paras state = ParasTranslate(par);
		std::string_view format;
		if (state.IsHex)
			format = "%I32x";
		else
			format = "%I32u";
		char data[50];
		size_t used_size = sprintf_s(data, format.data(), Input);
		StrEncode::AsWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(wrapper.ConsumeBuffer(used_size), used_size);
	}

	size_t Formatter<int32_t>::StringLength(std::u32string_view par, int32_t Input)
	{
		char data[50];
		return sprintf_s(data, "%I32d", Input);
	}

	void Formatter<int32_t>::Format(FormatWrapper& wrapper, std::u32string_view par, int32_t Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data, 50, "%I32d", Input);
		StrEncode::AsWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(wrapper.ConsumeBuffer(used_size), used_size);
	}

	size_t Formatter<uint64_t>::StringLength(std::u32string_view par, uint64_t Input)
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

	void Formatter<uint64_t>::Format(FormatWrapper& wrapper, std::u32string_view par, uint64_t Input)
	{
		Paras state = ParasTranslate(par);
		std::string_view format;
		if (state.IsHex)
			format = "%I64x";
		else
			format = "%I64u";
		char data[50];
		size_t used_size = sprintf_s(data, 50, format.data(), Input);
		StrEncode::AsWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(wrapper.ConsumeBuffer(used_size), used_size);
	}
	
	size_t Formatter<int64_t>::StringLength(std::u32string_view par, int64_t Input)
	{
		char data[50];
		return sprintf_s(data, "%I64d", Input);
	}

	void Formatter<int64_t>::Format(FormatWrapper& wrapper, std::u32string_view par, int64_t Input)
	{
		char data[50];
		size_t used_size = sprintf_s(data, 50, "%I64d", Input);
		StrEncode::AsWrapper(reinterpret_cast<char8_t*>(data), used_size).To<char32_t>(wrapper.ConsumeBuffer(used_size), used_size);
	}
	
}