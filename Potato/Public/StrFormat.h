#pragma once
#include <vector>
#include <variant>
#include <string>
#include <cassert>
#include <typeinfo>
#include <span>
#include <optional>
#include "Misc.h"
namespace Potato
{
	template<typename SourceType>
	struct Formatter
	{
		size_t Preformat(std::u32string_view par, SourceType const& Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, SourceType const& Input);
		
		Formatter(){ static_assert(false, "unsupport formatter"); }
	};
}

namespace Potato
{

	

	struct FormatterPattern
	{

		enum class Type
		{
			NormalString,
			Parameter,
		};

		struct Element
		{
			Type type;
			std::u32string_view string;
		};
		std::u32string_view string;
		std::vector<Element> patterns;
		template<typename ...AT>
		std::u32string operator()(AT&& ...at) const;
	};

	template<typename ...AT>
	struct FormatterWrapper;

	template<typename T, typename ...OT>
	struct FormatterWrapper<T, OT...>
	{
		size_t Preformat(std::span<FormatterPattern::Element const> pars, T input, OT... o_input);
		void Format(std::span<FormatterPattern::Element const> pars, std::span<char32_t>& output_buffer, T input, OT... o_input);
	private:
		Formatter<std::remove_cvref_t<T>> current_formatter;
		FormatterWrapper<OT...> other_formatter;
	};

	template<>
	struct FormatterWrapper<>
	{
		size_t Preformat(std::span<FormatterPattern::Element const> pars);
		void Format(std::span<FormatterPattern::Element const> pars, std::span<char32_t>& output_buffer);
		static void FormatNormalString(FormatterPattern::Element const& pars, std::span<char32_t>& output_buffer);
		static size_t PreformatNormalString(FormatterPattern::Element const& pars){ return pars.string.size(); }
	};

	FormatterPattern CreateFormatterPattern(std::u32string_view Ref);

	template<typename ...Type>
	std::u32string Format(FormatterPattern const& pattern, Type&&... args)
	{
		return pattern(std::forward<Type>(args)...);
	}

	template<typename ...Type>
	std::u32string Format(std::u32string_view string, Type&&... args)
	{
		auto pattern = CreateFormatterPattern(string);
		return pattern(std::forward<Type>(args)...);
	}

	template<typename ...AT>
	std::u32string FormatterPattern::operator()(AT&& ...at) const
	{
		FormatterWrapper<AT...> wrapper;
		auto size = wrapper.Preformat(patterns, std::forward<AT>(at)...);
		std::u32string result;
		result.resize(size);
		std::span<char32_t> output_buffer(result);
		wrapper.Format(patterns, output_buffer, std::forward<AT>(at)...);
		return result;
	}

	template<typename Type>
	std::u32string DirectFormat(std::u32string_view par, Type&& type)
	{
		Formatter<std::remove_cvref_t<Type>> wrapper;
		auto size = wrapper.Preformat(std::forward<Type>(type), par);
		std::u32string result;
		result.resize(size);
		std::span<char32_t> output_buffer(result);
		wrapper.Format(output_buffer, std::forward<Type>(type), par);
		return result;
	}
}

namespace Potato::Exception
{

	struct FormatterInterface
	{
		virtual ~FormatterInterface() = default;
	};

	struct FormatterUnsupportPatternString {
		std::u32string pattern;
		std::u32string total_str;
		using ExceptionInterface = DefineInterface<FormatterInterface>;
	};

	struct FormatterLackOfFormatParas {
		std::u32string pattern;
		size_t index;
		using ExceptionInterface = DefineInterface<FormatterInterface>;
	};

	struct FormatterRequireMoreSpace
	{
		using ExceptionInterface = DefineInterface<FormatterInterface>;
	};
}

namespace Potato
{
	template<typename T, typename ...OT>
	size_t FormatterWrapper<T, OT...>::Preformat(std::span<FormatterPattern::Element const> pars, T input, OT... o_input)
	{
		if (!pars.empty())
		{
			size_t result = 0;
			while (!pars.empty())
			{
				auto& ref = *pars.begin();
				if (ref.type == FormatterPattern::Type::NormalString)
				{
					result += FormatterWrapper<>::PreformatNormalString(ref);
					pars = pars.subspan(1);
				}
				else {
					result += current_formatter.Preformat(ref.string, input);
					return result + other_formatter.Preformat(pars.subspan(1), std::forward<OT>(o_input)...);
				}
			}
			return result;
		}
		else {
			return 0;
		}
	}

	template<typename T, typename ...OT>
	void FormatterWrapper<T, OT...>::Format(std::span<FormatterPattern::Element const> pars, std::span<char32_t>& output_buffer, T input, OT... o_input)
	{
		if (!pars.empty())
		{
			while (!pars.empty())
			{
				auto& ref = *pars.begin();
				if (ref.type == FormatterPattern::Type::NormalString)
				{
					FormatterWrapper<>::FormatNormalString(ref, output_buffer);
					pars = pars.subspan(1);
				}
				else {
					std::optional<size_t> offset = current_formatter.Format(output_buffer, ref.string, input);
					if (offset)
					{
						output_buffer = output_buffer.subspan(*offset);
						other_formatter.Format(pars.subspan(1), output_buffer, std::forward<OT>(o_input)...);
						break;
					}
					else {
						throw Exception::MakeExceptionTuple(Exception::FormatterRequireMoreSpace{});
					}
				}
			}
		}
	}
}

namespace Potato
{
	template<>
	struct Formatter<char32_t*>
	{
		size_t Preformat(std::u32string_view par, char32_t const* Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, char32_t const* Input);
	};

	template<>
	struct Formatter<char32_t>
	{
		size_t Preformat(std::u32string_view par, char32_t Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, char32_t Input);
	};

	template<>
	struct Formatter<std::u32string>
	{
		size_t Preformat(std::u32string_view par, std::u32string const& Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, std::u32string const& Input);
	};

	template<>
	struct Formatter<std::u32string_view>
	{
		size_t Preformat(std::u32string_view par, std::u32string_view Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, std::u32string_view Input);
	};

	template<>
	struct Formatter<float>
	{
		size_t Preformat(std::u32string_view par, float Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, float Input);
	};

	template<>
	struct Formatter<double>
	{
		size_t Preformat(std::u32string_view par, double Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, double Input);
	};

	template<>
	struct Formatter<uint32_t>
	{
		size_t Preformat(std::u32string_view par, uint32_t Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, uint32_t Input);
	};

	template<>
	struct Formatter<int32_t>
	{
		size_t Preformat(std::u32string_view par, int32_t Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, int32_t Input);
	};

	template<>
	struct Formatter<uint64_t>
	{
		size_t Preformat(std::u32string_view par, uint64_t Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, uint64_t Input);
	};

	template<>
	struct Formatter<int64_t>
	{
		size_t Preformat(std::u32string_view par, int64_t Input);
		std::optional<size_t> Format(std::span<char32_t> output, std::u32string_view par, int64_t Input);
	};
}