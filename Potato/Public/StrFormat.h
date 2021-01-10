#pragma once
#include <vector>
#include <variant>
#include <string>
#include <cassert>
#include <typeinfo>
namespace Potato::StrFormat
{

	namespace Error
	{
		struct UnsupportPatternString {
			std::u32string pattern;
			std::u32string total_str;
		};

		struct LackOfFormatParas {
			std::u32string pattern;
			size_t index;
		};

		struct LackOfBufferLength
		{
			std::u32string current_result;
			size_t require_length;
		};
	}

	struct FormatWrapper
	{
		size_t TotalLength() const noexcept { return total_length; }
		char32_t* ConsumeBuffer(size_t require_length);
		size_t LastLength() const noexcept { return last_length; }
		FormatWrapper(std::u32string& out_buffer) : buffer(out_buffer.data()), last_length(out_buffer.size()), total_length(out_buffer.size()) {}
	private:
		char32_t* buffer = nullptr;;
		size_t last_length = 0;
		size_t ite = 0;
		size_t total_length = 0;
	};
	
	//std::u32string operator()(std::u32string_view par, SourceType&&) { return {}; }
	template<typename SourceType>
	struct Formatter
	{
		size_t StringLength(std::u32string_view par, SourceType const& Input)
		{
			static_assert(false, "Undefine Formatter");
			return 0;
		}
		void Format(FormatWrapper& Wraspper, std::u32string_view par, SourceType const& Input)
		{
			static_assert(false, "Undefine Formatter");
		}
	};

	enum class PatternType
	{
		NormalString,
		Parameter,
	};

	struct PatternRef
	{
		struct Element
		{
			PatternType type;
			std::u32string_view string;
		};
		std::u32string_view string;
		std::vector<Element> patterns;
	};

	PatternRef CreatePatternRef(std::u32string_view Ref);

	namespace Implement
	{
		
		size_t StringLengthImpLocateParmeters(size_t& total_size, PatternRef const& pattern, size_t pattern_index);

		void StringLengthImp(size_t& total_size, PatternRef const& pattern, size_t pattern_index);
		
		template<typename CurType, typename ...Type>
		void StringLengthImp(size_t& total_size, PatternRef const& pattern, size_t pattern_index, CurType&& input1, Type&&... oinput)
		{
			auto PIndex = StringLengthImpLocateParmeters(total_size, pattern, pattern_index);
			if (PIndex < pattern.patterns.size())
			{
				auto [type, pars] = pattern.patterns[PIndex];
				assert(type == PatternType::Parameter);
				size_t length = Formatter<std::remove_cv_t<std::remove_reference_t<CurType>>>{}.StringLength(pars, std::forward<CurType>(input1));
				total_size += length;
				StringLengthImp(total_size, pattern, PIndex + 1, std::forward<Type>(oinput)...);
			}
		}

		size_t FormatImpLocateParmeters(FormatWrapper& Wrapper, PatternRef const& pattern, size_t pattern_index);

		void FormatImp(FormatWrapper& Wrapper, PatternRef const& pattern, size_t pattern_index);
		
		template<typename CurType, typename ...Type>
		void FormatImp(FormatWrapper& Wrapper, PatternRef const& pattern, size_t pattern_index, CurType&& input1, Type&&... oinput)
		{
			auto PIndex = FormatImpLocateParmeters(Wrapper, pattern, pattern_index);
			if (PIndex < pattern.patterns.size())
			{
				auto [type, pars] = pattern.patterns[PIndex];
				assert(type == PatternType::Parameter);
				Formatter<std::remove_cv_t<std::remove_reference_t<CurType>>>{}.Format(Wrapper, pars, std::forward<CurType>(input1));
				FormatImp(Wrapper, pattern, PIndex + 1, std::forward<Type>(oinput)...);
			}
		}
	}

	template<typename... Type>
	size_t StringLength(std::u32string_view Pattern, Type&&... args)
	{
		auto Patterns = CreatePatternRef(Pattern);
		return StringLength(Patterns, std::forward<Type>(args)...);
	}

	template<typename... Type>
	size_t StringLength(PatternRef const& Pattern, Type&&... args)
	{
		size_t total_length = 0;
		Implement::StringLengthImp(total_length, Pattern, 0, std::forward<Type>(args)...);
		return total_length;
	}

	template<typename ...Type>
	void Process(FormatWrapper& Wrapper, PatternRef const& Patterns, Type&&... args)
	{
		Implement::FormatImp(Wrapper, Patterns, 0, std::forward<Type>(args)...);
	}

	template<typename ...Type>
	void Process(FormatWrapper& Wrapper, std::u32string_view pattern, Type&&... args)
	{
		auto Patterns = CreatePatternRef(pattern);
		Process(Wrapper, Patterns, std::forward<Type>(args)...);
	}

	template<typename... Type>
	std::u32string Process(std::u32string_view Pattern, Type&&... args)
	{	
		auto Patterns = CreatePatternRef(Pattern);
		return Process(Patterns, std::forward<Type>(args)...);
	}

	template<typename... Type>
	std::u32string Process(PatternRef const& Patterns, Type&&... args)
	{
		size_t total_length = 0;
		Implement::StringLengthImp(total_length, Patterns, 0, std::forward<Type>(args)...);
		std::u32string result(total_length, U'\0');
		FormatWrapper wrapper(result);
		Implement::FormatImp(wrapper, Patterns, 0, std::forward<Type>(args)...);
		return std::move(result);
	}

	template<typename Type>
	std::u32string DirectProcess(std::u32string_view pars, Type&& args)
	{
		size_t require_size = Formatter<std::remove_reference_t<std::remove_cv_t<Type>>>{}.StringLength(pars, std::forward<Type>(args));
		std::u32string result(require_size, U'\0');
		FormatWrapper wrapper(result);
		Formatter<std::remove_reference_t<std::remove_cv_t<Type>>>{}.Format(wrapper, pars, std::forward<Type>(args));
		return std::move(result);
	}
}

namespace Potato::StrFormat
{
	template<>
	struct Formatter<char32_t*>
	{
		size_t StringLength(std::u32string_view par, char32_t const* Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, char32_t const* Input);
	};

	template<>
	struct Formatter<char32_t>
	{
		size_t StringLength(std::u32string_view par, char32_t Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, char32_t Input);
	};

	template<>
	struct Formatter<std::u32string>
	{
		size_t StringLength(std::u32string_view par, std::u32string const& Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, std::u32string const& Input);
	};

	template<>
	struct Formatter<std::u32string_view>
	{
		size_t StringLength(std::u32string_view par, std::u32string_view Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, std::u32string_view Input);
	};

	template<>
	struct Formatter<float>
	{
		size_t StringLength(std::u32string_view par, float Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, float Input);
	};

	template<>
	struct Formatter<double>
	{
		size_t StringLength(std::u32string_view par, double Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, double Input);
	};

	template<>
	struct Formatter<uint32_t>
	{
		size_t StringLength(std::u32string_view par, uint32_t Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, uint32_t Input);
	};

	template<>
	struct Formatter<int32_t>
	{
		size_t StringLength(std::u32string_view par, int32_t Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, int32_t Input);
	};

	template<>
	struct Formatter<uint64_t>
	{
		size_t StringLength(std::u32string_view par, uint64_t Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, uint64_t Input);
	};

	template<>
	struct Formatter<int64_t>
	{
		size_t StringLength(std::u32string_view par, int64_t Input);
		void Format(FormatWrapper& wrapper, std::u32string_view par, int64_t Input);
	};
}