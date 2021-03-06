#include <limits>
#include <cassert>
#include <cstdint>
#include <tuple>
#include <any>
#include <vector>
#include <functional>
#include <set>
#include <optional>

#include "Misc.h"

namespace Potato::Lr
{
	using SymbolStorageT = int32_t;

	struct TerminalT {};
	struct NoTerminalT {};

	inline constexpr size_t symbol_storage_max = static_cast<size_t>(std::numeric_limits<SymbolStorageT>::max()) - 3;
	inline constexpr size_t symbol_storage_min = static_cast<size_t>(0) + 1;
	inline constexpr TerminalT terminal;
	inline constexpr NoTerminalT noterminal;

	struct Symbol
	{
		SymbolStorageT value;
		constexpr Symbol() : Symbol(Symbol::EndOfFile()) {}
		constexpr Symbol(size_t input, TerminalT) : value(static_cast<SymbolStorageT>(input) + 1) {
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

	struct TElement
	{
		Symbol value;
		size_t token_index;
		TElement(Step const& value) : value(value.value), token_index(value.shift.token_index) { assert(value.IsTerminal()); }
		TElement(TElement const&) = default;
		TElement& operator=(TElement const&) = default;
	};

	struct NTElementData
	{
		Symbol symbol;
		std::any data;
		operator Symbol() const { return symbol; }
		template<typename Type>
		Type Consume() { return std::move(std::any_cast<Type>(data)); }
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

	struct NTElement
	{

		Symbol value;
		size_t production_index;
		size_t mask;
		std::span<NTElementData> datas;
		NTElementData& operator[](size_t index) { return datas[index]; }
		NTElement(Step const& value, NTElementData* data_ptr) :
			value(value.value), production_index(value.reduce.production_index), datas(data_ptr, value.reduce.production_count), mask(value.reduce.mask)
		{
			assert(value.IsNoTerminal());
		}
		NTElement(NTElement const&) = default;
		NTElement& operator=(NTElement const&) = default;
	};

	struct History
	{
		std::vector<Step> steps;
		std::any operator()(std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFun) const;
	};

	inline std::any Process(History const& ref, std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFun) { return ref(std::move(NTFunc), std::move(TFun)); }

	template<typename RespondFunction>
	std::any Process(History const& ref, RespondFunction&& Func) { return ref(std::forward<RespondFunction>(Func)); }

	template<typename RequireType, typename RespondFunction>
	RequireType ProcessWrapper(History const& ref, RespondFunction&& Func) { return std::any_cast<RequireType>(ref(std::forward<RespondFunction>(Func))); }

	enum class Associativity
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

	
}

namespace Potato::Exception::Lr
{
	using Potato::Lr::Symbol;
	using Potato::Lr::History;

	struct Interface
	{
		virtual ~Interface() = default;
	};

	using BaseDefineInterface = DefineInterface<Interface>;

	struct NoterminalUndefined
	{
		using ExceptionInterface = BaseDefineInterface;
		Symbol value;
	};

	struct OperatorPriorityConflict
	{
		using ExceptionInterface = BaseDefineInterface;
		Symbol target_symbol;
		Symbol conflicted_symbol;
	};

	struct ProductionRedefined
	{
		using ExceptionInterface = BaseDefineInterface;
		std::vector<Symbol> productions;
		size_t production_index_1;
		size_t respond_mask_1;
		size_t production_index_2;
		size_t respond_mask_2;
	};

	struct UnaccableSymbol {
		using ExceptionInterface = BaseDefineInterface;
		size_t index;
		Symbol symbol;
		History backup_step;
	};
};