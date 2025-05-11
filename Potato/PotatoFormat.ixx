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
			Output.resize(Output.size() + Str.TargetSpace);
			auto OutputSpan = std::span(Output).subspan(OldSize);
			Encode::StrEncoder<UnicodeT, TargetType>::EncodeUnSafe(Par, OutputSpan);
			return true;
		}
	};

	/*
	template<typename NumberType, typename UnicodeType>
	struct BuildInNumberFormatter
	{
		bool operator()(FormatWriter<UnicodeType>& Writter, std::basic_string_view<UnicodeType> Pars, NumberType Input) {
			std::wstringstream wss;
			wss << Input;
			while (true)
			{
				wchar_t Temp;
				wss.read(&Temp, 1);
				if (wss.good())
				{
					auto Last = Writter.GetLastBuffer();
					if (Last.has_value())
					{
						auto Result = Encode::CharEncoder<wchar_t, UnicodeType>::EncodeOnceUnSafe({ &Temp, 1 }, *Last);
						Writter.Allocate(Result.TargetSpace);
					}
					else {
						auto Result = Encode::CharEncoder<wchar_t, UnicodeType>::RequireSpaceOnceUnSafe({ &Temp, 1 });
						Writter.Allocate(Result.TargetSpace);
					}
				}
				else {
					break;
				}
			}
			return true;
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
		bool operator()(FormatWriter<UnicodeType>& Writter, std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			
			auto Last = Writter.GetLastBuffer();
			if (Last.has_value())
			{
				auto Result = Encode::StrEncoder<SUnicodeT, UnicodeType>::EncodeUnSafe(Input, *Last);
				Writter.Allocate(Result.TargetSpace);
			}
			else {
				auto Result = Encode::StrEncoder<SUnicodeT, UnicodeType>::RequireSpaceUnSafe(Input);
				Writter.Allocate(Result.TargetSpace);
			}
			return true;
		}
	};

	template<typename SUnicodeT, typename CharTrais, typename Allocator, typename UnicodeType>
	struct Formatter<std::basic_string<SUnicodeT, CharTrais, Allocator>, UnicodeType>: Formatter<std::basic_string_view<SUnicodeT, CharTrais>, UnicodeType> {};
	*/
}