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
			Encode::StrEncoder<UnicodeType, wchar_t> encoder;
			wchar_t buffer[2];
			while (!par.empty())
			{
				auto info = encoder.Encode(std::span(par), { buffer, 2 });
				wss.write(buffer, info.target_space);
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

	template<typename TargetType, typename CharaTrai, typename Allocator, typename UnicodeT>
	struct Scanner<std::basic_string<TargetType, CharaTrai, Allocator>, UnicodeT> : BuildInNumberScanner<int64_t, UnicodeT>
	{
		bool Scan(std::basic_string_view<UnicodeT> Par, std::basic_string<TargetType, CharaTrai, Allocator>& Output)
		{
			std::size_t OldSize = Output.size();
			Encode::EncodeInfo Str = Encode::StrEncoder<UnicodeT, TargetType>::RequireSpaceUnSafe(Par);
			Output.resize(Output.size() + Str.target_space);
			auto OutputSpan = std::span(Output).subspan(OldSize);
			Encode::StrEncoder<UnicodeT, TargetType>::EncodeUnSafe(Par, OutputSpan);
			return true;
		}
	};
}

export namespace Potato::Format
{
	enum class FormatterSyntaxType
	{
		String,
		Parameter
	};

	template<typename CharT, FormatterSyntaxType type, std::size_t index>
	struct FormatterSyntaxPoint
	{
		std::basic_string_view<CharT> syntax;
		constexpr std::size_t GetIndex() const { return index; }
	};

	struct FormatterSyntax
	{
		template<typename CharT> struct LeftBrace;
		template<> struct LeftBrace<char8_t> { static constexpr char8_t Value = u8'{'; static constexpr std::u8string_view Str = u8"{"; };
		template<> struct LeftBrace<wchar_t> { static constexpr wchar_t Value = L'{'; static constexpr std::wstring_view Str = L"{"; };
		template<> struct LeftBrace<char16_t> { static constexpr char16_t Value = u'{'; static constexpr std::u16string_view Str = u"{"; };
		template<> struct LeftBrace<char32_t> { static constexpr char32_t Value = U'{'; static constexpr std::u32string_view Str = U"{"; };

		template<typename CharT> struct RightBrace;
		template<> struct RightBrace<char8_t> { static constexpr char8_t Value = u8'}'; static constexpr std::u8string_view Str = u8"}"; };
		template<> struct RightBrace<wchar_t> { static constexpr wchar_t Value = L'}'; static constexpr std::wstring_view Str = L"}"; };
		template<> struct RightBrace<char16_t> { static constexpr char16_t Value = u'}'; static constexpr std::u16string_view Str = u"}"; };
		template<> struct RightBrace<char32_t> { static constexpr char32_t Value = U'}'; static constexpr std::u32string_view Str = U"}"; };
	
		template<typename CharT>
		struct TopSyntaxPoint
		{
			std::basic_string_view<CharT> string;
			FormatterSyntaxType type;
			std::basic_string_view<CharT> next_string;
		};

		template<typename CharT>
		static constexpr std::optional<TopSyntaxPoint<CharT>> FindSyntaxPoint(std::basic_string_view<CharT> formatter_pattern)
		{
			auto first_brace = std::find_if(
				formatter_pattern.begin(),
				formatter_pattern.end(),
				[](CharT character) {
					return character == LeftBrace<CharT>::Value || character == RightBrace<CharT>::Value;
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
					{first_brace + 2, formatter_pattern.end()}
				};
			}

			if (cur == LeftBrace<CharT>::Value)
			{
				if (first_brace != formatter_pattern.begin())
				{
					return TopSyntaxPoint<CharT>{
						{formatter_pattern.begin(), first_brace},
						FormatterSyntaxType::String,
						{first_brace, formatter_pattern.end()}
					};
				}

				auto second_brace = std::find_if(
					first_brace + 1,
					formatter_pattern.end(),
					[](CharT character) {
						return character == LeftBrace<CharT>::Value || character == RightBrace<CharT>::Value;
					}
				);

				if (second_brace == formatter_pattern.end())
				{
					return std::nullopt;
				}

				if (*second_brace == RightBrace<CharT>::Value)
				{
					return TopSyntaxPoint<CharT>{
						{first_brace + 1, second_brace},
						FormatterSyntaxType::Parameter,
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
		static constexpr std::optional<std::size_t> GetSyntaxPointCount(std::basic_string_view<CharT> format_pattern)
		{
			std::size_t count = 0;
			while (!format_pattern.empty())
			{
				auto point = FindSyntaxPoint(format_pattern);
				if (!point.has_value())
				{
					return std::nullopt;
				}
				format_pattern = point->next_string;
				++count;
			}
			return count;
		}

		template<typename CharT, typename ...Parameter>
		consteval auto Translate(std::basic_string_view<CharT> str, std::tuple<Parameter...> const& par)
		{
			if (str.empty())
				return par;

			return std::tuple_cat(par, std::make_tuple(k));
		}
	};

	template<TMP::TypeString type_string>
		requires(FormatterSyntax::GetSyntaxPointCount(type_string.GetStringView()).has_value())
	struct StaticFormatPattern
	{
		static constexpr std::size_t pattern_count = *FormatterSyntax::GetSyntaxPointCount(type_string.GetStringView());
		
		static constexpr auto pattern = FormatterSyntax::Translate(type_string.GetStringView(), std::tuple<>{});
		
		consteval StaticFormatPattern(StaticFormatPattern const& other) = default;
		consteval StaticFormatPattern() {}
		consteval std::size_t GetPatternCount() const { return pattern_count; }
	};

	/*
		template<bool GetSize, std::size_t CurIndex, typename CharT, typename CharTT>
		constexpr std::optional<std::size_t> FormatExe(std::span<CharT> Output, std::basic_string_view<CharT, CharTT> Par, std::size_t RequireSize)
		{
			return {};
		}

		template<bool GetSize, std::size_t CurIndex, typename CharT, typename CharTT, typename CurType, typename ...OtherType>
		constexpr std::optional<std::size_t> FormatExe(std::span<CharT> Output, std::basic_string_view<CharT, CharTT> Par, std::size_t RequireSize, CurType&& CT, OtherType&& ...OT)
		{
			if (RequireSize == CurIndex)
			{
				if constexpr (GetSize)
				{
					return Formatter<std::remove_cvref_t<CurType>, CharT>::FormatSize(Par, CT);
				}
				else
					return Formatter<std::remove_cvref_t<CurType>, CharT>::Format(Output, Par, CT);
			}
			else {
				return FormatExe<GetSize, CurIndex + 1>(Output, Par, RequireSize, std::forward<OtherType>(OT)...);
			}
		}

		struct ApplyResult
		{
			std::size_t Count;
			std::size_t Index;
			std::size_t OutputUsed;
		};

		template<bool GetSize, typename CharT, typename ...OtherType >
		constexpr std::optional<ApplyResult> ApplyFormat(std::span<CharT> Output, std::size_t Index, FormatPatternT Type, std::basic_string_view<CharT> Par, OtherType&& ...OT)
		{
			switch (Type)
			{
			case Implement::FormatPatternT::BadFormat:
				return {};
			case Implement::FormatPatternT::NormalString:
				if constexpr (!GetSize)
				{
					std::copy_n(Par.begin(), Par.size(), Output.begin());
				}
				return ApplyResult{ Par.size(), Index, Par.size() };
			case Implement::FormatPatternT::Parameter:
			{
				auto Re = FormatExe<GetSize, 0>(Output, Par, Index, std::forward<OtherType>(OT)...);
				if (Re.has_value())
				{
					return ApplyResult{ *Re, Index + 1, *Re };
				}
				else
					return {};
			}
			default:
				return {};
			}
		}
	}

	template<typename CharT, typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(std::basic_string_view<CharT> Formatter, OType&& ...OT) {
		std::basic_string_view Str = Formatter;
		std::size_t Count = 0;
		std::size_t Index = 0;
		while (!Str.empty())
		{
			auto [Type, Par, Last] = Implement::FindFomatterTopElement(Str);
			auto Re = Implement::ApplyFormat<true>({}, Index, Type, Par, std::forward<OType>(OT)...);
			if (Re.has_value())
			{
				Count += Re->Count;
				Index = Re->Index;
			}
			else
				return {};
			Str = Last;
		}
		return Count;
	}

	template<typename CharT, typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(const CharT* Formatter, OType&& ...OT) {
		return FormatSize(std::basic_string_view<CharT>{Formatter}, std::forward<OType>(OT)...);
	}

	template<typename CharT, typename ...OType>
	constexpr std::optional<std::size_t> FormatToUnSafe(std::span<CharT> OutputBuffer, std::basic_string_view<CharT> Formatter, OType&& ...OT) {
		auto Str = Formatter;
		std::size_t Count = 0;
		std::size_t Index = 0;
		while (!Str.empty())
		{
			auto [Type, Par, Last] = Implement::FindFomatterTopElement(Str);
			auto Re = Implement::ApplyFormat<false>(OutputBuffer, Index, Type, Par, std::forward<OType>(OT)...);
			if (Re.has_value())
			{
				Count += Re->Count;
				Index = Re->Index;
				OutputBuffer = OutputBuffer.subspan(Re->OutputUsed);
			}
			else
				return {};
			Str = Last;
		}
		return Count;
	}

	template<typename CharT, typename ...OType>
	constexpr std::optional<std::size_t> FormatToUnSafe(std::span<CharT> OutputBuffer, const CharT* Formatter, OType&& ...OT) {
		return FormatToUnSafe(OutputBuffer, std::basic_string_view<CharT>{Formatter}, std::forward<OType>(OT)...);
	}

	template<typename CharT, typename ...OType>
	auto Format(std::basic_string_view<CharT> Formatter, OType&& ...OT) -> std::optional<std::basic_string<CharT>> {
		std::basic_string<CharT> Buffer;
		auto Re = FormatSize(Formatter, std::forward<OType>(OT)...);
		if (Re)
		{
			Buffer.resize(*Re);
			auto Re2 = FormatToUnSafe(std::span(Buffer), Formatter, std::forward<OType>(OT)...);
			if (Re2)
			{
				return Buffer;
			}
		}
		return {};
	}

	template<typename CharT, typename ...OType>
	auto Format(const CharT* Formatter, OType&& ...OT) -> std::optional<std::basic_string<CharT>>
	{
		return Format(std::basic_string_view<CharT>{Formatter}, std::forward<OType>(OT)...);
	}

	namespace Implement
	{

		template<typename CharT>
		constexpr std::size_t StaticFormatCount(std::basic_string_view<CharT> Str)
		{
			std::size_t Count = 0;
			while (!Str.empty())
			{
				auto [Type, Par, Last] = FindFomatterTopElement(Str);
				++Count;
				Str = Last;
			}
			return Count;
		}

		template<std::size_t N, typename CharT>
		constexpr std::array<std::tuple<Implement::FormatPatternT, std::basic_string_view<CharT>>, N> StaticFormatExe(std::basic_string_view<CharT> Str)
		{
			std::array<std::tuple<Implement::FormatPatternT, std::basic_string_view<CharT>>, N> Tem;
			std::size_t Count = 0;
			while (!Str.empty())
			{
				auto [Type, Par, Last] = FindFomatterTopElement(Str);
				Tem[Count++] = { Type, Par };
				Str = Last;
			}
			return Tem;
		}

	}


	template<TMP::TypeString Formatter>
	struct StaticFormatPattern
	{
		using Type = typename decltype(Formatter)::Type;
		static constexpr std::basic_string_view<Type> StrFormatter{ Formatter.Storage, decltype(Formatter)::Len - 1 };
		static constexpr std::size_t ElementCount = Implement::StaticFormatCount(StrFormatter);
		static constexpr std::array<std::tuple<Implement::FormatPatternT, std::basic_string_view<Type>>, ElementCount> Patterns
			= Implement::StaticFormatExe<ElementCount>(StrFormatter);

		template<typename ...OT>
		static constexpr std::optional<std::size_t> FormatSize(OT&& ...ot) {
			std::size_t Count = 0;
			std::size_t Index = 0;
			for (auto Ite : Patterns)
			{
				auto [Type, Par] = Ite;
				auto Re = Implement::ApplyFormat<true>({}, Index, Type, Par, std::forward<OT>(ot)...);
				if (Re.has_value())
				{
					Count += Re->Count;
					Index = Re->Index;
				}
				else
					return {};
			}
			return Count;
		}

		template<typename ...OT>
		static constexpr  std::optional<std::size_t> FormatToUnsafe(std::span<Type> Output, OT&& ...ot) {
			std::size_t Count = 0;
			std::size_t Index = 0;
			for (auto Ite : Patterns)
			{
				auto [Type, Par] = Ite;
				auto Re = Implement::ApplyFormat<false>(Output, Index, Type, Par, std::forward<OT>(ot)...);
				if (Re.has_value())
				{
					Count += Re->Count;
					Index = Re->Index;
					Output = Output.subspan(Re->OutputUsed);
				}
				else
					return {};
			}
			return Count;
		}

	};

	template<TMP::TypeString Formatter, typename ...Type>
	auto StaticFormat(Type&& ...Cur) -> std::optional<std::basic_string<typename StaticFormatPattern<Formatter>::Type>>
	{
		std::basic_string<typename StaticFormatPattern<Formatter>::Type> Par;
		auto Re = StaticFormatPattern<Formatter>::FormatSize(std::forward<Type>(Cur)...);
		if (Re.has_value())
		{
			Par.resize(*Re);
			Re = StaticFormatPattern<Formatter>::FormatToUnsafe(std::span(Par), std::forward<Type>(Cur)...);
			if (*Re)
				return Par;
		}
		return {};
	}

	template<typename CharT, typename SourceT>
	std::optional<std::size_t> DirectFormatSize(std::basic_string_view<CharT> Par, SourceT&& S)
	{
		return Formatter<std::remove_cvref_t<SourceT>, CharT>::FormatSize(Par, std::forward<SourceT>(S));
	}

	template<typename CharT, typename SourceT>
	std::optional<std::size_t> DirectFormatToUnSafe(std::span<CharT> Output, std::basic_string_view<CharT> Par, SourceT& S)
	{
		return Formatter<std::remove_cvref_t<SourceT>, CharT>::Format(Output, Par, std::forward<SourceT>(S));
	}


	template<typename CharT, typename SourceT>
	auto DirectFormat(std::basic_string_view<CharT> Par, SourceT&& S) -> std::optional<std::basic_string<CharT>> {
		auto Size = Formatter<std::remove_cvref_t<SourceT>, CharT>::FormatSize(Par, std::forward<SourceT>(S));
		if (Size.has_value())
		{
			std::basic_string<CharT> Result(*Size, 0);
			auto Re = Formatter<std::remove_cvref_t<SourceT>, CharT>::Format(std::span(Result), Par, std::forward<SourceT>(S));
			if (Re.has_value() && *Size == *Re)
				return Result;
		}
		return {};
	}

	template<typename CharT, typename SourceT>
	auto DirectFormat(const CharT* Par, SourceT&& S) -> std::optional<std::basic_string<CharT>> {
		return DirectFormat(std::basic_string_view<CharT>{Par}, std::forward<SourceT>(S));
	}

}
*/


}

namespace std
{
	template<class CharT>
	concept PotatoEncodeableChar = Potato::TMP::IsOneOfV<CharT, char8_t, char16_t, char32_t>;

	template<class CharT>
	concept PotatoEncodeableTarget = Potato::TMP::IsOneOfV<CharT, char, wchar_t>;

	template<PotatoEncodeableChar CharT, PotatoEncodeableTarget TargetCharT>
	struct formatter<std::basic_string_view<CharT>, TargetCharT>
	{
		constexpr auto parse(std::basic_format_parse_context<TargetCharT>& parse_context)
		{
			return parse_context.end();
		}
		template<typename FormatContext>
		constexpr auto format(std::basic_string_view<CharT> view, FormatContext& format_context) const
		{
			TargetCharT tem_buffer[4];
			Potato::Encode::StrEncoder<CharT, TargetCharT> encoder;
			while (!view.empty())
			{
				auto info = encoder.Encode(view, std::span<TargetCharT, 4>{ tem_buffer, 4 });
				std::copy_n(
					tem_buffer,
					info.target_space,
					format_context.out()
				);
				view = view.substr(info.source_space);
				if (info.source_space == 0)
					break;
			}
			return format_context.out();
		}
	};

	template<PotatoEncodeableChar CharT, std::size_t N, PotatoEncodeableTarget TargetCharT>
	struct formatter<CharT[N], TargetCharT>
		: formatter<std::basic_string_view<CharT>, TargetCharT>
	{
		using ParentType = formatter<std::basic_string_view<CharT>, TargetCharT>;
		constexpr auto parse(std::basic_format_parse_context<TargetCharT>& parse_context)
		{
			return ParentType::parse(parse_context);
		}
		template<typename FormatContext>
		constexpr auto format(CharT const str[N], FormatContext& format_context) const
		{
			return ParentType::format(std::basic_string_view<CharT>(str), format_context);
		}
	};
}