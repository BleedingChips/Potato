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

	template<typename UnicodeT>
	struct CaptureScanProcessor
	{
		template<typename ...TargetT>
		bool FormatTo(TargetT& ...T) const {
			return CaptureScanProcessor::FormatToImp(std::span(Capture), T...);
		}
		CaptureScanProcessor(CaptureScanProcessor&&) = default;
		CaptureScanProcessor(CaptureScanProcessor const&) = default;
		CaptureScanProcessor() = default;
		std::vector<std::basic_string_view<UnicodeT>> Capture;

	private:
		
		template<typename T, typename ...OT>
		static bool FormatToImp(std::span<std::basic_string_view<UnicodeT> const> Pars, T& ct, OT& ...ot) {
			return Pars.size() >= 1 && DirectScan(Pars[0], ct) && CaptureScanProcessor::FormatToImp(Pars.subspan(1), ot...);
		}
		static bool FormatToImp(std::span<std::basic_string_view<UnicodeT> const> Pars) {
			return true;
		}
		
		friend struct HeadMatchScanPatternProcessor;
		friend struct MatchScanPatternProcessor;
	};

	struct HeadMatchScanPatternProcessor
	{
		std::optional<CaptureScanProcessor<char8_t>> Scan(std::u8string_view Str);
		HeadMatchScanPatternProcessor(HeadMatchScanPatternProcessor const&) = default;
		HeadMatchScanPatternProcessor(HeadMatchScanPatternProcessor&) = default;
		HeadMatchScanPatternProcessor(Reg::TableWrapper Wrapper, bool Greedy = false) : Pro(Wrapper, Greedy) {}
	private:
		Reg::HeadMatchProcessor Pro;
	};

	struct MatchScanPatternProcessor
	{
		std::optional<CaptureScanProcessor<char8_t>> Scan(std::u8string_view Str);
		MatchScanPatternProcessor(MatchScanPatternProcessor const&) = default;
		MatchScanPatternProcessor(MatchScanPatternProcessor&) = default;
		MatchScanPatternProcessor(Reg::TableWrapper Wrapper) : Pro(Wrapper) {}
	private:
		Reg::MatchProcessor Pro;
	};

	struct ScanPattern
	{
		ScanPattern(std::u8string_view Str) : Table(Str, false, {}) {}
		ScanPattern(ScanPattern const&) = default;
		ScanPattern(ScanPattern&&) = default;
		MatchScanPatternProcessor AsMatchProcessor() const { return  MatchScanPatternProcessor{Table.AsWrapper()}; }
		HeadMatchScanPatternProcessor AsHeadMatchProcessor() const { return  HeadMatchScanPatternProcessor{ Table.AsWrapper() }; }
	private:
		Reg::Table Table;
	};

	template<typename UnicodeT, typename ...TargetType>
	bool Scan(CaptureScanProcessor<UnicodeT> const& Pro, TargetType& ... tar_type)
	{
		return Pro.FormatTo(tar_type...);
	}

	template<typename ...TargetType>
	bool Scan(HeadMatchScanPatternProcessor& Pro, std::u8string_view Code, TargetType& ... tar_type)
	{
		auto Processor = Pro.Scan(Code);
		if(Processor.has_value())
			return Scan(*Processor, tar_type...);
		return false;
	}

	template<typename ...TargetType>
	bool Scan(MatchScanPatternProcessor& Pro, std::u8string_view Code, TargetType& ... tar_type)
	{
		auto Processor = Pro.Scan(Code);
		if(Processor.has_value())
			return Scan(*Processor, tar_type...);
		return false;
	}

	template<typename ...TargetType>
	auto MatchScan(ScanPattern const& Pattern, std::u8string_view Code, TargetType& ... tar_type) {
		auto Wrapper = Pattern.AsMatchProcessor();
		return Scan(Wrapper, Code, tar_type...);
	}

	template<typename ...TargetType>
	auto MatchScan(std::u8string_view PatternStr, std::u8string_view Code, TargetType& ... tar_type) {
		ScanPattern Patten(PatternStr);
		return MatchScan(Patten, Code, tar_type...);
	}

	template<typename ...TargetType>
	auto HeadMatchScan(ScanPattern const& Pattern, std::u8string_view Code, TargetType& ... tar_type) {
		auto Wrapper = Pattern.AsHeadMatchProcessor();
		return Scan(Wrapper, Code, tar_type...);
	}

	template<typename ...TargetType>
	auto HeadMatchScan(std::u8string_view PatternStr, std::u8string_view Code, TargetType& ... tar_type) {
		ScanPattern Patten(PatternStr);
		return HeadMatchScan(Patten, Code, tar_type...);
	}


	template<typename SourceType, typename UnicodeType>
	struct Formatter
	{
		void Format(std::span<UnicodeType> Output, std::basic_string_view<UnicodeType> Parameter, SourceType const& Input);
		std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, SourceType const& Input) const;
	};

	

	namespace Implement
	{

		enum class PatternT
		{
			NormalString = 0,
			Parameter,
		};

		template<typename SourceT, typename UnicodeT>
		struct FormatterWrapper
		{
			SourceT&& Source;
			Formatter<std::remove_cvref_t<SourceT>, UnicodeT> Formatter;
			std::size_t Length;
		};
	}


	template<typename UnicodeT>
	struct FormatPattern
	{
		struct Element
		{
			Implement::PatternT Type;
			std::basic_string_view<UnicodeT> Par;
		};

		std::vector<Element> Elements;
	};

	std::optional<FormatPattern<char8_t>> CreateFormatPattern(std::u8string_view);

	template<typename UnicodeT, typename ...SourceT>
	struct FormatterReference
	{
		std::tuple<Implement::FormatterWrapper<SourceT, UnicodeT>...> Formatters;
		std::size_t TotalSize;
		std::span<typename FormatPattern<UnicodeT>::Element const> Elements;
		std::size_t GetRequireSize() const { return TotalSize; }
		bool FormatTo(std::span<UnicodeT> Buffer) const;
	};

	namespace Implement
	{
		template<std::size_t Index, typename UnicodeT, typename ...SourceT>
		std::optional<std::size_t> FlushFormatSize(std::size_t LastOffset, std::span<typename FormatPattern<UnicodeT>::Element const> Pars, FormatterReference<UnicodeT, SourceT...>& Tuples) {
			while (!Pars.empty())
			{
				if (Pars[0].Type == PatternT::Parameter)
				{
					if constexpr (Index < std::tuple_size_v<decltype(Tuples.Formatters)>)
					{
						auto& Ref = std::get<Index>(Tuples.Formatters);
						auto Result = Ref.Formatter.FormatSize(Pars[0].Par, Ref.Source);
						if (Result.has_value())
						{
							Ref.Length = *Result;
							return FlushFormatSize<Index + 1>(LastOffset + *Result, Pars.subspan(1), Tuples);
						}
						else {
							return {};
						}
					}
					else {
						return {};
					}
				}
				else {
					LastOffset += Pars[0].Par.size();
					Pars = Pars.subspan(1);
				}
			}
			if(Index <= std::tuple_size_v<decltype(Tuples.Formatters)>)
				return LastOffset;
			return {};
		}

		template<std::size_t Index, typename UnicodeT, typename ...SourceT>
		bool FormatTo(std::span<UnicodeT> Output, std::span<typename FormatPattern<UnicodeT>::Element const> Pars, FormatterReference<UnicodeT, SourceT...> const& Tuples) {
			while (!Pars.empty())
			{
				if (Pars[0].Type == PatternT::Parameter)
				{
					if constexpr (Index < std::tuple_size_v<decltype(Tuples.Formatters)>)
					{
						auto& Ref = std::get<Index>(Tuples.Formatters);
						assert(Output.size() >= Ref.Length);
						if (!Ref.Formatter.Format(Output, Pars[0].Par, Ref.Source))
							return false;
						return FormatTo<Index + 1>(Output.subspan(Ref.Length), Pars.subspan(1), Tuples);
					}
					else {
						return false;
					}
				}
				else {
					auto Ref = Pars[0].Par;
					assert(Ref.size() <= Output.size());
					std::memcpy(Output.data(), Ref.data(), Ref.size() * sizeof(UnicodeT));
					Pars = Pars.subspan(1);
					Output = Output.subspan(Ref.size());
				}
			}
			if (Index <= std::tuple_size_v<decltype(Tuples.Formatters)>)
				return true;
			return false;
		}
	}

	template<typename UnicodeT, typename ...SourceT>
	bool FormatterReference<UnicodeT, SourceT...>::FormatTo(std::span<UnicodeT> Buffer) const
	{
		if (Buffer.size() >= TotalSize)
		{
			return Implement::FormatTo<0>(Buffer, Elements, *this);
		}
		return false;
	}

	template<typename UnicodeT, typename ...SourceT>
	std::optional<FormatterReference<UnicodeT, SourceT...>> CreateFormatReference(FormatPattern<UnicodeT> const& Pattern, SourceT&& ...AT) {
		FormatterReference<UnicodeT, SourceT...> Result{
			std::tuple<Implement::FormatterWrapper<SourceT, UnicodeT>...>{
				Implement::FormatterWrapper<SourceT, UnicodeT>{std::forward<SourceT>(AT)}...
			},
			0, 
			std::span(Pattern.Elements)
		};
		auto Re = Implement::FlushFormatSize<0>(0, Pattern.Elements, Result);
		if (Re.has_value())
		{
			Result.TotalSize = *Re;
			return Result;
		}
		return {};
	}

	template<typename ...SourceT>
	std::optional<std::u8string> FormatTo(std::u8string_view PatternStr, SourceT&& ...T)
	{
		auto Pattern = CreateFormatPattern(PatternStr);
		if (Pattern.has_value())
		{
			auto Wrapper = CreateFormatReference(*Pattern, std::forward<SourceT>(T)...);
			if (Wrapper.has_value())
			{
				std::u8string Result;
				Result.resize(Wrapper->GetRequireSize());
				if(Wrapper->FormatTo(Result))
					return Result;
			}
		}
		return {};
	}

	template<typename UnicodeT, typename SourceT>
	struct DirectFormatterReference
	{
		Implement::FormatterWrapper<SourceT, UnicodeT> Formatter;
		std::basic_string_view<UnicodeT> Pars;
		std::size_t GetRequireSize() const { return Formatter.Length; }
		bool FormatTo(std::span<UnicodeT> Buffer) const {
			assert(Buffer.size() >= GetRequireSize());
			return Formatter.Formatter.Format(Buffer, Pars, Formatter.Source);
		}
	};

	template<typename UnicodeT, typename SourceT>
	std::optional<DirectFormatterReference<UnicodeT, SourceT>> CreateDirectFormatterReference(std::basic_string_view<UnicodeT> Str, SourceT&& S)
	{
		DirectFormatterReference<UnicodeT, SourceT> Re{
			{std::forward<SourceT>(S)}, Str
		};
		auto FS = Re.Formatter.Formatter.FormatSize(Str, S);
		if (FS.has_value())
		{
			Re.Formatter.Length = *FS;
			return Re;
		}
		else {
			return {};
		}
	}

	template<typename SourceT>
	std::optional<std::u8string> DirectFormat(std::u8string_view Par, SourceT&& S)
	{
		auto Re = CreateDirectFormatterReference(Par, std::forward<SourceT>(S));
		if (Re.has_value())
		{
			std::u8string TRe;
			TRe.resize(Re->GetRequireSize());
			if(Re->FormatTo(TRe))
				return TRe;
		}
		return {};
	}

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
		mutable std::wstringstream wss;
		bool Format(std::span<UnicodeType> Output, std::basic_string_view<UnicodeType> Pars, NumberType Input) const {
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
			return true;
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