#pragma once
#include <vector>
#include <variant>
#include <string>
#include <string_view>
#include <span>
#include <optional>
#include <cassert>
#include <sstream>
#include "PotatoReg.h"
#include "PotatoStrEncode.h"

namespace Potato::StrFormat
{

	template<typename SourceType, typename UnicodeType>
	struct Scanner
	{
		bool Scan(std::span<UnicodeType const> Par, SourceType& Input);
	};


	template<typename UnicodeType, typename TargetType>
	bool DirectScan(std::basic_string_view<UnicodeType> Parameter, TargetType& tar_type)
	{
		return Scanner<std::remove_cvref_t<TargetType>, UnicodeType>{}.Scan(Parameter, tar_type);
	}


	struct ScanPattern
	{

		struct ScanResult
		{
			std::size_t Count;
			std::size_t SearchLength;
		};

		template<typename UnicodeT, typename ...TargetType>
		std::optional<size_t> March(std::span<UnicodeT const> code, TargetType&... all_target) const;

		template<typename UnicodeT, typename ...TargetType>
		std::optional<ScanResult> Search(std::span<UnicodeT const> code, TargetType&... all_target) const;

		ScanPattern(std::vector<Reg::TableWrapper::StorageT> Tables) : Tables(std::move(Tables)) {};
		ScanPattern(ScanPattern&&) = default;
		ScanPattern(ScanPattern const&) = default;
		ScanPattern(std::u32string_view Pattern) : ScanPattern(Create(Pattern)) {}

		static ScanPattern Create(std::span<char32_t const> pattern) { return { Reg::TableWrapper::Create(pattern) }; }
		
	private:
		std::vector<Reg::TableWrapper::StorageT> Tables;

		template<typename CurTarget, typename ...TargetType>
		static std::optional<std::size_t> Dispatch(std::size_t Record, std::span<Reg::Capture const> Records, std::span<char32_t const> Chars, CurTarget& target, TargetType&... other)
		{
			auto Result = TranslateCapture(Records, Chars);
			if (Result.has_value())
			{
				auto [Str, Span] = *Result;
				if(DirectScan(Str, target))
					return Dispatch(Record + 1, Span, Chars, other...);
				else
					return {};
			}
			return Record;
		}
		static std::optional<std::size_t> Dispatch(std::size_t Record, std::span<Reg::Capture const> Records, std::span<char32_t const> Chars) { return Record; }
		static std::optional<std::tuple<std::span<char32_t const>, std::span<Reg::Capture const>>> TranslateCapture(std::span<Reg::Capture const> Records, std::span<char32_t const> Str);
	};

	template<typename UnicodeT, typename ...TargetType>
	std::optional<size_t> ScanPattern::March(std::span<UnicodeT const> code, TargetType&... all_target) const
	{
		auto result = Reg::MarchProcessor::Process(Reg::TableWrapper(Tables), std::span(code));
		if (result)
		{
			return ScanPattern::Dispatch(0, std::span(result->Captures), code, all_target...);
		}
		return {};
	}

	template<typename UnicodeT, typename ...TargetType>
	std::optional<ScanPattern::ScanResult> ScanPattern::Search(std::span<UnicodeT const> code, TargetType&... all_target) const
	{
		auto result = Reg::SearchProcessor::Process(Reg::TableWrapper(Tables), std::span(code));
		if (result)
		{
			auto re2 = ScanPattern::Dispatch(0, std::span(result->Captures), code, all_target...);
			if (re2.has_value())
			{
				return ScanResult{*re2, result->MainCapture.End()};
			}
		}
		return {};
	}


	template<typename UnicodeT, typename ...TargetType>
	std::optional<size_t> MarchScan(ScanPattern const& pattern, std::span<UnicodeT const> in, TargetType& ... tar_type) { return pattern.March(in, tar_type...); }
	template<typename UnicodeT, typename ...TargetType>
	std::optional<size_t> MarchScan(std::span<UnicodeT const> pattern, std::span<UnicodeT const> in, TargetType& ... tar_type) {
		ScanPattern cur_pattern{ pattern };
		return cur_pattern.March(in, tar_type...);
	}

	template<typename UnicodeT, typename ...TargetType>
	std::optional<ScanPattern::ScanResult> SearchScan(ScanPattern const& pattern, std::span<UnicodeT const> in, TargetType& ... tar_type) { return pattern.Search(in, tar_type...); }
	template<typename UnicodeT, typename ...TargetType>
	std::optional<ScanPattern::ScanResult> SearchScan(std::span<UnicodeT const> pattern, std::span<UnicodeT const> in, TargetType& ... tar_type) {
		ScanPattern cur_pattern{ pattern };
		return cur_pattern.Search(in, tar_type...);
	}

	template<typename SourceType, typename UnicodeType>
	struct Formatter
	{
		bool Format(std::span<UnicodeType> Output, SourceType const& Input);
		std::optional<std::size_t> FormatSize(std::span<UnicodeType const> Parameter, SourceType const& Input);
	};


	template<typename UnicodeType, typename TargetType>
	bool DirectoFormatTo(std::basic_string<UnicodeType>& Output, std::basic_string_view<UnicodeType> Str, TargetType const& Type)
	{
		Formatter<std::remove_cvref_t<TargetType>, UnicodeType> Matter;
		auto Size = Matter.FormatSize(Str, Type);
		if (Size.has_value())
		{
			auto OldSize = Output.size();
			Output.resize(Output.size() + *Size);
			auto Span = std::span(Output);
			if (Matter.Format(Span.subspan(OldSize), Type))
				return true;
			Output.resize(OldSize);
			return false;
		}
		return false;
	}

	template<typename UnicodeType, typename TargetType>
	std::basic_string<UnicodeType> DirectoFormat(std::basic_string_view<UnicodeType> Str, TargetType const& Type)
	{
		std::basic_string<UnicodeType> Result;
		if(DirectoFormatTo(Result, Str, Type))
			return Result;
		return {};
	}

	struct CoreFormatPattern
	{
		enum class TypeT
		{
			NormalString = 0,
			Parameter,
		};

		struct Element
		{
			TypeT Type;
			Misc::IndexSpan<> Index;
		};

		CoreFormatPattern(Misc::IndexSpan<> Index, Reg::CodePoint(*F1)(std::size_t Index, void* Data), void* Data);
		CoreFormatPattern(std::u32string_view Str);
		CoreFormatPattern(CoreFormatPattern&&) = default;
		CoreFormatPattern(CoreFormatPattern const&) = default;

		std::vector<Element> Elements;
	};

	template<typename UnicodeT>
	struct FormatPattern
	{

		static FormatPattern Create(std::basic_string_view<UnicodeT> Str){ return FormatPattern(Str); }

		FormatPattern(FormatPattern&&) = default;
		FormatPattern(FormatPattern const&) = default;
		FormatPattern(std::basic_string_view<UnicodeT> Str) : string(Str), Elements(Str) {}

		template<typename ...AT>
		bool FormatTo(std::u32string& Output, AT&& ...at) const;

		template<typename ...AT>
		std::optional<std::u32string> Format(AT&& ...at) const
		{
			std::u32string Result;
			if (FormatTo(Result, std::forward<AT>(at)...))
			{
				return Result;
			}
			return {};
		}	

	private:

		/*
		template<typename CT, typename ...OT>
		static bool Dispatch(std::span<Element const> Elements, std::u32string& Output, CT&& ct, OT&& ...at);

		static bool Dispatch(std::span<Element const> Elements, std::u32string& Output);
		*/

		std::basic_string_view<UnicodeT> string;
		CoreFormatPattern Elements;
	};

	/*
	template<typename ...AT>
	bool FormatPattern::FormatTo(std::u32string& Output, AT&& ...at) const
	{
		return FormatPattern::Dispatch(patterns, Output, std::forward<AT>(at)...);
	}

	template<typename CT, typename ...OT>
	bool FormatPattern::Dispatch(std::span<Element const> Elements, std::u32string& Output, CT&& ct, OT&& ...at)
	{
		std::size_t Index = 0;
		for(; Index < Elements.size(); ++Index)
			if(Elements[Index].type != Type::NormalString)
				break;
		if (Index != 0)
		{
			auto Re = FormatPattern::Dispatch(Elements.subspan(0, Index), Output);
			assert(Re);
		}
		if (Index < Elements.size())
		{
			auto Cur = Elements[Index];
			assert(Cur.type == Type::Parameter);
			return DirectFormatTo(Output, Cur.string, std::forward<CT>(ct)) && Dispatch(Elements.subspan(Index + 1), Output, std::forward<OT>(at)...);
		}
		return true;
	}

	template<typename ...Type>
	bool FormatTo(std::u32string_view PatternStr, std::u32string& Output, Type&&... Args)
	{
		FormatPattern Pat(PatternStr);
		return Pat.FormatTo(Output, std::forward<Type>(Args)...);
	}

	template<typename ...Type>
	std::optional<std::u32string> Format(std::u32string_view PatternStr, Type&&... Args)
	{
		FormatPattern Pat(PatternStr);
		return FormatTo(Pat, std::forward<Type>(Args)...);
	}
	*/
}

namespace Potato::StrFormat
{

	template<typename NumberType, typename UnicodeType>
	struct BuildInNumberScanner
	{
		std::wstringstream wss;
		bool Scan(std::basic_string_view<UnicodeType> Par, NumberType& Input)
		{
			wchar_t Buffer[2];
			while (!Par.empty())
			{
				auto P = StrEncode::CoreEncoder<UnicodeType, wchar_t>::EncodeOnceUnSafe(std::span(Par), { Buffer, 2});
				wss.write(Buffer, P.TargetSpace);
				Par = Par.substr(P.SourceSpace);
			}
			wss >> Input;
			return true;
		}
	};

	template<>
	struct Scanner<float, char32_t> : BuildInNumberScanner<float, char32_t> {};

	template<>
	struct Scanner<int32_t, char32_t> : BuildInNumberScanner<int32_t, char32_t> {};

	template<>
	struct Scanner<uint32_t, char32_t> : BuildInNumberScanner<uint32_t, char32_t> {};

	template<>
	struct Scanner<uint64_t, char32_t> : BuildInNumberScanner<uint64_t, char32_t> {};

	template<>
	struct Scanner<int64_t, char32_t> : BuildInNumberScanner<int64_t, char32_t> {};

	template<typename NumberType, typename UnicodeType>
	struct BuildInNumberFormatter
	{
		std::wstringstream wss;
		bool Format(std::span<UnicodeType> Output, NumberType Input) {
			std::size_t Index = 0;
			while (true)
			{
				wchar_t Temp;
				wss.read(&Temp, 1);
				if (wss.good())
				{
					auto Result = StrEncode::CoreEncoder<wchar_t, UnicodeType>::EncodeOnceUnSafe({ &Temp, 1 }, Output.subspan(Index));
					Index += Result.RequireSpace;
				}
				else {
					break;
				}
			}
			return true;
		}
		std::optional<std::size_t> FormatSize(std::span<UnicodeType const> Parameter, NumberType Input) {
			wss << Input;
			return static_cast<std::size_t>(wss.tellp());
		}
	};

	template<>
	struct Formatter<uint64_t, char32_t> : BuildInNumberFormatter<uint64_t, char32_t> {};

	template<>
	struct Formatter<int64_t, char32_t> : BuildInNumberFormatter<int64_t, char32_t> {};

	template<>
	struct Formatter<float, char32_t> : BuildInNumberFormatter<float, char32_t> {};

	template<>
	struct Formatter<double, char32_t> : BuildInNumberFormatter<double, char32_t> {};

}

namespace Potato::StrFormat::Exception
{

	struct FormatterInterface : std::exception
	{
		virtual ~FormatterInterface() = default;
		virtual char const* what() const override { return nullptr; }
	};

	struct UnsupportPatternString : public FormatterInterface
	{
		std::u32string Pattern;
		UnsupportPatternString(std::u32string Pattern) : Pattern(std::move(Pattern)) {};
		UnsupportPatternString(UnsupportPatternString const&) = default;
		UnsupportPatternString(UnsupportPatternString&&) = default;
		virtual char const* what() const override { return nullptr; }
	};
}