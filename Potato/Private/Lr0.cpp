#include "../Public/Lr0.h"
#include <optional>

namespace Potato
{

	struct SearchElement
	{
		size_t StepsRecord;
		std::vector<size_t> State;
		size_t TokenIndex;
	};

	bool HandleInputToken(Lr0Table const& Table, std::vector<LrStep>& Steps, SearchElement& SE, LrSymbol Sym, size_t token_index)
	{
		auto CurState = *SE.State.rbegin();
		auto Nodes = Table.nodes[CurState];
		for (size_t i = 0; i < Nodes.shift_count; ++i)
		{
			auto TarShift = Table.shifts[i + Nodes.shift_adress];
			if (TarShift.require_symbol == static_cast<LrSymbolStorageT>(Sym))
			{
				SE.State.push_back(TarShift.shift_state);
				if (Sym.IsTerminal() && !Sym.IsEndOfFile())
				{
					LrStep ShiftStep;
					ShiftStep.value = Sym;
					ShiftStep.shift.token_index = token_index;
					Steps.push_back(ShiftStep);
					++SE.StepsRecord;
				}
				return true;
			}
		}
		return false;
	}

	struct SearchStackElement
	{
		SearchElement search;
		bool is_reduce;
		size_t production_index;
		size_t production_count;
		size_t mask;
		LrSymbolStorageT sym;
	};

	void ExpandProductions(Lr0Table const& Table, SearchElement&& Pros, std::vector<SearchStackElement>& SearchStack, LrSymbolStorageT sym, std::map<LrSymbolStorageT, std::set<size_t>> const& force_reduce)
	{
		auto CurNode = Table.nodes[*Pros.State.rbegin()];
		auto Find = force_reduce.find(sym);
		std::vector<SearchStackElement> ForceReduceList;
		for (size_t i = 0; i < CurNode.reduce_count; ++i)
		{
			size_t reduce_index = CurNode.reduce_count - 1 - i;
			auto Reduce = Table.reduces[CurNode.reduce_adress + reduce_index];
			auto Production = Table.productions[Reduce.production_index];
			if (Find == force_reduce.end() || Find->second.find(Reduce.production_index) == Find->second.end())
				SearchStack.push_back({ Pros, true, CurNode.reduce_count - 1 - i, Production.production_count, Production.mask, Production.value });
			else
				ForceReduceList.push_back({ Pros, true, CurNode.reduce_count - 1 - i, Production.production_count, Production.mask, Production.value });
		}
		SearchStack.push_back({ std::move(Pros), false, 0, 0, 0, 0 });
		SearchStack.insert(SearchStack.end(), std::move_iterator(ForceReduceList.begin()), std::move_iterator(ForceReduceList.end()));
	}

	LrSymbol GetSymbol(size_t index, LrSymbol const* TokenArray, size_t TokenLength)
	{
		LrSymbol Sym;
		if (index < TokenLength)
			return TokenArray[index];
		else if (index == TokenLength)
			Sym = LrSymbol::EndOfFile();
		else
			assert(false);
		return LrSymbol::EndOfFile();
	}

	LrHistory Process(Lr0Table const& Table, LrSymbol const* TokenArray, size_t TokenLength)
	{

		size_t MaxTokenUsed = 0;
		std::vector<LrStep> BackupSteps;

		std::vector<LrStep> Steps;
		std::vector<SearchStackElement> SearchStack;
		ExpandProductions(Table, { 0, {0}, 0 }, SearchStack, GetSymbol(0, TokenArray, TokenLength), Table.force_reduce);
		while (!SearchStack.empty())
		{
			SearchStackElement Cur = std::move(*SearchStack.rbegin());
			SearchStack.pop_back();
			Steps.resize(Cur.search.StepsRecord);
			if (Cur.is_reduce)
			{
				LrStep ReduceStep;
				ReduceStep.value = LrSymbol::MakeSymbol(Cur.sym);
				ReduceStep.reduce.production_index = Cur.production_index;
				ReduceStep.reduce.production_count = Cur.production_count;
				ReduceStep.reduce.mask = Cur.mask;
				Steps.push_back(ReduceStep);
				++Cur.search.StepsRecord;
				Cur.search.State.resize(Cur.search.State.size() - Cur.production_count);
				bool re = HandleInputToken(Table, Steps, Cur.search, ReduceStep.value, 0);
				assert(re);
				ExpandProductions(Table, std::move(Cur.search), SearchStack, GetSymbol(Cur.search.TokenIndex, TokenArray, TokenLength), Table.force_reduce);
			}
			else {
				LrSymbol Sym = GetSymbol(Cur.search.TokenIndex, TokenArray, TokenLength);
				bool re = HandleInputToken(Table, Steps, Cur.search, Sym, Cur.search.TokenIndex);
				if (re) {
					if (Sym == LrSymbol::EndOfFile())
						return { std::move(Steps) };
					else {
						++Cur.search.TokenIndex;
						ExpandProductions(Table, std::move(Cur.search), SearchStack, GetSymbol(Cur.search.TokenIndex, TokenArray, TokenLength), Table.force_reduce);
					}
				}else {
					if (MaxTokenUsed < Cur.search.TokenIndex)
					{
						MaxTokenUsed = Cur.search.TokenIndex;
						BackupSteps = Steps;
					}
				}
			}
		}
		throw Exception::MakeExceptionTuple(Exception::LrUnaccableSymbol{ MaxTokenUsed, MaxTokenUsed < TokenLength ? TokenArray[MaxTokenUsed] : LrSymbol::EndOfFile(), std::move(BackupSteps) });
	}
	/*
	void ExpandProductions2(Lr0Table const& Table, size_t Index, std::vector<SearchStackElement>& SearchStack)
	{
		SearchElement Pros = std::move(SearchStack[Index].search);
		auto CurNode = Table.nodes[*Pros.State.rbegin()];
		for (size_t i = 0; i < CurNode.reduce_count; ++i)
		{
			size_t reduce_index = CurNode.reduce_count - 1 - i;
			auto Reduce = Table.reduces[CurNode.reduce_adress + reduce_index];
			auto Production = Table.productions[Reduce.production_index];
			auto NewPros = Pros;
			SearchStack.push_back({ Pros, true, CurNode.reduce_count - 1 - i, Production.production_count, Production.mask, Production.value });
		}
		SearchStack[Index] = { std::move(Pros), false, 0, 0, 0, 0 };
	}

	LrHistory Process2(Lr0Table const& Table, LrSymbol const* TokenArray, size_t TokenLength)
	{
		std::vector<SearchStackElement> SearchStack;
		SearchStack.push_back(SearchStackElement{{0, {0}, 0}, true, 0, 0, 0, 0});
		while (!SearchStack.empty())
		{
			for (size_t index = 0; index < SearchStack.size(); ++index)
			{
				if (SearchStack[index].is_reduce)
				{
					ExpandProductions2(Table, index, SearchStack);
				}
			}
			volatile int i =0;
		}
		return {};
	}
	*/

	Lr0ProcessContent::Lr0ProcessContent(Lr0Table const& InputTable)
		: Table(InputTable)
	{
		Cores.push_back({ ++UsedBranch, 0, 0, 0, 1});
		States.push_back(0);
		ExpandSearchCore(false);
	}

	void Lr0ProcessContent::InsertTerminalSymbol(LrSymbol InputSymbol)
	{
		assert(InputSymbol.IsTerminal());
		for (auto& Ite : Cores)
		{
			Ite.InputSymbol = InputSymbol;
		}
		ExpandSearchCore(false);
		++TokenIndex;
	}

	LrHistory Lr0ProcessContent::EndOfSymbolStream()
	{
		for (auto& Ite : Cores)
		{
			Ite.InputSymbol = LrSymbol::EndOfFile();
		}
		ExpandSearchCore(true);
		return {};
	}

	void Lr0ProcessContent::ExpandSearchCore(bool IsFinalSymbol)
	{
		TemporaryStates.clear();
		TemporaryCores.clear();
		size_t StateOffset = 0;
		for (size_t I = Cores.size(); I > 0; --I)
		{
			SearchCore Cur = Cores[I-1];
			TemporaryStates.insert(TemporaryStates.end(), States.end() - StateOffset - Cur.StateCount, States.end() - StateOffset);
			TemporaryCores.push_back(Cur);
			StateOffset += Cur.StateCount;
		}
		Cores.clear();
		States.clear();
		StateOffset = 0;
		while (!TemporaryCores.empty())
		{
			bool KeepState = true;
			SearchCore Cur = *TemporaryCores.rbegin();
			TemporaryCores.pop_back();
			auto TempStart = TemporaryStates.end() - Cur.StateCount;
			States.insert(States.end(), TempStart, TemporaryStates.end());
			TemporaryStates.erase(TempStart, TemporaryStates.end());
			assert(States.size() > StateOffset);
			if (Cur.InputSymbol.has_value())
			{
				uint32_t LastState = *States.rbegin();
				assert(LastState < Table.nodes.size());
				auto Node = Table.nodes[LastState];
				assert(Table.shifts.size() >= Node.shift_adress + Node.shift_count);
				bool Find = false;
				for (size_t I2 = 0; I2 < Node.shift_count; ++I2)
				{
					auto ShiftNode = Table.shifts[I2 + Node.shift_adress];
					if (LrSymbol::MakeSymbol(ShiftNode.require_symbol) == *Cur.InputSymbol)
					{
						Find = true;
						States.push_back(ShiftNode.shift_state);
						++Cur.StateCount;
						if (Cur.InputSymbol->IsTerminal() && !Cur.InputSymbol->IsEndOfFile())
						{
							LrStep Ste;
							Ste.value = LrSymbol::MakeSymbol(ShiftNode.require_symbol);
							Ste.shift.token_index = TokenIndex;
							StepTuple Tup{ Ste, Cur.CurrentBranch, Cur.StepCount };
							Steps.push_back(Tup);
							++Cur.StepCount;
						}
						Cur.InputSymbol = {};
						break;
					}
				}
				if (Find)
				{
					size_t CurStateCount = 0;
					for (auto& Ite : Cores)
					{
						if (Ite.StateCount == Cur.StateCount)
						{
							bool Same = true;
							for (uint32_t Index = 0; Index < Ite.StateCount; ++Index)
							{
								if (States[Index + CurStateCount] != States[Index + StateOffset])
								{
									Same = false;
									break;
								}
							}
							if (Same)
							{
								Find = false;
								break;
							}
						}
						CurStateCount += Ite.StateCount;
					}
				}
				if (!Find)
				{
					RemoveStepFromTemporaryCores(Cur.CurrentBranch);
					States.resize(StateOffset);
					continue;
				}
			}
			uint32_t LastState = *States.rbegin();
			assert(Table.nodes.size() > LastState);
			auto Node = Table.nodes[LastState];
			assert(Node.reduce_adress + Node.reduce_count <= Table.reduces.size());
			for (size_t i = 0; i < Node.reduce_count; ++i)
			{
				auto production_index = Table.reduces[Node.reduce_adress + i].production_index;
				assert(production_index <= Table.productions.size());
				Lr0Table::Production production;
				if(production_index < Table.productions.size())
					production = Table.productions[production_index];
				else
					production = {LrSymbol::StartSymbol(), 1};
				auto Symbol = LrSymbol::MakeSymbol(production.value);
				assert(States.size() > production.production_count + StateOffset);
				SearchCore NewExpand{
					++UsedBranch, Cur.StepCount, Cur.CurrentBranch, Cur.StepCount, Cur.StateCount - production.production_count,
					Symbol
				};
				if (!Symbol.IsStartSymbol())
				{
					++NewExpand.StepCount;
					LrStep Step;
					Step.value = Symbol;
					Step.reduce.production_index = production_index;
					Step.reduce.production_count = production.production_count;
					Step.reduce.mask = production.mask;
					Steps.push_back({Step, NewExpand .CurrentBranch, NewExpand.StepCount});
				}
				TemporaryCores.push_back(NewExpand);
				TemporaryStates.insert(TemporaryStates.end(), States.begin() + StateOffset, States.end());
				TemporaryStates.resize(TemporaryStates.size() - production.production_count);
			}
			if (Node.shift_count != 0 || IsFinalSymbol)
			{
				Cores.push_back(Cur);
				StateOffset += Cur.StateCount;
			}
			else {
				RemoveStepFromTemporaryCores(Cur.CurrentBranch);
				States.resize(StateOffset);
			}
		}
	}

	LrHistory Lr0ProcessContent::GeneratorHistory(uint32_t RequireBranch)
	{

	}

	void Lr0ProcessContent::RemoveStepFromTemporaryCores(uint32_t RemoveBranch)
	{
		std::optional<uint32_t> OverwriteBranch;
		size_t MaxOverwriteStepCount = 0;
		for (auto Ite2 = TemporaryCores.rbegin(); Ite2 != TemporaryCores.rend(); ++Ite2)
		{
			if (Ite2->DependentedBranch == RemoveBranch)
			{
				if (OverwriteBranch.has_value())
					Ite2->DependentedBranch = *OverwriteBranch;
				else
				{
					OverwriteBranch = Ite2->CurrentBranch;
					Ite2->DependentedBranch = 0;
					Steps.erase(std::remove_if(Steps.begin(), Steps.end(), [=](StepTuple& Tuple) -> bool {
						if (Tuple.OwneredBranch == RemoveBranch)
						{
							if (Tuple.StepCount < Ite2->StepCount)
							{
								Tuple.OwneredBranch = Ite2->CurrentBranch;
							}
							else
								return true;
						}
						return false;
					}), Steps.end());
				}
			}
		}
		if (!OverwriteBranch.has_value())
		{
			Steps.erase(std::remove_if(Steps.begin(), Steps.end(), [&](StepTuple& Tuple) -> bool {
				if (Tuple.OwneredBranch == RemoveBranch)
				{
					return true;
				}
				return false;
			}), Steps.end());
		}
	}

	/*
	void Lr0ProcessContent::RemoveStep(uint32_t RemoveBranch, std::optional<uint32_t> OverwriteBranch, size_t MaxOverwriteStepCount)
	{
		Steps.erase(std::remove_if(Steps.begin(), Steps.end(), [&](StepTuple& Tuple) -> bool{
			if (Tuple.OwneredBranch == RemoveBranch)
			{
				if (OverwriteBranch.has_value() && Tuple.StepCount < MaxOverwriteStepCount)
				{
					Tuple.OwneredBranch = *OverwriteBranch;
				}else
					return true;
			}
			return false;
		}), Steps.end());
	}
	*/

	std::set<LrSymbol> CalNullableSet(const std::vector<LrProductionInput>& production)
	{
		std::set<LrSymbol> result;
		bool set_change = true;
		while (set_change)
		{
			set_change = false;
			for (auto& ite : production)
			{
				assert(ite.production.size() >= 1);
				if (ite.production.size() == 1)
				{
					set_change |= result.insert(ite.production[0]).second;
				}
				else {
					bool nullable_set = true;
					for (size_t index = 1; index < ite.production.size(); ++index)
					{
						LrSymbol symbol = ite.production[index];
						if (symbol.IsTerminal() || result.find(symbol) == result.end())
						{
							nullable_set = false;
							break;
						}
					}
					if (nullable_set)
						set_change |= result.insert(ite.production[0]).second;
				}
			}
		}
		return result;
	}

	std::map<LrSymbol, std::set<LrSymbol>> CalNoterminalFirstSet(
		const std::vector<LrProductionInput>& production
	)
	{
		auto nullable_set = CalNullableSet(production);
		std::map<LrSymbol, std::set<LrSymbol>> result;
		bool set_change = true;
		while (set_change)
		{
			set_change = false;
			for (auto& ite : production)
			{
				assert(ite.production.size() >= 1);
				for (size_t index = 1; index < ite.production.size(); ++index)
				{
					auto head = ite.production[0];
					auto target = ite.production[index];
					if (target.IsTerminal())
					{
						set_change |= result[head].insert(target).second;
						break;
					}
					else {
						if (nullable_set.find(target) == nullable_set.end())
						{
							auto& ref = result[head];
							auto find = result.find(target);
							if (find != result.end())
								for (auto& ite3 : find->second)
									set_change |= ref.insert(ite3).second;
							break;
						}
					}
				}
			}
		}
		return result;
	}

	struct ProductionIndex
	{
		size_t index;
		size_t element_index;
		bool operator<(const ProductionIndex& pe) const
		{
			return index < pe.index || (index == pe.index && element_index < pe.element_index);
		}
		bool operator==(const ProductionIndex& pe) const
		{
			return index == pe.index && element_index == pe.element_index;
		}
	};

	std::set<ProductionIndex> ExpandProductionIndexSet(std::set<ProductionIndex> const& Pro, std::vector<LrProductionInput> const& Productions, std::vector<LrSymbol> const& AddProduction)
	{
		std::set<ProductionIndex> Result;
		std::vector<ProductionIndex> SearchStack(Pro.begin(), Pro.end());
		while (!SearchStack.empty())
		{
			auto Ptr = *SearchStack.rbegin();
			SearchStack.pop_back();
			auto Re = Result.insert(Ptr);
			if (Re.second)
			{
				std::vector<LrSymbol> const* Target;
				if (Ptr.index < Productions.size())
					Target = &(Productions[Ptr.index].production);
				else
					Target = &AddProduction;
				size_t SymbolIndex = Ptr.element_index;
				if (SymbolIndex< Target->size())
				{
					auto TargetSymbol = (*Target)[SymbolIndex];
					if (TargetSymbol.IsNoTerminal())
					{
						bool Finded = false;
						for (size_t i = 0; i < Productions.size(); ++i)
						{
							if (Productions[i].production[0] == TargetSymbol)
							{
								Finded = true;
								SearchStack.push_back({ i, 1 });
							}
						}
						if (AddProduction[0] == TargetSymbol) {
							Finded = true;
							SearchStack.push_back({ Productions.size(), 1 });
						}
						if (!Finded)
							throw Exception::MakeExceptionTuple(Exception::LrNoterminalUndefined{ TargetSymbol });
					}
				}
			}
		}
		return Result;
	}

	std::map<LrSymbol, std::set<LrSymbol>> TranslateOperatorPriority(
		const std::vector<LrOpePriority>& priority
	)
	{
		std::map<LrSymbol, std::set<LrSymbol>> ope_priority;
		for (size_t index = 0; index < priority.size(); ++index)
		{
			auto& [opes, left] = priority[index];
			for (auto ite : opes)
			{
				auto Inserted = ope_priority.insert({ ite, {} });
				if (Inserted.second)
				{
					if (left == LrAssociativity::Left)
					{
						for (auto& ite2 : opes)
							Inserted.first->second.insert(ite2);
					}
					for (size_t index2 = index + 1; index2 < priority.size(); ++index2)
					{
						auto& [opes2, left2] = priority[index2];
						for (auto ite2 : opes2)
							Inserted.first->second.insert(ite2);
					}
				}else
					throw Exception::MakeExceptionTuple(Exception::LrOperatorPriorityConflict{ ite, ite });
			}
		}
		return ope_priority;
	}

	Lr0Table CreateLr0Table(LrSymbol start_symbol, std::vector<LrProductionInput> const& production, std::vector<LrOpePriority> const& priority)
	{
		Lr0Table result;

		auto OperatorPriority = TranslateOperatorPriority(priority);

		for (size_t index = 0; index < production.size(); ++index)
		{
			auto& CurPro = production[index];
			Lr0Table::Production P{ CurPro.production[0], CurPro.production.size() - 1 , CurPro.function_mask };
			if (CurPro.production.size() >= 3)
			{
				auto Sym = CurPro.production[CurPro.production.size() - 2];
				if (Sym.IsTerminal())
				{
					auto FindIte = OperatorPriority.find({ Sym });
					if (FindIte != OperatorPriority.end())
					{
						for (auto& ite2 : FindIte->second)
							result.force_reduce[ite2.Value()].insert(index);
					}
				}
			}
			result.productions.push_back(std::move(P));
		}

		std::vector<LrSymbol> AppendProduction = { LrSymbol::StartSymbol(), start_symbol, LrSymbol::EndOfFile()};

		std::set<ProductionIndex> PP({ ProductionIndex{ production.size(), 1} });
		PP = ExpandProductionIndexSet(PP, production, AppendProduction);
		std::map<std::set<ProductionIndex>, size_t> StateMapping;
		auto First = StateMapping.insert({ PP, 0 });
		std::vector<decltype(StateMapping)::const_iterator> StateMappingVec({ First.first });
		for (size_t i = 0; i < StateMapping.size(); ++i)
		{
			auto Cur = StateMappingVec[i];
			std::map<LrSymbol, std::set<ProductionIndex>> ShiftMapping;
			std::set<size_t> ReduceState;
			for (auto& Ite : Cur->first)
			{
				std::vector<LrSymbol> const* Target;
				if (Ite.index < production.size())
					Target = &(production[Ite.index].production);
				else
					Target = &AppendProduction;
				if (Ite.element_index < Target->size())
				{
					auto Re = ShiftMapping.insert({ (*Target)[Ite.element_index], {{Ite.index, Ite.element_index + 1}} });
					if (!Re.second)
						Re.first->second.insert({ Ite.index, Ite.element_index + 1 });
				}
				else {
					ReduceState.insert(Ite.index);
				}
			}
			std::vector<Lr0Table::Shift> Shiftting;
			for (auto& ite : ShiftMapping)
			{
				auto ExpanedSet = ExpandProductionIndexSet(ite.second, production, AppendProduction);
				auto Inserted = StateMapping.insert({ std::move(ExpanedSet), StateMapping.size() });
				if(Inserted.second)
					StateMappingVec.push_back(Inserted.first);
				Shiftting.push_back({ ite.first, Inserted.first->second });
			}
			std::vector<Lr0Table::Reduce> Reduceing;
			for (auto& ite : ReduceState)
				Reduceing.push_back(Lr0Table::Reduce{ ite });
			result.nodes.push_back({result.reduces.size(), Reduceing.size(), result.shifts.size(),  Shiftting.size()});
			result.reduces.insert(result.reduces.end(), Reduceing.begin(), Reduceing.end());
			result.shifts.insert(result.shifts.end(), Shiftting.begin(), Shiftting.end());
		}
		return result;
	}
}

namespace PineApple::StrFormat
{
	/*
	template<typename Type, typename T1> struct Formatter<std::vector<Type, T1>>
	{
		std::u32string operator()(std::u32string_view, std::vector<Type, T1> const& RS)
		{
			static auto pat = CreatePatternRef(U"{}, ");
			std::u32string Result;
			Result += U'{';
			for (auto& ite : RS)
				Result += Process(pat, ite);
			Result += U'}';
			return std::move(Result);
		}
	};

	template<typename Type, typename T1, typename T2, typename T3> struct Formatter<std::map<Type, T1, T2, T3>>
	{
		std::u32string operator()(std::u32string_view, std::map<Type, T1, T2, T3> const& RS)
		{
			static auto pat = CreatePatternRef(U"{{{}, {}}}, ");
			std::u32string Result;
			Result += U'{';
			for (auto& [i1, i2] : RS)
				Result += Process(pat, i1, i2);
			Result += U'}';
			return std::move(Result);
		}
	};

	template<typename Type, typename T1, typename T2> struct Formatter<std::set<Type, T1, T2>>
	{
		std::u32string operator()(std::u32string_view, std::set<Type, T1, T2> const& RS)
		{
			static auto pat = CreatePatternRef(U"{}, ");
			std::u32string Result;
			Result += U'{';
			for (auto& ite : RS)
				Result += Process(pat, ite);
			Result += U'}';
			return std::move(Result);
		}
	};

	template<typename Type, typename T1> struct Formatter<std::set<Type, T1>>
	{
		std::u32string operator()(std::u32string_view, std::set<Type, T1> const& RS)
		{
			static auto pat = CreatePatternRef(U"{}, ");
			std::u32string Result;
			Result += U'{';
			for (auto& ite : RS)
				Result += Process(pat, ite);
			Result += U'}';
			return std::move(Result);
		}
	};

	template<> struct Formatter<Lr0::Table::Production>
	{
		std::u32string operator()(std::u32string_view, Lr0::Table::Production const& RS)
		{
			static auto pat1 = CreatePatternRef(U"{{{}, {} }}");
			static auto pat2 = CreatePatternRef(U"{{{}, {}, 0x{-hex}}}");
			if(RS.mask != Lr0::ProductionInput::default_mask())
				return Process(pat2, RS.value, RS.production_count, RS.mask);
			else
				return Process(pat1, RS.value, RS.production_count);
		}
	};

	template<> struct Formatter<Lr0::Table::Reduce>
	{
		std::u32string operator()(std::u32string_view, Lr0::Table::Reduce const& RS)
		{
			static auto pat = CreatePatternRef(U"{{{}}}");
			return Process(pat, RS.production_index);
		}
	};

	template<> struct Formatter<Lr0::Table::Shift>
	{
		std::u32string operator()(std::u32string_view, Lr0::Table::Shift const& RS)
		{
			static auto pat = CreatePatternRef(U"{{{}, {} }}");
			return Process(pat, RS.require_symbol, RS.shift_state);
		}
	};

	template<> struct Formatter<Lr0::Table::Node>
	{
		std::u32string operator()(std::u32string_view, Lr0::Table::Node const& RS)
		{
			static auto pat = CreatePatternRef(U"{{{}, {}, {}, {}}}");
			return Process(pat, RS.reduce_adress, RS.reduce_count, RS.shift_adress, RS.shift_count);
		}
	};

	std::u32string Formatter<Lr0::Table>::operator()(std::u32string_view, Lr0::Table const& tab)
	{
		static auto pat = CreatePatternRef(U"{{{}, {}, {}, {}, {}}}");
		return Process(pat, tab.productions, tab.reduces, tab.shifts, tab.nodes, tab.force_reduce);
	}
	*/
}