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

		SLRX::LRX const& GetSyntaxWrapper() const { return Syntax; }
		Reg::DFA const& GetLexicalWrapper() const { return Lexical; }
		std::optional<std::u8string_view> ReadSymbol(SLRX::Symbol Value) const;
		bool IsNoName(SLRX::Symbol Value) const;
	};

	using StandardT = Reg::StandardT;

	struct TableWrapper
	{
		static std::size_t CalculateRequireSpace(EBNFX const& Ref);
		static std::size_t SerilizeTo(std::span<StandardT> OutputBuffer, EBNFX const& Ref);
		static std::vector<StandardT> Create(EBNFX const& Le);
		static std::vector<StandardT> Create(std::u8string_view Str) {
			return Create(EBNFX::Create(Str));
		}

		SLRX::TableWrapper GetSyntaxWrapper() const;
		Reg::TableWrapper GetLexicalWrapper() const;
		std::optional<std::u8string_view> ReadSymbol(SLRX::Symbol Value) const;
		operator bool() const { return !Wrapper.empty(); }
		bool IsNoName(SLRX::Symbol Value) const;

		std::span<StandardT const> Wrapper; 
	};

	struct LexicalElement
	{
		std::u8string_view CaptureValue;
		std::u8string_view SymbolStr;
		SLRX::Symbol Value;
		std::size_t Mask;
		Misc::IndexSpan<> StrIndex;
	};

	template<typename RT>
	struct Result
	{
		std::optional<RT> Element;
		std::size_t LastOffset;
		operator bool() const { return Element.has_value(); }
	};

	struct LexicalProcessor
	{

		std::optional<std::u8string_view> Consume(std::u8string_view Str, std::size_t MarkedOffset);

		LexicalProcessor(EBNFX const& Table) : Pro(Table.GetLexicalWrapper(), true), Table(Table) {}
		LexicalProcessor(TableWrapper Table) : Pro(Table.GetLexicalWrapper(), true), Table(Table) {}
		std::span<LexicalElement> GetSpan() { return std::span(Elements); }
		void Clear() { Elements.clear(); }

	protected:

		std::vector<LexicalElement> Elements;
		Reg::HeadMatchProcessor Pro;
		std::variant<TableWrapper, std::reference_wrapper<EBNFX const>> Table;
	};

	struct ParsingStep
	{
		std::u8string_view Symbol;

		struct ReduceT {
			std::size_t Mask;
			std::size_t ProductionElementCount;
			bool IsPredict;
			bool IsNoNameReduce;
		};

		struct ShiftT {
			std::u8string_view CaptureValue;
			std::size_t Mask;
			Misc::IndexSpan<> StrIndex;
		};
		std::variant<ReduceT, ShiftT> Data;
		bool IsTerminal() const { return std::holds_alternative<ShiftT>(Data); }
	};

	struct SyntaxProcessor
	{
		bool Consume(LexicalElement Element);
		bool EndOfFile();
		std::span<ParsingStep> GetSteps() { return std::span(ParsingSteps); }
		static Result<std::vector<ParsingStep>> Process(EBNFX const& Table, std::span<LexicalElement const> Eles, bool Predict = false) {
			SyntaxProcessor Pro{Table};
			return Process(Pro, Eles, Predict);
		}
		static Result<std::vector<ParsingStep>> Process(TableWrapper Table, std::span<LexicalElement const> Eles, bool Predict = false) {
			SyntaxProcessor Pro{ Table };
			return Process(Pro, Eles, Predict);
		}
		SyntaxProcessor(EBNFX const& Table) : Pro(Table.GetSyntaxWrapper()), Table(Table) {}
		SyntaxProcessor(TableWrapper Table) : Pro(Table.GetSyntaxWrapper()), Table(Table) {}
	protected:
		static Result<std::vector<ParsingStep>> Process(SyntaxProcessor& Pro, std::span<LexicalElement const> Eles, bool Predict = false);
		SLRX::SymbolProcessor Pro;
		std::vector<LexicalElement> TemporaryElementBuffer;
		std::vector<ParsingStep> ParsingSteps;
		std::variant<TableWrapper, std::reference_wrapper<EBNFX const>> Table;
		std::size_t HandleStep(std::span<SLRX::ParsingStep const> Steps);
		std::tuple<ParsingStep, std::size_t> Translate(SLRX::ParsingStep In, std::span<LexicalElement const> TemporaryElementBuffer) const;
	};

	struct TElement
	{
		std::u8string_view Symbol;
		ParsingStep::ShiftT Shift;
	};

	struct NTElement
	{
		struct MetaData
		{
			std::u8string_view Symbol;
			Misc::IndexSpan<> StrIndex;
		};

		struct DataT
		{
			MetaData Meta;
			SLRX::ElementData Data;

			template<typename Type>
			decltype(auto) Consume() { return Data.Consume<Type>(); }

			decltype(auto) Consume() { return Data.Consume(); }
			template<typename Type>
			decltype(auto) TryConsume() { return Data.TryConsume<Type>(); }
		};

		std::u8string_view Symbol;
		ParsingStep::ReduceT Reduce;
		std::span<DataT> Datas;
		DataT& operator[](std::size_t index) { return Datas[index]; }
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
		std::vector<NTElement::DataT> Datas;
	};

	struct PasringStepProcessor
	{
		struct Result
		{
			std::optional<VariantElement> Element;
			std::optional<NTElement::MetaData> MetaData;
		};

		std::optional<Result> Consume(ParsingStep Step);
		void Push(NTElement::MetaData MetaData, std::any Data) { TemporaryDatas.clear(); Datas.push_back({ MetaData, std::move(Data)}); }
		std::optional<std::any> EndOfFile();

	private:

		std::vector<NTElement::DataT> Datas;
		std::vector<NTElement::DataT> TemporaryDatas;
	};

	template<typename Func>
	std::optional<std::any> ProcessStep(std::span<ParsingStep const> Steps, Func&& F)
	{
		PasringStepProcessor Pro;
		for (auto Ite : Steps)
		{
			auto Re = Pro.Consume(Ite);
			if (Re.has_value())
			{
				if (Re->Element.has_value())
				{
					auto Ie = std::forward<Func>(F)(*Re->Element);
					if (Re->MetaData.has_value())
					{
						Pro.Push(*Re->MetaData, std::move(Ie));
					}
				}
			}
			else {
				return {};
			}
		}
		return Pro.EndOfFile();
	}

	/*

	struct CoreSyntaxProcessor
	{
		static ParsingStep Translate(std::span<LexicalElement const> Tokens, SLRX::ParsingStep Step);
	};

	struct EBNFXProcessor
	{
		bool Consume(LexicalElement Ele, Misc::IndexSpan<> StrIndex);
		bool EndOfFile();
		EBNFXProcessor(EBNFX const& Table) : Table(Table), Processor(Table.Syntax) {}
		void Reset();
	private:
		std::vector<std::tuple<LexicalElement, Misc::IndexSpan<>>> TemporaryBuffer;
		SLRX::LRXProcessor Processor;
		std::vector<ParsingStep> Steps;
		EBNFX const& Table;
	};

	struct StepProcessor
	{
		std::optional<std::tuple<VariantElement, NTElement::MetaData>> Translate(ParsingStep InputStep);
		void Push(NTElement::MetaData Data, std::any InputData);
		bool EndOfFile();

	private:

		std::vector<NTElement::StoragetT> TemporaryDatasBuffer;
		std::vector<NTElement::StoragetT> DataBuffer;
	};

	*/


	


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