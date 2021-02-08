#include "../Public/Lr0.h"
#include <optional>
namespace Potato::Lr0
{
	std::any History::operator()(std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFun) const
	{
		if(NTFunc && TFun)
		{
			std::vector<std::tuple<Symbol, std::any>> DataBuffer;
			for (auto& ite : steps)
			{
				if(ite.IsTerminal())
				{
					TElement ele{ite};
					if(TFun)
					{
						auto Result = TFun(ele);
						DataBuffer.push_back({ite.value, std::move(Result)});
					}
				}else {
					NTElement ele{ite};
					assert(DataBuffer.size() >= ite.reduce.production_count);
					size_t CurrentAdress = DataBuffer.size() - ite.reduce.production_count;
					ele.datas = DataBuffer.data() + CurrentAdress;
					auto Result = NTFunc(ele);
					DataBuffer.resize(CurrentAdress);
					DataBuffer.push_back({ ite.value, std::move(Result) });
				}
			}
			assert(DataBuffer.size() == 1);
			return std::move(std::get<1>(DataBuffer[0]));
		}
		return {};
	}

	struct SearchElement
	{
		size_t StepsRecord;
		std::vector<size_t> State;
		size_t TokenIndex;
	};

	bool HandleInputToken(Table const& Table, std::vector<Step>& Steps, SearchElement& SE, Symbol Sym, size_t token_index)
	{

		auto CurState = *SE.State.rbegin();
		auto Nodes = Table.nodes[CurState];
		for (size_t i = 0; i < Nodes.shift_count; ++i)
		{
			auto TarShift = Table.shifts[i + Nodes.shift_adress];
			if (TarShift.require_symbol == Sym)
			{
				SE.State.push_back(TarShift.shift_state);
				if (Sym.IsTerminal() && !Sym.IsEndOfFile())
				{
					Step ShiftStep;
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
		SymbolStorageT sym;
	};

	void ExpandProductions(Table const& Table, SearchElement&& Pros, std::vector<SearchStackElement>& SearchStack, SymbolStorageT sym, std::map<SymbolStorageT, std::set<size_t>> const& force_reduce)
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

	Symbol GetSymbol(size_t index, Symbol const* TokenArray, size_t TokenLength)
	{
		Symbol Sym;
		if (index < TokenLength)
			return TokenArray[index];
		else if (index == TokenLength)
			Sym = Symbol::EndOfFile();
		else
			assert(false);
		return Symbol::EndOfFile();
	}

	History Process(Table const& Table, Symbol const* TokenArray, size_t TokenLength)
	{

		size_t MaxTokenUsed = 0;
		std::vector<Step> BackupSteps;

		std::vector<Step> Steps;
		std::vector<SearchStackElement> SearchStack;
		ExpandProductions(Table, { 0, {0}, 0 }, SearchStack, GetSymbol(0, TokenArray, TokenLength), Table.force_reduce);
		while (!SearchStack.empty())
		{
			SearchStackElement Cur = std::move(*SearchStack.rbegin());
			SearchStack.pop_back();
			Steps.resize(Cur.search.StepsRecord);
			if (Cur.is_reduce)
			{
				Step ReduceStep;
				ReduceStep.value = Symbol::MakeSymbol(Cur.sym);
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
				Symbol Sym = GetSymbol(Cur.search.TokenIndex, TokenArray, TokenLength);
				bool re = HandleInputToken(Table, Steps, Cur.search, Sym, Cur.search.TokenIndex);
				if (re) {
					if (Sym == Symbol::EndOfFile())
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
		throw MakeException(Exception::UnaccableSymbol{ MaxTokenUsed, MaxTokenUsed < TokenLength ? TokenArray[MaxTokenUsed] : Symbol::EndOfFile(), std::move(BackupSteps) });
	}

	std::set<Symbol> CalNullableSet(const std::vector<ProductionInput>& production)
	{
		std::set<Symbol> result;
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
						Symbol symbol = ite.production[index];
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

	std::map<Symbol, std::set<Symbol>> CalNoterminalFirstSet(
		const std::vector<ProductionInput>& production
	)
	{
		auto nullable_set = CalNullableSet(production);
		std::map<Symbol, std::set<Symbol>> result;
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

	std::set<ProductionIndex> ExpandProductionIndexSet(std::set<ProductionIndex> const& Pro, std::vector<ProductionInput> const& Productions, std::vector<Symbol> const& AddProduction)
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
				std::vector<Symbol> const* Target;
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
							throw MakeException(Exception::NoterminalUndefined{ TargetSymbol });
					}
				}
			}
		}
		return Result;
	}

	std::map<Symbol, std::set<Symbol>> TranslateOperatorPriority(
		const std::vector<OpePriority>& priority
	)
	{
		std::map<Symbol, std::set<Symbol>> ope_priority;
		for (size_t index = 0; index < priority.size(); ++index)
		{
			auto& [opes, left] = priority[index];
			for (auto ite : opes)
			{
				auto Inserted = ope_priority.insert({ ite, {} });
				if (Inserted.second)
				{
					if (left == Associativity::Left)
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
					throw MakeException(Exception::OperatorPriorityConflict{ ite, ite });
			}
		}
		return std::move(ope_priority);
	}

	Table CreateTable(Symbol start_symbol, std::vector<ProductionInput> const& production, std::vector<OpePriority> const& priority)
	{
		Table result;

		auto OperatorPriority = TranslateOperatorPriority(priority);

		for (size_t index = 0; index < production.size(); ++index)
		{
			auto& CurPro = production[index];
			Table::Production P{ CurPro.production[0], CurPro.production.size() - 1 , CurPro.function_mask };
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

		std::vector<Symbol> AppendProduction = {Symbol::StartSymbol(), start_symbol, Symbol::EndOfFile()};

		std::set<ProductionIndex> PP({ ProductionIndex{ production.size(), 1} });
		PP = ExpandProductionIndexSet(PP, production, AppendProduction);
		std::map<std::set<ProductionIndex>, size_t> StateMapping;
		auto First = StateMapping.insert({ PP, 0 });
		std::vector<decltype(StateMapping)::const_iterator> StateMappingVec({ First.first });
		for (size_t i = 0; i < StateMapping.size(); ++i)
		{
			auto Cur = StateMappingVec[i];
			std::map<Symbol, std::set<ProductionIndex>> ShiftMapping;
			std::set<size_t> ReduceState;
			for (auto& Ite : Cur->first)
			{
				std::vector<Symbol> const* Target;
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
			std::vector<Table::Shift> Shiftting;
			for (auto& ite : ShiftMapping)
			{
				auto ExpanedSet = ExpandProductionIndexSet(ite.second, production, AppendProduction);
				auto Inserted = StateMapping.insert({ std::move(ExpanedSet), StateMapping.size() });
				if(Inserted.second)
					StateMappingVec.push_back(Inserted.first);
				Shiftting.push_back({ ite.first, Inserted.first->second });
			}
			std::vector<Table::Reduce> Reduceing;
			for (auto& ite : ReduceState)
				Reduceing.push_back(Table::Reduce{ ite });
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