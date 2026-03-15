module;

#include <stdint.h>

export module PotatoFormat;

import std;
import PotatoTMP;
import PotatoMisc;
import PotatoEncode;
import PotatoReg;


export namespace Potato::Format
{

	template<typename SourceType, typename UnicodeType>
	struct Scanner
	{
		bool Scan(std::span<UnicodeType const> parameter, SourceType& input) = delete;
	};

	template<typename CharT, typename CharTraiT, typename TargetT>
	bool DirectScan(std::basic_string_view<CharT, CharTraiT> parameter, TargetT& target)
	{
		return Scanner<std::remove_cvref_t<TargetT>, CharT>{}.Scan(parameter, target);
	}

	template<typename CharT, typename TargetT>
	bool DirectScan(CharT const* parameters, TargetT& target)
	{
		return DirectScan(std::basic_string_view<CharT>{parameters}, target);
	}

	template<typename CharT, typename CharTraits>
	bool CaptureScan(std::span<std::size_t const>, std::basic_string_view<CharT, CharTraits> input)
	{
		return true;
	}

	template<typename CharT, typename CharTraits, typename CT, typename ...OT>
	bool CaptureScan(std::span<std::size_t const> capture_index, std::basic_string_view<CharT, CharTraits> input, CT& target, OT& ...other_target)
	{
		return capture_index.size() >= 2 && DirectScan(Misc::IndexSpan<>{capture_index[0], capture_index[1]}.Slice(input), target) && CaptureScan(capture_index.subspan(2), input, other_target...);
	}

	template<typename CharT, typename CT, typename ...OT>
	bool CaptureScan(std::span<Misc::IndexSpan<> const> capture_index, CharT const* input, CT& target, OT& ...other_target)
	{
		return CaptureScan(capture_index, std::basic_string_view<CharT>(input), target, other_target...);
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool RegAcceptScan(Reg::ProcessorAcceptRef accept, std::basic_string_view<CharT, CharTraits> input,  OT& ...other_target)
	{
		return accept && CaptureScan(accept.Capture, input, other_target...);
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool RegAcceptScan(Reg::ProcessorAcceptRef accept, CharT const* input, OT& ...other_target)
	{
		return RegAcceptScan(accept, std::basic_string_view<CharT>(input), other_target...);
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool ProcessorScan(Reg::DfaProcessor& processor, std::basic_string_view<CharT, CharTraits> input, OT& ...other_target)
	{
		auto accept = Reg::Process(processor, input);
		return RegAcceptScan(accept, input, other_target...);
	}

	template<typename CharT1, typename CharTT1, typename CharT2, typename CharTT2, typename ...OT>
	bool MatchScan(std::basic_string_view<CharT1, CharTT1> regex, std::basic_string_view<CharT2, CharTT2> input, OT&... other_target)
	{
		Reg::Dfa table(Reg::Dfa::FormatE::Match, regex, false, 0);
		Reg::DfaProcessor processor;
		processor.SetObserverTable(table);
		processor.Clear();
		return ProcessorScan(processor, input, other_target...);
	}

	template<typename CharT1, typename CharTT1, typename CharT2, typename CharTT2, typename ...OT>
	bool HeadMatchScan(std::basic_string_view<CharT1, CharTT1> regex, std::basic_string_view<CharT2, CharTT2> input, OT&... other_target)
	{
		Reg::Dfa table(Reg::Dfa::FormatE::HeadMatch, regex, false, 0);
		Reg::DfaProcessor processor;
		processor.SetObserverTable(table);
		processor.Clear();
		return ProcessorScan(processor, input, other_target...);
	}
}

export namespace Potato::Format
{
	template<typename NumberType, typename UnicodeType>
	struct BuildInNumberScanner
	{
		std::wstringstream wss;
		bool Scan(std::basic_string_view<UnicodeType> par, NumberType& output)
		{
			std::array<wchar_t, 16> buffer;
			while (!par.empty())
			{
				auto info = Encode::UnicodeEncoder<UnicodeType, wchar_t>::EncodeTo(std::span(par), std::span(buffer));
				wss.write(buffer.data(), info.target_space);
				par = par.substr(info.source_space);
			}
			wss >> output;
			return true;
		}
	};

	template<typename UnicodeT>
	struct Scanner<float, UnicodeT> : BuildInNumberScanner<float, UnicodeT> {};

	template<typename UnicodeT>
	struct Scanner<int32_t, UnicodeT> : BuildInNumberScanner<int32_t, UnicodeT> {};

	template<typename UnicodeT>
	struct Scanner<uint32_t, UnicodeT> : BuildInNumberScanner<uint32_t, UnicodeT> {};

	template<typename UnicodeT>
	struct Scanner<uint64_t, UnicodeT> : BuildInNumberScanner<uint64_t, UnicodeT> {};

	template<typename UnicodeT>
	struct Scanner<int64_t, UnicodeT> : BuildInNumberScanner<int64_t, UnicodeT> {};

	/*
	template<typename TargetType, typename CharaTrai, typename Allocator, typename UnicodeT>
	struct Scanner<std::basic_string<TargetType, CharaTrai, Allocator>, UnicodeT> //: BuildInNumberScanner<int64_t, UnicodeT>
	{
		bool Scan(std::basic_string_view<UnicodeT> Par, std::basic_string<TargetType, CharaTrai, Allocator>& Output)
		{
			Encode::UnicodeEncoder<UnicodeT, TargetType>::EncodeTo(Par, std::back_inserter(Output));
			return true;
		}
	};
	*/
}

export namespace Potato::Format
{
	enum class FormatterSyntaxType
	{
		String,
		Parameter
	};

	template<typename ValueT, typename CharT>
	concept HasExistSTDFormat = requires()
	{
		std::formatter<ValueT, CharT>{};
	};

	template<typename ValueT, typename CharT>
	struct Formatter;

	template<typename ValueT, typename CharT>
		requires(HasExistSTDFormat<ValueT, CharT>)
	struct Formatter<ValueT, CharT>
	{
		std::formatter<ValueT, CharT> formatter;
		constexpr Formatter(std::basic_string_view<CharT> syntax)
			: formatter([=]() {
				std::basic_format_parse_context<CharT> parse_context{ 
					"}", 0
				};
				std::formatter<ValueT, CharT> std_formatter;
				std_formatter.parse(parse_context);
				return std_formatter;
				}()) 
		{
		}
		template<typename Iterator, typename Par>
		Iterator Format(Iterator ite, Par&& par) const
		{
			using ContextType = std::basic_format_context<
				std::remove_cvref_t<Iterator>,
				CharT
			>;

			auto args = std::make_format_args<ContextType>(par);
			auto format_context = ContextType::_Make_from(
				std::move(ite), args, {}
			);

			return formatter.format(std::forward<Par>(par), format_context);
		}
	};


	template<typename CharT>
	struct FormatterStringWrapper
	{
		std::basic_string_view<CharT> syntax;
		constexpr FormatterStringWrapper(std::basic_string_view<CharT> syntax) : syntax(syntax) {}
		constexpr FormatterStringWrapper(FormatterStringWrapper const&) = default;
		template<typename IteratorT, typename ...AT>
		IteratorT Format(IteratorT iterator, AT&&... parameter) const
		{
			return std::copy_n(
				syntax.begin(),
				syntax.size(),
				std::move(iterator)
			);
		}
	};

	template<typename ValueT, typename CharT, std::size_t index>
	struct FormatterWrapper
	{
		Formatter<ValueT, CharT> formatter;
		constexpr FormatterWrapper(std::basic_string_view<CharT> syntax) : formatter(syntax) {}
		constexpr FormatterWrapper(FormatterWrapper const&) = default;
		template<typename IteratorT, typename ...AT>
		IteratorT Format(IteratorT iterator, AT&&... parameter) const
		{
			return formatter.Format(std::move(iterator), TMP::ParameterPicker<index>::Pick(std::forward<AT>(parameter)...));
		}
	};

	template<typename CharT, FormatterSyntaxType type, std::size_t index>
	struct FormatterSyntaxPoint
	{
		std::basic_string_view<CharT> syntax;
		template<typename ...AT>
			requires(type == FormatterSyntaxType::Parameter)
		consteval auto GetFormatter() const
		{
			return FormatterWrapper<
				std::remove_cvref_t<decltype(TMP::ParameterPicker<index>::Pick(std::declval<AT>()...))>,
				CharT,
				index
			>{syntax};
		}
		template<typename ...AT>
			requires(type == FormatterSyntaxType::String)
		consteval auto GetFormatter() const
		{
			return FormatterStringWrapper<
				CharT
			>{syntax};
		}
	};

	struct FormatterSyntax
	{
		template<typename CharT>
		struct ParameterIndexSyntax
		{
			std::basic_string_view<CharT> syntax;
			std::optional<std::size_t> parameter_index;
		};

		template<typename CharT>
		static constexpr std::optional<ParameterIndexSyntax<CharT>> GetParameterIndexSyntax(std::basic_string_view<CharT> parameter_syntax)
		{
			std::size_t state = 0;
			std::optional<std::size_t> parameter_index;
			std::size_t index = 0;
			for (; index < parameter_syntax.size(); ++index)
			{
				auto cur = parameter_syntax[index];
				if (cur == static_cast<CharT>(':'))
					return ParameterIndexSyntax<CharT>{parameter_syntax.substr(index + 1), parameter_index};
				else if (
					(cur > static_cast<CharT>('0') && cur <= static_cast<CharT>('9'))
					|| (parameter_index.has_value() && cur == static_cast<CharT>('0'))
					)
				{
					if (!parameter_index.has_value())
						parameter_index = static_cast<std::size_t>(cur - static_cast<CharT>('0'));
					else
					{
						*parameter_index *= 10;
						*parameter_index += static_cast<std::size_t>(cur - static_cast<CharT>('0'));
					}
				}
				else {
					return std::nullopt;
				}
			}
			return ParameterIndexSyntax<CharT>{parameter_syntax.substr(index), parameter_index};
		}

		template<typename CharT>
		struct TopSyntaxPoint
		{
			std::basic_string_view<CharT> string;
			FormatterSyntaxType type;
			std::optional<std::size_t> parameter_index;
			std::basic_string_view<CharT> next_string;
		};

		template<typename CharT>
		static constexpr std::optional<TopSyntaxPoint<CharT>> FindSyntaxPoint(std::basic_string_view<CharT> formatter_pattern)
		{
			auto first_brace = std::find_if(
				formatter_pattern.begin(),
				formatter_pattern.end(),
				[](CharT character) {
					return character == static_cast<CharT>('{') || character == static_cast<CharT>('}');
				}
			);

			if (first_brace == formatter_pattern.end())
			{
				return TopSyntaxPoint<CharT>{
					{formatter_pattern.begin(), formatter_pattern.end()},
					FormatterSyntaxType::String,
					{}
				};
			}

			auto cur = *first_brace;

			if (
				first_brace + 1 != formatter_pattern.end()
				&& cur == *(first_brace + 1)
				)
			{
				return TopSyntaxPoint<CharT>{
					{formatter_pattern.begin(), first_brace + 1},
					FormatterSyntaxType::String,
					0,
					{first_brace + 2, formatter_pattern.end()}
				};
			}

			if (cur == static_cast<CharT>('{'))
			{
				if (first_brace != formatter_pattern.begin())
				{
					return TopSyntaxPoint<CharT>{
						{formatter_pattern.begin(), first_brace},
						FormatterSyntaxType::String,
						0,
						{first_brace, formatter_pattern.end()}
					};
				}

				auto second_brace = std::find_if(
					first_brace + 1,
					formatter_pattern.end(),
					[](CharT character) {
						return character == static_cast<CharT>('{') || character == static_cast<CharT>('}');
					}
				);

				if (second_brace == formatter_pattern.end())
				{
					return std::nullopt;
				}

				if (*second_brace == static_cast<CharT>('}'))
				{
					std::basic_string_view<CharT> syntax{ first_brace + 1, second_brace };

					auto parameter_index_info = GetParameterIndexSyntax(syntax);

					if (!parameter_index_info.has_value())
						return std::nullopt;

					return TopSyntaxPoint<CharT>{
						parameter_index_info->syntax,
						FormatterSyntaxType::Parameter,
						parameter_index_info->parameter_index,
						{second_brace + 1, formatter_pattern.end()}
					};
				}
			}

			return std::nullopt;
		}

		template<typename CharT, std::size_t index>
		static constexpr std::optional<TopSyntaxPoint<CharT>> FindSyntaxPoint(const CharT (&str)[index])
		{
			return FindSyntaxPoint(std::basic_string_view<CharT>{ str, index - 1});
		}

		template<typename CharT>
		struct SyntaxPoint
		{
			std::basic_string_view<CharT> syntax;
			FormatterSyntaxType type = FormatterSyntaxType::String;
			std::size_t parameter_index = 0;
		};

		struct PatternInfo
		{
			std::size_t section_count;
			std::size_t max_parameter_index;
		};

		template<typename CharT>
		static constexpr std::optional<PatternInfo> GetSyntaxPointCount(std::basic_string_view<CharT> format_pattern)
		{
			std::size_t section_count = 0;
			std::size_t max_parameter_index = 0;
			std::size_t is_automatic_mode = 0;
			while (!format_pattern.empty())
			{
				auto point = FindSyntaxPoint(format_pattern);
				if (!point.has_value())
				{
					return std::nullopt;
				}
				format_pattern = point->next_string;
				section_count += 1;
				if (point->type == FormatterSyntaxType::Parameter)
				{
					if (point->parameter_index.has_value())
					{
						if (is_automatic_mode == 0)
						{
							is_automatic_mode = 2;
						}
						else if (is_automatic_mode == 1)
							return std::nullopt;

						max_parameter_index = std::max(max_parameter_index, *point->parameter_index + 1);
					}
					else
					{
						if (is_automatic_mode == 0)
						{
							is_automatic_mode = 1;
						}
						else if (is_automatic_mode == 2)
							return std::nullopt;

						max_parameter_index += 1;
					}
				}
			}
			return PatternInfo{ section_count, max_parameter_index };
		}

		template<std::size_t index, std::size_t target>
		struct FormatterExecutor
		{
			template<typename ...ParT, typename IteratorT, typename ...ValT>
			IteratorT operator() (std::tuple<ParT...> const& pattern, IteratorT iterator, ValT&&... val)
			{
				iterator = std::get<index>(pattern).Format(std::move(iterator), std::forward<ValT>(val)...);
				return FormatterExecutor<index + 1, target>{}(pattern, std::move(iterator), std::forward<ValT>(val)...);
			}
		};

		template<std::size_t index>
		struct FormatterExecutor<index, index>
		{
			template<typename ...ParT, typename IteratorT, typename ...ValT>
			IteratorT operator() (std::tuple<ParT...> const& pattern, IteratorT iterator, ValT&&... val)
			{
				return iterator;
			}
		};
	};

	template<typename IteratorT, typename CharT, typename ValueT>
	concept HasExistFormatter = requires()
	{
		Formatter<std::remove_cvref_t<ValueT>, CharT>(std::basic_string_view<CharT>()).Format(std::declval<IteratorT>(), std::declval<ValueT>());
	};

	template<TMP::TypeString type_string>
	struct StaticFormatPattern
	{
		using Type = typename decltype(type_string)::Type;
		static constexpr auto pattern_info = FormatterSyntax::GetSyntaxPointCount(type_string.GetStringView());
		static_assert(pattern_info.has_value(), "check format pattern syntax");
		static constexpr auto pattern = []<std::size_t ...i>(std::index_sequence<i...>) {
			constexpr std::array<
				FormatterSyntax::SyntaxPoint<Type>, pattern_info->section_count
			> pattern = []() {
				std::array<
					FormatterSyntax::SyntaxPoint<Type>, pattern_info->section_count
				> pattern;
				
				std::size_t max_parameter_index = 0;
				std::size_t is_automic = false;
				auto iterator = pattern.begin();

				auto ite_format = type_string.GetStringView();
				while (!ite_format.empty())
				{
					auto point = FormatterSyntax::FindSyntaxPoint(ite_format);
					iterator->type = point->type;
					if (point->type == FormatterSyntaxType::Parameter)
					{
						iterator->syntax = point->string;
						if (point->parameter_index.has_value())
						{
							if (is_automic == 0)
							{
								is_automic = 2;
							}
							else if (is_automic == 1)
								throw "formatter index is automic mode";
							iterator->parameter_index = *point->parameter_index;
						}
						else {
							if (is_automic == 0)
							{
								is_automic = 1;
							}
							else if (is_automic == 2)
								throw "formatter index is definded mode";
							iterator->parameter_index = max_parameter_index++;
						}
					}
					else {
						iterator->syntax = point->string;
						iterator->parameter_index = 0;
					}
					iterator += 1;
					ite_format = point->next_string;
				}

				return pattern;
				}();

			return std::make_tuple(
				FormatterSyntaxPoint<typename decltype(type_string)::Type, pattern[i].type, pattern[i].parameter_index>{
					pattern[i].syntax
				}...
			);
			
		}(std::make_index_sequence<pattern_info->section_count>());
		
		consteval StaticFormatPattern(StaticFormatPattern const& other) = default;
		consteval StaticFormatPattern() {}

		template<std::output_iterator<Type> IteratorT, typename ...AT>
		constexpr IteratorT Format(IteratorT iterator, AT&& ...at) const
		{
			static_assert(sizeof...(AT) >= pattern_info->max_parameter_index, "format require input count equal or bigger then {} pair");
			
			constexpr bool has_exist_formatter = (true && ... && HasExistFormatter<IteratorT, Type, AT>);
			
			static_assert(has_exist_formatter, "require formatter");

			constexpr auto formatter = []<std::size_t ...i>(std::index_sequence<i...>) {
				return std::make_tuple(
					std::get<i>(pattern).GetFormatter<AT...>()...
				);
			}(std::make_index_sequence<pattern_info->section_count>());

			return FormatterSyntax::FormatterExecutor<0, pattern_info->section_count>{}(
				formatter,
				std::move(iterator),
				std::forward<AT>(at)...
			);
		}
	};


	template<typename ValueT, typename CharT>
	struct Deformatter
	{
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context) = delete;
		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, ValueT& output) const = delete;
	};

	template<typename CharT>
	struct Deformatter<void, CharT>
	{
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
		{
			if (context.size() != 0)
			{
				return std::nullopt;
			}
			return context.size();
		}
		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, TMP::ItSelf<void> output) const
		{
			return string.size();
		}
	};

	struct DeformatInfo
	{
		std::size_t consumed_string = 0;
		std::size_t section_count = 0;
		bool deformat_complate = true;
		constexpr operator bool() const { return deformat_complate; }
	};

	struct DeformatSyntax
	{

		struct SyntaxPointInfo
		{
			std::size_t count;
			std::size_t parameter_offset;
		};

		template<typename CharT, typename Traits>
		static constexpr SyntaxPointInfo FindSyntaxPoint(std::basic_string_view<CharT, Traits> string, std::size_t offset = 0)
		{
			while (offset < string.size())
			{
				auto iterator = string.find(static_cast<CharT>('{'), offset);
				if (iterator >= string.size())
					return { string.size(), string.size() };
				if (iterator != 0 && string[iterator - 1] == static_cast<CharT>('\\'))
				{
					offset = iterator + 1;
					continue;
				}
				else
				{
					return { iterator, iterator + 1 };
				}
			}
			return { string.size(), string.size() };
		}

		
		template<std::size_t index>
		struct ExecuteDeformat
		{
			template<typename CharT, typename ...Tuple, typename ...AT>
			static constexpr DeformatInfo Execute(std::basic_string_view<CharT> string, std::tuple<Tuple...> const& pattern, AT&& ...at)
			{
				DeformatInfo info = ExecuteDeformat<index - 1>::Execute(string, pattern, std::forward<AT>(at)...);
				if (!info.deformat_complate)
					return info;
				auto sub_string = string.substr(info.consumed_string);
				auto result = std::get<index - 1>(pattern).Deformat(sub_string, std::forward<AT>(at)...);
				if (result.has_value())
				{
					info.consumed_string += *result;
					info.section_count += 1;
				}
				else {
					info.deformat_complate = false;
				}
				return info;
			}
		};

		template<>
		struct ExecuteDeformat<0>
		{
			template<typename CharT, typename ...Tuple, typename ...AT>
			static constexpr DeformatInfo Execute(std::basic_string_view<CharT> string, std::tuple<Tuple...> const& pattern, AT&& ...at)
			{
				return {};
			}
		};

		template<TMP::TypeString type_string, Misc::IndexSpan<> string_index,  typename ValueT, typename CharT, std::size_t index>
		struct PatternSection
		{
			Deformatter<ValueT, CharT> deformatter;
			static Reg::Dfa const& GetReg() requires(string_index.Size() > 0)
			{
				auto str = string_index.Slice(type_string.GetStringView());
				try {
					static const Reg::Dfa reg(Reg::Dfa::FormatE::HeadMatch, string_index.Slice(type_string.GetStringView()), false, 0);
					return reg;
				}
				catch (...)
				{
					throw "check deformat pattern syntax";
				}
			}

			template<typename CharT, typename ...AT>
			std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, AT&& ...output) const
			{
				std::size_t offset = 0;
				if constexpr(string_index.Size() > 0)
				{
					Reg::DfaProcessor process;
					process.SetObserverTable(GetReg());
					Reg::ProcessorAcceptRef capture = Reg::Process(process, string);
					if (capture)
					{
						offset = capture.MainCapture.End();
						string = string.substr(offset);
					}
					else {
						return std::nullopt;
					}
				}
				auto info = deformatter.Deformat(string, TMP::ParameterPicker<index>::Pick(std::forward<AT>(output)...));
				if (info.has_value())
				{
					return *info + offset;
				}
				return std::nullopt;
			}
		};

		struct TemporaryPatternSection
		{
			Misc::IndexSpan<> string_index = {};
			std::size_t parameter_index = 0;
		};

		template<std::size_t index>
		struct Parser
		{
			template<typename CharT, std::size_t count, typename Formatter>
			constexpr static std::optional<std::size_t> Parse(std::basic_string_view<CharT> string, std::size_t offset, std::array<TemporaryPatternSection, count>& section, Formatter& formatter)
			{
				auto string_index = Parser<index - 1>::Parse(string, offset, section, formatter);
				if (string_index.has_value())
				{
					DeformatSyntax::SyntaxPointInfo info = DeformatSyntax::FindSyntaxPoint(string, *string_index);
					auto context_string = string.substr(info.parameter_offset);
					auto end = std::get<index - 1>(formatter).Parse(context_string);
					if (!end.has_value())
						return std::nullopt;
					section[index - 1].string_index = { *string_index, info.count };
					section[index - 1].parameter_index = index - 1;
					if (*end < context_string.size())
					{
						if (context_string[*end] == static_cast<CharT>('}'))
							return *end + info.parameter_offset + 1;
					}else if(*end == context_string.size()){
						if constexpr (
							std::is_same_v<std::remove_cvref_t<decltype(std::get<index - 1>(formatter))>, Deformatter<void, CharT>>
							)
						{
							return *end + info.parameter_offset;
						}
					}
				}
				return std::nullopt;
			}
		};
		
		template<>
		struct Parser<0>
		{
			template<typename CharT, std::size_t count, typename Formatter>
			constexpr static std::optional<std::size_t> Parse(std::basic_string_view<CharT> string, std::size_t offset, std::array<TemporaryPatternSection, count>& section, Formatter& formatter)
			{
				return offset;
			}
		};
	};

	template<TMP::TypeString type_string, typename ...AT>
	struct DeformatPattern
	{
		using CharT = decltype(type_string)::Type;
		static constexpr std::size_t GetSectionCount() { return sizeof...(AT) + 1; }

		static constexpr auto pattern = []<std::size_t ...i>(std::index_sequence<i...>) {

			constexpr auto pattern_pairs = []() {
				constexpr auto string = type_string.GetStringView();
				
				std::tuple<
					std::array<DeformatSyntax::TemporaryPatternSection, GetSectionCount()>,
					std::tuple<Deformatter<AT, CharT>..., Deformatter<void, CharT>>
				> temporary_pattern;

				auto result = DeformatSyntax::Parser<GetSectionCount()>::Parse(
					string, 0, std::get<0>(temporary_pattern), std::get<1>(temporary_pattern)
				);

				if (!result.has_value())
				{
					throw "bad deformatter parameter";
				}

				return temporary_pattern;
				}();

			return std::make_tuple(
				(
					DeformatSyntax::PatternSection<
						type_string,
						std::get<0>(pattern_pairs)[i].string_index,
						TMP::ParameterPicker<i>::template TupleT<AT..., void>,
						CharT,
						std::get<0>(pattern_pairs)[i].parameter_index
					>{std::get<i>(std::get<1>(pattern_pairs))}
				)...
			);
		}(std::make_index_sequence<GetSectionCount()>());
	};


	template<TMP::TypeString type_string, typename CharT, typename CharTraits, typename ...AT>
	DeformatInfo Deformat(std::basic_string_view<CharT, CharTraits> str, AT&& ...at)
	{
		static constexpr DeformatPattern<type_string, std::remove_cvref_t<AT>...> pattern;
		return DeformatSyntax::ExecuteDeformat<sizeof...(AT) + 1>::Execute(str, pattern.pattern, std::forward<AT>(at)..., TMP::ItSelf<void>{});
	}

	struct BuildInDeformatter
	{
		template<typename CharT>
		static constexpr std::size_t RemoveSpace(std::basic_string_view<CharT> str, std::size_t offset = 0)
		{
			for (std::size_t i = offset; i < str.size(); ++i)
			{
				if (str[i] != static_cast<CharT>(' '))
					return i;
			}
			return str.size();
		}

		template<typename CharT>
		static constexpr std::tuple<bool, std::size_t> GetPlusOrMinus(std::basic_string_view<CharT> str, std::size_t offset = 0)
		{
			if (offset < str.size())
			{
				if (str[offset] == static_cast<CharT>('-'))
				{
					return { false, offset + 1 };
				}
				else if (str[offset] == static_cast<CharT>('+'))
					return { true, offset + 1 };
				else
					return { true, offset };
			}
			return {true, str.size()};
		}

		template<typename CharT>
		static constexpr std::size_t GetNumbers(std::basic_string_view<CharT> str, std::size_t offset = 0)
		{
			while (offset < str.size())
			{
				auto character = str[offset];
				if (character >= static_cast<CharT>('0') && character <= static_cast<CharT>('9'))
				{
					++offset;
				}
				else {
					return offset;
				}
			}
			return str.size();
		}
	};

	template<typename ValueT, typename CharT>
		requires(TMP::IsOneOfV<ValueT, float, double>)
	struct Deformatter<ValueT, CharT>
	{
		static constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
		{
			return 0;
		}

		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, ValueT& output) const
		{
			auto offset = BuildInDeformatter::RemoveSpace(string);
			auto [is_positive, new_offset] = BuildInDeformatter::GetPlusOrMinus(string, offset);
			new_offset = BuildInDeformatter::GetNumbers(string, new_offset);
			if (offset == new_offset)
			{
				return std::nullopt;
			}
			if (new_offset < string.size() && string[new_offset] == static_cast<CharT>('.'))
			{
				auto new_new_offset = BuildInDeformatter::GetNumbers(string, new_offset + 1);
				if (new_offset != new_new_offset)
				{
					new_offset = new_new_offset;
				}
				else {
					return std::nullopt;
				}
			}
			auto str = string.substr(offset, new_offset - offset);
			std::array<std::byte, 32 * sizeof(char)> buffer;
			std::pmr::monotonic_buffer_resource resource(buffer.data(), buffer.size());
			std::pmr::string temp_str(&resource);
			Encode::UnicodeEncoder<CharT, char>::EncodeTo(str, std::back_inserter(temp_str));
			auto re = std::from_chars(temp_str.data(), temp_str.data() + temp_str.size(), output);
			if (re.ec == std::errc{})
				return new_offset;
			return std::nullopt;
		}
	};

	template<typename ValueT, typename CharT>
		requires(TMP::IsOneOfV<ValueT, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::int8_t, std::int16_t, std::int32_t, std::int64_t>)
	struct Deformatter<ValueT, CharT>
	{
		static constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
		{
			return 0;
		}

		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, ValueT& output) const
		{
			auto offset = BuildInDeformatter::RemoveSpace(string);
			auto [is_positive, new_offset] = BuildInDeformatter::GetPlusOrMinus(string, offset);
			if constexpr (TMP::IsOneOfV<ValueT, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>)
			{
				if (!is_positive)
				{
					return std::nullopt;
				}
			}
			new_offset = BuildInDeformatter::GetNumbers(string, new_offset);
			if (offset == new_offset)
			{
				return std::nullopt;
			}
			auto str = string.substr(offset, new_offset - offset);
			std::array<std::byte, 32 * sizeof(char)> buffer;
			std::pmr::monotonic_buffer_resource resource(buffer.data(), buffer.size());
			std::pmr::string temp_str(&resource);
			Encode::UnicodeEncoder<CharT, char>::EncodeTo(str, std::back_inserter(temp_str));
			auto re = std::from_chars(temp_str.data(), temp_str.data() + temp_str.size(), output);
			if (re.ec == std::errc{})
				return new_offset;
			return std::nullopt;
		}
	};
}