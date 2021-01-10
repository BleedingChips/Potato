#pragma once
#include <assert.h>
#include <vector>
#include <map>
#include <set>
#include <any>
#include <functional>
#include <limits>
#undef max
namespace Potato::Lr0
{
	using SymbolStorageT = int32_t;

	constexpr size_t SymbolStorageMax = static_cast<size_t>(std::numeric_limits<SymbolStorageT>::max()) - 3;
	constexpr size_t SymbolStorageMin = static_cast<size_t>(0) + 1;

	constexpr struct TerminalT {} terminal;
	constexpr struct NoTerminalT {} noterminal;

	struct Symbol
	{
		SymbolStorageT value;
		constexpr Symbol() : Symbol(Symbol::EndOfFile()){}
		constexpr Symbol(size_t input, TerminalT) : value(static_cast<SymbolStorageT>(input) + 1){
			assert(input < static_cast<uint64_t>(std::numeric_limits<SymbolStorageT>::max()) - 2);
		}
		constexpr Symbol(size_t input, NoTerminalT) : value(-static_cast<SymbolStorageT>(input) - 1) {
			assert(input < static_cast<uint64_t>(std::numeric_limits<SymbolStorageT>::max()) - 2);
		}
		constexpr bool operator<(Symbol const& input) const noexcept { return value < input.value; }
		constexpr bool operator== (Symbol const& input) const noexcept { return value == input.value; }
		constexpr bool IsTerminal() const noexcept { assert(value != 0); return value > 0; }
		constexpr bool IsNoTerminal() const noexcept { assert(value != 0); return value < 0; }
		constexpr static Symbol EndOfFile() noexcept { return Symbol(std::numeric_limits<SymbolStorageT>::max()); }
		constexpr static Symbol StartSymbol() noexcept { return Symbol(0); }
		constexpr bool IsEndOfFile() const noexcept { return *this == EndOfFile(); }
		constexpr bool IsStartSymbol() const noexcept { return *this == StartSymbol(); }
		constexpr SymbolStorageT Value() const noexcept { return value; }
		constexpr size_t Index() const noexcept { 
			assert(IsTerminal() || IsNoTerminal());
			return static_cast<size_t>((value < 0 ? -value : value)) - 1;
		}
		constexpr operator SymbolStorageT() const noexcept { return value; }
		static inline constexpr Symbol MakeSymbol(SymbolStorageT input) { return Symbol(input); }
	private:
		constexpr Symbol(SymbolStorageT input) : value(input) {}
	};

	struct Table {

		struct Production
		{
			SymbolStorageT value;
			size_t production_count;
			size_t mask;
		};

		struct Reduce
		{
			size_t production_index;
		};

		struct Shift
		{
			SymbolStorageT require_symbol;
			size_t shift_state;
		};

		struct Node
		{
			size_t reduce_adress;
			size_t reduce_count;
			size_t shift_adress;
			size_t shift_count;
		};

		std::vector<Production> productions;
		std::vector<Reduce> reduces;
		std::vector<Shift> shifts;
		std::vector<Node> nodes;
		std::map<SymbolStorageT, std::set<size_t>> force_reduce;
	};

	struct Step
	{
		Symbol value;
		union
		{
			struct {
				size_t production_index;
				size_t production_count;
				size_t mask;
			}reduce;

			struct {
				size_t token_index;
			}shift;
		};
		constexpr bool IsTerminal() const noexcept { return value.IsTerminal(); }
		constexpr bool IsNoTerminal() const noexcept { return value.IsNoTerminal(); }
		constexpr bool IsEndOfFile() const noexcept { return value.IsEndOfFile(); }
		constexpr bool IsStartSymbol() const noexcept { return value.IsStartSymbol(); }
	};

	struct Element : Step
	{
		std::tuple<Symbol, std::any>* datas = nullptr;
		std::tuple<Symbol, std::any>& operator[](size_t index) { return datas[index]; }
		Symbol& GetSymbol(size_t index) { return std::get<0>((*this)[index]); }
		template<typename Type>
		Type GetData(size_t index) { return std::any_cast<Type>(std::get<1>((*this)[index])); }
		template<typename Type>
		std::remove_reference_t<Type> MoveData(size_t index) {return std::move(std::any_cast<std::add_lvalue_reference_t<Type>>(std::get<1>((*this)[index])));}
		std::any MoveRawData(size_t index) { return std::move(std::get<1>((*this)[index])); }
		std::any& GetRawData(size_t index) { return std::get<1>((*this)[index]); }
		Element& operator=(Step const& ref) { Step::operator=(ref); return *this; }
		Element(Step const& ref) : Step(ref) {}
	};

	struct History
	{
		std::vector<Step> steps;
		std::any operator()(std::any(*Function)(void*, Element&), void* FunctionBody) const;
		template<typename RespondFunction>
		std::any operator()(RespondFunction&& Func) const {
			auto FunctionImp = [](void* FunctionBody, Element& data) -> std::any {
				return  std::forward<RespondFunction>(*static_cast<std::remove_reference_t<RespondFunction>*>(FunctionBody))(data);
			};
			return operator()(FunctionImp, static_cast<void*>(&Func));
		}
	};

	History Process(Table const& Table, Symbol const* TokenArray, size_t TokenLength);

	inline std::any Process(History const& ref, std::any(*Function)(void*, Element&), void* FunctionBody) { return ref(Function, FunctionBody); }

	inline std::any Process(Table const& Table, Symbol const* TokenArray, size_t TokenLength, std::any(*Function)(void*, Element&), void* FunctionBody)
	{
		auto His = Process(Table, TokenArray, TokenLength);
		return Process(His, Function, FunctionBody);
	}

	template<typename RespondFunction>
	std::any Process(History const& ref, RespondFunction&& Func) { return ref(std::forward<RespondFunction>(Func)); }
	template<typename RequireType, typename RespondFunction>
	RequireType ProcessWrapper(History const& ref, RespondFunction&& Func) { return std::any_cast<RequireType>(ref(std::forward<RespondFunction>(Func))); }

	enum class  Associativity
	{
		Left,
		Right,
	};

	struct OpePriority
	{
		//OpePriority(std::initializer_list<Symbol> sym) : OpePriority(std::move(sym), Associativity::Left) {}
		OpePriority(std::vector<Symbol> sym) : OpePriority(std::move(sym), Associativity::Left) {}
		OpePriority(std::vector<Symbol> sym, Associativity lp) : sym(std::move(sym)), left_priority(lp) {}
		//OpePriority(Symbol sym) : OpePriority(std::vector<Symbol>{ sym }, true) {}
		//OpePriority(Symbol sym, bool lp) : OpePriority(std::vector<Symbol>{ sym }, lp) {}
		std::vector<Symbol> sym;
		Associativity left_priority;
	};

	struct ProductionInput
	{
		constexpr static size_t default_mask() { return std::numeric_limits<size_t>::max(); }

		ProductionInput(std::vector<Symbol> input) : ProductionInput(std::move(input), default_mask()) {}
		ProductionInput(std::vector<Symbol> input, size_t funtion_enum) : production(std::move(input)), function_mask(funtion_enum) {}
		ProductionInput(std::vector<Symbol> input, std::set<Symbol> remove, size_t funtion_enum) : production(std::move(input)), function_mask(funtion_enum), force_reduce(std::move(remove)) {}
		ProductionInput(const ProductionInput&) = default;
		ProductionInput(ProductionInput&&) = default;
		ProductionInput& operator=(const ProductionInput&) = default;
		ProductionInput& operator=(ProductionInput&&) = default;

		std::vector<Symbol> production;
		std::set<Symbol> force_reduce;
		size_t function_mask;
	};

	Table CreateTable(Symbol start_symbol, std::vector<ProductionInput> const& production, std::vector<OpePriority> const& priority);

	namespace Error
	{
		struct NoterminalUndefined {
			Symbol value;
		};

		struct OperatorPriorityConflict {
			Symbol target_symbol;
			Symbol conflicted_symbol;
		};

		struct ProductionRedefined
		{
			std::vector<Symbol> productions;
			size_t production_index_1;
			size_t respond_mask_1;
			size_t production_index_2;
			size_t respond_mask_2;
		};

		struct UnaccableSymbol {
			size_t index;
			Symbol symbol;
			History backup_step;
		};
	}
}

namespace PineApple::StrFormat
{
	/*
	template<>
	struct Formatter<Lr0::Table>
	{
		std::u32string operator()(std::u32string_view, Lr0::Table const& tab);
	};
	*/
}