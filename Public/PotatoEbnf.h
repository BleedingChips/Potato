#pragma once

#include <assert.h>
#include "PotatoReg.h"
#include "PotatoSLRX.h"

namespace Potato::Ebnf
{

	struct EBNFX
	{
		static EBNFX Create(std::u8string_view Str);
		std::vector<std::u8string> MappingTerminalSymbols;
		std::vector<std::u8string> MappingNoterminalSymbols;
		Reg::DFA Lexical;
		SLRX::LRX Syntax;
	};

	struct ParsingStep
	{
		SLRX::Symbol Value;

		struct ReduceT {
			std::size_t ProductionIndex;
			std::size_t ProductionCount;
			SLRX::StandardT Mask;
		};

		struct ShiftT {
			std::u8string_view CaptureValue;
			Reg::StandardT Mask;
			std::size_t TokenIndex;
		};

		std::variant<ReduceT, ShiftT> Datas;
	};

	struct TElement
	{
		SLRX::Symbol Value;
		ParsingStep::ShiftT Shift;
	};

	struct NTElement
	{
		SLRX::Symbol Value;
		ParsingStep::ReduceT Reduce;
		std::span<SLRX::NTElement::DataT> Datas;
		SLRX::NTElement::DataT& operator[](std::size_t index) { return Datas[index]; }
	};

	struct VariantElement
	{
		std::variant<TElement, NTElement> Element;
		bool IsTerminal() const { return std::holds_alternative<TElement>(Element); }
		bool IsNoTerminal() const { return std::holds_alternative<NTElement>(Element); }
		TElement& AsTerminal() { return std::get<TElement>(Element); }
		NTElement& AsNoTerminal() { return std::get<NTElement>(Element); }
	};

	struct Condition
	{
		enum class TypeT
		{
			Normal, // Normal
			Or,
			Parentheses, // ()
			SquareBrackets, // []
			CurlyBrackets, // {}
		};

		TypeT Type;
		std::size_t AppendData;
		std::vector<SLRX::NTElement::DataT> Datas;
	};

	

	struct LexicalElement
	{
		std::u8string_view CaptureValue;
		SLRX::Symbol Value;
		std::size_t Mask;
	};

	struct LexicalElementTuple
	{
		LexicalElement Ele;
		Misc::IndexSpan<> StrIndex;
	};

	struct CoreProcessor
	{

		template<typename RT>
		struct Result
		{
			std::optional<RT> Element;
			std::size_t Length;
			operator bool() const { return Element.has_value(); }
		};

		struct LexicalResult
		{
			std::optional<LexicalElement> Element;
			std::size_t Length;
			operator bool() const { return Element.has_value(); }
		};

		static Result<LexicalElement> LexicalConsumeOnce(EBNFX const& Table, std::u8string_view Str);
		static Result<std::vector<LexicalElementTuple>> LexicalProcess(EBNFX const& Table, std::u8string_view Str);
		static Result<std::vector<ParsingStep>> SynatxProcess(EBNFX const& Table, std::span<LexicalElementTuple const> Tuples);
		static std::any StepProcess(std::span<ParsingStep const> Tuples, std::function<std::any(VariantElement& Ele)> Func);

		template<typename RequireType>
		static std::optional<RequireType> StepProcessWithOutput(std::span<ParsingStep const> Tuples, std::function<std::any(VariantElement& Ele)> Func) {
			auto Re = StepProcess(Tuples, std::move(Fun));
			if (Re.has_value())
			{
				auto Re2 = std::any_cast<RequireType*>(Re);
				if (Re2 != nullptr)
				{
					return std::move(*Re2);
				}
			}
			return {};
		}
	};


	


	//std::any ProcessEbnf(std::u8string_view Str, std::function<std::any(VariantElement&)> RespondFunction);



	/*
	struct TableWrapper
	{
		using StorageT = uint32_t;
		using SubStorageT = uint16_t;
		
		struct ZipHeader
		{
			StorageT NameStringCount;
			SubStorageT NameIndexCount;
			SubStorageT TerminalNameCount;
			StorageT RegCount;
			StorageT DLrCount;
		};

		static std::vector<StorageT> Create(std::u32string_view Str) { return Create(UnserilizeTable{Str}); }
		static std::vector<StorageT> Create(UnserilizeTable const& Table);
		operator bool() const { return IsAvailable(); }
		bool IsAvailable() const { return !Wrapper.empty(); }

		TableWrapper(std::span<StorageT const>);
		TableWrapper(TableWrapper const&) = default;
		TableWrapper& operator=(TableWrapper const&) = default;
		Reg::TableWrapper AsRegWrapper() const { return RegWrapper; }
		SLRX::TableWrapper AsSLRXWrapper() const { return SLRXWrapper; }
		std::u32string_view FindSymbolName(SLRX::Symbol Symbol) const;

	private:

		std::span<StorageT const> Wrapper;
		std::span<Misc::IndexSpan<SubStorageT> const> StrIndex;
		std::size_t TerminalCount = 0;
		std::u32string_view TotalName;
		std::span<Reg::TableWrapper::StandardT const> RegWrapper;
		std::span<SLRX::TableWrapper::StandardT const> SLRXWrapper;
	};
	*/

	/*
	struct EbnfSymbolTuple
	{
		SLRX::Symbol Value;
		Misc::IndexSpan<> StrIndex;
		Misc::IndexSpan<> CaptureMain;
		Reg::TableWrapper::StandardT Mask;
	};

	struct SymbolProcessor
	{
		
		struct Result
		{
			std::optional<EbnfSymbolTuple> Accept;
			bool IsEndOfFile;
		};

		std::size_t GetReuqireTokenIndex() const { return Processor.GetRequireTokenIndex(); }
		std::optional<Result> ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex);

		SymbolProcessor(TableWrapper Wrapper, std::size_t StartupTokenIndex = 0) : Processor(Wrapper.AsRegWrapper(), StartupTokenIndex) {}

	private:

		Reg::GreedyFrontMatchProcessor Processor;
	};

	std::vector<EbnfSymbolTuple> ProcessSymbol(TableWrapper Wrapper, std::u32string_view Str);

	struct TElement
	{
		SLRX::Symbol Value;
		std::size_t Mask;
		Misc::IndexSpan<> Index;
	};

	struct NTElement
	{
		SLRX::Symbol Value;
		std::size_t Mask;
		Misc::IndexSpan<> Index;
		std::span<SLRX::NTElement::DataT> Datas;
		SLRX::NTElement::DataT& operator[](std::size_t index) { return Datas[index]; }
	};

	struct Condition
	{
		enum class TypeT
		{
			Or,
			Parentheses, // ()
			SquareBrackets, // []
			CurlyBrackets, // {}
		};

		TypeT Type;
		std::size_t AppendData;
		Misc::IndexSpan<> Index;
		std::vector<SLRX::NTElement::DataT> Datas;
	};

	struct StepElement
	{
		std::variant<TElement, NTElement> Element;
		bool IsTerminal() const { return std::holds_alternative<TElement>(Element); }
		bool IsNoTerminal() const { return std::holds_alternative<NTElement>(Element); }
		TElement& AsTerminal() { return std::get<TElement>(Element); }
		NTElement& AsNoTerminal() { return std::get<NTElement>(Element); }
	};

	std::any ProcessStep(TableWrapper Wrapper, std::span<EbnfSymbolTuple const> InputSymbol, std::any(*F)(StepElement Element, void* Data), void* Data);

	template<typename Func>
	std::any ProcessStep(TableWrapper Wrapper, std::span<EbnfSymbolTuple const> InputSymbol, Func&& F) requires(std::is_invocable_r_v<std::any, Func, StepElement>)
	{
		return ProcessStep(Wrapper, InputSymbol, [](StepElement Ele, void* Data)->std::any{
			return (*reinterpret_cast<std::remove_cvref_t<Func>*>(Data))(Ele);
		}, &F);
	}

	template<typename FN, typename Func>
	FN ProcessStepWithOutputType(TableWrapper Wrapper, std::span<EbnfSymbolTuple const> InputSymbol, Func&& F) requires(std::is_invocable_r_v<std::any, Func, StepElement>)
	{
		auto Result = ProcessStep(Wrapper, InputSymbol, F);
		return std::any_cast<FN>(Result);
	}
	*/
}

namespace Potato::Ebnf::Exception
{

	struct Interface : public std::exception
	{
		virtual ~Interface() = default;
		virtual char const* what() const override;
	};

	struct IllegalEbnfProduction : public std::exception
	{
		SLRX::Exception::IllegalSLRXProduction::Category Type;
		std::size_t MaxForwardDetectNum;
		std::size_t DetectNum;

		struct TMap
		{
			std::u32string Mapping;
		};

		struct NTMap
		{
			std::u32string Mapping;
			std::size_t ProductionCount;
			std::size_t ProductionMask;
		};

		std::vector<std::variant<TMap, NTMap>> Steps1;
		std::vector<std::variant<TMap, NTMap>> Steps2;

		/*
		struct Productions
		{
			std::u32string Value;
			std::size_t ProductionCount;
			std::size_t ProductionMask;
			std::size_t ProductionIndex;
		};

		std::vector<Productions> EffectProductions;
		*/

		IllegalEbnfProduction(SLRX::Exception::IllegalSLRXProduction const&, std::vector<std::u32string> const& TMapping, std::vector<std::u32string> const& NTMapping);
		IllegalEbnfProduction(IllegalEbnfProduction const&) = default;
		IllegalEbnfProduction(IllegalEbnfProduction&&) = default;
		static std::vector<std::variant<TMap, NTMap>> Translate(std::span<SLRX::ParsingStep const> Steps, std::vector<std::u32string> const& TMapping, std::vector<std::u32string> const& NTMapping);
		virtual char const* what() const override;
	};

	struct UnacceptableRegex : public Interface
	{
		std::u8string Regex;
		UnacceptableRegex(std::u8string Regex) : Regex(std::move(Regex)) {}
		UnacceptableRegex(UnacceptableRegex const&) = default;
		virtual char const* what() const override;
	};

	struct UnacceptableEbnf : public Interface
	{
		enum class TypeT
		{
			WrongEbnfLexical,
			WrongEbnfSyntax,
			WrongRegex,
			WrongLr,
			WrongPriority,
			RegexOutofRange,
			LrOutOfRange,
			UnrecognizableSymbol,
			UnrecognizableRex,
			UnrecognizableTerminal,
			UnsetProductionHead,
			StartSymbolAreadySet,
			UnfindedStartSymbol,
		};
		TypeT Type;
		std::u8string LastStr;
		std::size_t TokenIndex;
		UnacceptableEbnf(TypeT Type, std::u8string_view TotalStr, std::size_t TokenIndex) : Type(Type), TokenIndex(TokenIndex) {
			LastStr = std::u8string{ TokenIndex < 10 ? TotalStr.substr(0, TokenIndex) : TotalStr.substr(TokenIndex - 10, 10)};
		}
		UnacceptableEbnf(UnacceptableEbnf const&) = default;
		virtual char const* what() const override;
	};

	struct OutofRange : public Interface
	{
		enum class TypeT
		{
			TempMaskedEmptyProduction,
			FromRegex,
			FromSLRX,
			Header,
			EbnfMask,
			SymbolCount,
			TerminalCount,
		};
		TypeT Type;
		std::size_t Value;
		OutofRange(TypeT Type, std::size_t TokenIndex) : Type(Type), Value(TokenIndex) {}
		OutofRange(OutofRange const&) = default;
		virtual char const* what() const override;
	};

	struct UnacceptableSymbol : public Interface
	{
		enum class TypeT
		{
			Ebnf,
			DLr,
			Reg,
		};
		TypeT Type;
		std::size_t TokenIndex;
		UnacceptableSymbol(TypeT Type, std::size_t TokenIndex) : Type(Type), TokenIndex(TokenIndex) {}
		UnacceptableSymbol(UnacceptableSymbol const&) = default;
		virtual char const* what() const override;
	};

	struct UnacceptableInput : public Interface
	{
		std::u32string Name;
		std::size_t TokenIndex;
		UnacceptableInput(std::u32string Name, std::size_t TokenIndex) : Name(Name), TokenIndex(TokenIndex) {}
		UnacceptableInput(UnacceptableInput const&) = default;
		virtual char const* what() const override;
	};

	/*
	struct EbnfInterface
	{
		virtual ~EbnfInterface() = default;
	};

	using EbnfBaseDefineInterface = DefineInterface<EbnfInterface>;

	struct EbnfExceptionStep
	{
		using ExceptionInterface = EbnfBaseDefineInterface;
		std::u32string name;
		bool is_terminal = false;
		size_t production_mask = LrProductionInput::default_mask();
		size_t production_count = 0;
		std::u32string capture;
		Section section;
	};

	struct EbnfMissingStartSymbol { using ExceptionInterface = EbnfBaseDefineInterface;  };

	struct EbnfUndefinedTerminal {
		using ExceptionInterface = EbnfBaseDefineInterface;
		std::u32string token;
		Section section;
	};

	struct EbnfUndefinedNoterminal {
		using ExceptionInterface = EbnfBaseDefineInterface;
		std::u32string token;
	};

	struct EbnfUnsetDefaultProductionHead { using ExceptionInterface = EbnfBaseDefineInterface; };

	struct EbnfRedefinedStartSymbol {
		using ExceptionInterface = EbnfBaseDefineInterface;
		Section section;
	};

	struct EbnfUncompleteEbnf
	{
		using ExceptionInterface = EbnfBaseDefineInterface;
		size_t used;
	};

	struct EbnfUnacceptableToken {
		using ExceptionInterface = EbnfBaseDefineInterface;
		std::u32string token;
		Section section;
	};

	struct EbnfUnacceptableSyntax {
		using ExceptionInterface = EbnfBaseDefineInterface;
		std::u32string type;
		std::u32string data;
		Section section;
		std::vector<std::u32string> exception_step;
	};

	struct EbnfUnacceptableRegex
	{
		using ExceptionInterface = EbnfBaseDefineInterface;
		std::u32string regex;
		size_t acception_mask;
	};
	*/

	/*
	struct ErrorMessage {
		std::u32string message;
		Nfa::Location loc;
	};
	*/
}

namespace Potato::StrFormat
{
	/*
	template<> struct Formatter<Ebnf::Table>
	{
		std::u32string operator()(std::u32string_view, Ebnf::Table const& ref);
	};
	*/
}