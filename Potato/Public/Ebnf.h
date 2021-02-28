#pragma once

#include <assert.h>

#include "Lr0.h"
#include "Lexical.h"


namespace Potato::Ebnf
{

	using Symbol = Lr0::Symbol;
	using SectionPoint = Lexical::SectionPoint;
	using Section = Lexical::Section;

	struct Table
	{
		std::u32string symbol_table;
		std::vector<uint32_t> state_to_mask;
		Lexical::Table lexical_table;
		std::vector<std::tuple<std::size_t, std::size_t>> symbol_map;
		size_t ter_count;
		Lr0::Table lr0_table;
		std::u32string_view FindSymbolString(size_t input, bool IsTerminal) const noexcept;
		std::optional<size_t> FindSymbolState(std::u32string_view sym) const noexcept { bool is_terminal; return FindSymbolState(sym, is_terminal); }
		std::optional<size_t> FindSymbolState(std::u32string_view sym, bool& Isterminal) const noexcept;
	};

	Table CreateTable(std::u32string_view Code);

	struct Step
	{
		size_t state = 0;
		std::u32string_view string;
		bool is_terminal = false;
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
		Step(){}
		bool IsTerminal() const noexcept { return is_terminal; }
		bool IsNoterminal() const noexcept { return !IsTerminal(); }
	};

	struct TElement
	{
		size_t state = 0;
		std::u32string_view string;
		Section section;
		std::u32string_view capture = 0;
		size_t mask = 0;
		TElement(Step const& step) : state(step.state), string(step.string), section(step.section), capture(step.shift.capture), mask(step.shift.mask)
		{
			assert(step.IsTerminal());
		}
		TElement(TElement const&) = default;
		TElement& operator=(TElement const&) = default;
	};

	struct NTElement
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
		size_t state = 0;
		std::u32string_view string;
		Section section;
		size_t mask = 0;
		size_t production_count = 0;
		Property* datas = nullptr;
		Property& operator[](size_t index) { return datas[index]; }
		Property* begin() { return datas; }
		Property* end() { return datas + production_count;}
		NTElement(Step const& ref) : state(ref.state), string(ref.string), section(ref.section), mask(ref.reduce.mask), production_count(ref.reduce.production_count)
		{
			assert(!ref.IsTerminal());
		}
		NTElement& operator=(NTElement const&) = default;
	};

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

	struct History
	{
		std::vector<Step> steps;
		std::any operator()(std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFunc) const;
		std::vector<std::u32string> Expand() const;
	};

	History Process(Table const& Tab, std::u32string_view Code);
	inline std::any Process(History const& His, std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFunc)
	{
		return His(std::move(NTFunc), std::move(TFunc));
	}
	template<typename RespondFunction>
	std::any Process(History const& ref, RespondFunction&& Func) { return ref(std::forward<RespondFunction>(Func));}
	template<typename RequireType, typename RespondFunction>
	RequireType ProcessWrapper(History const& ref, RespondFunction&& Func) { return std::any_cast<RequireType>(ref(std::forward<RespondFunction>(Func))); }
}

namespace Potato::Exception::Ebnf
{
	using Potato::Ebnf::Section;

	struct Interface
	{
		virtual ~Interface() = default;
	};

	using BaseDefineInterface = DefineInterface<Interface>;

	struct ExceptionStep
	{
		using ExceptionInterface = BaseDefineInterface;
		std::u32string name;
		bool is_terminal = false;
		size_t production_mask = Lr0::ProductionInput::default_mask();
		size_t production_count = 0;
		std::u32string capture;
		Section section;
	};

	struct MissingStartSymbol { using ExceptionInterface = BaseDefineInterface;  };

	struct UndefinedTerminal {
		using ExceptionInterface = BaseDefineInterface;
		std::u32string token;
		Section section;
	};

	struct UndefinedNoterminal {
		using ExceptionInterface = BaseDefineInterface;
		std::u32string token;
	};

	struct UnsetDefaultProductionHead { using ExceptionInterface = BaseDefineInterface; };

	struct RedefinedStartSymbol {
		using ExceptionInterface = BaseDefineInterface;
		Section section;
	};

	struct UncompleteEbnf
	{
		using ExceptionInterface = BaseDefineInterface;
		size_t used;
	};

	struct UnacceptableToken {
		using ExceptionInterface = BaseDefineInterface;
		std::u32string token;
		Section section;
	};

	struct UnacceptableSyntax {
		using ExceptionInterface = BaseDefineInterface;
		std::u32string type;
		std::u32string data;
		Section section;
		std::vector<std::u32string> exception_step;
	};

	struct UnacceptableRegex
	{
		using ExceptionInterface = BaseDefineInterface;
		std::u32string regex;
		size_t acception_mask;
	};

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