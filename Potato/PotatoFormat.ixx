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

	template<typename CharT, typename CharTraits>
	bool ScanCapture(std::span<Misc::IndexSpan<> const>, std::basic_string_view<CharT, CharTraits> InputStr)
	{
		return true;
	}

	template<typename CharT, typename CharTraits, typename CT, typename ...OT>
	bool ScanCapture(std::span<Misc::IndexSpan<> const> CaptureIndex, std::basic_string_view<CharT, CharTraits> InputStr, CT& O1, OT& ...OO)
	{
		return CaptureIndex.size() >= 1 && DirectScan(CaptureIndex[0].Slice(InputStr), O1) && ScanCapture(CaptureIndex.subspan(1), InputStr, OO...);
	}

	template<typename CharT, typename CharTraits, typename ...OT>
	bool ScanCapture(Reg::ProcessorAcceptT const& Accept, std::basic_string_view<CharT, CharTraits> InputStr,  OT& ...OO)
	{
		return ScanCapture(std::span(Accept.Capture), InputStr, OO...);
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
				}else
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
				return ApplyResult{Par.size(), Index, Par.size()};
			case Implement::FormatPatternT::Parameter:
			{
				auto Re = FormatExe<GetSize, 0>(Output, Par, Index, std::forward<OtherType>(OT)...);
				if (Re.has_value())
				{
					return ApplyResult{*Re, Index + 1, *Re};
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
			}else
				return {};
			Str = Last;
		}
		return Count;
	}

	template<typename CharT, typename ...OType>
	constexpr std::optional<std::size_t> FormatSize(const CharT * Formatter, OType&& ...OT) {
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
			}else
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
				}else
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
			if(*Re)
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
	auto DirectFormat(std::basic_string_view<CharT> Par, SourceT && S) -> std::optional<std::basic_string<CharT>> { 
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
					auto Result = Encode::CharEncoder<wchar_t, UnicodeType>::EncodeOnceUnSafe({ &Temp, 1 }, Output.subspan(Index));
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
					auto Result = Encode::CharEncoder<wchar_t, UnicodeType>::RequireSpaceOnceUnSafe({ &Temp, 1 });
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
			auto Info = Encode::StrEncoder<SUnicodeT, UnicodeType>::EncodeUnSafe(Input, Output);
			return Info.TargetSpace;
		}
		static std::optional<std::size_t> FormatSize(std::basic_string_view<UnicodeType> Parameter, std::basic_string_view<SUnicodeT, CharTrais> const& Input) {
			auto Info = Encode::StrEncoder<SUnicodeT, UnicodeType>::RequireSpaceUnSafe(Input);
			return Info.TargetSpace;
		}
	};

	template<typename SUnicodeT, typename CharTrais, typename Allocator, typename UnicodeType>
	struct Formatter<std::basic_string<SUnicodeT, CharTrais, Allocator>, UnicodeType>: Formatter<std::basic_string_view<SUnicodeT, CharTrais>, UnicodeType> {};

}