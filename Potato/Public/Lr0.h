#include <vector>
#include <map>
#include <set>

#include "Lr.h"

namespace Potato::Lr0
{
	using namespace Potato::Lr;

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

	History Process(Table const& Table, Symbol const* TokenArray, size_t TokenLength);

	inline std::any Process(Table const& Table, Symbol const* TokenArray, size_t TokenLength, std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFun)
	{
		auto His = Process(Table, TokenArray, TokenLength);
		return Process(His, std::move(NTFunc), std::move(TFun));
	}

	Table CreateTable(Symbol start_symbol, std::vector<ProductionInput> const& production, std::vector<OpePriority> const& priority);
}
