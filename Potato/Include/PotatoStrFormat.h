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

	/*
	template<typename UnicodeType, typename CharTrai, typename TargetType>
	bool DirectScan(std::basic_string_view<UnicodeType, CharTrai> Parameter, TargetType& tar_type)
	{
		return Scanner<std::remove_cvref_t<TargetType>, UnicodeType>{}.Scan(Parameter, tar_type);
	}
	*/
	
	/*
	template<typename UnicodeType, typename CharTrai, typename Allocator, typename TargetType>
	bool DirectScan(std::basic_string<UnicodeType, CharTrai, Allocator> Parameter, TargetType& tar_type)
	{
		return DirectScan(std::basic_string_view(Parameter), tar_type);
	}
	*/

	struct ScanPattern
	{

		struct ScanResult
		{
			std::size_t Count;
			std::size_t SearchLength;
		};

		template<typename UnicodeT, typename ...TargetType>
		std::optional<size_t> March(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const;

		template<typename UnicodeT, typename ...TargetType>
		std::optional<ScanResult> Search(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const;
		
		ScanPattern(ScanPattern&&) = default;
		ScanPattern(ScanPattern const&) = default;
		ScanPattern(std::vector<Reg::TableWrapper::StorageT> Tables) : Tables(std::move(Tables)) {};

		template<typename UnicodeT>
		ScanPattern(std::basic_string_view<UnicodeT> Pattern) : ScanPattern(Create(Pattern)) {}

		template<typename UnicodeT>
		static ScanPattern Create(std::basic_string_view<UnicodeT> pattern) { 
			return { Reg::TableWrapper::Create(pattern)};
		}

		template<typename UnicodeT, typename CurTarget, typename ...TargetType>
		static std::optional<std::size_t> CaptureScan(Reg::CaptureWrapper TopCapture, std::basic_string_view<UnicodeT> Chars, CurTarget& Target, TargetType&... OTarget)
		{
			if(TopCapture.HasSubCapture())
				return Dispatch(0, TopCapture.GetTopSubCapture(), Chars, Target, OTarget...);
			return {};
		}
		
	private:

		std::vector<Reg::TableWrapper::StorageT> Tables;

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
	std::optional<size_t> ScanPattern::March(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const
	{
		auto result = Reg::ProcessMarch(Reg::TableWrapper(Tables), std::span(code));
		if (result)
		{
			return ScanPattern::Dispatch(0, result->GetCaptureWrapper().GetTopSubCapture(), code, all_target...);
		}
		return {};
	}

	template<typename UnicodeT, typename ...TargetType>
	std::optional<ScanPattern::ScanResult> ScanPattern::Search(std::basic_string_view<UnicodeT> code, TargetType&... all_target) const
	{
		auto result = Reg::ProcessSearch(Reg::TableWrapper(Tables), std::span(code));
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
	auto MarchScan(std::u32string_view PatternStr, std::u32string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.March(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MarchScan(std::u16string_view PatternStr, std::u16string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.March(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MarchScan(std::u8string_view PatternStr, std::u8string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.March(Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MarchScan(std::wstring_view PatternStr, std::wstring_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.March(Code, tar_type...);
	}


	template<typename ...TargetType>
	auto SearchScan(std::u32string_view PatternStr, std::u32string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Search(Code, tar_type...);
	}
	template<typename ...TargetType>
	auto SearchScan(std::u16string_view PatternStr, std::u16string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Search(Code, tar_type...);
	}
	template<typename ...TargetType>
	auto SearchScan(std::u8string_view PatternStr, std::u8string_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Search(Code, tar_type...);
	}
	template<typename ...TargetType>
	auto SearchScan(std::wstring_view PatternStr, std::wstring_view Code, TargetType& ... tar_type) {
		return ScanPattern{ PatternStr }.Search(Code, tar_type...);
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

	/*
	template<typename UnicodeType, typename TargetType>
	std::basic_string<UnicodeType> DirectoFormat(std::basic_string_view<UnicodeType> Str, TargetType const& Type)
	{
		std::basic_string<UnicodeType> Result;
		if(DirectoFormatTo(Result, Str, Type))
			return Result;
		return {};
	}
	*/

	/*
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
		CoreFormatPattern(std::u32string_view Str) : CoreFormatPattern({0, std::numeric_limits<std::size_t>::max()}, Reg::StringViewWrapper<char32_t>{}, & Str){}
		CoreFormatPattern(std::u16string_view Str) : CoreFormatPattern({ 0, std::numeric_limits<std::size_t>::max() }, Reg::StringViewWrapper<char16_t>{}, & Str) {}
		CoreFormatPattern(std::u8string_view Str) : CoreFormatPattern({ 0, std::numeric_limits<std::size_t>::max() }, Reg::StringViewWrapper<char8_t>{}, & Str) {}
		CoreFormatPattern(std::wstring_view Str) : CoreFormatPattern({ 0, std::numeric_limits<std::size_t>::max() }, Reg::StringViewWrapper<wchar_t>{}, & Str) {}
		CoreFormatPattern(CoreFormatPattern&&) = default;
		CoreFormatPattern(CoreFormatPattern const&) = default;

		std::vector<Element> Elements;
	};

	template<typename UnicodeType, typename ...HoldingType>
	struct FormatterWrapper
	{
		std::optional<std::size_t> FormatSize(std::size_t TotalCount, std::basic_string_view<UnicodeType> Str, std::span<CoreFormatPattern::Element const> Patterns)
		{
			for (auto Ite : Patterns)
			{
				switch (Ite.Type)
				{
				case CoreFormatPattern::TypeT::NormalString:
					TotalCount += Cur.Index.size();
					break;
				case CoreFormatPattern::TypeT::Parameter:
					return {};
				}
			}
			return TotalCount;
		}
		void Format(std::span<UnicodeType> OutputBuffer, std::basic_string_view<UnicodeType> Str, std::span<CoreFormatPattern::Element const> Patterns)
		{
			for (std::size_t Index = 0; Index < Patterns.size(); ++Index)
			{
				auto Cur = Patterns[Index];
				switch (Cur.Type)
				{
				case CoreFormatPattern::TypeT::NormalString:
				{
					auto CurStr = Str.substr(Cur.Index.Begin(), Cur.Index.Count());
					std::memcpy(OutputBuffer.data(), CurStr.data(), sizeof(UnicodeType) * CurStr.size());
					OutputBuffer = OutputBuffer.subspan(CurStr.size());
					break;
				}
				case CoreFormatPattern::TypeT::Parameter:
					return;
				}
			}
		}
	};

	template<typename UnicodeType, typename CurType, typename ...HoldingType>
	struct FormatterWrapper<UnicodeType, CurType, HoldingType...>
	{
		Formatter<CurType, UnicodeType> CurFormatter;
		std::size_t LastSize = 0;
		FormatterWrapper<UnicodeType, HoldingType...> OtherFormatter;

		std::optional<std::size_t> FormatSize(std::size_t TotalCount, std::basic_string_view<UnicodeType> Str, std::span<CoreFormatPattern::Element const> Patterns, CurType const& C, HoldingType const& ...HT)
		{
			for (std::size_t Index = 0; Index < Patterns.size(); ++Index)
			{
				auto Cur = Patterns[Index];
				switch (Cur.Type)
				{
				case CoreFormatPattern::TypeT::NormalString:
				{
					TotalCount += Cur.Index.Count();
					break;
				}
				case CoreFormatPattern::TypeT::Parameter:
				{
					auto CurStr = Str.substr(Cur.Index.Begin(), Cur.Index.Count());
					auto Re = CurFormatter.FormatSize(CurStr, C);
					if(!Re.has_value())
						return {};
					LastSize = Re;
					TotalCount += *Re;
					return OtherFormatter.FormatSize(TotalCount + *Re, Str, Patterns.subspan(Index + 1), HT...);	
				}
				}
			}
			return TotalCount;
		}

		void Format(std::span<UnicodeType> OutputBuffer, std::basic_string_view<UnicodeType> Str, std::span<CoreFormatPattern::Element const> Patterns, CurType const& C, HoldingType const& ...HT)
		{
			for (std::size_t Index = 0; Index < Patterns.size(); ++Index)
			{
				auto Cur = Patterns[Index];
				switch (Cur.Type)
				{
				case CoreFormatPattern::TypeT::NormalString:
				{
					auto CurStr = Str.substr(Cur.Index.Begin(), Cur.Index.Count());
					std::memcpy(OutputBuffer.data(), CurStr.data(), sizeof(UnicodeType) * CurStr.size());
					OutputBuffer = OutputBuffer.subspan(CurStr.size());
					break;
				}
				case CoreFormatPattern::TypeT::Parameter:
				{
					auto Writed = OutputBuffer.subspan(0, LastSize);
					OutputBuffer = OutputBuffer.subspan(LastSize);
					CurFormatter.Format(Writed, C);
					return OtherFormatter.Format(OutputBuffer, Str, Patterns.subspan(Index + 1), HT...);
				}
				}
			}
		}
	};

	template<typename UnicodeT>
	struct FormatPattern
	{

		static FormatPattern Create(std::basic_string_view<UnicodeT> Str){ return FormatPattern(Str); }

		FormatPattern(FormatPattern&&) = default;
		FormatPattern(FormatPattern const&) = default;
		FormatPattern(std::basic_string_view<UnicodeT> Str) : string(Str), Elements(Str) {}

		template<typename FuncT, typename ...TargetType>
		std::optional<std::size_t> Format(FuncT&& Func, std::basic_string_view<UnicodeT> Str, TargetType const&... TT) 
			requires(std::is_invocable_r_v<std::span<UnicodeT>, FuncT, std::size_t>);

		template<typename CharT, typename Allocator, typename ...TargetType>
		std::optional<std::size_t> Format(std::basic_string<UnicodeT, CharT, Allocator>&& Func, std::basic_string_view<UnicodeT> Str, TargetType const&... TT)
			requires(std::is_invocable_r_v<std::span<UnicodeT>, FuncT, std::size_t>)
		{
			std::size_t OldSize = Func.size();
			return Format([]()->std::span<UnicodeT>);
		}

	private:

		std::basic_string_view<UnicodeT> String;
		CoreFormatPattern Elements;
	};

	template<typename UnicodeT> template<typename FuncT, typename ...TargetType>
	std::optional<std::size_t> FormatPattern<UnicodeT>::Format(FuncT&& Func, std::basic_string_view<UnicodeT> Str, TargetType const&... TT)
		requires(std::is_invocable_r_v<std::span<UnicodeT>, FuncT, std::size_t>)
	{
		FormatterWrapper<UnicodeT, std::remove_cvref_t<TargetType>...> Wrapper;
		auto Re = Wrapper.FormatSize(0, Str, Elements, TT...);
		if (Re.has_value())
		{
			auto Span = Func(*Re);
			if(!Span.empty())
				Wrapper.Format(*Span, Str, Elements, TT...);
			return true;
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
				auto P = StrEncode::CoreEncoder<UnicodeType, wchar_t>::EncodeOnceUnSafe(std::span(Par), { Buffer, 2});
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
			StrEncode::EncodeStrInfo Str = StrEncode::StrCodeEncoder<UnicodeT, TargetType>::RequireSpaceUnSafe(Par);
			Output.resize(Output.size() + Str.TargetSpace);
			auto OutputSpan = std::span(Output).subspan(OldSize);
			StrEncode::StrCodeEncoder<UnicodeT, TargetType>::EncodeUnSafe(Par, OutputSpan);
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
					auto Result = StrEncode::CoreEncoder<wchar_t, UnicodeType>::EncodeOnceUnSafe({ &Temp, 1 }, Output.subspan(Index));
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