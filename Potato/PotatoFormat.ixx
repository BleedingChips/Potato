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
				if (iterator == decltype(string)::npos)
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

		template<TMP::TypeString type_string>
		struct PatternSectionWrapper
		{

			static Reg::Dfa const& GetDFA()
			{
				static const auto dfa = Reg::Dfa(
					Reg::Dfa::FormatE::HeadMatch,
					type_string.GetStringView()
				);
				return dfa;
			}
			
			template<typename CharT>
			static std::optional<std::size_t> Execute(std::basic_string_view<CharT> string)
			{
				Reg::DfaProcessor processer;
				processer.SetObserverTable(GetDFA());
				auto accept = Reg::Process(processer, string);
				if (accept)
				{
					return accept.GetMainCapture().Size();
				}
				return std::nullopt;
			}
		};

		template<TMP::TypeString type_string>
			requires(type_string.Size() == 0 || type_string.string[0] == 0)
		struct PatternSectionWrapper<type_string>
		{
			template<typename CharT>
			static std::optional<std::size_t> Execute(std::basic_string_view<CharT> string)
			{
				return 0;
			}
		};

		template<TMP::TypeString type_string, typename ValueT, typename CharT, std::size_t index>
		struct PatternSection
		{
			Deformatter<ValueT, CharT> deformatter;
			using Type = PatternSectionWrapper<type_string>;


			template<typename CharT, typename ...AT>
			std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, AT&& ...output) const
			{
				std::size_t offset = 0;
				auto match = Type::Execute(string);

				if (match.has_value())
				{
					offset = *match;
					string = string.substr(offset);
				}
				else {
					return std::nullopt;
				}
				
				auto info = deformatter.Deformat(string, TMP::Picker<index>::PickValue(std::forward<AT>(output)...));
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
						TMP::TypeStringSlicer<type_string, 
							std::get<0>(pattern_pairs)[i].string_index.Begin(), 
							std::get<0>(pattern_pairs)[i].string_index.Size()
						>::string,
						TMP::Picker<i>::template PickTypeT<AT..., void>,
						CharT,
						std::get<0>(pattern_pairs)[i].parameter_index
					>{std::get<i>(std::get<1>(pattern_pairs))}
				)...
			);
		}(std::make_index_sequence<GetSectionCount()>());
	};


	template<TMP::TypeString type_string, typename CharT, typename CharTraits, typename ...AT>
		requires(std::is_same_v<typename decltype(type_string)::Type, CharT>)
	DeformatInfo Deformat(std::basic_string_view<CharT, CharTraits> str, AT&& ...at)
	{
		static constexpr DeformatPattern<type_string, std::remove_cvref_t<AT>...> pattern;
		return DeformatSyntax::ExecuteDeformat<sizeof...(AT) + 1>::Execute(str, pattern.pattern, std::forward<AT>(at)..., TMP::ItSelf<void>{});
	}

	template<TMP::TypeString type_string, typename CharT, std::size_t N, typename ...AT>
		requires(std::is_same_v<typename decltype(type_string)::Type, CharT>)
	DeformatInfo Deformat(const CharT (&str)[N], AT&& ...at)
	{
		return Deformat(std::basic_string_view<CharT>{str}, std::forward<AT>(at)...);
	}

	template<typename CharT, typename CharTraits, typename AT>
	DeformatInfo DirectDeformat(std::basic_string_view<CharT, CharTraits> str, AT&& at)
	{
		Deformatter<std::remove_cvref_t<AT>, CharT> deformatter;
		auto index = deformatter.Deformat(str, std::forward<AT>(at));
		if (index.has_value())
		{
			return { *index, 1, *index == str.size() };
		}
		return {0, 0, false};
	}

	template<typename CharT, std::size_t N, typename AT>
	DeformatInfo DirectDeformat(const CharT (&str)[N], AT&& at)
	{
		return DirectDeformat(std::basic_string_view<CharT>{str}, std::forward<AT>(at));
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
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
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
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
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

	template<typename Traits, typename CharT>
	struct Deformatter<std::basic_string_view<CharT, Traits>, CharT>
	{
		std::basic_string_view<CharT, Traits> end_string;
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
		{
			std::size_t start = 0;
			if (context.size() > 0 && context[start] == static_cast<CharT>('\\'))
			{
				start += 1;
			}
			std::size_t offset = (start == 0 ? 0 : 2);
			while (offset < context.size())
			{
				auto finded = context.find(static_cast<CharT>('}'), offset);
				if (finded == decltype(context)::npos)
					return context.size();
				if (finded != 0 && finded < context.size())
				{
					if (context[finded - 1] == static_cast<CharT>('\\'))
					{
						offset = finded + 1;
						continue;
					}
				}
				end_string = context.substr(start, finded - start);
				return finded;
			}
			return context.size();
		}

		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, std::basic_string_view<CharT, Traits>& output) const
		{
			if (end_string.size() > 0)
			{
				for (std::size_t index = 0; index < string.size(); ++index)
				{
					std::size_t sub_index = index;
					std::size_t end_string_index = 0;
					bool force_compares = false;
					while (sub_index < string.size() && end_string_index < end_string.size())
					{
						if (!force_compares && end_string[end_string_index] == static_cast<CharT>('\\'))
						{
							force_compares = true;
							++end_string_index;
							continue;
						}
						force_compares = false;
						if (end_string[end_string_index] == string[sub_index])
						{
							sub_index += 1;
							end_string_index += 1;
						}
						else {
							break;
						}
					}
					if (end_string_index == end_string.size())
					{
						output = string.substr(0, index);
						return index + 1;
					}
				}
			}
			output = string;
			return string.size();
		}
	};

	template<typename IteratorT, typename CharT>
		requires(std::output_iterator<IteratorT, CharT>)
	struct Deformatter<IteratorT, CharT> : public Deformatter<std::basic_string_view<CharT>, CharT>
	{
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
		{
			return Deformatter<std::basic_string_view<CharT>, CharT>::Parse(context);
		}

		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, IteratorT&& output) const
		{
			auto out = std::move(output);
			return Deformat(string, static_cast<IteratorT&>(out));
		}

		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, IteratorT& output) const
		{
			std::basic_string_view<CharT> out;
			auto result = Deformatter<std::basic_string_view<CharT>, CharT>::Deformat(string, out);
			if (result.has_value())
			{
				output = std::copy_n(
					out.begin(),
					out.size(),
					output
				);
				return result;
			}
			return std::nullopt;
		}
	};

	template<typename OCharT, typename TraitT, typename AllocatorT, typename CharT>
	struct Deformatter<std::basic_string<OCharT, TraitT, AllocatorT>, CharT> : public Deformatter<std::basic_string_view<CharT>, CharT>
	{
		constexpr std::optional<std::size_t> Parse(std::basic_string_view<CharT> context)
		{
			return Deformatter<std::basic_string_view<CharT>, CharT>::Parse(context);
		}

		std::optional<std::size_t> Deformat(std::basic_string_view<CharT> string, std::basic_string<OCharT, TraitT, AllocatorT>& output) const
		{
			std::basic_string_view<CharT> out;
			auto result = Deformatter<std::basic_string_view<CharT>, CharT>::Deformat(string, out);
			if (result)
			{
				Encode::UnicodeEncoder<CharT, OCharT>::EncodeTo(out, std::back_inserter(output));
			}
			return std::nullopt;
		}
	};
}