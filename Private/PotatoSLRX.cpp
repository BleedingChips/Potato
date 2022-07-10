#include "../Public/PotatoSLRX.h"
#include <algorithm>
#include <cassert>
#include <numbers>
#include <set>
namespace Potato::SLRX
{

	using namespace Exception;

	TElement::TElement(ParsingStep const& value) : Value(value.Value), TokenIndex(value.Shift.TokenIndex) {}

	NTElement::NTElement(ParsingStep const& StepValue, std::size_t FirstTokenIndex, DataT* DataPtr) :
		Value(StepValue.Value), ProductionIndex(StepValue.Reduce.ProductionIndex), Mask(StepValue.Reduce.Mask), 
		FirstTokenIndex(FirstTokenIndex), Datas(DataPtr, StepValue.Reduce.ProductionCount)
	{}

	bool ParsingStepProcessor::Consume(ParsingStep Input)
	{
		if (Input.IsTerminal())
		{
			TElement Ele{ Input };
			auto Result = ExecuteFunction(VariantElement{ std::move(Ele) });
			DataBuffer.push_back({ Ele.Value, Ele.TokenIndex, std::move(Result) });
		}
		else {
			if (DataBuffer.size() >= Input.Reduce.ProductionCount)
			{
				std::size_t CurrentAdress = DataBuffer.size() - Input.Reduce.ProductionCount;
				std::size_t TokenFirstIndex = 0;
				if (CurrentAdress < DataBuffer.size())
				{
					TokenFirstIndex = DataBuffer[CurrentAdress].FirstTokenIndex;
				}
				else if (CurrentAdress >= 1)
				{
					TokenFirstIndex = DataBuffer[CurrentAdress - 1].FirstTokenIndex;
				}
				NTElement ele{ Input, TokenFirstIndex, DataBuffer.data() + CurrentAdress };
				auto Result = ExecuteFunction(VariantElement{ std::move(ele) });
				DataBuffer.resize(CurrentAdress);
				DataBuffer.push_back({ ele.Value, TokenFirstIndex, std::move(Result) });
			}
			else {
				return false;
			}
		}
		return true;
	}

	std::optional<std::any> ParsingStepProcessor::EndOfFile()
	{
		if (DataBuffer.size() == 1)
		{
			auto Result = DataBuffer[0].Consume();
			DataBuffer.clear();
			return Result;
		}
		else
			return {};
	}

	ProductionInfo::ProductionInfo(Symbol StartSymbol, std::vector<ProductionBuilder> ProductionBuilders, std::vector<OpePriority> Priority)
	{
		
		std::vector<Symbol> NeedTrackProductionValue;

		{
			Production Desc;
			Desc.Symbol = Symbol::StartSymbol();
			{
				Element StartUpElement{ StartSymbol };
				Desc.Elements.push_back(std::move(StartUpElement));
				Element EOFElement{ Symbol::EndOfFile() };
				Desc.Elements.push_back(std::move(EOFElement));
			}
			Desc.ProductionMask = 0;
			ProductionDescs.push_back(std::move(Desc));
		}


		for (auto& Ite : ProductionBuilders)
		{
			if (Ite.ProductionValue.IsTerminal())
			{
				throw Exception::WrongProduction{ Exception::WrongProduction::Category::TerminalSymbolAsProductionBegin, std::move(Ite) };
			}
			Production Desc;
			Desc.Symbol = Ite.ProductionValue;
			Desc.ProductionMask = Ite.ProductionMask;
			std::size_t ElementIndex = 0;
			for (auto& Ite2 : Ite.Element)
			{
				if (Ite2.Type == ProductionBuilderElement::TypeT::IsMask || Ite2.Type == ProductionBuilderElement::TypeT::ItSelf)
				{
					if (Desc.Elements.size() == 0 || Desc.Elements.rbegin()->Symbol.IsTerminal())
					{
						throw Exception::WrongProduction{ Exception::WrongProduction::Category::MaskNotFolloedNoterminalSymbol, std::move(Ite) };
					}
					if (Ite2.Type == ProductionBuilderElement::TypeT::IsMask)
					{
						for (std::size_t Index2 = 0; Index2 < ProductionBuilders.size(); ++Index2)
						{
							auto& Ref = ProductionBuilders[Index2];
							if (Ref.ProductionMask == Ite2.ProductionMask)
							{
								Desc.Elements.rbegin()->AcceptableProductionIndex.push_back(Index2 + 1);
								NeedTrackProductionValue.push_back(Desc.Elements.rbegin()->Symbol);
							}
						}
					}
					else {
						Desc.Elements.rbegin()->AcceptableProductionIndex.push_back(ProductionDescs.size());
						NeedTrackProductionValue.push_back(Desc.Symbol);
					}
				}
				else {
					Desc.Elements.push_back({Ite2.ProductionValue, {}});
				}
				++ElementIndex;
			}
			ProductionDescs.push_back(std::move(Desc));
		}

		{
			struct OpePriorityTemp
			{
				Symbol Ope;
				Associativity Asso;
				std::size_t ProductionIndex;
				Symbol ProductionValue;
				std::size_t PriorityLevel;
				std::optional<std::size_t> LeftExpIndex;
				std::optional<std::size_t> RightExpIndex;
			};

			std::vector<OpePriorityTemp> PriorityTemp;
			for (std::size_t Level = 0; Level < Priority.size(); ++Level)
			{
				auto& Ite = Priority[Level];
				for (auto& Ite2 : Ite.Symbols)
				{
					for (auto& Ite3 : PriorityTemp)
					{
						if (Ite3.Ope == Ite2)
						{
							throw Exception::OpePriorityConflict{ Ite3.Ope};
						}
					}
					for (std::size_t Index = 0; Index < ProductionDescs.size(); ++Index)
					{
						auto& Ite3 = ProductionDescs[Index];
						std::optional<OpePriorityTemp> Temp;
						if (Ite3.Elements.size() == 2)
						{
							auto First = Ite3.Elements[0].Symbol;
							auto Second = Ite3.Elements[1].Symbol;
							if (Ite3.Symbol == First && Ite2 == Second)
								Temp = { Ite2, Ite.Priority, Index, Ite3.Symbol, Level, {0}, {} };
							else if (Ite3.Symbol == Second && Ite2 == First)
								Temp = { Ite2, Ite.Priority, Index, Ite3.Symbol, Level, {}, {1} };
						}
						else if (Ite3.Elements.size() == 3)
						{
							auto First = Ite3.Elements[0].Symbol;
							auto Second = Ite3.Elements[1].Symbol;
							auto Third = Ite3.Elements[2].Symbol;
							if (Ite3.Symbol == First && Ite3.Symbol == Third && Ite2 == Second)
								Temp = { Ite2, Ite.Priority, Index, Ite3.Symbol, Level, {0}, {2} };
						}
						
						if (Temp.has_value())
						{
							NeedTrackProductionValue.push_back(Temp->ProductionValue);
							PriorityTemp.push_back(*Temp);
							for (auto& Ite4 : PriorityTemp)
							{
								if (Ite4.ProductionValue == Temp->ProductionValue)
								{
									if (Ite4.PriorityLevel < Temp->PriorityLevel)
									{
										if (Ite4.RightExpIndex.has_value())
										{
											ProductionDescs[Ite4.ProductionIndex].Elements[*Ite4.RightExpIndex].AcceptableProductionIndex.push_back(Index);
										}
										if (Ite4.LeftExpIndex.has_value())
										{
											ProductionDescs[Ite4.ProductionIndex].Elements[*Ite4.LeftExpIndex].AcceptableProductionIndex.push_back(Index);
										}
									}
									else {
										switch (Temp->Asso)
										{
										case Associativity::Left:
										{
											if (Temp->RightExpIndex.has_value())
											{
												ProductionDescs[Index].Elements[*Temp->RightExpIndex].AcceptableProductionIndex.push_back(Ite4.ProductionIndex);
											}
											if (Ite4.Ope != Temp->Ope)
											{
												if (Ite4.RightExpIndex.has_value())
												{
													ProductionDescs[Ite4.ProductionIndex].Elements[*Ite4.RightExpIndex].AcceptableProductionIndex.push_back(Temp->ProductionIndex);
												}
											}
											break;
										}
										case Associativity::Right:
										{
											if (Temp->LeftExpIndex.has_value())
											{
												ProductionDescs[Index].Elements[*Temp->LeftExpIndex].AcceptableProductionIndex.push_back(Ite4.ProductionIndex);
											}
											if (Ite4.Ope != Temp->Ope)
											{
												if (Ite4.LeftExpIndex.has_value())
												{
													ProductionDescs[Ite4.ProductionIndex].Elements[*Ite4.LeftExpIndex].AcceptableProductionIndex.push_back(Temp->ProductionIndex);
												}
											}
											break;
										}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		for (auto& Ite : NeedTrackProductionValue)
		{
			auto FindIte = TrackedAcceptableNoTerminalSymbol.find(Ite);
			if (FindIte == TrackedAcceptableNoTerminalSymbol.end())
			{
				FindIte = TrackedAcceptableNoTerminalSymbol.insert({Ite, {}}).first;
				assert(FindIte != TrackedAcceptableNoTerminalSymbol.end());
				for (std::size_t Index = 0; Index < ProductionDescs.size(); ++Index)
				{
					if (ProductionDescs[Index].Symbol == Ite)
					{
						FindIte->second.push_back(Index);
					}
				}
			}
		}

		{
			std::vector<std::size_t> TempVector;
			for (auto& Ite : ProductionDescs)
			{
				for (auto& Ite2 : Ite.Elements)
				{
					auto Find = TrackedAcceptableNoTerminalSymbol.find(Ite2.Symbol);
					if (Find != TrackedAcceptableNoTerminalSymbol.end())
					{
						TempVector = Find->second;
						for (auto& Ite3 : Ite2.AcceptableProductionIndex)
						{
							TempVector.erase(std::remove_if(
								TempVector.begin(),
								TempVector.end(),
								[Ite3](std::size_t Input){ return Ite3 == Input; }
							),  TempVector.end());
						}
						Ite2.AcceptableProductionIndex = TempVector;
					}
				}
			}
		}
	}

	std::optional<Symbol> ProductionInfo::GetElementSymbol(std::size_t ProductionIndex, std::size_t ElementIndex) const
	{
		if (ProductionIndex < ProductionDescs.size())
		{
			auto& ProRef = ProductionDescs[ProductionIndex];
			if (ElementIndex < ProRef.Elements.size())
			{
				return ProRef.Elements[ElementIndex].Symbol;
			}
		}
		return {};
	}

	std::span<std::size_t const> ProductionInfo::GetAllProductionIndexs(Symbol Input) const
	{
		auto Find = TrackedAcceptableNoTerminalSymbol.find(Input);
		if(Find != TrackedAcceptableNoTerminalSymbol.end())
			return Find->second;
		return {};
	}

	std::span<std::size_t const> ProductionInfo::GetAcceptableProductionIndexs(std::size_t ProductionIndex, std::size_t ElementIndex) const
	{
		if (ProductionIndex < ProductionDescs.size())
		{
			auto& ProRef = ProductionDescs[ProductionIndex];
			if (ElementIndex < ProRef.Elements.size())
			{
				return ProRef.Elements[ElementIndex].AcceptableProductionIndex;
			}
		}
		return {};
	}

	bool ProductionInfo::IsAcceptableProductionIndex(std::size_t ProductionIndex, std::size_t ElementIndex, std::size_t RequireIndex) const
	{
		auto Result = GetAcceptableProductionIndexs(ProductionIndex, ElementIndex);
		if (Result.size() == 0 || std::find(Result.begin(), Result.end(), RequireIndex) != Result.end())
			return true;
		return false;
	}

	auto ProductionInfo::GetStartupSearchElements() const -> SearchElement {
		return {0, 0};
	}

	auto ProductionInfo::GetEndSearchElements() const -> SearchElement {
		return { 0, 2 };
	}

	void ProductionInfo::ExpandSearchElements(std::vector<SearchElement>& InoutElements) const
	{
		for (std::size_t Index = 0; Index < InoutElements.size(); ++Index)
		{
			auto CurrentSearch = InoutElements[Index];
			auto& Pro = ProductionDescs[CurrentSearch.ProductionIndex];
			auto CurElements = std::span(Pro.Elements);
			if (CurrentSearch.ElementIndex < CurElements.size())
			{
				auto CurSymbol = CurElements[CurrentSearch.ElementIndex].Symbol;
				if (CurSymbol.IsNoTerminal())
				{
					for (std::size_t PIndex = 0; PIndex < ProductionDescs.size(); ++PIndex)
					{
						auto& CurDetectedProduction = ProductionDescs[PIndex];
						if (CurSymbol == CurDetectedProduction.Symbol)
						{
							SearchElement SE{ PIndex , 0};
							auto FindIte = std::find_if(InoutElements.begin(), InoutElements.end(), [=](SearchElement const& Cur) {
								return SE <=> Cur == std::strong_ordering::equivalent;
								});
							if (FindIte == InoutElements.end())
							{
								if (IsAcceptableProductionIndex(CurrentSearch.ProductionIndex, CurrentSearch.ElementIndex, PIndex))
									InoutElements.push_back(SE);
							}
						}
					}
				}
			}
		}
		std::sort(InoutElements.begin(), InoutElements.end());
	}

	std::tuple<std::size_t, std::size_t> ProductionInfo::IntersectionSet(std::span<std::size_t> Source1AndOutout, std::span<std::size_t> Source2)
	{
		if(Source1AndOutout.empty() || Source2.empty())
			return { static_cast<std::size_t>(Source1AndOutout .size()), static_cast<std::size_t>(Source2.size()) };
		auto EndIte1 = Source1AndOutout.end();
		auto EndIte2 = Source2.end();
		for (auto Ite1 = Source1AndOutout.begin(); Ite1 != EndIte1;)
		{
			auto FindIte = std::find(Source2.begin(), EndIte2, *Ite1);
			if (FindIte != EndIte2)
			{
				EndIte1 -= 1;
				EndIte2 -= 1;
				if (Ite1 != EndIte1)
					std::swap(*Ite1, *(EndIte1));
				if(FindIte != EndIte2)
					std::swap(*FindIte, *(EndIte2));
			}
			else {
				++Ite1;
			}
		}
		return {static_cast<std::size_t>(std::distance(Source1AndOutout.begin(), EndIte1)), static_cast<std::size_t>(std::distance(Source2.begin(), EndIte2))};
	}

	std::vector<std::size_t> ForwardDetect(std::vector<std::vector<std::size_t>> const& ReverseRef, std::size_t Startup, std::size_t ReduceCount)
	{
		std::vector<std::size_t> Temporary1;
		std::vector<std::size_t> Temporary2;
		Temporary1.push_back(Startup);
		while (ReduceCount > 0)
		{
			for (auto& Ite : Temporary1)
			{
				auto& Ref = ReverseRef[Ite];
				Temporary2.insert(Temporary2.end(), Ref.begin(), Ref.end());
			}
			std::swap(Temporary2, Temporary1);
			Temporary2.clear();
			--ReduceCount;
		}
		return Temporary1;
	}



	LR0Table::LR0Table(ProductionInfo Infos)
	{
		std::vector<ProductionInfo::SearchElement> Status;
		std::vector<Misc::IndexSpan<>> PreNode;

		Status.push_back(Infos.GetStartupSearchElements());
		Infos.ExpandSearchElements(Status);
		PreNode.push_back({ Status.size(), 1 });
		PreNode.push_back({ 0, Status.size() });
		Status.push_back(Infos.GetEndSearchElements());

		struct SearchProperty
		{
			std::optional<Symbol> ShiftSymbol;
			std::span<std::size_t const> AcceptableProductionIndex;
		};

		struct ShiftMapping
		{
			Symbol Symbol;
			std::vector<ProductionInfo::SearchElement> Status;
			Misc::IndexSpan<> AcceptableProductionIndex;
			bool ReserveStorage = false;
		};

		std::vector<SearchProperty> SearchPropertyTemporary;
		std::vector<std::size_t> AccepatbelProductionIndexs;
		std::vector<ShiftMapping> Mappings;

		for (std::size_t SearchIte = 0; SearchIte < PreNode.size(); ++SearchIte)
		{
			auto CurrentState = PreNode[SearchIte].Slice(Status);
			SearchPropertyTemporary.clear();
			AccepatbelProductionIndexs.clear();
			Mappings.clear();
			SearchPropertyTemporary.reserve(CurrentState.size());
			for (auto& Ite : CurrentState)
			{
				SearchProperty Pro{ Infos.GetElementSymbol(Ite.ProductionIndex, Ite.ElementIndex), Infos.GetAcceptableProductionIndexs(Ite.ProductionIndex, Ite.ElementIndex) };
				SearchPropertyTemporary.push_back(Pro);
			}
			Node TemporaryNode;
			for (std::size_t Index = 0; Index < SearchPropertyTemporary.size(); ++Index)
			{
				auto CurSearch = SearchPropertyTemporary[Index];
				if (CurSearch.ShiftSymbol.has_value())
				{
					if (CurSearch.AcceptableProductionIndex.empty())
					{
						auto FindIte = std::find_if(Mappings.begin(), Mappings.end(), [=](ShiftMapping const& Map) {
							return Map.Symbol <=> *CurSearch.ShiftSymbol == std::strong_ordering::equal;
							});
						if (FindIte != Mappings.end())
							FindIte->Status.push_back(CurrentState[Index]);
						else {
							ShiftMapping sm{ *CurSearch.ShiftSymbol, {CurrentState[Index]}, {AccepatbelProductionIndexs.size(), 0}, false };
							Mappings.push_back(std::move(sm));
						}
					}
					else {
						ShiftMapping Cur{ *CurSearch.ShiftSymbol, {CurrentState[Index]}, {AccepatbelProductionIndexs.size(), CurSearch.AcceptableProductionIndex.size() }, false };
						AccepatbelProductionIndexs.insert(AccepatbelProductionIndexs.end(), CurSearch.AcceptableProductionIndex.begin(), CurSearch.AcceptableProductionIndex.end());
						for (std::size_t Index2 = 0; Index2 < Mappings.size(); ++Index2)
						{
							auto& Ite = Mappings[Index2];
							if (Ite.Symbol == Cur.Symbol)
							{
								auto S1 = Ite.AcceptableProductionIndex.Slice(AccepatbelProductionIndexs);
								auto S2 = Cur.AcceptableProductionIndex.Slice(AccepatbelProductionIndexs);
								auto [S1s, S2s] = ProductionInfo::IntersectionSet(S1, S2);
								if (S1s == 0)
									//CFH::InsertSortedUniquedVectorElement(Status, CurrentState[Index]);
									Ite.Status.push_back(CurrentState[Index]);
								else if (S1.size() != S1s) {
									auto New = Ite;
									Ite.AcceptableProductionIndex.Offset += S1s;
									Ite.AcceptableProductionIndex.Length -= S1s;
									New.AcceptableProductionIndex.Length = S1s;
									Ite.Status.push_back(CurrentState[Index]);
									Mappings.insert(Mappings.begin() + Index2, std::move(New));
									++Index2;
								}
								Cur.AcceptableProductionIndex.Length = S2s;
								AccepatbelProductionIndexs.resize(Cur.AcceptableProductionIndex.End());
								if (S2s == 0)
									break;
							}
						}
						if (Cur.AcceptableProductionIndex.Length != 0)
							Mappings.push_back(std::move(Cur));
					}
				}
			}

			for (auto Ite = Mappings.begin(); Ite != Mappings.end(); ++Ite)
			{
				for (auto& Ite2 : Ite->Status)
					Ite2.ElementIndex += 1;
				Infos.ExpandSearchElements(Ite->Status);
				if (Ite->AcceptableProductionIndex.Count() != 0)
				{
					bool Find = false;
					for (auto Ite2 = Mappings.begin(); Ite2 != Mappings.end(); ++Ite2)
					{
						if (Ite2 != Ite && Ite2->Symbol == Ite->Symbol)
						{
							Find = true;
							break;
						}
					}
					if (!Find)
						Ite->AcceptableProductionIndex.Length = 0;
				}
			}

			for (auto& Ite : Mappings)
			{
				if (Ite.AcceptableProductionIndex.Count() != 0)
				{
					auto FullIndexCount = Infos.GetAllProductionIndexs(Ite.Symbol);
					assert(FullIndexCount.size() > Ite.AcceptableProductionIndex.Count());
					if (FullIndexCount.size() - Ite.AcceptableProductionIndex.Count() < Ite.AcceptableProductionIndex.Count())
					{
						auto OldStateSize = AccepatbelProductionIndexs.size();
						AccepatbelProductionIndexs.insert(AccepatbelProductionIndexs.end(), FullIndexCount.begin(), FullIndexCount.end());
						auto OldState = Ite.AcceptableProductionIndex.Slice(AccepatbelProductionIndexs);
						auto Result = std::remove_if(AccepatbelProductionIndexs.begin() + OldStateSize, AccepatbelProductionIndexs.end(), [=](std::size_t const Input) {
							return std::find(OldState.begin(), OldState.end(), Input) != OldState.end();
							});
						AccepatbelProductionIndexs.erase(Result, AccepatbelProductionIndexs.end());
						assert(AccepatbelProductionIndexs.size() > OldStateSize);
						Ite.ReserveStorage = true;
						Ite.AcceptableProductionIndex = { OldStateSize, AccepatbelProductionIndexs.size() - OldStateSize };
					}
				}
			}


			for (std::size_t IndexIte2 = SearchPropertyTemporary.size(); IndexIte2 > 0; --IndexIte2)
			{
				auto& Ite = SearchPropertyTemporary[IndexIte2 - 1];
				if (!Ite.ShiftSymbol.has_value())
				{
					auto ProductionIndex = CurrentState[IndexIte2 - 1].ProductionIndex;
					auto& ProduceRef = Infos.ProductionDescs[ProductionIndex];
					TemporaryNode.Reduces.push_back(
						Reduce{ ProduceRef.Symbol, ProductionIndex, ProduceRef.Elements.size(), ProduceRef.ProductionMask }
					);
				}
			}

			for (auto& Ite : Mappings)
			{
				std::optional<std::size_t> FindState;
				std::span<ProductionInfo::SearchElement> Ref = Ite.Status;
				for (std::size_t Index = 0; Index < PreNode.size(); ++Index)
				{
					auto Cur = PreNode[Index].Slice(Status);
					if (Ref.size() == Cur.size() && std::equal(Cur.begin(), Cur.end(), Ref.begin(), Ref.end(), [](auto const& I1, auto const I2) { return I1 == I2; }))
					{
						FindState = Index;
						break;
					}
				}
				if (!FindState.has_value())
				{
					Misc::IndexSpan<> NewIndex{ Status.size(), Ite.Status.size() };
					Status.insert(Status.end(), Ite.Status.begin(), Ite.Status.end());
					FindState = PreNode.size();
					PreNode.push_back(NewIndex);
				}
				auto AcceptablPISpan = Ite.AcceptableProductionIndex.Slice(AccepatbelProductionIndexs);
				std::vector<std::size_t> TempAcce(AcceptablPISpan.begin(), AcceptablPISpan.end());
				TemporaryNode.Shifts.push_back(
					ShiftEdge{Ite.Symbol, *FindState, Ite.ReserveStorage, std::move(TempAcce)}
				);
			}
			auto CurStatusElementSpan = PreNode[SearchIte].Slice(Status);

			for(auto& Ite : CurStatusElementSpan)
			{
				auto FindIte = std::find_if(TemporaryNode.MappedProduction.begin(), TemporaryNode.MappedProduction.end(),
				[Ite](MappedProduction const& Finded) -> bool {
					return Finded.ProductionIndex == Ite.ProductionIndex;
				});
				if(FindIte == TemporaryNode.MappedProduction.end())
				{
					MappedProduction Pro;
					Pro.Production = Infos.ProductionDescs[Ite.ProductionIndex];
					Pro.ProductionIndex = Ite.ProductionIndex;
					Pro.DetectedPoint.push_back(Ite.ElementIndex);
					TemporaryNode.MappedProduction.push_back(std::move(Pro));
				}else{
					FindIte->DetectedPoint.push_back(Ite.ElementIndex);
				}
			}

			Nodes.push_back(std::move(TemporaryNode));
		}
	}

	struct FastReduce
	{
		std::size_t ToNode;
		std::ptrdiff_t Reduce;
		std::size_t LastState;
		LR0Table::Reduce Property;
	};

	struct FastShift
	{
		std::size_t ToNode;
		Symbol RequireSymbol;
	};

	struct FastNode
	{
		std::vector<FastReduce> Reduces;
		std::size_t DiffReduceCount = 0;
		std::vector<FastShift> Shifts;
	};

	struct Walkway
	{
		std::size_t Node;
		std::ptrdiff_t Reduce;
	};

	std::vector<ParsingStep> CollectSteps(std::span<FastNode const> TempTable, std::span<Walkway const> History)
	{
		std::vector<ParsingStep> Result;
		std::optional<std::size_t> LastState;
		for (auto CurIte : History)
		{
			if (LastState.has_value())
			{
				auto& Ref = TempTable[*LastState];
				bool Find = false;
				for (auto& Ite : Ref.Reduces)
				{
					if (Ite.ToNode == CurIte.Node)
					{
						Result.push_back(Ite.Property);
						Find = true;
						break;
					}
				}
				if (!Find)
				{
					for (auto& Ite : Ref.Shifts)
					{
						if (Ite.ToNode == CurIte.Node)
						{
							ParsingStep Re;
							Re.Value = Ite.RequireSymbol;
							Re.Shift.TokenIndex = 0;
							Result.push_back(Re);
							Find = true;
							break;
						}
					}
				}
			}
			LastState = CurIte.Node;
		}
		return Result;
	}



	SLRXTable::SLRXTable(LR0Table const& Table, std::size_t MaxForwardDetect)
	{
		

		std::vector<FastNode> TempNode;

		{
			std::vector<std::vector<std::size_t>> ReversoNodes;

			ReversoNodes.resize(Table.Nodes.size());

			for (std::size_t Index = 0; Index < Table.Nodes.size(); ++Index)
			{
				auto& CurRef = Table.Nodes[Index];

				for (auto& Ite : CurRef.Shifts)
				{
					auto& Ref = ReversoNodes[Ite.ToNode];
					assert(std::find(Ref.begin(), Ref.end(), Index) == Ref.end());
					Ref.push_back(Index);
				}
			}

			TempNode.resize(Table.Nodes.size());

			struct SearchCore
			{
				std::size_t Node;
				std::size_t FastForwardShift;
			};

			std::vector<SearchCore> SearchRecord;

			for (std::size_t Index = 1; Index < Table.Nodes.size(); ++Index)
			{
				auto& Ref = Table.Nodes[Index];
				auto& TempRef = TempNode[Index];

				auto& SHRef = TempRef.Shifts;
				auto& FFRef = TempRef.Reduces;

				for (auto& Ite : Ref.Shifts)
				{
					if (Ite.RequireSymbol.IsTerminal())
					{
						SHRef.push_back({ Ite.ToNode, Ite.RequireSymbol });
					}
				}

				TempRef.DiffReduceCount = Ref.Reduces.size();

				for (std::size_t ReduceIndex = 0; ReduceIndex < Ref.Reduces.size(); ++ReduceIndex)
				{
					std::size_t OldFFRefSize = FFRef.size();

					auto Ite = Ref.Reduces[ReduceIndex];

					SearchRecord.push_back({ Index, Ite.ProductionElementCount });

					while (!SearchRecord.empty())
					{
						auto Top = std::move(*SearchRecord.rbegin());
						SearchRecord.pop_back();
						if (Top.FastForwardShift == 0)
						{
							auto& CurNode = Table.Nodes[Top.Node];

							auto LastState = Top.Node;

							bool Fined = false;
							for (auto& ShiftIte : CurNode.Shifts)
							{
								if (Ite.ReduceSymbol == ShiftIte.RequireSymbol)
								{
									bool Accept = ShiftIte.ProductionIndex.empty() ||
										((std::find(ShiftIte.ProductionIndex.begin(), ShiftIte.ProductionIndex.end(), Ite.ProductionIndex) != ShiftIte.ProductionIndex.end()) == !ShiftIte.ReverseStorage);
									if (Accept)
									{
										Fined = true;
										Top.Node = ShiftIte.ToNode;
										break;
									}
								}
							}

							assert(Fined);

							auto LastSpan = std::span(FFRef).subspan(OldFFRefSize);

							if (std::find_if(LastSpan.begin(), LastSpan.end(), [=](FastReduce const& Element) -> bool {
								return Element.ToNode == Top.Node && Element.LastState == LastState;
								}) == LastSpan.end())
							{
								FFRef.push_back(FastReduce{ Top.Node, 1 - static_cast<std::ptrdiff_t>(Ite.ProductionElementCount), LastState, Ite });
							}
						}
						else {
							auto& FastShift = ReversoNodes[Top.Node];
							assert(FastShift.size() != 0);
							for (auto& Ite2 : FastShift)
								SearchRecord.push_back({ Ite2, Top.FastForwardShift - 1 });
						}
					}
				}
			}
		}

#ifdef _DEBUG
		{
			auto DetectedNodes = std::span(TempNode).subspan(1);
			for (auto& Ite : DetectedNodes)
			{
				assert(!(Ite.Reduces.empty() && Ite.Shifts.empty()));
			}
		}
#endif

		Nodes.resize(TempNode.size());

		std::vector<std::size_t> ConfligNode;

		for (std::size_t I1 = 0; I1 < TempNode.size(); ++I1)
		{
			auto& Ref = TempNode[I1];
			Nodes[I1].MappedProduction = Table.Nodes[I1].MappedProduction;
			if (Ref.DiffReduceCount >= 2 || (!Ref.Shifts.empty() && Ref.DiffReduceCount >= 1))
			{
				if (MaxForwardDetect != 0)
				{
					ConfligNode.push_back(I1);
				}
				else {
					throw IllegalSLRXProduction{
						IllegalSLRXProduction::Category::MaxForwardDetectNotPassed,
						0,
						0,
						{},
						{},
						Nodes[I1].MappedProduction
						};
				}
			}
			else {
				auto& CurRef = Nodes[I1];
				for (auto& Ite : Ref.Shifts)
				{
					std::vector<Symbol> Symbols;
					Symbols.push_back(Ite.RequireSymbol);

					auto FindIte = std::find_if(CurRef.Shifts.begin(), CurRef.Shifts.end(), [&](Shift const& R){
						return R.ToNode == Ite.ToNode;
					});

					if(FindIte != CurRef.Shifts.end())
						FindIte->RequireSymbolss.push_back(std::move(Symbols));
					else {
						Shift NewShift;
						NewShift.RequireSymbolss.push_back(std::move(Symbols));
						NewShift.ToNode = Ite.ToNode;
						CurRef.Shifts.push_back(std::move(NewShift));
					}
				}

				for (auto& Ite : Ref.Reduces)
				{

					auto FindIte = std::find_if(CurRef.Reduces.begin(), CurRef.Reduces.end(), [&](Reduce const& R) {
						return R.Property.ProductionIndex == Ite.Property.ProductionIndex;
						});

					if (FindIte != CurRef.Reduces.end())
						FindIte->RequireSymbolss.push_back({});
					else {
						Reduce NewReduce;
						NewReduce.Property = Ref.Reduces[0].Property;
						NewReduce.RequireSymbolss.push_back({});
						for (auto& Ite : Ref.Reduces)
						{
							NewReduce.ReduceShifts.push_back({ Ite.LastState, Ite.ToNode });
						}
						CurRef.Reduces.push_back(std::move(NewReduce));
					}
				}	
			}
		}

		while (!ConfligNode.empty())
		{
			auto ConfligNodeIte = *ConfligNode.rbegin();
			ConfligNode.pop_back();

			struct DetecedElement
			{
				std::size_t Node;
				std::ptrdiff_t Reduce;
				std::size_t UniqueID;
				std::vector<Walkway> History;
				std::size_t HistoryOffset;
			};

			struct DetectedSet
			{
				std::vector<Symbol> DetectedTerminalSymbol;
				std::vector<DetecedElement> Elements;
			};

			constexpr std::size_t ShiftSimulateProductionIndex = std::numeric_limits<std::size_t>::max();

			std::vector<DetectedSet> DetectedSets;

			{

				DetectedSet StartupSet;
				auto& Ref = TempNode[ConfligNodeIte];

				for (auto& Ite2 : Ref.Reduces)
					StartupSet.Elements.push_back({ Ite2.ToNode, Ite2.Reduce, Ite2.Property.ProductionIndex, {{ConfligNodeIte, 0}, {Ite2.ToNode, Ite2.Reduce}}, 0});

				if (!Ref.Shifts.empty())
					StartupSet.Elements.push_back({ ConfligNodeIte, 0, ShiftSimulateProductionIndex, {{ConfligNodeIte, 0}}, 0 });

				DetectedSets.push_back(std::move(StartupSet));
			}

			while (!DetectedSets.empty())
			{
				auto TopSet = std::move(*DetectedSets.rbegin());
				DetectedSets.pop_back();

				struct NoTerminalMappingElement
				{
					std::size_t Index;
					std::size_t ToNode;
				};

				std::map<Symbol, std::vector<NoTerminalMappingElement>> NoTerminalMapping;

				std::vector<DetecedElement> SearchedElements;
				std::vector<DetecedElement> SearchingElements;

				SearchingElements = std::move(TopSet.Elements);

				while (!SearchingElements.empty())
				{

					auto ElementIte = *std::move(SearchingElements.rbegin());
					SearchingElements.pop_back();


					auto& CurNode = TempNode[ElementIte.Node];

					bool NeedInsertSearchedElement = false;

					if (!CurNode.Shifts.empty())
					{
						auto FindedIte = std::find_if(SearchedElements.begin(), SearchedElements.end(), [&](DetecedElement& Ele)->bool {
							return Ele.Node == ElementIte.Node && Ele.Reduce == ElementIte.Reduce;
							});

						if (FindedIte != SearchedElements.end())
						{
							if (FindedIte->UniqueID == ElementIte.UniqueID)
								continue;
							else {
								throw IllegalSLRXProduction{
									IllegalSLRXProduction::Category::ConfligReduce,
									MaxForwardDetect,
									TopSet.DetectedTerminalSymbol.size() + 1,
									CollectSteps(TempNode, FindedIte->History),
									CollectSteps(TempNode, ElementIte.History),
									Nodes[ElementIte.Node].MappedProduction
								};
							}
						}
						
						NeedInsertSearchedElement = true;
					}

					if (ElementIte.History.size() > 1 && !CurNode.Reduces.empty())
					{
						for (std::size_t I1 = 0; I1 < CurNode.Reduces.size(); ++I1)
						{
							auto& ReduceIte = CurNode.Reduces[I1];
							std::size_t FindCount = 0;
							std::ptrdiff_t Reduce = ElementIte.Reduce + ReduceIte.Reduce;

							for (std::size_t FindedIndex = ElementIte.HistoryOffset; FindedIndex < ElementIte.History.size(); ++FindedIndex)
							{
								auto FindIte = ElementIte.History[FindedIndex];
								if (FindIte.Node == ReduceIte.ToNode)
								{
									if (FindIte.Reduce <= Reduce)
									{
										ElementIte.History.push_back({ ReduceIte.ToNode, Reduce});
										auto OldSpan = std::span(ElementIte.History).subspan(0, FindedIndex + 1);
										throw IllegalSLRXProduction{
											IllegalSLRXProduction::Category::EndlessReduce,
											MaxForwardDetect,
											TopSet.DetectedTerminalSymbol.size() + 1,
											CollectSteps(TempNode, OldSpan),
											CollectSteps(TempNode, ElementIte.History),
											Nodes[ElementIte.Node].MappedProduction
										};
									}
									++FindCount;
									if (FindCount >= MaxForwardDetect)
										break;
								}
							}

							if (FindCount < MaxForwardDetect)
							{
								DetecedElement NewOnde;
								if(!NeedInsertSearchedElement && I1 + 1 == CurNode.Reduces.size())
									NewOnde = std::move(ElementIte);
								else
									NewOnde = ElementIte;
								NewOnde.Node = ReduceIte.ToNode;
								NewOnde.Reduce = Reduce;
								NewOnde.HistoryOffset = ElementIte.HistoryOffset;
								NewOnde.History.push_back({ReduceIte.ToNode, Reduce});
								SearchingElements.push_back(std::move(NewOnde));
							}
						}
					}

					if (NeedInsertSearchedElement)
					{
						std::size_t CurIndex = SearchedElements.size();
						SearchedElements.push_back(std::move(ElementIte));
						for (auto& Ite : CurNode.Shifts)
							NoTerminalMapping[Ite.RequireSymbol].push_back({ CurIndex, Ite.ToNode });
					}
					
				}

				bool Find = false;
				for (auto& Ite : NoTerminalMapping)
				{
					assert(Ite.second.size() >= 1);

					bool Find = false;
					if (Ite.second.size() >= 2)
					{
						std::optional<std::size_t> LastId;
						for (auto& Ite2 : Ite.second)
						{
							if (LastId.has_value())
							{
								auto NewUniqueID = SearchedElements[Ite2.Index].UniqueID;
								if (NewUniqueID != *LastId)
								{
									Find = true;
									break;
								}
							}
							else {
								LastId = SearchedElements[Ite2.Index].UniqueID;
							}
						}
					}
					else {
						Find = false;
					}

					if (!Find)
					{
						auto& TopEle = SearchedElements[Ite.second[0].Index];

						std::vector<Symbol> Symbols = TopSet.DetectedTerminalSymbol;
						Symbols.push_back(Ite.first);
						Symbol TopSymbol = *Symbols.begin();
						auto& TempNodeRef = TempNode[ConfligNodeIte];

						if(TopEle.UniqueID == ShiftSimulateProductionIndex)
						{
							
							auto Find1 = std::find_if(TempNodeRef.Shifts.begin(), TempNodeRef.Shifts.end(), [=](FastShift const& P){
								return P.RequireSymbol == TopSymbol;
							});

							assert(Find1 != TempNodeRef.Shifts.end());

							std::size_t ToNode = Find1->ToNode;

							auto& ShiftRef = Nodes[ConfligNodeIte].Shifts;
							auto Find = std::find_if(ShiftRef.begin(), ShiftRef.end(), [=](Shift const& R){
								return R.ToNode == ToNode;
							});

							if (Find != ShiftRef.end())
							{
								Find->RequireSymbolss.push_back(std::move(Symbols));
							}
							else {
								Shift NewShift;
								NewShift.RequireSymbolss.push_back(std::move(Symbols));
								NewShift.ToNode = ToNode;
								ShiftRef.push_back(std::move(NewShift));
							}
						}
						else {

							
							auto Find1 = std::find_if(TempNodeRef.Reduces.begin(), TempNodeRef.Reduces.end(), [&](FastReduce const& P) {
								return P.Property.ProductionIndex == TopEle.UniqueID;
							});

							assert(Find1 != TempNodeRef.Reduces.end());

							auto& ReduceRef = Nodes[ConfligNodeIte].Reduces;

							auto Find = std::find_if(ReduceRef.begin(), ReduceRef.end(), [=](Reduce const& R){
								return R.Property.ProductionIndex == TopEle.UniqueID;
							});

							if (Find != ReduceRef.end())
							{
								Find->RequireSymbolss.push_back(std::move(Symbols));
							}
							else {
								Reduce NewReduce;
								NewReduce.RequireSymbolss.push_back(std::move(Symbols));
								
								auto& TempNodeRef = TempNode[ConfligNodeIte];
								bool Set = false;

								for (auto& Ite22 : TempNodeRef.Reduces)
								{
									if (Ite22.Property.ProductionIndex == TopEle.UniqueID)
									{
										NewReduce.ReduceShifts.push_back({ Ite22.LastState, Ite22.ToNode });
										if (!Set)
										{
											Set = true;
											NewReduce.Property = Ite22.Property;
										}
									}
								}
								ReduceRef.push_back(std::move(NewReduce));
							}
						}
						auto& CurNode = Nodes[ConfligNodeIte];
						CurNode.MaxForwardDetectCount = std::max(CurNode.MaxForwardDetectCount, TopSet.DetectedTerminalSymbol.size() + 1);
					}
					else {

						if (TopSet.DetectedTerminalSymbol.size() + 1 >= MaxForwardDetect)
						{
							DetecedElement const* Ptr1 = nullptr;
							DetecedElement const* Ptr2 = nullptr;

							for (auto& Ite2 : Ite.second)
							{
								if (Ptr1 == nullptr)
								{
									Ptr1 = &(SearchedElements[Ite2.Index]);
								}
								else{
									auto& Ref = SearchedElements[Ite2.Index];
									if (Ref.UniqueID != Ptr1->UniqueID)
									{
										Ptr2 = &Ref;
										break;
									}
								}
							}

							assert(Ptr1 != nullptr && Ptr2 != nullptr);

							throw IllegalSLRXProduction{
								IllegalSLRXProduction::Category::MaxForwardDetectNotPassed,
								MaxForwardDetect,
								TopSet.DetectedTerminalSymbol.size() + 1,
								CollectSteps(TempNode, Ptr1->History),
								CollectSteps(TempNode, Ptr2->History),
								Nodes[ConfligNodeIte].MappedProduction
							};
						}
						else {
							DetectedSet NewSet;
							NewSet.DetectedTerminalSymbol = TopSet.DetectedTerminalSymbol;
							NewSet.DetectedTerminalSymbol.push_back(Ite.first);

							for (auto& Ite3 : Ite.second)
							{
								auto NewElement = SearchedElements[Ite3.Index];
								NewElement.Reduce += 1;
								NewElement.Node = Ite3.ToNode;
								NewElement.HistoryOffset = NewElement.History.size();
								NewElement.History.push_back({ NewElement.Node, NewElement.Reduce });
								NewSet.Elements.push_back(std::move(NewElement));
							}
							DetectedSets.push_back(std::move(NewSet));
						}
					}
				}
			}
		}
	}

	std::vector<TableWrapper::StandardT> TableWrapper::Create(SLRXTable const& Table)
	{
		std::vector<StandardT> FinalBuffer;
		FinalBuffer.resize(3);
		std::vector<std::size_t> NodeOffset;
		
		struct ShiftPropertyMapping
		{
			std::size_t Offset;
			std::size_t Node;
		};

		struct ReducePropertyMapping
		{
			std::size_t Offset;
			std::span<SLRXTable::ReduceTuple const> Mapping; 
		};
		
		std::vector<ShiftPropertyMapping> ShiftMapping;
		std::vector<ReducePropertyMapping> ReduceMapping;

		for (auto& Ite : Table.Nodes)
		{
			NodeStorageT NewNode;
			Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(NewNode.ForwardDetectedCount, Ite.MaxForwardDetectCount, OutOfRange::TypeT::NodeForwardDetectCount, Ite.MaxForwardDetectCount);
			std::size_t EdgeSize = 0;

			for(auto& Ite2 : Ite.Shifts)
				EdgeSize += Ite2.RequireSymbolss.size();
			for (auto& Ite2 : Ite.Reduces)
				EdgeSize += Ite2.RequireSymbolss.size();

			Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(NewNode.EdgeCount, EdgeSize, OutOfRange::TypeT::NodeEdgeCount, EdgeSize);
			auto NodeRe = Misc::SerilizerHelper::WriteObject(FinalBuffer, NewNode);
			NodeOffset.push_back(NodeRe.StartOffset);

			struct RecordElement
			{
				std::size_t RequireSymbolOffset;
				std::size_t RequireSymbolLastSpanOffset;
				std::size_t Index;
			};

			std::vector<RecordElement> PropertyReocords;
			std::size_t Index = 0;
			for (auto& Ite2 : Ite.Shifts)
			{
				for (auto& IteSyms : Ite2.RequireSymbolss)
				{
					bool HasEndOfDile = false;
					std::vector<StandardT> Value;
					for (auto Ite3 : IteSyms)
					{
						assert(!HasEndOfDile);
						if (Ite3.IsEndOfFile())
						{
							HasEndOfDile = true;
						}
						else {
							Value.push_back(Ite3.Value);
						}
					}

					RequireSymbolStorageT NewRequireSymbol{ HasEndOfDile, 1, static_cast<HalfStandardT>(Value.size()), 0 };
					if (NewRequireSymbol.TerminalSymbolRequireCount != Value.size())
					{
						throw OutOfRange{ OutOfRange::TypeT::RequireSymbolCount, Value.size() };
					}

					auto RSResult = Misc::SerilizerHelper::WriteObject(FinalBuffer, NewRequireSymbol);
					auto ArrayResult = Misc::SerilizerHelper::WriteObjectArray(FinalBuffer, std::span(Value));
					PropertyReocords.push_back({ RSResult.StartOffset, ArrayResult.StartOffset + ArrayResult.WriteLength, Index });
				}

				Index += 1;
			}

			for (auto& Ite2 : Ite.Reduces)
			{
				for (auto& IteSyms : Ite2.RequireSymbolss)
				{
					bool HasEndOfDile = false;
					std::vector<StandardT> Value;
					for (auto Ite3 : IteSyms)
					{
						assert(!HasEndOfDile);
						if (Ite3.IsEndOfFile())
						{
							HasEndOfDile = true;
						}
						else {
							Value.push_back(Ite3.Value);
						}
					}

					RequireSymbolStorageT NewRequireSymbol{ HasEndOfDile, 0, static_cast<HalfStandardT>(Value.size()), 0 };
					if (NewRequireSymbol.TerminalSymbolRequireCount != Value.size())
					{
						throw OutOfRange{ OutOfRange::TypeT::RequireSymbolCount, Value.size() };
					}
					auto RSResult = Misc::SerilizerHelper::WriteObject(FinalBuffer, NewRequireSymbol);
					auto ArrayResult = Misc::SerilizerHelper::WriteObjectArray(FinalBuffer, std::span(Value));
					PropertyReocords.push_back({ RSResult.StartOffset, ArrayResult.StartOffset + ArrayResult.WriteLength, Index });
				}

				Index += 1;
			}

			auto RecordSpan = std::span(PropertyReocords);

			Index = 0;
			for (auto& Ite2 : Ite.Shifts)
			{
				ShiftPropertyT Pro{ 0, 0 };
				auto ShiftW = Misc::SerilizerHelper::WriteObject(FinalBuffer, Pro);

				ShiftMapping.push_back({ ShiftW.StartOffset, Ite2.ToNode});

				assert(!RecordSpan.empty());
				while (!RecordSpan.empty())
				{
					if (RecordSpan[0].Index == Index)
					{
						auto Read = Misc::SerilizerHelper::ReadObject<RequireSymbolStorageT>(std::span(FinalBuffer).subspan(RecordSpan[0].RequireSymbolOffset));
						auto Offset = ShiftW.StartOffset - RecordSpan[0].RequireSymbolLastSpanOffset;
						Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Read->PropertyOffset, Offset, OutOfRange::TypeT::PropertyOffset, Offset );
						RecordSpan = RecordSpan.subspan(1);
					}
					else {
						break;
					}
				}

				Index += 1;
			}

			for (auto& Ite2 : Ite.Reduces)
			{
				std::vector<ToNodeMappingT> Mapping;
				Mapping.resize(Ite2.ReduceShifts.size());

				ReducePropertyT Pro{
						Ite2.Property.ReduceSymbol.IsStartSymbol(),
						static_cast<HalfStandardT>(Mapping.size()),
						static_cast<HalfStandardT>(Ite2.Property.ProductionIndex),
						static_cast<HalfStandardT>(Ite2.Property.ProductionElementCount),
						Ite2.Property.ReduceMask,
						Ite2.Property.ReduceSymbol.Value
				};

				if (Pro.ToNodeMappingCount != Mapping.size())
					throw OutOfRange{ OutOfRange::TypeT::ToNodeMappingCount, Mapping.size() };

				if (Pro.ProductionIndex != Ite2.Property.ProductionIndex)
					throw OutOfRange{ OutOfRange::TypeT::ProductionIndex, Ite2.Property.ProductionIndex };

				if (Pro.ProductionCount != Ite2.Property.ProductionElementCount)
					throw OutOfRange{ OutOfRange::TypeT::ProductionIndex,  Ite2.Property.ProductionElementCount };

				auto RSResult = Misc::SerilizerHelper::WriteObject(FinalBuffer, Pro);
				Misc::SerilizerHelper::WriteObjectArray(FinalBuffer, std::span(Mapping));

				ReduceMapping.push_back({ RSResult.StartOffset, std::span(Ite2.ReduceShifts)});
				
				assert(!RecordSpan.empty());
				while (!RecordSpan.empty())
				{
					if (RecordSpan[0].Index == Index)
					{
						auto Read = Misc::SerilizerHelper::ReadObject<RequireSymbolStorageT>(std::span(FinalBuffer).subspan(RecordSpan[0].RequireSymbolOffset));
						auto Offset = RSResult.StartOffset - RecordSpan[0].RequireSymbolLastSpanOffset;
						Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Read->PropertyOffset, Offset, OutOfRange::TypeT::PropertyOffset, Offset );
						RecordSpan = RecordSpan.subspan(1);
					}
					else {
						break;
					}
				}
				Index += 1;
			}

			assert(RecordSpan.empty());
		}
		
		for (auto Ite : ShiftMapping)
		{
			auto Property = Misc::SerilizerHelper::ReadObject<ShiftPropertyT>(std::span(FinalBuffer).subspan(Ite.Offset));
			std::size_t RN = NodeOffset[Ite.Node];
			std::size_t RD = Table.Nodes[Ite.Node].MaxForwardDetectCount;
			Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Property->ToNode, RN, OutOfRange::TypeT::NodeOffset, RN );
			Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Property->ForwardDetectCount, RD, OutOfRange::TypeT::NodeForwardDetectCount, RN );
		}

		for (auto Ite : ReduceMapping)
		{
			auto Property = Misc::SerilizerHelper::ReadObject<ReducePropertyT>(std::span(FinalBuffer).subspan(Ite.Offset));
			auto Mapping = Misc::SerilizerHelper::ReadObjectArray<ToNodeMappingT>(Property.LastSpan, Property->ToNodeMappingCount);
			
			assert(Ite.Mapping.size() == Mapping->size());

			for (std::size_t T1 = 0; T1 < Ite.Mapping.size(); ++T1)
			{
				auto& Mapping1 = Ite.Mapping[T1];
				auto& Mapping2 = (*Mapping)[T1];

				std::size_t LS = NodeOffset[Mapping1.LastState];
				std::size_t TS = NodeOffset[Mapping1.TargetState];

				Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Mapping2.LastState, LS, OutOfRange::TypeT::NodeOffset, LS );
				Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Mapping2.ToState, TS, OutOfRange::TypeT::NodeOffset, TS );
				std::size_t MaxForward = Table.Nodes[Mapping1.TargetState].MaxForwardDetectCount;
				Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(Mapping2.ForwardDetectCount, MaxForward, OutOfRange::TypeT::NodeForwardDetectCount, MaxForward);

			}
		}

		/*
		for (std::size_t I1 = 0; I1 < Table.Nodes.size(); ++I1)
		{
			auto& NodeRef = Table.Nodes[I1];
			auto CurSpan = std::span(FinalBuffer).subspan(NodeOffset[I1]);
			auto CurNode = Misc::SerilizerHelper::ReadObject<NodeStorageT>(CurSpan);
			auto SpanIte = CurNode.LastSpan;

			std::optional<std::size_t> LastIndex;

			for (auto& Ite : NodeRef.Shifts)
			{
				if (!LastIndex.has_value() || Ite.ToNode != *LastIndex)
				{
					LastIndex = Ite.ToNode;
					auto PropertyEdge = Misc::SerilizerHelper::ReadObject<RequireSymbolStorageT>(SpanIte);
					auto Symbol = Misc::SerilizerHelper::ReadObjectArray<StandardT>(PropertyEdge.LastSpan, PropertyEdge->TerminalSymbolRequireCount);
					auto Property = Misc::SerilizerHelper::ReadObject<ShiftPropertyT>(Symbol.LastSpan.subspan(PropertyEdge->PropertyOffset));
					std::size_t RN = NodeOffset[Ite.ToNode];
					std::size_t RD = Table.Nodes[Ite.ToNode].MaxForwardDetectCount;
					Misc::SerilizerHelper::TryCrossTypeSet(Property->ToNode, RN, OutOfRange{ OutOfRange::TypeT::NodeOffset, RN });
					Misc::SerilizerHelper::TryCrossTypeSet(Property->ForwardDetectCount, RD, OutOfRange{ OutOfRange::TypeT::NodeForwardDetectCount, RN });
					SpanIte = Symbol.LastSpan;
				}	
			}

			LastIndex.reset();

			for (auto& Ite : NodeRef.Reduces)
			{

				if (!LastIndex.has_value() || Ite.Property.ProductionIndex != *LastIndex)
				{
					LastIndex = Ite.Property.ProductionIndex;
					auto PropertyEdge = Misc::SerilizerHelper::ReadObject<RequireSymbolStorageT>(SpanIte);
					auto Symbol = Misc::SerilizerHelper::ReadObjectArray<StandardT>(PropertyEdge.LastSpan, PropertyEdge->TerminalSymbolRequireCount);
					auto Property = Misc::SerilizerHelper::ReadObject<ReducePropertyT>(Symbol.LastSpan.subspan(PropertyEdge->PropertyOffset));
					auto ToNodeMap = Misc::SerilizerHelper::ReadObjectArray<ToNodeMappingT>(Property.LastSpan, Property->ToNodeMappingCount);


					for (std::size_t I2 = 0; I2 < ToNodeMap->size(); ++I2)
					{
						auto& Ref = Ite.ReduceShifts[I2];
						std::size_t LS = NodeOffset[Ref.LastState];
						std::size_t TS = NodeOffset[Ref.TargetState];
						auto& Ref2 = (*ToNodeMap)[I2];
						Misc::SerilizerHelper::TryCrossTypeSet(Ref2.LastState, LS, OutOfRange{ OutOfRange::TypeT::NodeOffset, LS });
						Misc::SerilizerHelper::TryCrossTypeSet(Ref2.ToState, TS, OutOfRange{ OutOfRange::TypeT::NodeOffset, TS });
						std::size_t MaxForward = Table.Nodes[Ref.TargetState].MaxForwardDetectCount;
						Misc::SerilizerHelper::TryCrossTypeSet(Ref2.ForwardDetectCount, MaxForward, OutOfRange{ OutOfRange::TypeT::NodeForwardDetectCount, MaxForward });
					}

					SpanIte = Symbol.LastSpan;
				}
			}
		}
		*/

		Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(FinalBuffer[0], Table.Nodes.size(), OutOfRange::TypeT::NodeCount, Table.Nodes.size() );
		Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(FinalBuffer[1], NodeOffset[1], OutOfRange::TypeT::NodeCount, Table.Nodes.size());
		Misc::SerilizerHelper::TryCrossTypeSet<OutOfRange>(FinalBuffer[2], Table.Nodes[1].MaxForwardDetectCount, OutOfRange::TypeT::NodeForwardDetectCount, Table.Nodes.size());

#ifdef _DEBUG
		
		struct DebugShift
		{
			std::vector<Symbol> RequireSymbols;
			std::size_t ToNode;
		};

		struct DebugReduce
		{
			std::vector<Symbol> RequireSymbols;
			std::span<ToNodeMappingT const> Mapping;
			HalfStandardT ProductionIndex;
			HalfStandardT ProductionCount;
			StandardT Mask;
			Symbol NoTerminalSymbol;
		};

		struct DebugNode
		{
			std::size_t Node;
			std::size_t MaxForward;
			std::vector<DebugShift> Shifts;
			std::vector<DebugReduce> Reduces;
		};

		std::vector<DebugNode> DebugNodes;

		TableWrapper Wrapper(FinalBuffer);

		for (auto Index : NodeOffset)
		{

			auto NodeRef = Wrapper.ReadNode(Index);

			DebugNode DNode;

			DNode.Node = Index;
			DNode.MaxForward = NodeRef.Storage.ForwardDetectedCount;

			for (auto Ite = NodeRef.Iterate(); Ite; Ite = Ite->Next())
			{

				std::vector<Symbol> Symbols;

				for (auto Ite2 : Ite->Symbols)
				{
					Symbols.push_back(Symbol::AsTerminal(Ite2));
				}

				if(Ite->Storage.IncludeEndOfFile)
					Symbols.push_back(Symbol::EndOfFile());

				if (Ite->Storage.IsShift)
				{
					DebugShift Shift;
					Shift.RequireSymbols = std::move(Symbols);
					Shift.ToNode = Ite->ReadShiftProperty().ToNode;
					DNode.Shifts.push_back(std::move(Shift));
				}
				else {
					DebugReduce NewReduce;
					NewReduce.RequireSymbols = std::move(Symbols);
					auto ReducePro = Ite->ReadReduceProperty();
					NewReduce.Mask = ReducePro.Storage.Mask;
					NewReduce.ProductionIndex = ReducePro.Storage.ProductionIndex;
					NewReduce.ProductionCount = ReducePro.Storage.ProductionCount;
					if (ReducePro.Storage.IsStartSymbol)
						NewReduce.NoTerminalSymbol = Symbol::StartSymbol();
					else
						NewReduce.NoTerminalSymbol = Symbol::AsNoTerminal(ReducePro.Storage.NoTerminalValue);
					NewReduce.Mapping = ReducePro.Mapping;
					DNode.Reduces.push_back(std::move(NewReduce));

				}
			}
			DebugNodes.push_back(std::move(DNode));
		}

		volatile int i = 0;

#endif

		return FinalBuffer;
	}

	bool CoreProcessor::DetectSymbolEqual(std::deque<CacheSymbol> const& T1, std::span<StandardT const> T2)
	{
		if (T1.size() >= T2.size())
		{
			bool Equal = true;
			std::size_t Count = 0;
			for (auto Ite = T1.begin(); Ite != T1.end() && Count < T2.size(); ++Ite, ++Count)
			{
				if (Ite->Value != T2[Count])
				{
					Equal = false;
					break;
				}
			}
			return Equal;
		}
		return false;
		
	}

	void CoreProcessor::Consume(Symbol InputSymbol, std::size_t TokenIndex)
	{
		if (States.empty())
			throw UnaccableSymbol{ InputSymbol, TokenIteratorIndex, TokenIndex, {}};

		assert(InputSymbol.IsTerminal());
		bool IsEndOfFile = InputSymbol.IsEndOfFile();

		if(!IsEndOfFile)
			CacheSymbols.push_back({ TokenIteratorIndex, TokenIndex, InputSymbol.Value});

		bool Consumed = false;

		while (!CacheSymbols.empty() && MaxForwardDetect <= CacheSymbols.size() || IsEndOfFile)
		{
			auto NodeWrapper = Wrapper.ReadNode(*States.rbegin());
			bool Find = false;

			for (auto Ite = NodeWrapper.Iterate(); Ite.has_value(); Ite = Ite->Next())
			{
				if (Ite->Storage.IsShift)
				{
					if (IsEndOfFile == Ite->Storage.IncludeEndOfFile && DetectSymbolEqual(CacheSymbols, Ite->Symbols))
					{
						auto ShiftPro = Ite->ReadShiftProperty();
						States.push_back(ShiftPro.ToNode);
						MaxForwardDetect = ShiftPro.ForwardDetectCount;
						
						if (!CacheSymbols.empty())
						{
							auto Last = *CacheSymbols.begin();
							ParsingStep NewStep;
							NewStep.Value = Symbol::AsTerminal(Last.Value);
							NewStep.Shift.TokenIndex = Last.TokenIndex;
							Steps.push_back(NewStep);
							CacheSymbols.pop_front();
						}	
						Consumed = true;
						Find = true;
						break;
					}
				}
				else {
					if (
						(!Ite->Storage.IncludeEndOfFile && Ite->Symbols.empty())
						|| IsEndOfFile == Ite->Storage.IncludeEndOfFile && DetectSymbolEqual(CacheSymbols, Ite->Symbols)
						)
					{
						auto Reduce = Ite->ReadReduceProperty();

						assert(States.size() >= Reduce.Storage.ProductionCount);
						States.resize(States.size() - Reduce.Storage.ProductionCount);
						std::size_t LastState = *States.rbegin();
						bool Find2 = false;
						for (auto Ite3 : Reduce.Mapping)
						{
							if (LastState == Ite3.LastState)
							{
								States.push_back(Ite3.ToState);
								MaxForwardDetect = Ite3.ForwardDetectCount;
								ParsingStep NewStep;
								NewStep.Value = Symbol::AsNoTerminal(Reduce.Storage.NoTerminalValue);
								NewStep.Reduce.Mask = Reduce.Storage.Mask;
								NewStep.Reduce.ProductionIndex = Reduce.Storage.ProductionIndex;
								NewStep.Reduce.ProductionCount = Reduce.Storage.ProductionCount;
								Steps.push_back(NewStep);
								Find = true;
								Find2 = true;
								break;
							}
						}
						assert(Find2);
						break;
					}
				}
			}
			if (!Find)
			{

				if (IsEndOfFile && CacheSymbols.empty() && NodeWrapper.Storage.ForwardDetectedCount == 0)
				{
					States.clear();
					break;
				}

				std::size_t LastState = *States.rbegin();

				std::vector<UnaccableSymbol::Tuple> RequireSymbols;

				for (auto Ite = NodeWrapper.Iterate(); Ite.has_value(); Ite = Ite->Next())
				{

					std::optional<UnaccableSymbol::Tuple> RS;

					std::size_t I1 = 0;
					for (; I1 < Ite->Symbols.size() && I1 < CacheSymbols.size(); ++I1)
					{
						if(Ite->Symbols[I1] != CacheSymbols[I1].Value)
							break;
					}

					if (I1 < Ite->Symbols.size())
					{
						if (I1 < CacheSymbols.size())
						{
							RS = { Symbol::AsTerminal(Ite->Symbols[I1]), CacheSymbols [I1].TokenIndex};
						}
						else {
							RS = { Symbol::AsTerminal(Ite->Symbols[I1]), TokenIteratorIndex };
						}
					}
					else {
						RS = {Symbol::EndOfFile(), TokenIteratorIndex };
					}

					if (RS.has_value())
					{
						if(std::find(RequireSymbols.begin(), RequireSymbols.end(), *RS) == RequireSymbols.end())
							RequireSymbols.push_back(*RS);
					}
				}

				throw UnaccableSymbol{
					InputSymbol,
					TokenIteratorIndex,
					TokenIndex,
					std::move(RequireSymbols)
				};
			}
		}
		TokenIteratorIndex += 1;
	}

	std::vector<ParsingStep> CoreProcessor::EndOfFile()
	{
		Consume(Symbol::EndOfFile(), TokenIteratorIndex);
		assert(States.empty());
		return std::move(Steps);
	}


	TableWrapper::NodeWrapperT TableWrapper::ReadNode(std::size_t Offset) const
	{
		TableWrapper::NodeWrapperT NewWrapper;
		auto Read = Misc::SerilizerHelper::ReadObject<NodeStorageT const>(Wrapper.subspan(Offset));
		NewWrapper.Storage = *Read;
		NewWrapper.LastSpan = Read.LastSpan;
		return NewWrapper;
	}

	auto TableWrapper::NodeWrapperT::Iterate() const -> std::optional<RequireSymbolWrapperT>
	{
		if (Storage.EdgeCount > 0)
		{
			auto Read = Misc::SerilizerHelper::ReadObject<RequireSymbolStorageT const>(LastSpan);
			auto Datas = Misc::SerilizerHelper::ReadObjectArray<StandardT const>(Read.LastSpan, Read->TerminalSymbolRequireCount);
			RequireSymbolWrapperT Wrapper{ *Read, Storage.EdgeCount, *Datas , Datas.LastSpan };
			return Wrapper;
		}
		else {
			return {};
		}
	}

	auto TableWrapper::RequireSymbolWrapperT::Next() const ->std::optional<RequireSymbolWrapperT>
	{
		if (Last > 1)
		{
			auto Read = Misc::SerilizerHelper::ReadObject<RequireSymbolStorageT const>(LastSpan);
			auto Datas = Misc::SerilizerHelper::ReadObjectArray<StandardT const>(Read.LastSpan, Read->TerminalSymbolRequireCount);
			return RequireSymbolWrapperT{ *Read, Last - 1, *Datas , Datas.LastSpan };
		}
		else {
			return {};
		}
	}

	auto  TableWrapper::RequireSymbolWrapperT::ReadShiftProperty() const ->ShiftPropertyT
	{
		auto Read = Misc::SerilizerHelper::ReadObject<ShiftPropertyT const>(LastSpan.subspan(Storage.PropertyOffset));
		return *Read;
	}

	auto  TableWrapper::RequireSymbolWrapperT::ReadReduceProperty() const ->ReducePropertyWrapperT
	{
		auto Read = Misc::SerilizerHelper::ReadObject<ReducePropertyT const>(LastSpan.subspan(Storage.PropertyOffset));
		auto Data = Misc::SerilizerHelper::ReadObjectArray<ToNodeMappingT const>(Read.LastSpan, Read->ToNodeMappingCount);
		return {*Read, *Data};
	}

	namespace Exception
	{
		char const* Interface::what() const {  return "PotatoDLrException"; };
		char const* WrongProduction::what() const { 
			switch (Type)
			{
			case Category::MaskNotFolloedNoterminalSymbol:
				return "PotatoSLRXException::WrongProduction::MaskNotFolloedNoterminalSymbol";
			case Category::TerminalSymbolAsProductionBegin:
				return "PotatoSLRXException::WrongProduction::TerminalSymbolAsProductionBegin";
			default:
				return "PotatoSLRXException::WrongProduction::Unknow";
			}
		}

		char const* IllegalSLRXProduction::what() const {
			switch (Type)
			{
			case Category::EndlessReduce:
				return "PotatoSLRXException::IllegalSLRXProduction::EndlessReduce";
			case Category::ConfligReduce:
				return "PotatoSLRXException::IllegalSLRXProduction::ConfligReduce";
			case Category::MaxForwardDetectNotPassed:
				return "PotatoSLRXException::IllegalSLRXProduction::MaxForwardDetectNotPassed";
			default:
				return "PotatoSLRXException::IllegalSLRXProduction::Unknow";
			}
		}
		char const* BadProduction::what() const { return "PotatoSLRXException::WrongProduction"; };
		char const* OpePriorityConflict::what() const { return "PotatoSLRXException::OpePriorityConflict"; };
		char const* UnaccableSymbol::what() const { return "PotatoSLRXException::UnaccableSymbol"; };
		char const* MaxAmbiguityProcesser::what() const { return "PotatoSLRXException::MaxAmbiguityProcesser"; };
		char const* UnsupportedStep::what() const { return "PotatoSLRXException::UnsupportedStep"; };
		char const* OutOfRange::what() const { return "PotatoSLRXException::OutOfRange"; };
	}

}