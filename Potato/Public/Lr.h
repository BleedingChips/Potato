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

namespace Potato
{
	using LrSymbolStorageT = int32_t;

	struct LrTerminalT {};
	struct LrNoTerminalT {};

	inline constexpr size_t lr_symbol_storage_max = static_cast<size_t>(std::numeric_limits<LrSymbolStorageT>::max()) - 3;
	inline constexpr size_t lr_symbol_storage_min = static_cast<size_t>(0) + 1;
	inline constexpr LrTerminalT terminal;
	inline constexpr LrNoTerminalT noterminal;

	struct LrSymbol
	{
		LrSymbolStorageT value;
		constexpr LrSymbol() : LrSymbol(LrSymbol::EndOfFile()) {}
		constexpr LrSymbol(size_t input, LrTerminalT) : value(static_cast<LrSymbolStorageT>(input) + 1) {
			assert(input < static_cast<uint64_t>(std::numeric_limits<LrSymbolStorageT>::max()) - 2);
		}
		constexpr LrSymbol(size_t input, LrNoTerminalT) : value(-static_cast<LrSymbolStorageT>(input) - 1) {
			assert(input < static_cast<uint64_t>(std::numeric_limits<LrSymbolStorageT>::max()) - 2);
		}
		constexpr bool operator<(LrSymbol const& input) const noexcept { return value < input.value; }
		constexpr bool operator== (LrSymbol const& input) const noexcept { return value == input.value; }
		constexpr bool IsTerminal() const noexcept { assert(value != 0); return value > 0; }
		constexpr bool IsNoTerminal() const noexcept { assert(value != 0); return value < 0; }
		constexpr static LrSymbol EndOfFile() noexcept { return LrSymbol(std::numeric_limits<LrSymbolStorageT>::max()); }
		constexpr static LrSymbol StartSymbol() noexcept { return LrSymbol(0); }
		constexpr bool IsEndOfFile() const noexcept { return *this == EndOfFile(); }
		constexpr bool IsStartSymbol() const noexcept { return *this == StartSymbol(); }
		constexpr LrSymbolStorageT Value() const noexcept { return value; }
		constexpr size_t Index() const noexcept {
			assert(IsTerminal() || IsNoTerminal());
			return static_cast<size_t>((value < 0 ? -value : value)) - 1;
		}
		constexpr operator LrSymbolStorageT() const noexcept { return value; }
		static inline constexpr LrSymbol MakeSymbol(LrSymbolStorageT input) { return LrSymbol(input); }
	private:
		constexpr LrSymbol(LrSymbolStorageT input) : value(input) {}
	};

	struct LrStep
	{
		LrSymbol value;
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

	struct LrTElement
	{
		LrSymbol value;
		size_t token_index;
		LrTElement(LrStep const& value) : value(value.value), token_index(value.shift.token_index) { assert(value.IsTerminal()); }
		LrTElement(LrTElement const&) = default;
		LrTElement& operator=(LrTElement const&) = default;
	};

	struct LrNTElementData
	{
		LrSymbol symbol;
		std::any data;
		operator LrSymbol() const { return symbol; }
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

	struct LrNTElement
	{

		LrSymbol value;
		size_t production_index;
		size_t mask;
		std::span<LrNTElementData> datas;
		LrNTElementData& operator[](size_t index) { return datas[index]; }
		LrNTElement(LrStep const& value, LrNTElementData* data_ptr) :
			value(value.value), production_index(value.reduce.production_index), datas(data_ptr, value.reduce.production_count), mask(value.reduce.mask)
		{
			assert(value.IsNoTerminal());
		}
		LrNTElement(LrNTElement const&) = default;
		LrNTElement& operator=(LrNTElement const&) = default;
	};

	struct LrHistory
	{
		std::vector<LrStep> steps;
		std::any operator()(std::function<std::any(LrNTElement&)> NTFunc, std::function<std::any(LrTElement&)> TFun) const;
	};

	inline std::any Process(LrHistory const& ref, std::function<std::any(LrNTElement&)> NTFunc, std::function<std::any(LrTElement&)> TFun) { return ref(std::move(NTFunc), std::move(TFun)); }

	template<typename RespondFunction>
	std::any Process(LrHistory const& ref, RespondFunction&& Func) { return ref(std::forward<RespondFunction>(Func)); }

	template<typename RequireType, typename RespondFunction>
	RequireType ProcessWrapper(LrHistory const& ref, RespondFunction&& Func) { return std::any_cast<RequireType>(ref(std::forward<RespondFunction>(Func))); }

	enum class LrAssociativity
	{
		Left,
		Right,
	};

	struct LrOpePriority
	{
		//OpePriority(std::initializer_list<Symbol> sym) : OpePriority(std::move(sym), Associativity::Left) {}
		LrOpePriority(std::vector<LrSymbol> sym) : LrOpePriority(std::move(sym), LrAssociativity::Left) {}
		LrOpePriority(std::vector<LrSymbol> sym, LrAssociativity lp) : sym(std::move(sym)), left_priority(lp) {}
		//OpePriority(Symbol sym) : OpePriority(std::vector<Symbol>{ sym }, true) {}
		//OpePriority(Symbol sym, bool lp) : OpePriority(std::vector<Symbol>{ sym }, lp) {}
		std::vector<LrSymbol> sym;
		LrAssociativity left_priority;
	};

	struct LrProductionInput
	{
		constexpr static size_t default_mask() { return std::numeric_limits<size_t>::max(); }

		LrProductionInput(std::vector<LrSymbol> input) : LrProductionInput(std::move(input), default_mask()) {}
		LrProductionInput(std::vector<LrSymbol> input, size_t funtion_enum) : production(std::move(input)), function_mask(funtion_enum) {}
		LrProductionInput(std::vector<LrSymbol> input, std::set<LrSymbol> remove, size_t funtion_enum) : production(std::move(input)), function_mask(funtion_enum), force_reduce(std::move(remove)) {}
		LrProductionInput(const LrProductionInput&) = default;
		LrProductionInput(LrProductionInput&&) = default;
		LrProductionInput& operator=(const LrProductionInput&) = default;
		LrProductionInput& operator=(LrProductionInput&&) = default;

		std::vector<LrSymbol> production;
		std::set<LrSymbol> force_reduce;
		size_t function_mask;
	};

	
}

namespace Potato::Exception
{

	struct LrInterface
	{
		virtual ~LrInterface() = default;
	};

	using LrBaseDefineInterface = DefineInterface<LrInterface>;

	struct LrNoterminalUndefined
	{
		using ExceptionInterface = LrBaseDefineInterface;
		LrSymbol value;
	};

	struct LrOperatorPriorityConflict
	{
		using ExceptionInterface = LrBaseDefineInterface;
		LrSymbol target_symbol;
		LrSymbol conflicted_symbol;
	};

	struct LrProductionRedefined
	{
		using ExceptionInterface = LrBaseDefineInterface;
		std::vector<LrSymbol> productions;
		size_t production_index_1;
		size_t respond_mask_1;
		size_t production_index_2;
		size_t respond_mask_2;
	};

	struct LrUnaccableSymbol {
		using ExceptionInterface = LrBaseDefineInterface;
		size_t index;
		LrSymbol symbol;
		LrHistory backup_step;
	};
};