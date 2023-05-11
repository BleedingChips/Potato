module;

export module Potato.Format;

export import Potato.STD;
export import Potato.Reg;
export import Potato.Encode;

export namespace Potato::Format
{

	template<typename SourceType, typename UnicodeType>
	struct Scanner
	{
		bool Scan(std::span<UnicodeType const> Par, SourceType& Input) = delete;
	};

	template<typename CharT, typename CharTraiT, typename TargetT>
	bool DirectScan(std::basic_string_view<CharT, CharTraiT> Pars, TargetT& TarType)
	{
		return Scanner<std::remove_cvref_t<TargetT>, CharT>{}.Scan(Pars, TarType);
	}

	template<typename CharT, typename CharTraits>
	bool CaptureScan(std::span<Misc::IndexSpan<> const>, std::basic_string_view<CharT, CharTraits> InputStr)
	{
		return true;
	}

	template<typename CharT, typename CharTraits, typename CT, typename ...OT>
	bool CaptureScan(std::span<Misc::IndexSpan<> const> CaptureIndex, std::basic_string_view<CharT, CharTraits> InputStr, CT& O1, OT& ...OO)
	{
		return CaptureIndex.size() >= 1 && DirectScan(CaptureIndex[0].Slice(InputStr), O1) && CaptureScan(CaptureIndex.subspan(1), InputStr, OO...);
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool RegAcceptScan(Reg::ProcessorAcceptT const& Accept, std::basic_string_view<CharT, CharTraits> InputStr,  OT& ...OO)
	{
		return CaptureScan(std::span(Accept.Capture), InputStr, OO...);
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool ProcessorScan(Reg::DfaProcessor& Processor, std::basic_string_view<CharT, CharTraits> Str, OT& ...OO)
	{
		auto Accept = Reg::CoreProcessor(Processor, Str);
		if (Accept.has_value())
			return RegAcceptScan(*Accept, Str, OO...);
		return false;
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool ProcessorScan(Reg::DfaBinaryTableWrapperProcessor& Processor, std::basic_string_view<CharT, CharTraits> Str, OT& ...OO)
	{
		auto Accept = Reg::CoreProcessor(Processor, Str);
		if (Accept.has_value())
			return RegAcceptScan(*Accept, Str, OO...);
		return false;
	}

	template<typename CharT1, typename CharTT1, typename CharT2, typename CharTT2, typename ...OT>
	bool MatchScan(std::basic_string_view<CharT1, CharTT1> Regex, std::basic_string_view<CharT2, CharTT2> Source, OT&... OO)
	{
		Reg::DfaT Table(Reg::DfaT::FormatE::Match, Regex, false, 0);
		Reg::DfaProcessor Processor(Table);
		return ProcessorScan(Processor, Source, OO...);
	}

	template<typename CharT1, typename CharTT1, typename CharT2, typename CharTT2, typename ...OT>
	bool HeadMatchScan(std::basic_string_view<CharT1, CharTT1> Regex, std::basic_string_view<CharT2, CharTT2> Source, OT&... OO)
	{
		Reg::DfaT Table(Reg::DfaT::FormatE::HeadMatch, Regex, false, 0);
		Reg::DfaProcessor Processor(Table);
		return ProcessorScan(Processor, Source, OO...);
	}


	template<typename SourceType>
	struct FormatWritter
	{
		constexpr FormatWritter() = default;
		constexpr FormatWritter(std::span<SourceType> Output) : Output(Output) {}

		constexpr bool IsWritting() const { return Output.has_value(); }

		constexpr std::optional<std::span<SourceType>> GetLastBuffer() const{
			if (Output.has_value())
			{
				return Output->subspan(WritedCount);
			}
			return {};
		}

		constexpr std::size_t GetWritedSize() const { return WritedCount; }

		constexpr std::size_t Write(SourceType Input)
		{
			if (Output.has_value())
			{
				Output->operator[](WritedCount) = Input;
			}
			auto Old = WritedCount;
			WritedCount += 1;
			return Old;
		}

		constexpr std::size_t Write(std::span<SourceType const> Input)
		{
			if (Output.has_value())
			{
				std::copy_n(Input.begin(), Input.size(), Output->subspan(WritedCount).begin());
			}
			auto Old = WritedCount;
			WritedCount += Input.size();
			return Old;
		}

		constexpr std::size_t Allocate(std::size_t Size)
		{
			auto Old = WritedCount;
			WritedCount += Size;
			return Old;
		}

	protected:

		std::size_t WritedCount = 0;
		std::optional<std::span<SourceType>> Output;
	};

	template<typename SourceType, typename UnicodeType>
	struct Formatter
	{
		bool operator()(FormatWritter<SourceType>& Writer, std::basic_string_view<UnicodeType> Parameter, SourceType const& Input) = delete;
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


		template<std::size_t CurIndex, typename CharT, typename CharTT>
		constexpr bool FormatExe(FormatWritter<CharT>& Writter, std::basic_string_view<CharT, CharTT> Par, std::size_t RequireSize)
		{
			return false;
		}

		template<std::size_t CurIndex, typename CharT, typename CharTT, typename CurType, typename ...OtherType>
		constexpr bool FormatExe(FormatWritter<CharT>& Writter, std::basic_string_view<CharT, CharTT> Par, std::size_t RequireSize, CurType&& CT, OtherType&& ...OT)
		{
			if (RequireSize == CurIndex)
			{
				return Formatter<std::remove_cvref_t<CurType>, CharT>{}(Writter, Par, CT);
			}
			else {
				return FormatExe<CurIndex + 1>(Writter, Par, RequireSize, std::forward<OtherType>(OT)...);
			}
		}

		template<typename CharT, typename ...OtherType>
		constexpr std::optional<std::size_t> ApplyFormat(FormatWritter<CharT>& Writter, std::size_t Index, FormatPatternT Type, std::basic_string_view<CharT> Par, OtherType&& ...OT)
		{
			switch (Type)
			{
			case Implement::FormatPatternT::BadFormat:
				return {};
			case Implement::FormatPatternT::NormalString:
				Writter.Write(Par);
				return Index;
			case Implement::FormatPatternT::Parameter:
			{
				if (FormatExe<0>(Writter, Par, Index, std::forward<OtherType>(OT)...))
				{
					return Index + 1;
				}
				else
					return {};
			}
			default:
				return {};
			}
		}
	}

	template<typename CharT, typename CharTraisT, typename ...OType>
	constexpr bool Format(FormatWritter<CharT>& Writter, std::basic_string_view<CharT, CharTraisT> Formatter, OType&& ...OT)
	{
		std::size_t Index = 0;
		while (!Formatter.empty())
		{
			auto [Type, Par, Last] = Implement::FindFomatterTopElement(Formatter);
			auto Re = Implement::ApplyFormat(Writter, Index, Type, Par, std::forward<OType>(OT)...);
			if (Re.has_value())
			{
				Index = *Re;
			}
			else
				return false;
			Formatter = Last;
		}
		return true;
	}

	template<typename CharT, typename CharTrais, typename ...OType>
	auto FormatToString(std::basic_string_view<CharT, CharTrais> Formatter, OType&& ...OT) -> std::optional<std::basic_string<CharT, CharTrais>> {
		
		FormatWritter<CharT> Predict;

		if (Format(Predict, Formatter, OT...))
		{
			std::basic_string<CharT, CharTrais> Buffer;
			Buffer.resize(Predict.GetWritedSize());
			FormatWritter<CharT> Writter(Buffer);
			if (Format(Writter, Formatter, OT...))
			{
				return Buffer;
			}
		}
		return {};
	}

	namespace Implement
	{
		
		template<typename CharT, typename CharTT>
		constexpr std::size_t StaticFormatCount(std::basic_string_view<CharT, CharTT> Str)
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

		template<std::size_t N, typename CharT, typename CharTT>
		constexpr std::array<std::tuple<Implement::FormatPatternT, std::basic_string_view<CharT, CharTT>>, N> StaticFormatExe(std::basic_string_view<CharT, CharTT> Str)
		{
			std::array<std::tuple<Implement::FormatPatternT, std::basic_string_view<CharT, CharTT>>, N> Tem;
			std::size_t Count = 0;
			while (!Str.empty())
			{
				auto [Type, Par, Last] = FindFomatterTopElement(Str);
				Tem[Count++] = {Type, Par};
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
		static constexpr bool Format(FormatWritter<Type>& Writter, OT&& ...ot)
		{
			std::size_t Index = 0;
			for (auto Ite : Patterns)
			{
				auto [Type, Par] = Ite;
				auto Re = Implement::ApplyFormat(Writter, Index, Type, Par, std::forward<OT>(ot)...);
				if (Re.has_value())
				{
					Index = *Re;
				}
				else
					return false;
			}
			return true;
		}

		template<typename ...OT>
		static auto FormatToString(OT&& ...ot) -> std::optional<std::basic_string<Type>>
		{
			FormatWritter<Type> Predicte;
			if (StaticFormatPattern::Format(Predicte, ot...))
			{
				std::basic_string<Type> Result;
				Result.resize(Predicte.GetWritedSize());
				FormatWritter<Type> Writter{std::span(Result)};
				if (StaticFormatPattern::Format(Writter, ot...))
				{
					return Result;
				}
			}
			return {};
		}

	};

	template<TMP::TypeString Formatter, typename ...Type>
	auto StaticFormatToString(Type&& ...Cur) -> std::optional<std::basic_string<typename StaticFormatPattern<Formatter>::Type>>
	{
		return StaticFormatPattern<Formatter>::FormatToString(Cur...);
	}

	template<typename CharT, typename CharTT, typename SourceT>
	auto DirectFormatToString(std::basic_string_view<CharT, CharTT> Par, SourceT && S) -> std::optional<std::basic_string<CharT, CharTT>> {
		
		FormatWritter<CharT> Predicte;
		using Formatter = Formatter<std::remove_cvref_t<SourceT>, CharT>;
		if (Formatter{}(Predicte, Par, S))
		{
			std::basic_string<CharT, CharTT> Result;
			Result.resize(Predicte.GetWritedSize());
			FormatWritter<CharT> Writter{ std::span(Result) };
			if (Formatter{}(Writter, Par, S))
			{
				return Result;
			}
		}
		return {};
	}

}

export namespace Potato::Format
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
				auto P = Encode::CharEncoder<UnicodeType, wchar_t>::EncodeOnceUnSafe(std::span(Par), { Buffer, 2});
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
			Encode::EncodeInfo Str = Encode::StrEncoder<UnicodeT, TargetType>::RequireSpaceUnSafe(Par);
			Output.resize(Output.size() + Str.TargetSpace);
			auto OutputSpan = std::span(Output).subspan(OldSize);
			Encode::StrEncoder<UnicodeT, TargetType>::EncodeUnSafe(Par, OutputSpan);
			return true;
		}
	};

	template<typename NumberType, typename UnicodeType>
	struct BuildInNumberFormatter
	{
		bool operator()(FormatWritter<UnicodeType>& Writter, std::basic_string_view<UnicodeType> Pars, NumberType Input) {
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
		bool operator()(FormatWritter<UnicodeType>& Writter, std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			
			auto Last = Writter.GetLastBuffer();
			if (Last.has_value())
			{
				auto Result = Encode::CharEncoder<wchar_t, UnicodeType>::EncodeOnceUnSafe(Input, *Last);
				Writter.Allocate(Result.TargetSpace);
			}
			else {
				auto Result = Encode::CharEncoder<wchar_t, UnicodeType>::RequireSpaceUnSafe(Input);
				Writter.Allocate(Result.TargetSpace);
			}
			return true;
		}
		static std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			auto Info = Encode::StrEncoder<SUnicodeT, UnicodeType>::RequireSpaceUnSafe(Input);
			return Info.TargetSpace;
		}
	};

	template<typename SUnicodeT, typename CharTrais, typename Allocator, typename UnicodeType>
	struct Formatter<std::basic_string<SUnicodeT, CharTrais, Allocator>, UnicodeType>: Formatter<std::basic_string_view<SUnicodeT, CharTrais>, UnicodeType> {};

}