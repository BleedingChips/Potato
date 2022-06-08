#pragma once

#include <assert.h>
#include "PotatoReg.h"
#include "PotatoDLr.h"

namespace Potato::Ebnf
{

	struct UnserilizeTable
	{

		UnserilizeTable(std::u32string_view Str);
		UnserilizeTable(UnserilizeTable const&) = default;
		UnserilizeTable(UnserilizeTable&&) = default;

		std::vector<std::u32string> TerminalMappings;
		std::vector<std::u32string> NoTermialMapping;
		std::vector<Reg::SerilizeT> RegTable;
		std::vector<DLr::TableWrapper::SerilizedT> DLrTable;


		static constexpr std::size_t SmallBrace() { return 0; }
		static constexpr std::size_t MiddleBrace() { return 1; }
		static constexpr std::size_t BigBrace() { return 2; }
		static constexpr std::size_t OrLeft() { return 3; }
		static constexpr std::size_t OrRight() { return 4; }
		static constexpr std::size_t MinMask() { return 5; }
	};

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
		DLr::TableWrapper AsDLrWrapper() const { return DLrWrapper; }
		std::u32string_view FindSymbolName(DLr::Symbol Symbol) const;

	private:

		std::span<StorageT const> Wrapper;
		std::span<Misc::IndexSpan<SubStorageT> const> StrIndex;
		std::size_t TerminalCount = 0;
		std::u32string_view TotalName;
		std::span<Reg::SerilizeT const> RegWrapper;
		std::span<DLr::TableWrapper::SerilizedT const> DLrWrapper;
	};

	struct EbnfSymbolTuple
	{
		DLr::Symbol Value;
		Misc::IndexSpan<> StrIndex;
		Misc::IndexSpan<> CaptureMain;
		Reg::TableWrapper::StorageT Mask;
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
		DLr::Symbol Value;
		std::size_t Mask;
		Misc::IndexSpan<> Index;
	};

	struct NTElement
	{
		DLr::Symbol Value;
		std::size_t Mask;
		Misc::IndexSpan<> Index;
		std::span<DLr::NTElementData> Datas;
		DLr::NTElementData& operator[](std::size_t index) { return Datas[index]; }
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
		std::vector<DLr::NTElementData> Datas;
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
		return std::any_cast<FN>(ProcessStep(Wrapper, InputSymbol, F));
	}


	/*
	struct EbnfTable
	{
		std::u32string symbol_table;
		std::vector<uint32_t> state_to_mask;
		LexicalTable lexical_table;
		std::vector<std::tuple<std::size_t, std::size_t>> symbol_map;
		size_t ter_count;
		Lr0Table lr0_table;
		std::u32string_view FindSymbolString(size_t input, bool IsTerminal) const noexcept;
		std::optional<size_t> FindSymbolState(std::u32string_view sym) const noexcept { bool is_terminal; return FindSymbolState(sym, is_terminal); }
		std::optional<size_t> FindSymbolState(std::u32string_view sym, bool& Isterminal) const noexcept;
	};

	EbnfTable CreateEbnfTable(std::u32string_view Code);

	enum class EbnfStepCategory
	{
		TERMINAL,
		NOTERMINAL,
		PREDEFINETERMINAL,
		UNKNOW
	};

	struct EbnfStep
	{
		size_t state = 0;
		std::u32string_view string;
		EbnfStepCategory category = EbnfStepCategory::UNKNOW;
		Section section;
		union {
			struct {
				size_t mask = 0;
				size_t production_count = 0;
			}reduce;
			struct {
				std::u32string_view capture;
				size_t mask = 0;
			}shift;
		};
		EbnfStep(){}
		bool IsTerminal() const noexcept { return category == EbnfStepCategory::TERMINAL; }
		bool IsNoterminal() const noexcept { return category == EbnfStepCategory::NOTERMINAL; }
		bool IsPreDefineNoterminal() const noexcept { return category == EbnfStepCategory::PREDEFINETERMINAL; }
	};

	struct EbnfTElement
	{
		size_t state = 0;
		std::u32string_view string;
		Section section;
		std::u32string_view capture;
		size_t mask = 0;
		EbnfTElement(EbnfStep const& step) : state(step.state), string(step.string), section(step.section), capture(step.shift.capture), mask(step.shift.mask)
		{
			assert(step.IsTerminal());
		}
		EbnfTElement(EbnfTElement const&) = default;
		EbnfTElement& operator=(EbnfTElement const&) = default;
	};

	

	struct EbnfNTElement
	{
		struct Property : EbnfStep
		{
			Property() {}
			Property(EbnfStep step, std::any datas) : EbnfStep(step), data(std::move(datas)){}
			Property(Property const&) = default;
			Property(Property&&) = default;
			Property& operator=(Property&& p) = default;
			Property& operator=(Property const& p) = default;
			std::any data;
			template<typename Type>
			std::remove_reference_t<Type> Consume() { return std::move(std::any_cast<std::add_lvalue_reference_t<Type>>(data)); }
			std::any Consume() { return std::move(data); }
			template<typename Type>
			std::optional<Type> TryConsume() {
				auto P = std::any_cast<Type>(&data);
				if (P != nullptr)
					return std::move(*P);
				else
					return std::nullopt;
			}
		};

		size_t state = 0;
		size_t mask = 0;
		std::u32string_view string;
		Section section;
		bool is_predefine = false;
		std::span<Property> production;
		Property& operator[](size_t index) { return production[index]; }
		bool IsPredefine() const noexcept {return is_predefine;};
		auto begin() { return production.begin(); }
		auto end() { return production.end();}
		EbnfNTElement(EbnfStep const& ref, Property* data) : state(ref.state), string(ref.string), section(ref.section), mask(ref.reduce.mask), 
			production((ref.category == EbnfStepCategory::NOTERMINAL ? data : nullptr), ref.reduce.production_count)
			,is_predefine(ref.category == EbnfStepCategory::PREDEFINETERMINAL)
		{
			assert(ref.IsNoterminal() || ref.IsPreDefineNoterminal());
		}
		EbnfNTElement& operator=(EbnfNTElement const&) = default;
	};

	enum class EbnfSequencerType : uint32_t
	{
		OR,
		OPTIONAL,
		REPEAT,
	};

	constexpr uint32_t operator*(EbnfSequencerType input) noexcept { return static_cast<uint32_t>(input); }

	struct EbnfSequencer
	{
		EbnfSequencerType type;
		std::vector<EbnfNTElement::Property> datas;
		EbnfNTElement::Property& operator[](size_t input) { return datas[input]; }
		decltype(auto) begin() { return datas.begin(); }
		decltype(auto) end() { return datas.end(); }
		decltype(auto) size() { return datas.size(); }
		bool empty() const noexcept { return datas.empty(); }
	};
	*/

	

	/*
	struct Element : Step
	{
		struct Property : Step
		{
			Property() {}
			Property(Step step, std::any datas) : Step(step), data(std::move(datas)){}
			Property(Property const&) = default;
			Property(Property&&) = default;
			Property& operator=(Property&& p) = default;
			Property& operator=(Property const& p) = default;
			std::any data;
			template<typename Type>
			Type GetData() { return std::any_cast<Type>(data); }
			template<typename Type>
			Type* TryGetData() { return std::any_cast<Type>(&data); }
			template<typename Type>
			std::remove_reference_t<Type> MoveData() { return std::move(std::any_cast<std::add_lvalue_reference_t<Type>>(data)); }
			std::any MoveRawData() { return std::move(data); }
		};
		Property* datas = nullptr;
		Property& operator[](size_t index) { return datas[index]; }
		Property* begin() {assert(Step::IsNoterminal()); return datas;}
		Property* end() {assert(Step::IsNoterminal()); return datas + reduce.production_count;}
		Element(Step const& ref) : Step(ref) {}
	};
	*/

	/*
	struct EbnfHistory
	{
		std::vector<EbnfStep> steps;
		std::any operator()(std::function<std::any(EbnfNTElement&)> NTFunc, std::function<std::any(EbnfTElement&)> TFunc) const;
		std::vector<std::u32string> Expand() const;
	};

	EbnfHistory Process(EbnfTable const& Tab, std::u32string_view Code);
	inline std::any Process(EbnfHistory const& His, std::function<std::any(EbnfNTElement&)> NTFunc, std::function<std::any(EbnfTElement&)> TFunc)
	{
		return His(std::move(NTFunc), std::move(TFunc));
	}
	template<typename RespondFunction>
	std::any Process(EbnfHistory const& ref, RespondFunction&& Func) { return ref(std::forward<RespondFunction>(Func));}
	template<typename RequireType, typename RespondFunction>
	RequireType ProcessWrapper(EbnfHistory const& ref, RespondFunction&& Func) { return std::any_cast<RequireType>(ref(std::forward<RespondFunction>(Func))); }
	*/
}

namespace Potato::Ebnf::Exception
{

	struct Interface : public std::exception
	{
		virtual ~Interface() = default;
		virtual char const* what() const override;
	};

	struct UnacceptableEbnf : public Interface
	{
		enum class TypeT
		{
			WrongEbnf,
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
		std::size_t TokenIndex;
		UnacceptableEbnf(TypeT Type, std::size_t TokenIndex) : Type(Type), TokenIndex(TokenIndex) {}
		UnacceptableEbnf(UnacceptableEbnf const&) = default;
		virtual char const* what() const override;
	};

	struct OutofRange : public Interface
	{
		enum class TypeT
		{
			Regex,
			DLr,
			Header
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
		std::size_t TokenIndex;
		UnacceptableInput(std::size_t TokenIndex) : TokenIndex(TokenIndex) {}
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