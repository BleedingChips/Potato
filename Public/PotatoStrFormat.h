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

	template<typename TargetType>
	bool DirectScan(std::u32string_view Parameter, TargetType& tar_type)
	{
		return Scanner<std::remove_cvref_t<TargetType>, char32_t>{}.Scan(Parameter, tar_type);
	}

	template<typename TargetType>
	bool DirectScan(std::u16string_view Parameter, TargetType& tar_type)
	{
		return Scanner<std::remove_cvref_t<TargetType>, char16_t>{}.Scan(Parameter, tar_type);
	}

	template<typename TargetType>
	bool DirectScan(std::u8string_view Parameter, TargetType& tar_type)
	{
		return Scanner<std::remove_cvref_t<TargetType>, char8_t>{}.Scan(Parameter, tar_type);
	}

	template<typename TargetType>
	bool DirectScan(std::wstring_view Parameter, TargetType& tar_type)
	{
		return Scanner<std::remove_cvref_t<TargetType>, wchar_t>{}.Scan(Parameter, tar_type);
	}

	struct ScanPattern
	{

		struct ScanResult
		{
			std::size_t Count;
			std::size_t SearchLength;
		};

		template<typename UnicodeT, typename ...TargetType>
		std::optional<size_t> Match(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const;

		template<typename UnicodeT, typename ...TargetType>
		std::optional<ScanResult> HeadMatch(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const;
		
		ScanPattern(ScanPattern&&) = default;
		ScanPattern(ScanPattern const&) = default;
		ScanPattern(Reg::DFATable Tables) : Tables(std::move(Tables)) {};

		template<typename UnicodeT>
		ScanPattern(std::basic_string_view<UnicodeT> Pattern) : ScanPattern(Create(Pattern)) {}

		template<typename UnicodeT>
		static ScanPattern Create(std::basic_string_view<UnicodeT> pattern) { 
			return Reg::DFATable{ pattern };
		}

		template<typename UnicodeT, typename CurTarget, typename ...TargetType>
		static std::optional<std::size_t> CaptureScan(Reg::CaptureWrapper TopCapture, std::basic_string_view<UnicodeT> Chars, CurTarget& Target, TargetType&... OTarget)
		{
			if(TopCapture.HasSubCapture())
				return Dispatch(0, TopCapture.GetTopSubCapture(), Chars, Target, OTarget...);
			return {};
		}
		
	private:

		Reg::DFATable Tables;

		template<typename UnicodeT, typename CurTarget, typename ...TargetType>
		static std::optional<std::size_t> Dispatch(std::size_t Record, Reg::CaptureWrapper Wrapper, std::basic_string_view<UnicodeT> Chars, CurTarget& Target, TargetType&... OTarget)
		{
			if (Wrapper.HasCapture())
			{
				auto Capture = Wrapper.GetCapture();
				if (DirectScan(Chars.substr(Capture.Begin(), Capture.Count()), Target))
				{
					if(Wrapper.HasNextCapture())
						return Dispatch(Record + 1, Wrapper.GetNextCapture(), Chars, OTarget...);
					else
						return Record;
				}
				else
					return {};
			}
			return Record;
		}
		template<typename UnicodeT>
		static std::optional<std::size_t> Dispatch(std::size_t Record, Reg::CaptureWrapper Wrapper, std::basic_string_view<UnicodeT> Chars) { return Record; }
	};

	template<typename UnicodeT, typename ...TargetType>
	std::optional<size_t> ScanPattern::Match(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const
	{
		auto result = Reg::Match(Tables, std::span(code));
		if (result)
		{
			return ScanPattern::Dispatch(0, result->GetCaptureWrapper().GetTopSubCapture(), code, all_target...);
		}
		return {};
	}

	template<typename UnicodeT, typename ...TargetType>
	std::optional<ScanPattern::ScanResult> ScanPattern::HeadMatch(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const
	{
		auto result = Reg::HeadMatch(Tables, std::span(code));
		if (result)
		{
			auto re2 = ScanPattern::Dispatch(0, result->GetCaptureWrapper().GetTopSubCapture(), code, all_target...);
			if (re2.has_value())
			{
				return ScanResult{*re2, result->MainCapture.End()};
			}
		}
		return {};
	}

	template<typename ...TargetType>
	auto MatchScan(std::u32string_view PatternStr, std::u32string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Match(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MatchScan(std::u16string_view PatternStr, std::u16string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Match(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MatchScan(std::u8string_view PatternStr, std::u8string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Match(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MatchScan(std::wstring_view PatternStr, std::wstring_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Match(Code, tar_type...);
	}


	template<typename ...TargetType>
	auto HeadMatchScan(std::u32string_view PatternStr, std::u32string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.HeadMatch(Code, tar_type...);
	}
	template<typename ...TargetType>
	auto HeadMatchScan(std::u16string_view PatternStr, std::u16string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.HeadMatch(Code, tar_type...);
	}
	template<typename ...TargetType>
	auto HeadMatchScan(std::u8string_view PatternStr, std::u8string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.HeadMatch(Code, tar_type...);
	}
	template<typename ...TargetType>
	auto HeadMatchScan(std::wstring_view PatternStr, std::wstring_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.HeadMatch(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto CaptureScan(Reg::CaptureWrapper Wrapper, std::u32string_view Code, TargetType& ... tar_type) {
		return ScanPattern::CaptureScan(Wrapper, Code, tar_type...);
	}
	template<typename ...TargetType>
	auto CaptureScan(Reg::CaptureWrapper Wrapper, std::u16string_view Code, TargetType& ... tar_type) {
		return ScanPattern::CaptureScan(Wrapper, Code, tar_type...);
	}
	template<typename ...TargetType>
	auto CaptureScan(Reg::CaptureWrapper Wrapper, std::u8string_view Code, TargetType& ... tar_type) {
		return ScanPattern::CaptureScan(Wrapper, Code, tar_type...);
	}
	template<typename ...TargetType>
	auto CaptureScan(Reg::CaptureWrapper Wrapper, std::wstring_view Code, TargetType& ... tar_type) {
		return ScanPattern::CaptureScan(Wrapper, Code, tar_type...);
	}


	template<typename SourceType, typename UnicodeType>
	struct Formatter
	{
		void Format(std::span<UnicodeType> Output, SourceType const& Input);
		std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, SourceType const& Input);
	};

	/*
	template<typename Function, typename UnicodeType, typename CharTria, typename TargetType>
	std::optional<std::size_t> DirectFormat(Function&& Func, std::basic_string_view<UnicodeType, CharTria> Par, TargetType const& Type)
		requires(std::is_invocable_r_v<std::span<UnicodeType>, Function, std::size_t>)
	{
		Formatter<std::remove_cvref_t<TargetType>, UnicodeType> Matter;
		auto Size = Matter.FormatSize(Str, Type);
		if (Size.has_value())
		{
			auto OutputSpan = Func(*Size);
			if (!OutputSpan.empty())
			{
				
			}
		}
	}
	*/

	template<typename UnicodeType, typename Function, typename TargetType>
	bool DirectFormat(Function&& F, std::basic_string_view<UnicodeType> Str, TargetType const& T) requires(std::is_invocable_r_v<std::span<UnicodeType>, Function, std::size_t>)
	{
		Formatter<std::remove_cvref_t<TargetType>, UnicodeType> Format;
		auto Size = Format.FormatSize(Str, T);
		if (Size.has_value())
		{
			auto Span = F(*Size);
			if (!Span.empty())
			{
				Format.Format(Span, T);
			}
			return true;
		}
		return false;
	}

	template<typename UnicodeType, typename CharTrai, typename Allocator, typename TargetType>
	bool DirectFormat(std::basic_string<UnicodeType, CharTrai, Allocator>& Output, std::basic_string_view<UnicodeType> Str, TargetType const& Type)
	{
		std::size_t OldSize = Output.size();
		return DirectFormat([&](std::size_t Count)->std::span<UnicodeType>{
			Output.resize(OldSize + Count);
			return std::span(Output).subspan(OldSize);
		}, Str, Type);
	}

	enum class PatternElementType
	{
		NormalString = 0,
		Parameter,
	};

	template<typename UnicodeT>
	struct PatternElement;

	template<>
	struct PatternElement<char32_t>
	{
		PatternElementType Type;
		std::basic_string_view<char32_t> String;
		static std::size_t FlushSize(std::size_t Count, std::span<PatternElement<char32_t> const>& Source);
		static void FlushBuffer(std::span<PatternElement<char32_t> const>& Source, std::span<char32_t>& Output);
		static std::vector<PatternElement<char32_t>> Create(std::u32string_view Str);
	};

	template<>
	struct PatternElement<char16_t>
	{
		PatternElementType Type;
		std::basic_string_view<char16_t> String;
		static std::size_t FlushSize(std::size_t Count, std::span<PatternElement<char16_t> const>& Source);
		static void FlushBuffer(std::span<PatternElement<char16_t> const>& Source, std::span<char16_t>& Output);
		static std::vector<PatternElement<char16_t>> Create(std::u16string_view Str);
	};

	template<>
	struct PatternElement<char8_t>
	{
		PatternElementType Type;
		std::basic_string_view<char8_t> String;
		static std::size_t FlushSize(std::size_t Count, std::span<PatternElement<char8_t> const>& Source);
		static void FlushBuffer(std::span<PatternElement<char8_t> const>& Source, std::span<char8_t>& Output);
		static std::vector<PatternElement<char8_t>> Create(std::u8string_view Str);
	};

	template<>
	struct PatternElement<wchar_t>
	{
		PatternElementType Type;
		std::basic_string_view<wchar_t> String;
		static std::size_t FlushSize(std::size_t Count, std::span<PatternElement<wchar_t> const>& Source);
		static void FlushBuffer(std::span<PatternElement<wchar_t> const>& Source, std::span<wchar_t>& Output);
		static std::vector<PatternElement<wchar_t>> Create(std::wstring_view Str);
	};


	template<typename UnicodeT, typename ...HoldingType>
	struct FormatterWrapper;;

	template<typename UnicodeT>
	struct FormatterWrapper<UnicodeT>
	{
		std::optional<std::size_t> FormatSize(std::size_t TotalCount, std::span<PatternElement<UnicodeT> const> Patterns)
		{
			auto Result = PatternElement<UnicodeT>::FlushSize(TotalCount, Patterns);
			if (Patterns.empty())
				return Result;
			else
				return {};
		}

		void Format(std::span<UnicodeT> OutputBuffer, std::span<PatternElement<UnicodeT> const> Patterns)
		{
			PatternElement<UnicodeT>::FlushBuffer(Patterns, OutputBuffer);
			assert(Patterns.empty());
		}
	};

	template<typename UnicodeType, typename CurType, typename ...HoldingType>
	struct FormatterWrapper<UnicodeType, CurType, HoldingType...>
	{
		Formatter<CurType, UnicodeType> CurFormatter;
		std::size_t LastSize = 0;
		FormatterWrapper<UnicodeType, HoldingType...> OtherFormatter;

		std::optional<std::size_t> FormatSize(std::size_t TotalCount, std::span<PatternElement<UnicodeType> const> Patterns, CurType const& C, HoldingType const& ...HT)
		{
			auto Result = PatternElement<UnicodeType>::FlushSize(TotalCount, Patterns);
			if (Patterns.empty())
				return Result;
			else {
				auto Top = Patterns[0];
				auto Re = CurFormatter.FormatSize(Top.String, C);
				if (Re.has_value())
				{
					LastSize = *Re;
					Patterns = Patterns.subspan(1);
					return OtherFormatter.FormatSize(Result + LastSize, Patterns, HT...);
				}else
					return {};
			}
		}

		void Format(std::span<UnicodeType> OutputBuffer, std::span<PatternElement<UnicodeType> const> Patterns, CurType const& C, HoldingType const& ...HT)
		{
			PatternElement<UnicodeType>::FlushBuffer(Patterns, OutputBuffer);
			if (!Patterns.empty())
			{
				auto Writed = OutputBuffer.subspan(0, LastSize);
				OutputBuffer = OutputBuffer.subspan(LastSize);
				CurFormatter.Format(Writed, C);
				Patterns = Patterns.subspan(1);
				return OtherFormatter.Format(OutputBuffer, Patterns, HT...);
			}
		}
	};

	template<typename UnicodeT>
	struct FormatPattern
	{

		FormatPattern(std::basic_string_view<UnicodeT> Str) : Elements(PatternElement<UnicodeT>::Create(Str)) {}

		template<typename FuncT, typename ...OTarget>
		bool Format(FuncT&& F, OTarget const& ...OT) requires(std::is_invocable_r_v<std::span<UnicodeT>, FuncT, std::size_t>)
		{
			FormatterWrapper<UnicodeT, std::remove_cvref_t<OTarget>...> Wrapper;
			auto Re = Wrapper.FormatSize(0, Elements, OT...);
			if (Re.has_value())
			{
				std::span<UnicodeT> Span = F(*Re);
				if(!Span.empty())
					Wrapper.Format(Span, Elements, OT...);
				return true;
			}
			return false;
		}

		template<typename CharTrai, typename Allocator, typename ...OTarget>
		bool Format(std::basic_string<UnicodeT, CharTrai, Allocator>& Output, OTarget const& ...OT)
		{
			std::size_t Old = Output.size();
			return Format([&](std::size_t Index) -> std::span<UnicodeT>{
				Output.resize(Old + Index);
				return std::span(Output).subspan(Old);
			}, OT...);
		}

		std::vector<PatternElement<UnicodeT>> Elements;
	};

	

	/*
	template<typename UnicodeT>
	struct FormatPattern
	{

		static FormatPattern Create(std::basic_string_view<UnicodeT> Str){ return FormatPattern(Str); }

		FormatPattern(FormatPattern&&) = default;
		FormatPattern(FormatPattern const&) = default;
		FormatPattern(std::basic_string_view<UnicodeT> Str) : String(Str), Elements(Str) {}

		template<typename FuncT, typename ...TargetType>
		std::optional<std::size_t> Format(FuncT&& Func, TargetType const&... TT) 
			requires(std::is_invocable_r_v<std::span<UnicodeT>, FuncT, std::size_t>);

		template<typename CharT, typename Allocator, typename ...TargetType>
		std::optional<std::size_t> Format(std::basic_string<UnicodeT, CharT, Allocator>& Output, TargetType const&... TT)
		{
			std::size_t OldSize = Output.size();
			return Format([&](std::size_t Index)->std::span<UnicodeT>{
				Output.resize(OldSize + Index);
				return std::span(Output).subspan(OldSize);
			});
		}

	private:

		std::basic_string_view<UnicodeT> String;
		CoreFormatPattern Elements;
	};

	template<typename UnicodeT> template<typename FuncT, typename ...TargetType>
	std::optional<std::size_t> FormatPattern<UnicodeT>::Format(FuncT&& Func, TargetType const&... TT)
		requires(std::is_invocable_r_v<std::span<UnicodeT>, FuncT, std::size_t>)
	{
		FormatterWrapper<UnicodeT, std::remove_cvref_t<TargetType>...> Wrapper;
		auto Re = Wrapper.FormatSize(0, String, Elements.Elements, TT...);
		if (Re.has_value())
		{
			auto Span = Func(*Re);
			if(!Span.empty())
				Wrapper.Format(*Span, String, Elements.Elements, TT...);
			return Re;
		}
		return {};
	}
	*/

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
				auto P = StrEncode::CharEncoder<UnicodeType, wchar_t>::EncodeOnceUnSafe(std::span(Par), { Buffer, 2});
				wss.write(Buffer, P.TargetSpace);
				Par = Par.substr(P.SourceSpace);
			}
			wss >> Input;
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
			StrEncode::EncodeInfo Str = StrEncode::StrEncoder<UnicodeT, TargetType>::RequireSpaceUnSafe(Par);
			Output.resize(Output.size() + Str.TargetSpace);
			auto OutputSpan = std::span(Output).subspan(OldSize);
			StrEncode::StrEncoder<UnicodeT, TargetType>::EncodeUnSafe(Par, OutputSpan);
			return true;
		}
	};

	template<typename NumberType, typename UnicodeType>
	struct BuildInNumberFormatter
	{
		std::wstringstream wss;
		void Format(std::span<UnicodeType> Output, NumberType Input) {
			std::size_t Index = 0;
			while (true)
			{
				wchar_t Temp;
				wss.read(&Temp, 1);
				if (wss.good())
				{
					auto Result = StrEncode::CharEncoder<wchar_t, UnicodeType>::EncodeOnceUnSafe({ &Temp, 1 }, Output.subspan(Index));
					Index += Result.TargetSpace;
				}
				else {
					break;
				}
			}
		}
		std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, NumberType Input) {
			wss << Input;
			return static_cast<std::size_t>(wss.tellp());
		}
	};

	template<typename UnicodeType>
	struct Formatter<uint64_t, UnicodeType> : BuildInNumberFormatter<uint64_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<int64_t, UnicodeType> : BuildInNumberFormatter<int64_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<uint32_t, UnicodeType> : BuildInNumberFormatter<uint32_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<int32_t, UnicodeType> : BuildInNumberFormatter<int32_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<uint16_t, UnicodeType> : BuildInNumberFormatter<uint16_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<int16_t, UnicodeType> : BuildInNumberFormatter<int16_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<uint8_t, UnicodeType> : BuildInNumberFormatter<uint8_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<int8_t, UnicodeType> : BuildInNumberFormatter<int8_t, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<float, UnicodeType> : BuildInNumberFormatter<float, UnicodeType> {};
	template<typename UnicodeType>
	struct Formatter<double, UnicodeType> : BuildInNumberFormatter<double, UnicodeType> {};

	template<typename SUnicodeT, typename CharTrais, typename UnicodeType>
	struct Formatter<std::basic_string_view<SUnicodeT, CharTrais>, UnicodeType>
	{
		void Format(std::span<UnicodeType> Output, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			auto Info = StrEncode::StrEncoder<SUnicodeT, UnicodeType>::EncodeUnSafe(Input, Output);
		}
		std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			auto Info = StrEncode::StrEncoder<SUnicodeT, UnicodeType>::RequireSpaceUnSafe(Input);
			return Info.TargetSpace;
		}
	};

	template<typename SUnicodeT, typename CharTrais, typename Allocator, typename UnicodeType>
	struct Formatter<std::basic_string<SUnicodeT, CharTrais, Allocator>, UnicodeType>: Formatter<std::basic_string_view<SUnicodeT, CharTrais>, UnicodeType> {};;

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