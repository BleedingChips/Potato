#include "../Public/PotatoDLr.h"
#include <algorithm>
#include <cassert>
#include <numbers>
#include <set>
namespace Potato::DLr
{

	using namespace Exception;

	TElement::TElement(Step const& value) : Value(value.Value), TokenIndex(value.Shift.TokenIndex) {}

	NTElement::NTElement(Step const& StepValue, std::size_t FirstTokenIndex, NTElementData* DataPtr) :
		Value(StepValue.Value), ProductionIndex(StepValue.Reduce.ProductionIndex), Mask(StepValue.Reduce.Mask), 
		FirstTokenIndex(FirstTokenIndex), Datas(DataPtr, StepValue.Reduce.ProductionCount)
	{}

	std::size_t StepProcessor::Consume(Step Input, std::any(*F1)(StepElement, void* Data), void* Data)
	{
		if (Input.IsTerminal())
		{
			TElement Ele{ Input };
			auto Result = F1(StepElement{std::move(Ele)}, Data);
			DataBuffer.push_back({ Ele.Value, Ele.TokenIndex, std::move(Result) });
			LastTokenIndex = Ele.TokenIndex;
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
				auto Result = F1(StepElement{ std::move(ele) }, Data);

				DataBuffer.resize(CurrentAdress);
				DataBuffer.push_back({ ele.Value, TokenFirstIndex, std::move(Result) });
			}
			else {
				throw Exception::UnsupportedStep(Input.Value, LastTokenIndex);
			}
		}
		return DataBuffer.size();
	}

	std::any StepProcessor::EndOfFile()
	{
		if (DataBuffer.size() == 1)
		{
			return DataBuffer[0].Consume();
		}else
			throw Exception::UnsupportedStep{Symbol::EndOfFile(), LastTokenIndex };
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
				throw Exception::BadProduction{ Exception::BadProduction::Category::UnaccaptableSymbol, ProductionDescs.size(), 0};
			}
			Production Desc;
			Desc.Symbol = Ite.ProductionValue;
			Desc.ProductionMask = Ite.ProductionMask;
			std::size_t ElementIndex = 0;
			for (auto& Ite2 : Ite.Element)
			{
				if (Ite2.IsMask)
				{
					if (Desc.Elements.size() == 0 || Desc.Elements.rbegin()->Symbol.IsTerminal())
					{
						throw Exception::BadProduction{ Exception::BadProduction::Category::WrongProductionMask, ProductionDescs.size(), ElementIndex };
					}
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

	UnserilizeTable::UnserilizeTable(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, bool ForceLR0)
	{

		ProductionInfo Infos(StartSymbol, std::move(Production), std::move(Priority));
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

			if (ForceLR0) [[unlikely]]
			{
				if(
					TemporaryNode.Reduces.size() >= 2 
					|| (!TemporaryNode.Reduces.empty() && !TemporaryNode.Shifts.empty())
					) [[unlikely]]
					throw Exception::BadProduction{ Exception::BadProduction::Category::NotLR0Production, 0, 0};
			}

			Nodes.push_back(std::move(TemporaryNode));
		}
	}

	auto TableWrapper::Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, bool ForceLR0) ->std::vector<SerilizedT>
	{
		UnserilizeTable Temp(StartSymbol, std::move(Production), std::move(Priority), ForceLR0);
		return TableWrapper::Create(Temp);
	}

	TableWrapper::SerilizeSymbol TableWrapper::TranslateSymbol(Symbol Input)
	{
		TableWrapper::SerilizeSymbol NewSymbol;

		NewSymbol.Type = static_cast<decltype(NewSymbol.Type)>(Input.Type);
		NewSymbol.Value = static_cast<decltype(NewSymbol.Value)>(Input.Value);
		if((static_cast<std::size_t>(NewSymbol.Type) != static_cast<std::size_t>(Input.Type)) ||
			NewSymbol.Value != Input.Value
			) [[unlikely]]
		{
			throw OutOfRange{ OutOfRange::TypeT::Symbol, Input.Value };
		}
		return NewSymbol;
	}

	Symbol TableWrapper::TranslateSymbol(SerilizeSymbol Input)
	{
		Symbol NewSymbol;
		NewSymbol.Type = static_cast<decltype(NewSymbol.Type)>(Input.Type);
		NewSymbol.Value = static_cast<decltype(NewSymbol.Value)>(Input.Value);
		return NewSymbol;
	}

	auto TableWrapper::Create(UnserilizeTable const& Table) ->std::vector<SerilizedT>
	{

		std::vector<SerilizedT> ResultBuffer;
		std::vector<std::size_t> NodeOffset;
		struct ShiftRecord
		{
			std::size_t ShiftDataOffset;
			std::size_t RequireNode;
		};
		std::vector<ShiftRecord> Records;
		ResultBuffer.resize(2);
		Misc::SerilizerHelper::TryCrossTypeSet(ResultBuffer[0], Table.Nodes.size(), OutOfRange{ OutOfRange ::TypeT::NodeCount, Table.Nodes.size() });

		std::vector<SerlizedReduce> SerlizedReduces;
		std::vector<HalfSeilizeT> AcceptableProductionIndex;

		for (auto& Ite : Table.Nodes)
		{
			SerlizedNode TemporaryNode;
			Misc::SerilizerHelper::TryCrossTypeSet(TemporaryNode.ReduceCount, Ite.Reduces.size(), OutOfRange{ OutOfRange::TypeT::ReduceCount, Ite.Reduces.size() });
			Misc::SerilizerHelper::TryCrossTypeSet(TemporaryNode.ShiftCount, Ite.Shifts.size(), OutOfRange{ OutOfRange::TypeT::ShiftCount, Ite.Shifts.size() });
			
			auto Result = Misc::SerilizerHelper::WriteObject(ResultBuffer, TemporaryNode);
			NodeOffset.push_back(Result.StartOffset);

			SerlizedReduces.clear();
			for (auto& ReduceIte : Ite.Reduces)
			{
				SerlizedReduce NewReduce;
				NewReduce.Symbol = TranslateSymbol(ReduceIte.ReduceSymbol);
				Misc::SerilizerHelper::TryCrossTypeSet(NewReduce.ProductionIndex, ReduceIte.ProductionIndex, OutOfRange{ OutOfRange::TypeT::ProductionIndex, ReduceIte.ProductionIndex });
				Misc::SerilizerHelper::TryCrossTypeSet(NewReduce.ProductionElementCount, ReduceIte.ProductionElementCount, OutOfRange{ OutOfRange::TypeT::ProductionElementIndex, ReduceIte.ProductionIndex });
				Misc::SerilizerHelper::TryCrossTypeSet(NewReduce.Mask, ReduceIte.ReduceMask, OutOfRange{ OutOfRange::TypeT::Mask, ReduceIte.ReduceMask });

				SerlizedReduces.push_back(NewReduce);
			}

			auto ResuceResult = Misc::SerilizerHelper::WriteObjectArray(ResultBuffer, std::span(SerlizedReduces));

			for (auto& ShiftIte : Ite.Shifts)
			{
				SerlizedShift NewShift;
				NewShift.Symbol = TranslateSymbol(ShiftIte.RequireSymbol);
				NewShift.ToNodeOffset = 0;
				NewShift.ReverseStorage = ShiftIte.ReverseStorage == 0 ? 0 : 1;
				NewShift.AcceptableProductionCount = static_cast<decltype(NewShift.AcceptableProductionCount)>(ShiftIte.ProductionIndex.size());
				if (
					NewShift.ReverseStorage != ShiftIte.ReverseStorage ||
					NewShift.AcceptableProductionCount != ShiftIte.ProductionIndex.size()
					) [[unlikely]]
				{
					throw OutOfRange{ OutOfRange::TypeT::AcceptableProductionCount, ShiftIte.ProductionIndex.size() };
				}

				AcceptableProductionIndex.clear();
				AcceptableProductionIndex.reserve(ShiftIte.ProductionIndex.size());

				for (auto AcceptableIte : ShiftIte.ProductionIndex)
				{
					auto TempAcceptableIndex = static_cast<decltype(AcceptableProductionIndex)::value_type>(AcceptableIte);
					if(TempAcceptableIndex != AcceptableIte) [[unlikely]]
						throw OutOfRange{ OutOfRange::TypeT::AcceptableProductionIndex, AcceptableIte };
					AcceptableProductionIndex.push_back(TempAcceptableIndex);
				}

				auto ShiftOffset = Misc::SerilizerHelper::WriteObject(ResultBuffer, NewShift);
				Records.push_back({ ShiftOffset.StartOffset, ShiftIte.ToNode});

				Misc::SerilizerHelper::WriteObjectArray(ResultBuffer, std::span(AcceptableProductionIndex));
			}

		}

		for (auto& Ite : Records)
		{
			auto Re = Misc::SerilizerHelper::ReadObject<SerlizedShift>(std::span(ResultBuffer).subspan(Ite.ShiftDataOffset));
			Misc::SerilizerHelper::TryCrossTypeSet(Re->ToNodeOffset, NodeOffset[Ite.RequireNode], OutOfRange{ OutOfRange::TypeT::NodeOffset, NodeOffset[Ite.RequireNode] });
		}

		Misc::SerilizerHelper::TryCrossTypeSet(ResultBuffer[1], NodeOffset[1], OutOfRange{ OutOfRange::TypeT::NodeOffset, NodeOffset[1] });

#ifdef _DEBUG
		{
			TableWrapper Wrapper(ResultBuffer);

			struct ShiftEdge
			{
				std::size_t From;
				std::size_t To;
				ShiftViewer Viewer;
			};

			struct ReduceEdge
			{
				std::size_t Owner;
				SerlizedReduce Viewer;
			};

			std::vector<ShiftEdge> ShiftEdges;
			std::vector<ReduceEdge> ReduceEdges;

			for (std::size_t Index = 0; Index < NodeOffset.size(); ++Index)
			{
				auto TableNode = Wrapper.ReadNode(static_cast<SerilizedT>(NodeOffset[Index]));
				for (auto Ite = TableNode.Reduces.rbegin(); Ite != TableNode.Reduces.rend(); ++Ite)
				{
					ReduceEdges.push_back({ TableNode.NodeOffset, *Ite });
				}
				auto ShiftSpan = TableNode.ShiftDatas;
				for (std::size_t Index = 0; Index < TableNode.ShiftCount; ++Index)
				{
					auto [Viewer, Last] = TableWrapper::ReadShiftFromSpan(ShiftSpan);
					ShiftEdges.push_back({ TableNode.NodeOffset, Viewer.NodeOffset, Viewer });
					ShiftSpan = Last;
				}
			}
			volatile int i = 0;
		}
#endif

		return ResultBuffer;
	}

	auto TableWrapper::ReadNodeFromSpan(std::span<SerilizedT const> Input) const ->NodeViewer
	{
		auto Node = Misc::SerilizerHelper::ReadObject<SerlizedNode const>(Input);
		NodeViewer Viewer;
		Viewer.NodeOffset = static_cast<SerilizedT>(Input.data() - Wrapper.data());
		auto Result = Misc::SerilizerHelper::ReadObjectArray<SerlizedReduce const>(Node.LastSpan, Node->ReduceCount);
		Viewer.Reduces = Result.Result;
		Viewer.ShiftDatas = Result.LastSpan;
		Viewer.ShiftCount = Node->ShiftCount;
		return Viewer;
	}

	auto TableWrapper::ReadShiftFromSpan(std::span<SerilizedT const> ShiftData) -> std::tuple<ShiftViewer, std::span<SerilizedT const>>
	{
		auto ShiftResult = Misc::SerilizerHelper::ReadObject<SerlizedShift const>(ShiftData);
		ShiftViewer Viewer;
		Viewer.ShiftSymbol = ShiftResult->Symbol;
		Viewer.NodeOffset = ShiftResult->ToNodeOffset;
		auto AccptableResult = Misc::SerilizerHelper::ReadObjectArray<HalfSeilizeT const>(ShiftResult.LastSpan, ShiftResult->AcceptableProductionCount);
		Viewer.AcceptableProduction = AccptableResult.Result;
		Viewer.ReserverMeaning = ShiftResult->ReverseStorage;
		return { Viewer, AccptableResult.LastSpan };
	}

	auto TableWrapper::NodeViewer::Acceptable(Symbol ShiftValue, std::size_t ProductionIndex) const -> std::optional<SerilizedT>
	{
		auto ShiftSpan = ShiftDatas;
		for (std::size_t Index = 0; Index < ShiftCount; ++Index)
		{
			auto [Viewer, Last] = ReadShiftFromSpan(ShiftSpan);
			if (Viewer.ShiftSymbol == ShiftValue)
			{
				if (Viewer.AcceptableProduction.size() == 0)
				{
					return Viewer.NodeOffset;
				}
				else {
					auto Inside = std::find(Viewer.AcceptableProduction.begin(), Viewer.AcceptableProduction.end(), ProductionIndex) != Viewer.AcceptableProduction.end();
					if (Viewer.ReserverMeaning != Inside)
						return Viewer.NodeOffset;
				}
			}
			ShiftSpan = Last;
		}
		return {};
	}

	bool CoreProcessor::ConsumeSymbol(Symbol InputSymbol, std::size_t TokenIndex, bool InputSaveStep)
	{
		assert(!States.empty());
		auto Node = Wrapper.ReadNode(*States.rbegin());
		auto Acceptable = Node.Acceptable(InputSymbol, 0);
		if (!Node.Reduces.empty())
		{
			for (std::size_t ReduceIndex = Node.Reduces.size(); ReduceIndex > 0; --ReduceIndex)
			{
				auto ReduceIte = Node.Reduces[ReduceIndex - 1];
				assert(States.size() > ReduceIte.ProductionElementCount);
				std::size_t ReduceStateSize = (States.size() - ReduceIte.ProductionElementCount);
				assert(ReduceStateSize != 0);

				Step SStep;
				SStep.Value = TableWrapper::TranslateSymbol(ReduceIte.Symbol);
				SStep.Reduce.ProductionIndex = ReduceIte.ProductionIndex;
				SStep.Reduce.ProductionCount = ReduceIte.ProductionElementCount;
				SStep.Reduce.Mask = ReduceIte.Mask;

				std::size_t MaxStateCount = ReduceStateSize;
				if (!AmbiguousPoints.empty())
				{
					if(AmbiguousPoints.rbegin()->MaxState > MaxStateCount)
						MaxStateCount = AmbiguousPoints.rbegin()->MaxState;
				}

				AmbiguousPoints.emplace_back(
					ReduceStateSize,
					MaxStateCount,
					Steps.size(),
					TokenIndex,
					SStep
				);
			}
		}
		if (Acceptable.has_value())
		{
			States.emplace_back(*Acceptable);
			if (InputSaveStep)
			{
				Step SStep;
				SStep.Value = InputSymbol;
				SStep.Shift.TokenIndex = TokenIndex;
				Steps.push_back(SStep);
			}
			return true;
		}
		else {
			return false;
		}
	}

	auto CoreProcessor::ConsumAmbiguousPoint(CoreProcessor& Output)
		-> std::optional<AmbiguousPoint>
	{
		if (!AmbiguousPoints.empty())
		{
			std::optional<CoreProcessor> ReturnPro;
			auto AP = *AmbiguousPoints.rbegin();
			AmbiguousPoints.pop_back();
			if (!AmbiguousPoints.empty() && AP.MaxState > AP.StateIndex)
			{
				auto LastAp = *AmbiguousPoints.rbegin();
				auto StepSpa = std::span(Steps).subspan(0, LastAp.StepsIndex);
				auto StatSpa = std::span(States).subspan(0, LastAp.MaxState);
				Output.States.insert(Output.States.end(), StatSpa.begin(), StatSpa.end());
				Output.Steps.insert(Output.Steps.end(), StepSpa.begin(), StepSpa.end());
				Output.AmbiguousPoints = std::move(AmbiguousPoints);
			}
			States.resize(AP.StateIndex);
			Steps.resize(AP.StepsIndex);
			auto Node = Wrapper.ReadNode(*States.rbegin());
			auto Acceptable = Node.Acceptable(AP.Step.Value, AP.Step.Reduce.ProductionIndex);
			assert(Acceptable.has_value());
			States.push_back(*Acceptable);
			Steps.push_back(AP.Step);
			return AP;
		}
		return {};
	}

	auto CoreProcessor::Flush(std::optional<std::size_t> Startup, std::size_t End,
		Symbol(* F1)(std::size_t, void* Data), void* Data, bool (*F2)(AmbiguousPoint, CoreProcessor&, void* Data2), void* Data2) -> std::optional<FlushResult>
	{
		bool NeedAmbiguousPoint = !Startup.has_value();
		std::size_t Ite = NeedAmbiguousPoint ? 0 : *Startup;
		CoreProcessor Temporary(Wrapper);
		while (Ite < End)
		{
			if (NeedAmbiguousPoint)
			{
				NeedAmbiguousPoint = false;
				Temporary.Clear();
				auto AP = ConsumAmbiguousPoint(Temporary);
				if (AP.has_value())
				{
					Ite = AP->TokenIndex;
					if (!F2(*AP, Temporary, Data2))
					{
						NeedAmbiguousPoint = true;
						continue;
					}
				}
				else {
					return {};
				}
			}

			for (; Ite < End; ++Ite)
			{
				auto Input = F1(Ite, Data);
				if (!Input.IsEndOfFile())
				{
					if (!ConsumeSymbol(Input, Ite))
					{
						NeedAmbiguousPoint = true;
						break;
					}
				}
				else {
					if (ConsumeSymbol(Input, Ite, false))
					{
						return FlushResult{true};
					}
					else {
						NeedAmbiguousPoint = true;
						break;
					}
				}
			}
		}
		return FlushResult{false};
	}

	std::vector<Step> ProcessSymbol(TableWrapper Wrapper, std::size_t Startup, Symbol(*F1)(std::size_t, void* Data), void* Data)
	{
		struct SearchCore
		{
			std::size_t Age = 0;
			std::optional<std::size_t> Startup;
			std::size_t End;
			CoreProcessor Processor;
		};
		
		std::vector<CoreProcessor> MainProcessor;
		std::vector<Step> Steps;
		std::vector<SearchCore> SearchList;
		std::vector<Processor::Walkway> Walkways;
		std::size_t CurrentAge = 0;
		bool ReachEnd = false;

		{
			CoreProcessor StartupProcessor(Wrapper);
			StartupProcessor.States.push_back(Wrapper.StartupNode());
			MainProcessor.emplace_back(std::move(StartupProcessor));
		}

		for (std::size_t Index = Startup; !ReachEnd; ++Index)
		{
			CurrentAge = 0;
			std::size_t Age = 0;
			std::size_t CurrentEnd = Index + 1;
			std::size_t LastState = 0;
			if(!MainProcessor.empty())
				LastState = *MainProcessor.begin()->States.rbegin();
			else
				LastState = Wrapper.StartupNode();
			while (!MainProcessor.empty())
			{
				SearchList.emplace_back(Age, Index, CurrentEnd, std::move(*MainProcessor.rbegin()));
				MainProcessor.pop_back();
				++Age;
			}
			while (!SearchList.empty())
			{
				auto Cur = std::move(*SearchList.rbegin());
				SearchList.pop_back();
				if (CurrentAge != Cur.Age)
				{
					CurrentAge = Cur.Age;
					Walkways.clear();
				}
				auto Result = Cur.Processor.Flush(Cur.Startup, Cur.End, F1, Data, [&](CoreProcessor::AmbiguousPoint AP, CoreProcessor& Pro)->bool {
					Processor::Walkway WW;
					WW.StateCount = AP.StateIndex;
					WW.CachedSymbolIndex = AP.TokenIndex;
					WW.LastState = *Cur.Processor.States.rbegin();
					auto FindIte = std::find_if(Walkways.begin(), Walkways.end(), [&](Processor::Walkway ww)->bool {
						return WW <= ww;
						});
					if (Pro)
						SearchList.emplace_back(Cur.Age, std::optional<std::size_t>{}, CurrentEnd, std::move(Pro));
					if (FindIte == Walkways.end() || *FindIte != WW)
					{
						Walkways.insert(FindIte, WW);
						return true;
					}
					else
						return false;
					});
				if (Result.has_value())
				{
					if (!Result->ReachEndOfFile)
					{
						bool Find = false;
						for (auto& Ite : MainProcessor)
						{
							if (Ite.States == Cur.Processor.States)
							{
								Find = true;
								break;
								MainProcessor.push_back(std::move(Cur.Processor));
							}
						}
						if (!Find)
						{
							MainProcessor.push_back(std::move(Cur.Processor));
						}
						else {
							SearchList.emplace_back(Cur.Age, std::optional<std::size_t>{}, CurrentEnd, std::move(Cur.Processor));
							continue;
						}
					}
					else {
						if (Cur.Processor.States.size() == 3)
						{
							Steps.insert(Steps.end(), Cur.Processor.Steps.begin(), Cur.Processor.Steps.end());
							return Steps;
						}
					}
				}
			}
			if (MainProcessor.empty())
			{
				auto Sym = (*F1)(Index, Data);
				throw Exception::UnaccableSymbol{ Sym, LastState, Index};
			}
			else if(MainProcessor.size() == 1 && MainProcessor[0].AmbiguousPoints.empty())
			{
				auto& Ref = MainProcessor[0].Steps;
				Steps.insert(Steps.end(), Ref.begin(), Ref.end());
				Ref.clear();
			}
		}
		return Steps;
	}

	std::vector<Step> ProcessSymbol(TableWrapper Wrapper, std::span<Symbol const> Input)
	{
		return ProcessSymbol(Wrapper, 0, [=](std::size_t Index) -> Symbol{
			return (Index < Input.size()) ? Input[Index] : Symbol::EndOfFile();
		});
	}

	/*
	struct CoreProcessorConsumeContent
	{

		struct WalkWay
		{	
			std::size_t StateCount;
			std::size_t CachedSymbolIndex;
			TableWrapper::SerilizedT LastState;
			std::strong_ordering operator<=>(WalkWay const&) const = default;
		};

		

		std::array<SearchCore, 7> TempProcessor;
		std::size_t TopIndex = 0;
		std::span<ProcrssorInput const> CachedSymbol;
		std::vector<WalkWay> RecordedWalkway;
		TableWrapper Wrapper;
		std::size_t LastAge = 0;

		CoreProcessorConsumeContent(TableWrapper Wrapper, std::span<ProcrssorInput const> CacheSymbol)
			: TopIndex(0), CachedSymbol(CacheSymbol), Wrapper(Wrapper)
		{

		}

		void PushProcessor(SubCoreProcessor Processor, std::size_t Age, std::optional<std::size_t> CacheStartup, std::size_t CacheEnd)
		{
			if (CacheStartup.has_value() || !Processor.AmbiguousPoints.empty())
			{
				if (TopIndex >= TempProcessor.size()) [[unlikely]]
				{
					std::size_t SymbolIndex = 0;
					if (CacheStartup.has_value())
					{
						SymbolIndex = *CacheStartup;
					}
					else {
						assert(!Processor.AmbiguousPoints.empty());
						SymbolIndex = Processor.AmbiguousPoints.rbegin()->CacheSymbolIndex;
					}
					auto Sym = CachedSymbol[SymbolIndex];
					throw Exception::MaxAmbiguityProcesser{ Sym.Input, Sym.Index, TempProcessor.size() };
				}
				TempProcessor[TopIndex] = { Age , CacheStartup, CacheEnd, std::move(Processor) };
				++TopIndex;
			}
		}
		std::optional<SubCoreProcessor> Consume();
	};

	std::optional<SubCoreProcessor> CoreProcessorConsumeContent::Consume()
	{
		while (TopIndex > 0)
		{
			SearchCore Temporary = std::move(TempProcessor[TopIndex - 1]);
			--TopIndex;
			if (Temporary.Age != LastAge)
			{
				RecordedWalkway.clear();
				LastAge = Temporary.Age;
			}
			if (!Temporary.CacheSmbolStartup.has_value())
			{
				auto Result = Temporary.Processor.ConsumAmbiguousPoint(Wrapper);
				if (Result.has_value())
				{
					auto [AP, Pro] = std::move(*Result);
					if (Pro)
					{
						assert(TopIndex < TempProcessor.size());
						TempProcessor[TopIndex] = { Temporary.Age, {}, Temporary.CacheSmbolEnd, std::move(Pro)};
						++TopIndex;
					}
					Temporary.CacheSmbolStartup = AP.CacheSymbolIndex;
					assert(Temporary.Processor);
					WalkWay WW;
					WW.CachedSymbolIndex = AP.CacheSymbolIndex;
					WW.StateCount = AP.StepsIndex;
					WW.LastState = *Temporary.Processor.States.rbegin();
					auto FindIte = std::find_if(RecordedWalkway.begin(), RecordedWalkway.end(), [=](WalkWay ww) { return WW >= ww; });
					if (FindIte == RecordedWalkway.end() || WW > *FindIte)
					{
						RecordedWalkway.insert(FindIte, WW);
					}
					else {
						Temporary.Processor.Clear();
					}
				}
			}

			if (Temporary.Processor && Temporary.CacheSmbolStartup.has_value())
			{
				for (std::size_t StartupIte = *Temporary.CacheSmbolStartup; StartupIte < Temporary.CacheSmbolEnd; ++StartupIte)
				{
					auto [Sym, SymIndex] = CachedSymbol[StartupIte];
					if (!Temporary.Processor.ConsumeSymbol(Wrapper, StartupIte, Sym, SymIndex))
					{
						if (TopIndex >= TempProcessor.size()) [[unlikely]]
						{
							auto Sym = CachedSymbol[StartupIte];
							throw Exception::MaxAmbiguityProcesser{ Sym.Input, Sym.Index, TempProcessor.size() };
						}
						TempProcessor[TopIndex] = { Temporary.Age, {}, Temporary .CacheSmbolEnd, std::move(Temporary.Processor)};
						++TopIndex;
						break;
					}
				}
			}

			if(Temporary.Processor)
				return std::move(Temporary.Processor);
		}
		return {};
	}

	std::size_t CoreProcessor::ComsumeTerminalSymbol(std::size_t CachedSymbolOffset, std::vector<Step>& OutputStep)
	{
		assert(MainProcesserIndex > 0 && MainProcesserIndex <= MainProcessor.size());
		std::size_t OldStepCount = OutputStep.size();
		CoreProcessorConsumeContent Content(Wrapper, CacheSymbols);
		for (std::size_t CacheSmbolIndex = CachedSymbolOffset; CacheSmbolIndex < CacheSymbols.size(); ++CacheSmbolIndex)
		{
			Content.RecordedWalkway.clear();
			for (std::size_t ProcessorIte = MainProcesserIndex; ProcessorIte > 0; --ProcessorIte)
			{
				assert(MainProcessor[ProcessorIte - 1]);
				Content.PushProcessor(std::move(MainProcessor[ProcessorIte - 1]), ProcessorIte, CacheSmbolIndex, CacheSmbolIndex + 1);
			}
			MainProcesserIndex = 0;
			while (auto Ite = Content.Consume())
			{
				if(MainProcesserIndex >= MainProcessor.size()) [[unlikely]]
				{
					auto Sym = CacheSymbols[CacheSmbolIndex];
					throw Exception::MaxAmbiguityProcesser{ Sym.Input, Sym.Index, MainProcessor.size() };
				}
				MainProcessor[MainProcesserIndex] = std::move(*Ite);
				++MainProcesserIndex;
			}
			auto& Ref = MainProcessor[0];
			if (MainProcesserIndex == 1 && Ref.AmbiguousPoints.empty())
			{
				OutputStep.insert(OutputStep.end(), Ref.Steps.begin(), Ref.Steps.end());
				Ref.Steps.clear();
			}
			if (MainProcesserIndex == 0)
			{
				auto Sym = CacheSymbols[CacheSmbolIndex];
				throw Exception::UnaccableSymbol{Sym.Input, Sym.Index};
			}
		}
		return OutputStep.size() - OldStepCount;
	}

	std::size_t CoreProcessor::EndOfSymbolStream(std::vector<Step>& OutputStep)
	{
		assert(MainProcesserIndex > 0 && MainProcesserIndex <= MainProcessor.size());
		std::size_t OldStepCount = OutputStep.size();
		CoreProcessorConsumeContent Content(Wrapper, CacheSymbols);
		for (std::size_t ProIndex = 0; ProIndex < MainProcesserIndex; ++ProIndex)
		{
			auto& Tem = MainProcessor[ProIndex];
			if (
				Tem.ConsumeSymbol(Wrapper, CacheSymbols.size(), Symbol::EndOfFile(), std::numeric_limits<std::size_t>::max(), false)
				&& Tem.States.size() == 3
				)
			{
				OutputStep.insert(OutputStep.end(), Tem.Steps.begin(), Tem.Steps.end());
				return OutputStep.size() - OldStepCount;
			}
			else {
				Content.PushProcessor(std::move(Tem), ProIndex, {}, CacheSymbols.size());
				while (auto Result = Content.Consume())
				{
					if (
						Result->ConsumeSymbol(Wrapper, CacheSymbols.size(), Symbol::EndOfFile(), std::numeric_limits<std::size_t>::max(), false)
						&& Result->States.size() == 3
						)
					{
						OutputStep.insert(OutputStep.end(), Result->Steps.begin(), Result->Steps.end());
						return OutputStep.size() - OldStepCount;
					}
					else {
						Content.PushProcessor(std::move(*Result), ProIndex, {}, CacheSymbols.size());
					}
				}
			}
		}
		throw Exception::UnaccableSymbol{Symbol::EndOfFile(), std::numeric_limits<std::size_t>::max()};
	}

	std::vector<Step> Processor::Process(TableWrapper Wrapper, std::span<ProcrssorInput const> Source)
	{
		std::vector<Step> Steps;
		CoreProcessor Pro(Wrapper);
		Pro.OverwriteCacheSymbol(Source);
		Pro.ComsumeTerminalSymbol(0, Steps);
		Pro.EndOfSymbolStream(Steps);
		return Steps;
	}
	*/


	namespace Exception
	{
		char const* Interface::what() const {  return "PotatoDLrException"; };
		char const* BadProduction::what() const { return "PotatoDLrException::WrongProduction"; };
		char const* OpePriorityConflict::what() const { return "PotatoDLrException::OpePriorityConflict"; };
		char const* UnaccableSymbol::what() const { return "PotatoDLrException::UnaccableSymbol"; };
		char const* MaxAmbiguityProcesser::what() const { return "PotatoDLrException::MaxAmbiguityProcesser"; };
		char const* UnsupportedStep::what() const { return "PotatoDLrException::UnsupportedStep"; };
		char const* OutOfRange::what() const { return "PotatoDLrException::OutOfRange"; };
	}

}