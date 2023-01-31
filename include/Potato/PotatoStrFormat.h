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
#include "PotatoTMP.h"

namespace Potato::StrFormat
{

	template<typename SourceType, typename UnicodeType>
	struct Scanner
	{
		bool Scan(std::span<UnicodeType const> Par, SourceType& Input) = delete;
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
		static std::optional<std::size_t> Format(std::span<UnicodeType> Output, std::basic_string_view<UnicodeType> Parameter, SourceType const& Input) = delete;
		static std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, SourceType const& Input) = delete;
	};

	namespace Implement
	{

		enum class FormatPatternT : uint8_t
		{
			NormalString = 0,
			Parameter,
			BadFormat
		};

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
		constexpr std::tuple<FormatPatternT, std::basic_string_view<CharT>, std::basic_string_view<CharT>> FindFomatterTopElement(std::basic_string_view<CharT> Str)
		{
			std::size_t State = 0;
			std::size_t RecordIndex = 0;
			std::size_t Index = 0;
			while (Index < Str.size())
			{
				auto Ite = Str[Index];
				switch (State)
				{
				case 0:
					RecordIndex = Index;
					switch (Ite)
					{
					case LeftBrace<CharT>::Value:
						State = 2;
						break;
					case RightBrace<CharT>::Value:
						State = 3;
						break;
					default:
						State = 1;
						break;
					}
					++Index;
					break;
				case 1:
					switch (Ite)
					{
					case LeftBrace<CharT>::Value:
					case RightBrace<CharT>::Value:
						return { FormatPatternT::NormalString, Str.substr(RecordIndex, Index - RecordIndex), Str.substr(Index) };
					default:
						++Index;
						break;
					}
					break;
				case 2:
					switch (Ite)
					{
					case LeftBrace<CharT>::Value:
						return { FormatPatternT::NormalString, LeftBrace<CharT>::Str, Str.substr(Index + 1) };
					case RightBrace<CharT>::Value:
						return { FormatPatternT::Parameter, {}, Str.substr(Index + 1) };
					default:
						RecordIndex = Index;
						State = 4;
						++Index;
						break;
					}
					break;
				case 3:
					switch (Ite)
					{
					case RightBrace<CharT>::Value:
						return { FormatPatternT::NormalString, RightBrace<CharT>::Str, Str.substr(Index + 1) };
					default:
						return { FormatPatternT::BadFormat, LeftBrace<CharT>::Str, Str.substr(Index) };
					}
					break;
				case 4:
					switch (Ite)
					{
					case LeftBrace<CharT>::Value:
						return { FormatPatternT::BadFormat, Str.substr(RecordIndex, Index - 1 - RecordIndex), Str.substr(Index + 1) };
					case RightBrace<CharT>::Value:
						return { FormatPatternT::Parameter, Str.substr(RecordIndex, Index - 1 - RecordIndex), Str.substr(Index + 1) };
					default:
						++Index;
						break;
					}
					break;
				default:
					return { FormatPatternT::BadFormat, Str.substr(RecordIndex, Index - 1 - RecordIndex), Str.substr(Index + 1) };;
				}
			}
			switch (State)
			{
			case 1:
				return { FormatPatternT::NormalString, Str, {} };
			case 2:
			case 3:
			case 4:
			default:
				return { FormatPatternT::BadFormat, Str, {} };
			}
		}

		template<typename CharT>
		constexpr std::optional<std::size_t> FormatSizeImp(std::size_t LastSize, std::basic_string_view<CharT> Str)
		{
			while (!Str.empty())
			{
				auto [Type, Cur, Last] = FindFomatterTopElement(Str);

				if (Type == FormatPatternT::NormalString)
				{
					LastSize += Cur.size();
					Str = Last;
				}
				else
					return {};
			}
			return LastSize;
		}

		template<typename CharT, typename CurType, typename ...OType>
		constexpr std::optional<std::size_t> FormatSizeImp(std::size_t LastSize, std::basic_string_view<CharT> Str, CurType const& CT, OType const& ...OT)
		{
			while (!Str.empty())
			{
				auto [Type, Cur, Last] = FindFomatterTopElement(Str);

				if (Type == FormatPatternT::NormalString)
				{
					LastSize += Cur.size();
					Str = Last;
				}
				else if (Type == FormatPatternT::Parameter)
				{
					auto Re = Formatter<std::remove_cvref_t<CurType>, CharT>::FormatSize(Cur, CT);
					if (Re.has_value())
					{
						LastSize += *Re;
						return FormatSizeImp(LastSize, Last, OT...);
					}
					else {
						return {};
					}
				}
				else
					return {};
			}
			return LastSize;
		}

		template<typename CharT>
		constexpr std::optional<std::size_t> FormatToUnSafeImp(std::size_t LastSize, std::span<CharT> OutputBuffer, std::basic_string_view<CharT> Str)
		{
			while (!Str.empty())
			{
				auto [Type, Cur, Last] = FindFomatterTopElement(Str);

				if (Type == FormatPatternT::NormalString)
				{
					std::memcpy(OutputBuffer.data(), Cur.data(), Cur.size() * sizeof(CharT));
					OutputBuffer = OutputBuffer.subspan(Cur.size());
					LastSize += Cur.size();
					Str = Last;
				}
				else
					return {};
			}
			return LastSize;
		}

		template<typename CharT, typename CurType, typename ...OType>
		constexpr std::optional<std::size_t> FormatToUnSafeImp(std::size_t LastSize, std::span<CharT> OutputBuffer, std::basic_string_view<CharT> Str, CurType const& CT, OType const& ...OT)
		{
			while (!Str.empty())
			{
				auto [Type, Cur, Last] = FindFomatterTopElement(Str);

				if (Type == FormatPatternT::NormalString)
				{
					std::memcpy(OutputBuffer.data(), Cur.data(), Cur.size() * sizeof(CharT));
					OutputBuffer = OutputBuffer.subspan(Cur.size());
					LastSize += Cur.size();
					Str = Last;
				}
				else if (Type == FormatPatternT::Parameter)
				{
					auto Re = Formatter<std::remove_cvref_t<CurType>, CharT>::Format(OutputBuffer, Cur, CT);
					if (Re.has_value())
					{
						OutputBuffer = OutputBuffer.subspan(*Re);
						LastSize += *Re;
						return FormatToUnSafeImp(LastSize, OutputBuffer, Last, OT...);
					}
					else {
						return {};
					}
				}
				else
					return {};
			}
			return LastSize;
		}

		
	}

	
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(std::u8string_view Formatter, OType const& ...OT) { return Implement::FormatSizeImp(0, Formatter, OT...); }
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(std::wstring_view Formatter, OType const& ...OT) { return Implement::FormatSizeImp(0, Formatter, OT...); }
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(std::u16string_view Formatter, OType const& ...OT) { return Implement::FormatSizeImp(0, Formatter, OT...); }
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(std::u32string_view Formatter, OType const& ...OT) { return Implement::FormatSizeImp(0, Formatter, OT...); }

	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatToUnSafe(std::span<char8_t> OutputBuffer, std::basic_string_view<char8_t> Formatter, OType const& ...OT) { return Implement::FormatToUnSafeImp(0, OutputBuffer, Formatter, OT...); }
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatToUnSafe(std::span<wchar_t> OutputBuffer, std::basic_string_view<wchar_t> Formatter, OType const& ...OT) { return Implement::FormatToUnSafeImp(0, OutputBuffer, Formatter, OT...); }
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatToUnSafe(std::span<char16_t> OutputBuffer, std::basic_string_view<char16_t> Formatter, OType const& ...OT) { return Implement::FormatToUnSafeImp(0, OutputBuffer, Formatter, OT...); }
	template<typename ...OType>
	constexpr std::optional<std::size_t> FormatToUnSafe(std::span<char32_t> OutputBuffer, std::basic_string_view<char32_t> Formatter, OType const& ...OT) { return Implement::FormatToUnSafeImp(0, OutputBuffer, Formatter, OT...); }

	namespace Implement
	{
		template<typename CharT, typename ...OType>
		auto FormatImp(std::basic_string_view<CharT> Formatter, OType const& ...OT) -> std::optional<std::basic_string<CharT>> {
			auto Size = FormatSize(Formatter, OT...);
			if (Size.has_value())
			{
				std::basic_string<CharT> Result(*Size, 0);
				auto Re = FormatToUnSafe(Result, Formatter, OT...);
				if (Re.has_value() && *Size == *Re)
					return Result;
			}
			return {};
		}
	}

	template<typename ...OType>
	auto Format(std::u8string_view Formatter, OType const& ...OT) { return Implement::FormatImp(Formatter, OT...); }
	template<typename ...OType>
	auto Format(std::wstring_view Formatter, OType const& ...OT) { return Implement::FormatImp(Formatter, OT...); }
	template<typename ...OType>
	auto Format(std::u16string_view Formatter, OType const& ...OT) { return Implement::FormatImp(Formatter, OT...); }
	template<typename ...OType>
	auto Format(std::u32string_view Formatter, OType const& ...OT) { return Implement::FormatImp(Formatter, OT...); }

	namespace Implement
	{
		template<typename CharT, typename CurType>
		auto DirectFormatImp(std::basic_string_view<CharT> Par, CurType const& CT) -> std::optional<std::basic_string<CharT>> {
			auto Size = Formatter<std::remove_cvref_t<CurType>, CharT>::FormatSize(Par, CT);
			if (Size.has_value())
			{
				std::basic_string<CharT> Result(*Size, 0);
				auto Re = Formatter<std::remove_cvref_t<CurType>, CharT>::Format(Result, Par, CT);
				if (Re.has_value() && *Size == *Re)
					return Result;
			}
			return {};
		}
	}

	template<typename SourceT>
	auto DirectFormat(std::u8string_view Par, SourceT const& S) { return Implement::DirectFormatImp(Par, S); }

	template<typename SourceT>
	auto DirectFormat(std::wstring_view Par, SourceT const& S) { return Implement::DirectFormatImp(Par, S); }

	template<typename SourceT>
	auto DirectFormat(std::u16string_view Par, SourceT const& S) { return Implement::DirectFormatImp(Par, S); }

	template<typename SourceT>
	auto DirectFormat(std::u32string_view Par, SourceT const& S) { return Implement::DirectFormatImp(Par, S); }

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
		static std::optional<std::size_t> Format(std::span<UnicodeType> Output, std::basic_string_view<UnicodeType> Pars, NumberType Input) {
			std::wstringstream wss;
			wss << Input;
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
			return Index;
		}
		static std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, NumberType Input) {
			std::wstringstream wss;
			wss << Input;
			std::size_t Index = 0;
			while (true)
			{
				wchar_t Temp;
				wss.read(&Temp, 1);
				if (wss.good())
				{
					auto Result = StrEncode::CharEncoder<wchar_t, UnicodeType>::RequireSpaceOnceUnSafe({ &Temp, 1 });
					Index += Result.TargetSpace;
				}
				else {
					break;
				}
			}
			return Index;
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
		static std::optional<std::size_t> Format(std::span<UnicodeType> Output, std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			auto Info = StrEncode::StrEncoder<SUnicodeT, UnicodeType>::EncodeUnSafe(Input, Output);
			return Info.TargetSpace;
		}
		static std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			auto Info = StrEncode::StrEncoder<SUnicodeT, UnicodeType>::RequireSpaceUnSafe(Input);
			return Info.TargetSpace;
		}
	};

	template<typename SUnicodeT, typename CharTrais, typename Allocator, typename UnicodeType>
	struct Formatter<std::basic_string<SUnicodeT, CharTrais, Allocator>, UnicodeType>: Formatter<std::basic_string_view<SUnicodeT, CharTrais>, UnicodeType> {};

}