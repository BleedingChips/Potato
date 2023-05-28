module;

#include <cassert>

module Potato.SLRX;

namespace Potato::SLRX
{

	using namespace Exception;

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
				OpePriority::Associativity Asso;
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
										case OpePriority::Associativity::Left:
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
										case OpePriority::Associativity::Right:
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

	LR0 LR0::Create(ProductionInfo Infos)
	{
		std::vector<Node> Nodes;
		std::vector<ProductionInfo::SearchElement> Status;
		std::vector<Misc::IndexSpan<>> PreNode;

		Status.push_back(Infos.GetStartupSearchElements());
		Infos.ExpandSearchElements(Status);
		PreNode.push_back({ Status.size(), Status.size() + 1 });
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
			auto CurrentState = PreNode[SearchIte].Slice(std::span(Status));
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
							ShiftMapping sm{ *CurSearch.ShiftSymbol, {CurrentState[Index]}, {AccepatbelProductionIndexs.size(), AccepatbelProductionIndexs.size()}, false };
							Mappings.push_back(std::move(sm));
						}
					}
					else {
						ShiftMapping Cur{ *CurSearch.ShiftSymbol, {CurrentState[Index]}, {AccepatbelProductionIndexs.size(), AccepatbelProductionIndexs.size() + CurSearch.AcceptableProductionIndex.size() }, false };
						AccepatbelProductionIndexs.insert(AccepatbelProductionIndexs.end(), CurSearch.AcceptableProductionIndex.begin(), CurSearch.AcceptableProductionIndex.end());
						for (std::size_t Index2 = 0; Index2 < Mappings.size(); ++Index2)
						{
							auto& Ite = Mappings[Index2];
							if (Ite.Symbol == Cur.Symbol)
							{
								auto S1 = Ite.AcceptableProductionIndex.Slice(std::span(AccepatbelProductionIndexs));
								auto S2 = Cur.AcceptableProductionIndex.Slice(std::span(AccepatbelProductionIndexs));
								auto [S1s, S2s] = ProductionInfo::IntersectionSet(S1, S2);
								if (S1s == 0)
									//CFH::InsertSortedUniquedVectorElement(Status, CurrentState[Index]);
									Ite.Status.push_back(CurrentState[Index]);
								else if (S1.size() != S1s) {
									auto New = Ite;
									Ite.AcceptableProductionIndex = Ite.AcceptableProductionIndex.SubIndex(S1s);
									New.AcceptableProductionIndex.Normalize(S1s);
									Ite.Status.push_back(CurrentState[Index]);
									Mappings.insert(Mappings.begin() + Index2, std::move(New));
									++Index2;
								}
								Cur.AcceptableProductionIndex.Normalize(S2s);
								AccepatbelProductionIndexs.resize(Cur.AcceptableProductionIndex.End());
								if (S2s == 0)
									break;
							}
						}
						if (Cur.AcceptableProductionIndex.Size() != 0)
							Mappings.push_back(std::move(Cur));
					}
				}
			}

			for (auto Ite = Mappings.begin(); Ite != Mappings.end(); ++Ite)
			{
				for (auto& Ite2 : Ite->Status)
					Ite2.ElementIndex += 1;
				Infos.ExpandSearchElements(Ite->Status);
				if (Ite->AcceptableProductionIndex.Size() != 0)
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
						Ite->AcceptableProductionIndex.Normalize(0);
				}
			}

			for (auto& Ite : Mappings)
			{
				if (Ite.AcceptableProductionIndex.Size() != 0)
				{
					auto FullIndexCount = Infos.GetAllProductionIndexs(Ite.Symbol);
					assert(FullIndexCount.size() > Ite.AcceptableProductionIndex.Size());
					if (FullIndexCount.size() - Ite.AcceptableProductionIndex.Size() < Ite.AcceptableProductionIndex.Size())
					{
						auto OldStateSize = AccepatbelProductionIndexs.size();
						AccepatbelProductionIndexs.insert(AccepatbelProductionIndexs.end(), FullIndexCount.begin(), FullIndexCount.end());
						auto OldState = Ite.AcceptableProductionIndex.Slice(std::span(AccepatbelProductionIndexs));
						auto Result = std::remove_if(AccepatbelProductionIndexs.begin() + OldStateSize, AccepatbelProductionIndexs.end(), [=](std::size_t const Input) {
							return std::find(OldState.begin(), OldState.end(), Input) != OldState.end();
							});
						AccepatbelProductionIndexs.erase(Result, AccepatbelProductionIndexs.end());
						assert(AccepatbelProductionIndexs.size() > OldStateSize);
						Ite.ReserveStorage = true;
						Ite.AcceptableProductionIndex = { OldStateSize, AccepatbelProductionIndexs.size() };
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
					auto Cur = PreNode[Index].Slice(std::span(Status));
					if (Ref.size() == Cur.size() && std::equal(Cur.begin(), Cur.end(), Ref.begin(), Ref.end(), [](auto const& I1, auto const I2) { return I1 == I2; }))
					{
						FindState = Index;
						break;
					}
				}
				if (!FindState.has_value())
				{
					Misc::IndexSpan<> NewIndex{ Status.size(), Status.size() + Ite.Status.size() };
					Status.insert(Status.end(), Ite.Status.begin(), Ite.Status.end());
					FindState = PreNode.size();
					PreNode.push_back(NewIndex);
				}
				auto AcceptablPISpan = Ite.AcceptableProductionIndex.Slice(std::span(AccepatbelProductionIndexs));
				std::vector<std::size_t> TempAcce(AcceptablPISpan.begin(), AcceptablPISpan.end());
				TemporaryNode.Shifts.push_back(
					ShiftEdge{ Ite.Symbol, *FindState, Ite.ReserveStorage, std::move(TempAcce) }
				);
			}
			auto CurStatusElementSpan = PreNode[SearchIte].Slice(std::span(Status));

			for (auto& Ite : CurStatusElementSpan)
			{
				auto FindIte = std::find_if(TemporaryNode.MappedProduction.begin(), TemporaryNode.MappedProduction.end(),
					[Ite](MappedProduction const& Finded) -> bool {
						return Finded.ProductionIndex == Ite.ProductionIndex;
					});
				if (FindIte == TemporaryNode.MappedProduction.end())
				{
					MappedProduction Pro;
					Pro.Production = Infos.ProductionDescs[Ite.ProductionIndex];
					Pro.ProductionIndex = Ite.ProductionIndex;
					Pro.DetectedPoint.push_back(Ite.ElementIndex);
					TemporaryNode.MappedProduction.push_back(std::move(Pro));
				}
				else {
					FindIte->DetectedPoint.push_back(Ite.ElementIndex);
				}
			}

			Nodes.push_back(std::move(TemporaryNode));
		}

		return {std::move(Nodes)};
	}

	struct FastReduce
	{
		std::size_t ToNode;
		std::ptrdiff_t Reduce;
		std::size_t LastState;
		LR0::Reduce Property;
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

	LRX LRX::Create(LR0 const& Table, std::size_t MaxForwardDetect)
	{
		std::vector<Node> Nodes;
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

					SearchRecord.push_back({ Index, Ite.Reduce.ElementCount });

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
										((std::find(ShiftIte.ProductionIndex.begin(), ShiftIte.ProductionIndex.end(), Ite.Reduce.ProductionIndex) != ShiftIte.ProductionIndex.end()) == !ShiftIte.ReverseStorage);
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
								FFRef.push_back(FastReduce{ Top.Node, 1 - static_cast<std::ptrdiff_t>(Ite.Reduce.ElementCount), LastState, Ite });
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
			auto& Productions = Table.Nodes[I1].MappedProduction;
			Nodes[I1].MappedProduction = Productions;
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
						Productions
					};
				}
			}
			else {
				auto& CurRef = Nodes[I1];

				if (!Ref.Shifts.empty() && CurRef.RequireNodes.empty())
					CurRef.RequireNodes.push_back({});

				for (auto& Ite : Ref.Shifts)
				{
					CurRef.RequireNodes[0].push_back({
							RequireNodeType::ShiftProperty
						, Ite.RequireSymbol, Ite.ToNode });
				}

				for (auto& Ite : Ref.Reduces)
				{

					assert(CurRef.Reduces.size() <= 1 && CurRef.RequireNodes.empty());

					auto FindIte = std::find_if(CurRef.Reduces.begin(), CurRef.Reduces.end(), [&](ReduceProperty const& R) {
						return R.Property.Reduce.ProductionIndex == Ite.Property.Reduce.ProductionIndex;
						});

					if (FindIte == CurRef.Reduces.end())
					{
						ReduceProperty NewProperty;
						NewProperty.Property = Ref.Reduces[0].Property;
						for (auto& Ite : Ref.Reduces)
						{
							NewProperty.Tuples.push_back({ Ite.LastState, Ite.ToNode });
						}
						CurRef.Reduces.push_back(std::move(NewProperty));
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
					StartupSet.Elements.push_back({ Ite2.ToNode, Ite2.Reduce, Ite2.Property.Reduce.ProductionIndex, {{ConfligNodeIte, 0}, {Ite2.ToNode, Ite2.Reduce}}, 0 });

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
										ElementIte.History.push_back({ ReduceIte.ToNode, Reduce });
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
								if (!NeedInsertSearchedElement && I1 + 1 == CurNode.Reduces.size())
									NewOnde = std::move(ElementIte);
								else
									NewOnde = ElementIte;
								NewOnde.Node = ReduceIte.ToNode;
								NewOnde.Reduce = Reduce;
								NewOnde.HistoryOffset = ElementIte.HistoryOffset;
								NewOnde.History.push_back({ ReduceIte.ToNode, Reduce });
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

						auto WriteSpan = std::span(Symbols).subspan(0, Symbols.size() - 1);

						std::size_t NodeIte = 0;

						auto& NodeRef = Nodes[ConfligNodeIte];

						for (auto Ite : WriteSpan)
						{
							if (NodeRef.RequireNodes.size() <= NodeIte)
								NodeRef.RequireNodes.emplace_back();

							assert(NodeRef.RequireNodes.size() > NodeIte);

							auto& RNode = NodeRef.RequireNodes[NodeIte];

							bool Find = false;

							for (auto& Ite2 : RNode)
							{
								if (Ite2.RequireSymbol == Ite)
								{
									assert(Ite2.Type == RequireNodeType::SymbolValue);
									Find = true;
									NodeIte = Ite2.ReferenceIndex;
									break;
								}
							}

							if (!Find)
							{
								RNode.push_back(RequireNode{
										RequireNodeType::SymbolValue,
										Ite,
										NodeRef.RequireNodes.size()
									});
								NodeIte = NodeRef.RequireNodes.size();
							}
						}

						if (NodeRef.RequireNodes.size() <= NodeIte)
							NodeRef.RequireNodes.emplace_back();

						assert(NodeRef.RequireNodes.size() > NodeIte);

						auto& LastRNode = NodeRef.RequireNodes[NodeIte];

						assert(std::find_if(LastRNode.begin(), LastRNode.end(), [&](LRX::RequireNode Node) {
							return Node.RequireSymbol == *Symbols.rbegin();
							}) == LastRNode.end());

						if (TopEle.UniqueID == ShiftSimulateProductionIndex)
						{

							auto Find1 = std::find_if(TempNodeRef.Shifts.begin(), TempNodeRef.Shifts.end(), [=](FastShift const& P) {
								return P.RequireSymbol == TopSymbol;
								});

							assert(Find1 != TempNodeRef.Shifts.end());

							LastRNode.push_back(RequireNode{
								RequireNodeType::ShiftProperty,
								*Symbols.rbegin(),
								Find1->ToNode
								});
						}
						else {

							auto Find1 = std::find_if(TempNodeRef.Reduces.begin(), TempNodeRef.Reduces.end(), [&](FastReduce const& P) {
								return P.Property.Reduce.ProductionIndex == TopEle.UniqueID;
								});

							assert(Find1 != TempNodeRef.Reduces.end());

							auto& NodeRef = Nodes[ConfligNodeIte];
							auto& ReduceRef = NodeRef.Reduces;

							auto Find = std::find_if(ReduceRef.begin(), ReduceRef.end(), [=](ReduceProperty const& R) {
								return R.Property.Reduce.ProductionIndex == TopEle.UniqueID;
								});

							std::size_t PropertyOffset = NodeRef.Reduces.size();

							if (Find != ReduceRef.end())
							{
								PropertyOffset = std::distance(ReduceRef.begin(), Find);
							}
							else {
								ReduceProperty NewElement;
								NewElement.Property = Find1->Property;
								for (auto& Ite22 : TempNodeRef.Reduces)
								{
									if (Ite22.Property.Reduce.ProductionIndex == TopEle.UniqueID)
									{
										NewElement.Tuples.push_back({ Ite22.LastState, Ite22.ToNode });
									}
								}
								NodeRef.Reduces.push_back(std::move(NewElement));
							}

							LastRNode.push_back(RequireNode{
								RequireNodeType::ReduceProperty,
								*Symbols.rbegin(),
								PropertyOffset
								});
						}
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
								else {
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
		return LRX{std::move(Nodes)};
	}

	void LRXBinaryTableWrapper::Serilize(Misc::StructedSerilizerWritter<StandardT>& Writter, LRX const& Ref)
	{
		std::vector<std::size_t> NodeOffset;

		if(Writter.IsWritting())
			NodeOffset.reserve(Ref.Nodes.size());

		ZipHeadT Head;

		Misc::CrossTypeSetThrow<OutOfRange>(Head.NodeCount, Ref.Nodes.size(), OutOfRange::TypeT::NodeCount, Ref.Nodes.size());

		Writter.WriteObject(Head);

		std::size_t NodeIndex = 0;

		struct RequireNodeOffsetRecord
		{
			std::size_t Adress;
			std::size_t ArrayCount;
			std::size_t ArrayIndex;
			std::size_t Index;
		};

		struct ReduceTupleOffsetRecord
		{
			std::size_t Adress;
			std::size_t Count;
			//std::span<ZipReduceTupleT> Span;
			std::span<LRX::ReduceTuple const> Tuples;
		};

		std::vector<RequireNodeOffsetRecord> ReduceRecord;
		std::vector<RequireNodeOffsetRecord> ShiftRecord;
		std::vector<RequireNodeOffsetRecord> SymbolsRecord;

		std::vector<ReduceTupleOffsetRecord> ReduceTupleRecord;

		for (auto& Ite : Ref.Nodes)
		{
			ZipNodeT NewNode;

			Misc::CrossTypeSetThrow<OutOfRange>(NewNode.RequireNodeDescCount, Ite.RequireNodes.size(), OutOfRange::TypeT::RequireNodeCount, Ite.RequireNodes.size());
			Misc::CrossTypeSetThrow<OutOfRange>(NewNode.ReduceCount, Ite.Reduces.size(), OutOfRange::TypeT::RequireNodeCount, Ite.Reduces.size());

			auto NodeAdress = Writter.WriteObject(NewNode);
			auto CurNodeOffset = Writter.GetWritedSize();

			if(Writter.IsWritting())
				NodeOffset.push_back(NodeAdress);

			std::vector<std::size_t> DescOffset;
			for (auto& Ite2 : Ite.RequireNodes)
			{
				if (Writter.IsWritting())
					DescOffset.push_back(Writter.GetWritedSize() - CurNodeOffset);

				ZipRequireNodeDescT NodeDesc;
				Misc::CrossTypeSetThrow<OutOfRange>(NodeDesc.RequireNodeCount, Ite2.size(), OutOfRange::TypeT::RequireNodeCount, Ite2.size());

				Writter.WriteObject(NodeDesc);

				auto RequireNodeOffset = Writter.NewObjectArray<ZipRequireNodeT>(Ite2.size());

				auto Reader = Writter.GetReader();

				if (Reader.has_value())
				{
					Reader->SetPointer(RequireNodeOffset);
					auto Node = Reader->ReadObjectArray<ZipRequireNodeT>(Ite2.size());
					for (std::size_t I = 0; I < Node.size(); ++I)
					{
						auto& Target = Node[I];
						auto& Source = Ite2[I];
						Target.IsEndOfFile = Source.RequireSymbol.IsEndOfFile();
						Target.Type = Source.Type;
						Misc::CrossTypeSetThrow<OutOfRange>(Target.Value, Source.RequireSymbol.Value, OutOfRange::TypeT::SymbolValue, Source.RequireSymbol.Value);
						RequireNodeOffsetRecord Record{ RequireNodeOffset, Ite2.size(), I, Source.ReferenceIndex };
						switch (Target.Type)
						{
						case LRX::RequireNodeType::SymbolValue:
							SymbolsRecord.push_back(Record);
							break;
						case LRX::RequireNodeType::NeedPredictShiftProperty:
						case LRX::RequireNodeType::ShiftProperty:
							ShiftRecord.push_back(Record);
							break;
						case LRX::RequireNodeType::ReduceProperty:
							ReduceRecord.push_back(Record);
							break;
						default:
							assert(false);
							break;
						}
					}
				}
			}

			std::vector<std::size_t> ReduceOffset;

			for (auto& Ite2 : Ite.Reduces)
			{
				ZipReducePropertyT Pro;
				Misc::CrossTypeSetThrow<OutOfRange>(Pro.Mask, Ite2.Property.Reduce.Mask, OutOfRange::TypeT::Mask, Ite2.Property.Reduce.Mask);
				Misc::CrossTypeSetThrow<OutOfRange>(Pro.NoTerminalValue, Ite2.Property.ReduceSymbol.Value, OutOfRange::TypeT::SymbolValue, Ite2.Property.ReduceSymbol.Value);
				Misc::CrossTypeSetThrow<OutOfRange>(Pro.ProductionIndex, Ite2.Property.Reduce.ProductionIndex, OutOfRange::TypeT::ReduceProperty, Ite2.Property.Reduce.ProductionIndex);
				Misc::CrossTypeSetThrow<OutOfRange>(Pro.ProductionCount, Ite2.Property.Reduce.ElementCount, OutOfRange::TypeT::ReduceProperty, Ite2.Property.Reduce.ElementCount);
				Misc::CrossTypeSetThrow<OutOfRange>(Pro.ReduceTupleCount, Ite2.Tuples.size(), OutOfRange::TypeT::ReduceProperty, Ite2.Tuples.size());
				auto ProAdress = Writter.WriteObject(Pro);
				auto TupleAdress = Writter.NewObjectArray<ZipReduceTupleT>(Ite2.Tuples.size());
				if (Writter.IsWritting())
				{
					ReduceOffset.push_back(ProAdress - CurNodeOffset);
					ReduceTupleRecord.push_back({ TupleAdress, Ite2.Tuples.size(), std::span(Ite2.Tuples) });
				}
			}

			auto Reader = Writter.GetReader();

			if (Reader.has_value())
			{
				for (auto Ite2 : SymbolsRecord)
				{
					std::size_t Tar = DescOffset[Ite2.Index];
					Reader->SetPointer(Ite2.Adress);
					auto RequireNode = Reader->ReadObjectArray<ZipRequireNodeT>(Ite2.ArrayCount);
					Misc::CrossTypeSetThrow<OutOfRange>(RequireNode[Ite2.ArrayIndex].ToIndexOffset, Tar, OutOfRange::TypeT::RequireNodeOffset, Tar);
				}
				SymbolsRecord.clear();

				for (auto Ite2 : ReduceRecord)
				{
					Reader->SetPointer(Ite2.Adress);
					auto Node = Reader->ReadObjectArray<ZipRequireNodeT>(Ite2.ArrayCount);
					std::size_t Tar = ReduceOffset[Ite2.Index];
					Misc::CrossTypeSetThrow<OutOfRange>(Node[Ite2.ArrayIndex].ToIndexOffset, Tar, OutOfRange::TypeT::RequireNodeOffset, Tar);
				}

				ReduceRecord.clear();

			}
		}

		auto Reader = Writter.GetReader();

		if (Reader.has_value())
		{
			for (auto& Ite2 : ShiftRecord)
			{
				Reader->SetPointer(Ite2.Adress);
				auto Node = Reader->ReadObjectArray<ZipRequireNodeT>(Ite2.ArrayCount);
				std::size_t Tar = NodeOffset[Ite2.Index];
				Misc::CrossTypeSetThrow<OutOfRange>(Node[Ite2.ArrayIndex].ToIndexOffset, Tar, OutOfRange::TypeT::NodeOffset, Tar);
			}

			for (auto Ite : ReduceTupleRecord)
			{
				Reader->SetPointer(Ite.Adress);
				auto Span = Reader->ReadObjectArray<ZipReduceTupleT>(Ite.Count);

				for (std::size_t I = 0; I < Span.size(); ++I)
				{
					auto& Target = Span[I];
					auto& Source = Ite.Tuples[I];
					Misc::CrossTypeSetThrow<OutOfRange>(Target.LastState, NodeOffset[Source.LastState], OutOfRange::TypeT::RequireNodeOffset, NodeOffset[Source.LastState]);
					Misc::CrossTypeSetThrow<OutOfRange>(Target.ToState, NodeOffset[Source.TargetState], OutOfRange::TypeT::RequireNodeOffset, NodeOffset[Source.TargetState]);
					Target.NeedPredict = (Source.NeedPredict ? 1 : 0);
				}
			}

			Reader->SetPointer(0);
			auto Head = Reader->ReadObject<ZipHeadT>();
			Misc::CrossTypeSetThrow<OutOfRange>(Head->StartupOffset, NodeOffset[1], OutOfRange::TypeT::NodeOffset, NodeOffset[1]);
		}
	}

	auto LRXBinaryTableWrapper::Create(LRX const& Le) -> std::vector<StandardT>
	{
		Misc::StructedSerilizerWritter<StandardT> Predict;

		Serilize(Predict, Le);
		std::vector<StandardT> Re;
		Re.resize(Predict.GetWritedSize());
		auto Span = std::span(Re);
		Misc::StructedSerilizerWritter<StandardT> Writter{Span};
		Serilize(Writter, Le);

		return Re;
	}

	bool CoreProcessor::Consume(Symbol Value, Misc::IndexSpan<> TokenIndex, std::any AppendData)
	{
		assert(Value.IsTerminal());
		assert(ProcessorContext.States.rbegin()->TableState == ProcessorContext.CurrentTopState);

		std::optional<ConsumeResult> Result = TableConsume(Value);

		if (Result.has_value())
		{
			ProcessorContext.CacheSymbols.push_back(ProcessElement{ Result->State, SymbolElement{Value, TokenIndex}, {}, std::move(AppendData) });
			std::size_t SymbolsIndex = 0;
			while (SymbolsIndex <= ProcessorContext.CacheSymbols.size())
			{
				if (Result->Reduce.has_value())
				{
					Misc::IndexSpan<> Cur;
					auto Productions = std::span(ProcessorContext.States).subspan(ProcessorContext.States.size() - Result->Reduce->Reduce.ElementCount);
					for (auto& Ite : Productions)
					{
						if (Ite.Value.TokenIndex.Size() != 0)
						{
							if (Cur.Size() == 0)
							{
								Cur = Ite.Value.TokenIndex;
							}
							else {
								Cur = Cur.Expand(Ite.Value.TokenIndex);
							}
						}
					}

					SymbolElement SElement{
						Result->Reduce->ReduceSymbol,
						Cur,
					};

					ReduceDescription Desc{
						Result->Reduce->Reduce.ElementCount,
						Result->Reduce->Reduce.ProductionIndex,
						Result->Reduce->Reduce.Mask
					};

					ReduceProduction Pro{
						Desc,
						Productions
					};

					auto AppendData = HandleReduce(SElement, Pro);

					ProcessorContext.States.resize(ProcessorContext.States.size() - Result->Reduce->Reduce.ElementCount);
					ProcessorContext.States.push_back(
						ProcessElement{
							Result->State,
							SElement,
							Desc,
							std::move(AppendData)
						}
					);
					ProcessorContext.RequireNode = Result->RequireNode;
					ProcessorContext.CurrentTopState = Result->State;
					SymbolsIndex = 0;
					TryReduce();
				}
				else {
					if (Result->RequireNode == 0)
					{
						auto LastSymbol = std::move(*ProcessorContext.CacheSymbols.begin());
						auto LastSymbolValue = LastSymbol.Value;
						ProcessorContext.CacheSymbols.pop_front();
						LastSymbol.TableState = Result->State;
						ProcessorContext.States.push_back(std::move(LastSymbol));
						ProcessorContext.RequireNode = Result->RequireNode;
						ProcessorContext.CurrentTopState = Result->State;
						SymbolsIndex = 0;
						if (LastSymbolValue.Value != Symbol::EndOfFile())
						{
							TryReduce();
						}
						else {
							assert(ProcessorContext.CacheSymbols.empty());
							assert(ProcessorContext.States.size() == 3);
						}
					}
					else {
						ProcessorContext.RequireNode = Result->RequireNode;
						return true;
					}
				}
				if (!ProcessorContext.CacheSymbols.empty())
				{
					Result = TableConsume(ProcessorContext.CacheSymbols[SymbolsIndex].Value.Value);
					++SymbolsIndex;
					assert(Result.has_value());
				}
				else {
					return true;
				}
			}
			return true;
		}
		else {
			return false;
		}
	}

	void CoreProcessor::Clear(std::size_t StartupNode)
	{
		ProcessorContext.CacheSymbols.clear();
		ProcessorContext.States.clear();
		ProcessorContext.CurrentTopState = StartupNode;
		ProcessorContext.States.push_back({ ProcessorContext.CurrentTopState, {}, {}, {}});
		ProcessorContext.RequireNode = 0;
		TryReduce();
	}

	bool CoreProcessor::EndOfFile()
	{
		auto Re = Consume(Symbol::EndOfFile(), {}, {});
		if (Re)
		{
			assert(ProcessorContext.States.size() == 3);
			return true;
		}
		return false;
	}

	std::any& CoreProcessor::GetDataRaw()
	{
		assert(ProcessorContext.States.size() == 3);
		return ProcessorContext.States[1].AppendData;
	}

	void CoreProcessor::TryReduce()
	{
		if (ProcessorContext.RequireNode == 0)
		{
			while (true)
			{
				std::optional<ReduceResult> Result = TableReduce();
				if (Result.has_value())
				{
					Misc::IndexSpan<> Cur;
					auto Productions = std::span(ProcessorContext.States).subspan(ProcessorContext.States.size() - Result->Reduce.Reduce.ElementCount);
					for (auto& Ite : Productions)
					{
						if (Ite.Value.TokenIndex.Size() != 0)
						{
							if (Cur.Size() == 0)
							{
								Cur = Ite.Value.TokenIndex;
							}
							else {
								Cur = Cur.Expand(Ite.Value.TokenIndex);
							}
						}
					}

					SymbolElement SElement{
						Result->Reduce.ReduceSymbol,
						Cur,
					};

					ReduceDescription Desc{
						Result->Reduce.Reduce.ElementCount,
						Result->Reduce.Reduce.ProductionIndex,
						Result->Reduce.Reduce.Mask
					};

					ReduceProduction Pro{
						Desc,
						Productions
					};

					auto AppendData = HandleReduce(SElement, Pro);

					ProcessorContext.States.resize(ProcessorContext.States.size() - Result->Reduce.Reduce.ElementCount);
					ProcessorContext.States.push_back(
						ProcessElement{
							Result->State,
							SElement,
							Desc,
							std::move(AppendData)
						}
					);
					ProcessorContext.CurrentTopState = Result->State;
				}
				else {
					break;
				}
			}
		}
	}


	std::optional<CoreProcessor::ConsumeResult> LRXCoreProcessor::TableConsume(Symbol Value) const
	{
		assert(Value.IsTerminal());
		assert(Table.Nodes.size() > ProcessorContext.CurrentTopState);
		assert(ProcessorContext.States.rbegin()->TableState == ProcessorContext.CurrentTopState);

		auto& NodeRef = Table.Nodes[ProcessorContext.CurrentTopState];

		assert(NodeRef.RequireNodes.size() > ProcessorContext.RequireNode);

		auto& NodeRequireArray = NodeRef.RequireNodes[ProcessorContext.RequireNode];

		assert(!NodeRequireArray.empty());

		for (auto& Ite : NodeRequireArray)
		{
			if (Ite.RequireSymbol == Value)
			{
				switch (Ite.Type)
				{
				case LRX::RequireNodeType::SymbolValue:
				{
					ConsumeResult Re;
					Re.State = ProcessorContext.CurrentTopState;
					Re.RequireNode = Ite.ReferenceIndex;
					return Re;
				}
				case LRX::RequireNodeType::NeedPredictShiftProperty:
				case LRX::RequireNodeType::ShiftProperty:
				{
					ConsumeResult Re;
					Re.State = Ite.ReferenceIndex;
					Re.RequireNode = 0;
					return Re;
				}
				case LRX::RequireNodeType::ReduceProperty:
				{
					ConsumeResult Re;
					assert(NodeRef.Reduces.size() > Ite.ReferenceIndex);
					auto& CurReduceTuple = NodeRef.Reduces[Ite.ReferenceIndex];
					Re.Reduce = CurReduceTuple.Property;
					assert(ProcessorContext.States.size() > Re.Reduce->Reduce.ElementCount);
					auto RefState = ProcessorContext.States[ProcessorContext.States.size() - Re.Reduce->Reduce.ElementCount - 1].TableState;
					auto FindIte = std::find_if(CurReduceTuple.Tuples.begin(), CurReduceTuple.Tuples.end(), [=](LRX::ReduceTuple Tupe) {
						return Tupe.LastState == RefState;
						});
					assert(FindIte != CurReduceTuple.Tuples.end());
					Re.State = FindIte->TargetState;
					Re.RequireNode = 0;
					return Re;
				}
				}
			}
		}

		if (EnableSuggest)
		{
			for (auto Ite : NodeRequireArray)
				GetSuggest(Ite.RequireSymbol);
		}

		return {};
	}

	std::optional<CoreProcessor::ReduceResult> LRXCoreProcessor::TableReduce() const
	{
		assert(Table.Nodes.size() > ProcessorContext.CurrentTopState);
		auto& NodeRef = Table.Nodes[ProcessorContext.CurrentTopState];
		if (NodeRef.RequireNodes.empty())
		{
			assert(NodeRef.Reduces.size() <= 1);
			if (NodeRef.Reduces.size() == 1)
			{
				auto& Ref = *NodeRef.Reduces.begin();
				assert(ProcessorContext.States.size() > Ref.Property.Reduce.ElementCount);
				auto LastState = ProcessorContext.States[ProcessorContext.States.size() - Ref.Property.Reduce.ElementCount - 1].TableState;
				auto FindIte = std::find_if(Ref.Tuples.begin(), Ref.Tuples.end(), [=](LRX::ReduceTuple Tup) {
					return Tup.LastState == LastState;
					});
				assert(FindIte != Ref.Tuples.end());
				ReduceResult Result;
				Result.Reduce = Ref.Property;
				Result.State = FindIte->TargetState;
				return Result;
			}
		}
		return {};
	}

	auto LRXBinaryTableCoreProcessor::TableConsume(Symbol Value)  const ->std::optional<ConsumeResult>
	{
		assert(Table.TotalBufferSize() > ProcessorContext.CurrentTopState);
		assert(ProcessorContext.States.rbegin()->TableState == ProcessorContext.CurrentTopState);

		auto Reader = Table.GetReader();

		Reader.SetPointer(ProcessorContext.CurrentTopState);

		auto Node = Reader.ReadObject<LRXBinaryTableWrapper::ZipNodeT>();

		assert(Node->RequireNodeDescCount != 0);

		auto NodeDescReader = Reader.SubReader(ProcessorContext.RequireNode);

		auto Desc = NodeDescReader.ReadObject<LRXBinaryTableWrapper::ZipRequireNodeDescT>();

		assert(Desc->RequireNodeCount != 0);

		auto RequireNodes = NodeDescReader.ReadObjectArray<LRXBinaryTableWrapper::ZipRequireNodeT>(Desc->RequireNodeCount);

		if (Value.IsTerminal())
		{
			for (auto& Ite : RequireNodes)
			{
				if (Ite.IsEndOfFile && Value.IsEndOfFile() || (!Ite.IsEndOfFile && !Value.IsEndOfFile() && Ite.Value == Value.Value))
				{
					switch (Ite.Type)
					{
					case LRX::RequireNodeType::SymbolValue:
					{
						ConsumeResult Re;
						Re.State = ProcessorContext.CurrentTopState;
						Re.RequireNode = Ite.ToIndexOffset;
						return Re;
					}
					case LRX::RequireNodeType::NeedPredictShiftProperty:
					case LRX::RequireNodeType::ShiftProperty:
					{
						ConsumeResult Re;
						Re.State = Ite.ToIndexOffset;
						Re.RequireNode = 0;
						return Re;
					}
					case LRX::RequireNodeType::ReduceProperty:
					{
						ConsumeResult Re;
						auto ReducePropertyReader = Reader.SubReader(Ite.ToIndexOffset);
						auto ReduceP = ReducePropertyReader.ReadObject<LRXBinaryTableWrapper::ZipReducePropertyT>();
						LR0::Reduce Reduce;
						Reduce.ReduceSymbol = Symbol::AsNoTerminal(ReduceP->NoTerminalValue);
						Reduce.Reduce.Mask = ReduceP->Mask;
						Reduce.Reduce.ProductionIndex = ReduceP->ProductionIndex;
						Reduce.Reduce.ElementCount = ReduceP->ProductionCount;
						Re.Reduce = Reduce;
						assert(ProcessorContext.States.size() > Re.Reduce->Reduce.ElementCount);
						auto RefState = ProcessorContext.States[ProcessorContext.States.size() - Re.Reduce->Reduce.ElementCount - 1].TableState;
						auto ReduceTuples = ReducePropertyReader.ReadObjectArray<LRXBinaryTableWrapper::ZipReduceTupleT>(ReduceP->ReduceTupleCount);
						auto FindIte = std::find_if(ReduceTuples.begin(), ReduceTuples.end(), [=](LRXBinaryTableWrapper::ZipReduceTupleT Tupe) {
							return Tupe.LastState == RefState;
							});
						assert(FindIte != ReduceTuples.end());
						Re.State = FindIte->ToState;
						Re.RequireNode = 0;
						return Re;
					}
					}
				}
			}
		}

		if (EnableSuggest)
		{
			for (auto Ite : RequireNodes)
			{
				if (Ite.IsEndOfFile)
				{
					GetSuggest(Symbol::EndOfFile());
				}
				else {
					GetSuggest(Symbol::AsTerminal(Ite.Value));
				}
			}
		}

		return {};
	}

	auto LRXBinaryTableCoreProcessor::TableReduce() const -> std::optional<ReduceResult>
	{
		assert(Table.TotalBufferSize() > ProcessorContext.CurrentTopState);
		assert(ProcessorContext.States.rbegin()->TableState == ProcessorContext.CurrentTopState);
		auto NodeReader = Table.GetReader();
		NodeReader.SetPointer(ProcessorContext.CurrentTopState);
		auto Node = NodeReader.ReadObject<LRXBinaryTableWrapper::ZipNodeT>();
		if (Node->RequireNodeDescCount == 0)
		{
			assert(Node->ReduceCount <= 1);
			if (Node->ReduceCount == 1)
			{
				auto ReduceP = NodeReader.ReadObject<LRXBinaryTableWrapper::ZipReducePropertyT>();
				LR0::Reduce Reduce;
				Reduce.ReduceSymbol = Symbol::AsNoTerminal(ReduceP->NoTerminalValue);
				Reduce.Reduce.Mask = ReduceP->Mask;
				Reduce.Reduce.ProductionIndex = ReduceP->ProductionIndex;
				Reduce.Reduce.ElementCount = ReduceP->ProductionCount;
				ReduceResult Result;
				Result.Reduce = Reduce;
				assert(ProcessorContext.States.size() > Reduce.Reduce.ElementCount);
				auto RefState = ProcessorContext.States[ProcessorContext.States.size() - Reduce.Reduce.ElementCount - 1].TableState;
				auto ReduceTuples = NodeReader.ReadObjectArray<LRXBinaryTableWrapper::ZipReduceTupleT>(ReduceP->ReduceTupleCount);
				auto FindIte = std::find_if(ReduceTuples.begin(), ReduceTuples.end(), [=](LRXBinaryTableWrapper::ZipReduceTupleT Tupe) {
					return Tupe.LastState == RefState;
					});
				assert(FindIte != ReduceTuples.end());
				Result.State = FindIte->ToState;
				return Result;
			}

		}
		return {};
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
		char const* OutOfRange::what() const { return "PotatoSLRXException::OutOfRange"; };
	}

}