#include <vector>
#include <map>
#include <set>

#include "Lr.h"

namespace Potato
{

	struct Lr0Table {

		struct Production
		{
			LrSymbolStorageT value;
			size_t production_count;
			size_t mask;
		};

		struct Reduce
		{
			size_t production_index;
		};

		struct Shift
		{
			LrSymbolStorageT require_symbol;
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
		std::map<LrSymbolStorageT, std::set<size_t>> force_reduce;
	};

	LrHistory Process(Lr0Table const& Table, LrSymbol const* TokenArray, size_t TokenLength);

	inline std::any Process(Lr0Table const& Table, LrSymbol const* TokenArray, size_t TokenLength, std::function<std::any(LrNTElement&)> NTFunc, std::function<std::any(LrTElement&)> TFun)
	{
		auto His = Process(Table, TokenArray, TokenLength);
		return Process(His, std::move(NTFunc), std::move(TFun));
	}

	Lr0Table CreateLr0Table(LrSymbol start_symbol, std::vector<LrProductionInput> const& production, std::vector<LrOpePriority> const& priority);
}
