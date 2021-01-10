#pragma once
#include "Lr0.h"
#include "Lexical.h"
#include <assert.h>
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
				std::u32string_view capture = 0;
				size_t mask = 0;
			}shift;
		};
		Step(){}
		bool IsTerminal() const noexcept { return is_terminal; }
		bool IsNoterminal() const noexcept { return !IsTerminal(); }
	};

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

	struct History
	{
		std::vector<Step> steps;
		std::any operator()(std::any(*Function)(void*, Element&), void* FUnctionBody) const;
		template<typename RespondFunction>
		std::any operator()(RespondFunction&& Func) const{
			auto FunctionImp = [](void* FunctionBody, Element& data) -> std::any {
				return  std::forward<RespondFunction>(*static_cast<std::remove_reference_t<RespondFunction>*>(FunctionBody))(data);
			};
			return operator()(FunctionImp, static_cast<void*>(&Func));
		}
		std::vector<std::u32string> Expand() const;
	};

	History Process(Table const& Tab, std::u32string_view Code);
	inline std::any Process(History const& His, std::any(*Function)(void*, Element&), void* FUnctionBody) { return His(Function, FUnctionBody); }
	template<typename RespondFunction>
	std::any Process(History const& ref, RespondFunction&& Func) { return ref(std::forward<RespondFunction>(Func));}
	template<typename RequireType, typename RespondFunction>
	RequireType ProcessWrapper(History const& ref, RespondFunction&& Func) { return std::any_cast<RequireType>(ref(std::forward<RespondFunction>(Func))); }


	namespace Error
	{

		struct ExceptionStep
		{
			std::u32string name;
			bool is_terminal = false;
			size_t production_mask = Lr0::ProductionInput::default_mask();
			size_t production_count = 0;
			std::u32string capture;
			Section section;
		};

		struct MissingStartSymbol {};

		struct UndefinedTerminal {
			std::u32string token;
			Section section;
		};

		struct UndefinedNoterminal {
			std::u32string token;
		};

		struct UnsetDefaultProductionHead {};

		struct RedefinedStartSymbol {
			Section section;
		};

		struct UncompleteEbnf
		{
			size_t used;
		};

		struct UnacceptableToken {
			std::u32string token;
			Section section;
		};

		struct UnacceptableSyntax {
			std::u32string type;
			std::u32string data;
			Section section;
			std::vector<std::u32string> exception_step;
		};

		struct UnacceptableRegex
		{
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