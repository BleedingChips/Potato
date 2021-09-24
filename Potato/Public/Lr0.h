#include <vector>
#include <map>
#include <set>
#include <deque>
#include "Lr.h"

namespace Potato
{

	using StorageT = LrSymbolStorageT;

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
	LrHistory Process2(Lr0Table const& Table, LrSymbol const* TokenArray, size_t TokenLength);

	inline std::any Process(Lr0Table const& Table, LrSymbol const* TokenArray, size_t TokenLength, std::function<std::any(LrNTElement&)> NTFunc, std::function<std::any(LrTElement&)> TFun)
	{
		auto His = Process(Table, TokenArray, TokenLength);
		return Process(His, std::move(NTFunc), std::move(TFun));
	}

	Lr0Table CreateLr0Table(LrSymbol start_symbol, std::vector<LrProductionInput> const& production, std::vector<LrOpePriority> const& priority);

	struct Lr0ProcessContent
	{
		struct SearchCore
		{
			uint32_t CurrentBranch = 0;
			uint32_t StepCount = 0;
			uint32_t DependentedBranch = 0;
			uint32_t DependentedStepCount = 0;
			uint32_t StateCount = 0;
			std::optional<LrSymbol> InputSymbol;
		};

		struct StepTuple
		{
			LrStep Step;
			uint32_t OwneredBranch = 0;
			uint32_t StepCount = 0;
		};

		void InsertTerminalSymbol(LrSymbol InputSymbol);
		LrHistory EndOfSymbolStream();
		Lr0ProcessContent(Lr0Table const& Table);

		private:

		void ExpandSearchCore(bool IsFinalSymbol);

		void RemoveStepFromTemporaryCores(uint32_t RemoveBranch);
		LrHistory GeneratorHistory(uint32_t RequireBranch);

		Lr0Table const& Table;
		std::vector<StepTuple> Steps;
		std::vector<SearchCore> Cores;
		std::vector<uint32_t> States;
		std::vector<SearchCore> TemporaryCores;
		std::vector<uint32_t> TemporaryStates;
		uint32_t UsedBranch = 0;
		uint32_t TokenIndex = 0;
	};
}
