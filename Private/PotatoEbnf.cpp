#include <assert.h>
#include <vector>
#include "../Public/PotatoEbnf.h"
#include "../Public/PotatoStrFormat.h"

namespace Potato::Ebnf
{

	using namespace Exception;

	constexpr DLr::Symbol AsNoT(std::size_t Index) { return DLr::Symbol::AsNoTerminal(Index); }
	constexpr DLr::Symbol AsT(std::size_t Index) { return DLr::Symbol::AsTerminal(Index); }

	enum class T : std::size_t
	{
		Empty = 0,
		Terminal,
		Equal,
		Rex,
		NoTerminal,
		NoProductionNoTerminal,
		StartSymbol,
		LB_Brace,
		RB_Brace,
		LM_Brace,
		RM_Brace,
		LS_Brace,
		RS_Brace,
		LeftPriority,
		RightPriority,
		Or,
		Number,
		Start,
		Colon,
		Semicolon,
		Command,
		Barrier,
	};

	constexpr DLr::Symbol operator*(T sym) { return DLr::Symbol{ DLr::Symbol::ValueType::TERMINAL, static_cast<std::size_t>(sym) }; };

	enum class NT
	{
		Statement,
		FunctionEnum,
		Expression,
		NTExpression,
		ExpressionStatement,
		ExpressionList,
		LeftOrStatement,
		RightOrStatement,
		OrStatement,
	};

	constexpr DLr::Symbol operator*(NT sym) { return DLr::Symbol{ DLr::Symbol::ValueType::NOTERMIAL, static_cast<std::size_t>(sym) }; };

	void AddRegex(Reg::MulityRegexCreator& Creator, std::u32string_view Str, T Enum)
	{
		Creator.AddRegex(Str, {static_cast<Reg::SerilizeT>(Enum)}, Creator.GetCountedUniqueID(), false);
	}

	Reg::TableWrapper EbnfStepReg() {
		static auto List = []() {
			Reg::MulityRegexCreator Creator;
			AddRegex(Creator, UR"(%%%%)", T::Barrier);
			AddRegex(Creator, UR"([0-9]+)", T::Number);
			AddRegex(Creator, UR"([0-9a-zA-Z_\z]+)", T::Terminal);
			AddRegex(Creator, UR"(<[0-9a-zA-Z_\z]+>)", T::NoTerminal);
			AddRegex(Creator, UR"(<\$([0-9]+)>)", T::NoProductionNoTerminal);
			AddRegex(Creator, UR"(:=)", T::Equal);
			AddRegex(Creator, UR"(\'([^\s]+)\')", T::Rex);
			AddRegex(Creator, UR"(;)", T::Semicolon);
			AddRegex(Creator, UR"(:)", T::Colon);
			AddRegex(Creator, UR"(\s+)", T::Empty);
			AddRegex(Creator, UR"(\$)", T::Start);
			AddRegex(Creator, UR"(\|)", T::Or);
			AddRegex(Creator, UR"(\[)", T::LM_Brace);
			AddRegex(Creator, UR"(\])", T::RM_Brace);
			AddRegex(Creator, UR"(\{)", T::LB_Brace);
			AddRegex(Creator, UR"(\})", T::RB_Brace);
			AddRegex(Creator, UR"(\()", T::LS_Brace);
			AddRegex(Creator, UR"(\))", T::RS_Brace);
			AddRegex(Creator, UR"(\+\()", T::LeftPriority);
			AddRegex(Creator, UR"(\)\+)", T::RightPriority);
			AddRegex(Creator, UR"(/\*.*?\*/)", T::Command);
			AddRegex(Creator, UR"(//[^\n]*\n)", T::Command);
			return Creator.Generate();
		}();
		return Reg::TableWrapper(List);
	}

	/*
	struct RegDatas
	{
		char32_t Value;
		std::size_t TokenIndex;
	};

	std::vector<RegDatas> TranslateReg(Misc::IndexSpan<> Index, Reg::CodePoint(*F1)(std::size_t Index, void* Data), void* Data, bool AddEscape)
	{
		std::vector<RegDatas> Result;
		for (std::size_t Ite = Index.Begin(); Ite < Index.End(); )
		{
			auto Cur = F1(Ite, Data);
			if(AddEscape)
				Result.push_back({U'\\', Ite });
			Result.push_back({Cur.UnicodeCodePoint, Ite});
			Ite += Cur.NextUnicodeCodePointOffset;
		}
		return Result;
	}
	*/

	std::size_t FindOrAddSymbol(std::u32string_view Source, std::vector<std::u32string>& Output)
	{
		for (std::size_t IteIndex = 0; IteIndex < Output.size(); ++IteIndex)
		{
			auto& Ite = Output[IteIndex];
			if (Ite == Source)
			{
				return IteIndex;
			}
		}
		auto OldIndex = Output.size();
		Output.push_back(std::u32string{Source});
		return OldIndex;
	}

	struct LexicalTuple
	{
		DLr::Symbol Value;
		std::u32string_view Str;
		std::size_t Offset;
	};

	std::array<Misc::IndexSpan<>, 3> ProcessLexical(std::vector<LexicalTuple>& OutputTuple, std::u32string_view Str)
	{
		std::size_t State = 0;
		std::array<Misc::IndexSpan<>, 3> Indexs;
		auto StepReg = EbnfStepReg();
		std::size_t Offset = 0;
		std::u32string_view StrIte = Str;
		while (!StrIte.empty())
		{
			auto Re = Reg::ProcessGreedyFrontMatch(StepReg, StrIte);
			if (Re.has_value())
			{
				
				auto Enum = static_cast<T>(Re->AcceptData.Mask);
				switch (Enum)
				{
				case T::Command:
				case T::Empty:
					break;
				case T::Number:
				case T::NoTerminal:
				case T::Terminal:
					OutputTuple.push_back({*Enum, StrIte.substr(0, Re->MainCapture.Count()), Offset});
					break;
				case T::NoProductionNoTerminal:
				case T::Rex:
				{
					auto Wrapper = Re->GetCaptureWrapper();
					assert(Wrapper.HasSubCapture());
					auto SpanIndex = Wrapper.GetTopSubCapture().GetCapture();
					OutputTuple.push_back({ *Enum, 
						StrIte.substr(SpanIndex.Begin(), SpanIndex.Count()),
						Offset
						});
				}
					break;
				case T::Barrier:
					if (State >= 3) [[unlikely]]
					{
						throw Exception::UnacceptableEbnf{UnacceptableEbnf::TypeT::WrongEbnf, Re->MainCapture.Begin()};
					}
					else {
						Indexs[State].Length = OutputTuple.size() - Indexs[State].Offset;
						++State;
						Indexs[State].Offset = OutputTuple.size();
					}
					break;
				default:
					OutputTuple.push_back({ *Enum, StrIte.substr(0, Re->MainCapture.Count()), Offset });
					break;
				}
				Offset += Re->MainCapture.Count();
				StrIte = StrIte.substr(Re->MainCapture.Count());
				if (StrIte.empty())
				{
					Indexs[State].Length = OutputTuple.size() - Indexs[State].Offset;
				}
			}
			else {
				throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnf, Offset };
			}
		}
		return Indexs;
	}

	DLr::TableWrapper EbnfStep1DLr() {
		static DLr::Table Table(
			*NT::Statement,
			{
				{*NT::Statement, {}, 1},
				{*NT::Statement, {*NT::Statement, *T::Terminal, *T::Equal, *T::Rex}, 2},
				{*NT::Statement, {*NT::Statement, *T::Terminal, *T::Equal, *T::Rex, *T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace}, 3},
				{*NT::Statement, {*NT::Statement, *T::Start, *T::Equal, *T::Rex}, 4},
			},
			{}
		);
		return Table.Wrapper;
	};

	DLr::TableWrapper EbnfStep2DLr() {
		static DLr::Table Table(
			*NT::Statement,
			{
				{*NT::Expression, {*T::Terminal}, 1},
				{*NT::Expression, {*T::NoTerminal}, 1},
				{*NT::Expression, {*T::Rex}, 4},
				{*NT::Expression, {*T::Number}, 5},
				{*NT::Expression, {*T::NoProductionNoTerminal}, 40},

				{*NT::FunctionEnum, {*T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace}, 6},
				{*NT::FunctionEnum, {}, 7},

				{*NT::ExpressionStatement, {*NT::ExpressionStatement, 31, *NT::Expression}, 8},
				{*NT::ExpressionStatement, {*NT::Expression}, 9},

				{*NT::Expression, {*T::LS_Brace, *NT::ExpressionStatement, *T::RS_Brace}, 10},
				{*NT::Expression, {*T::LB_Brace, *NT::ExpressionStatement, *T::RB_Brace}, 11},
				{*NT::Expression, {*T::LM_Brace, *NT::ExpressionStatement, *T::RM_Brace}, 12},

				{*NT::LeftOrStatement, {*NT::LeftOrStatement, *NT::Expression}, 8},
				{*NT::LeftOrStatement, {*NT::Expression}, 9},
				{*NT::RightOrStatement, {*NT::Expression, *NT::RightOrStatement}, 15},
				{*NT::RightOrStatement, {*NT::Expression}, 9},
				{*NT::OrStatement, {*NT::LeftOrStatement, *T::Or, *NT::RightOrStatement}, 17},
				{*NT::OrStatement, {*NT::OrStatement, *T::Or, *NT::RightOrStatement}, 30},
				{*NT::ExpressionStatement, {*NT::OrStatement}, 31},

				{*NT::ExpressionList, {*NT::ExpressionStatement}, 18},
				{*NT::ExpressionList, {}, 19},

				{*NT::Statement, {*NT::Statement, *NT::Statement, 20}, 20},
				{*NT::Statement, {*T::NoTerminal, *T::Equal, *NT::ExpressionList, *NT::FunctionEnum, *T::Semicolon}, 21},
				{*NT::Statement, {*T::Equal, *NT::ExpressionList, *NT::FunctionEnum, *T::Semicolon}, 22},
				{*NT::Statement, {*T::Start, *T::Equal, *T::NoTerminal, *T::Semicolon}, 23},
			},
			{}
		);
		return Table.Wrapper;
	};

	DLr::TableWrapper EbnfStep3DLr() {
		static DLr::Table Table(
			*NT::Statement,
			{
				{*NT::Expression, {*T::Terminal}, 1},
				{*NT::Expression, {*T::Rex}, 1},
				{*NT::ExpressionStatement, {*NT::Expression}, 2},
				{*NT::ExpressionStatement, {*NT::ExpressionStatement, *NT::Expression}, 3},
				{*NT::Statement, {}},
				{*NT::Statement, {*NT::Statement, *T::LS_Brace, *NT::ExpressionStatement, *T::RightPriority }, 4},
				{*NT::Statement, {*NT::Statement, *T::LeftPriority, *NT::ExpressionStatement, *T::RS_Brace }, 5},
			},
			{}
		);
		return Table.Wrapper;
	};

	UnserilizeTable::UnserilizeTable(std::u32string_view Str)
	{
		struct Storaget
		{
			DLr::Symbol Sym;
			std::size_t Index;
		};

		std::vector<Storaget> Mapping;
		std::size_t TempNoTerminalCount = std::numeric_limits<std::size_t>::max();
		Reg::MulityRegexCreator Creator;

		std::vector<LexicalTuple> SymbolTuple;
		auto StepIndexs = ProcessLexical(SymbolTuple, Str);
		if(StepIndexs[1].Count() == 0) [[unlikely]]
			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnf, StepIndexs[0].End()};
#if _DEBUG
		{
			std::vector<std::u32string_view> Strs;
			for (auto& Ite : SymbolTuple)
			{
				Strs.push_back(Ite.Str);
			}
			volatile int i  = 0;
		}
#endif

		// Step1
		try
		{

			auto Steps = DLr::ProcessSymbol(EbnfStep1DLr(), StepIndexs[0].Begin(), [&](std::size_t Index) -> DLr::Symbol {
				if (Index < StepIndexs[0].End())
					return SymbolTuple[Index].Value;
				else
					return DLr::Symbol::EndOfFile();
				}
			);

			DLr::ProcessStep(Steps, [&](DLr::StepElement Ele) -> std::any
				{
					if (Ele.IsNoTerminal())
					{
						auto& NT = Ele.AsNoTerminal();
						switch (NT.Mask)
						{
						case 2:
						{
							auto Name = NT[1].Consume<std::u32string_view>();
							auto Index = FindOrAddSymbol(Name, TerminalMappings);
							auto Index2 = NT[3].Consume<std::u32string_view>();
							Creator.AddRegex(Index2, {0}, static_cast<Reg::SerilizeT>(Index + 1));
							return {};
						}
						case 3:
						{
							auto Name = NT[1].Consume<std::u32string_view>();
							auto Index = FindOrAddSymbol(Name, TerminalMappings);
							auto Index2 = NT[3].Consume<std::u32string_view>();
							std::size_t Mask = NT[6].Consume<std::size_t>();
							Reg::TableWrapper::StorageT SeriMask;
							Misc::SerilizerHelper::TryCrossTypeSet(SeriMask, Mask, Exception::OutofRange{ Exception::OutofRange::TypeT::Regex, NT.FirstTokenIndex });
							Creator.AddRegex(Index2, { SeriMask }, static_cast<Reg::SerilizeT>(Index + 1));
							return {};
						}
						case 4:
						{
							auto Index2 = NT[3].Consume<std::u32string_view>();
							Creator.AddRegex(Index2, {0}, 0, false);
							return {};
						}
						default:
							return {};
							break;
						}
					}
					else {
						auto& TRef = Ele.AsTerminal();
						T Enum = static_cast<T>(TRef.Value.Value);
						switch (Enum)
						{
						case T::Terminal:
						case T::Rex:
							return SymbolTuple[TRef.TokenIndex].Str;
						case T::Number:
						{
							std::size_t Index = 0;
							StrFormat::DirectScan(SymbolTuple[TRef.TokenIndex].Str, Index);
							return Index;
						}
						default:
							return {};
						}
					}
				}
			);
		}
		catch (DLr::Exception::UnaccableSymbol const& US)
		{
			std::size_t Token = 0;
			if(SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].Offset;
			else if(!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw UnacceptableEbnf{ UnacceptableEbnf ::TypeT::WrongRegex, Token };
		}
		catch (Reg::Exception::UnaccaptableRegex const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].Offset;
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongRegex, Token };
		}
		catch (...)
		{
			throw;
		}

		std::vector<DLr::ProductionBuilder> Builder;
		std::optional<DLr::Symbol> StartSymbol;
		std::size_t DefineTerminalStart = TerminalMappings.size();
		// Step2
		try
		{

			std::optional<DLr::Symbol> LastProductionStartSymbol;

			auto Steps = DLr::ProcessSymbol(EbnfStep2DLr(), StepIndexs[1].Begin(), [&](std::size_t Index) -> DLr::Symbol {
				if (Index < StepIndexs[1].End())
					return SymbolTuple[Index].Value;
				else
					return DLr::Symbol::EndOfFile();
			});

			DLr::ProcessStep(Steps, [&](DLr::StepElement Ele) -> std::any{
				if (Ele.IsNoTerminal())
				{
					auto Ref = Ele.AsNoTerminal();
					switch (Ref.Mask)
					{
					case 1:
					case 4:
						return DLr::ProductionBuilderElement{Ref[0].Consume<DLr::Symbol>()};
					case 5:
						return DLr::ProductionBuilderElement{Ref[0].Consume<std::size_t>() + MinMask()};
					case 40:
					{
						auto Exp = Ref[0].Consume<std::size_t>();
						auto CurSymbol = DLr::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Builder.push_back({
							CurSymbol,
							Exp + MinMask(),
							});
						return DLr::ProductionBuilderElement{ CurSymbol };
					}
					case 6:
						return Ref[2].Consume<std::size_t>() + MinMask();
					case 7:
						return MinMask();
					case 8:
					{
						auto Last = Ref[0].Consume<std::vector<DLr::ProductionBuilderElement>>();
						Last.push_back(Ref[1].Consume<DLr::ProductionBuilderElement>());
						return std::move(Last);
					}
					case 9:
					{
						std::vector<DLr::ProductionBuilderElement> Temp;
						Temp.push_back(Ref[0].Consume<DLr::ProductionBuilderElement>());
						return std::move(Temp);
					}
					case 10:
					{
						auto Exp = Ref[1].Consume<std::vector<DLr::ProductionBuilderElement>>();
						auto CurSymbol = DLr::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Builder.push_back({
							CurSymbol,
							std::move(Exp),
							UnserilizeTable::SmallBrace()
							});
						return DLr::ProductionBuilderElement{ CurSymbol};
					}
					case 11:
					{
						auto Exp = Ref[1].Consume<std::vector<DLr::ProductionBuilderElement>>();
						auto CurSymbol = DLr::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Exp.insert(Exp.begin(), DLr::ProductionBuilderElement{ CurSymbol});
						Builder.push_back({
							CurSymbol,
							std::move(Exp),
							UnserilizeTable::BigBrace()
							});
						Builder.push_back({
							CurSymbol,
							UnserilizeTable::BigBrace()
							});
						return DLr::ProductionBuilderElement{ CurSymbol};
					}
					case 12:
					{
						auto Exp = Ref[1].Consume<std::vector<DLr::ProductionBuilderElement>>();
						auto CurSymbol = DLr::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Builder.push_back({
							CurSymbol,
							std::move(Exp),
							UnserilizeTable::MiddleBrace()
							});
						Builder.push_back({
							CurSymbol,
							UnserilizeTable::MiddleBrace()
							});
						return DLr::ProductionBuilderElement{ CurSymbol};
					}
					case 15:
					{
						auto Last = Ref[1].Consume<std::vector<DLr::ProductionBuilderElement>>();
						Last.push_back(Ref[0].Consume<DLr::ProductionBuilderElement>());
						return std::move(Last);
					}
					case 17:
					{
						auto Last1 = Ref[0].Consume<std::vector<DLr::ProductionBuilderElement>>();
						auto Last2 = Ref[2].Consume<std::vector<DLr::ProductionBuilderElement>>();
						std::reverse(Last2.begin(), Last2.end());
						std::vector<std::vector<DLr::ProductionBuilderElement>> Temp;
						Temp.push_back(std::move(Last1));
						Temp.push_back(std::move(Last2));
						return std::move(Temp);
					}
					case 30:
					{
						auto Last1 = Ref[0].Consume<std::vector<std::vector<DLr::ProductionBuilderElement>>>();
						auto Last2 = Ref[2].Consume<std::vector<DLr::ProductionBuilderElement>>();
						std::reverse(Last2.begin(), Last2.end());
						Last1.push_back(std::move(Last2));
						return std::move(Last1);
					}
					case 31:
					{
						auto Last = Ref[0].Consume<std::vector<std::vector<DLr::ProductionBuilderElement>>>();
						while (Last.size() >= 2)
						{
							auto L1 = std::move(*Last.rbegin());
							Last.pop_back();
							auto L2 = std::move(*Last.rbegin());
							Last.pop_back();
							auto CurSymbol = DLr::Symbol::AsNoTerminal(TempNoTerminalCount--);
							Builder.push_back({
								CurSymbol,
								std::move(L1),
								UnserilizeTable::OrLeft()
								});
							Builder.push_back({
								CurSymbol,
								std::move(L2),
								UnserilizeTable::OrRight()
								});
							Last.push_back({CurSymbol});
						}
						return std::move(Last[0]);
					}
					case 18:
						return Ref[0].Consume();
					case 19:
						return std::vector<DLr::ProductionBuilderElement>{};
					case 20:
						return {};
					case 21:
					{
						LastProductionStartSymbol = Ref[0].Consume<DLr::Symbol>();
						auto List = Ref[2].Consume<std::vector<DLr::ProductionBuilderElement>>();
						auto Mask = Ref[3].Consume<std::size_t>();
						Builder.push_back({
							*LastProductionStartSymbol,
							std::move(List),
							Mask
							});
						return {};
					}
					case 22:
					{
						if(!LastProductionStartSymbol.has_value()) [[unlikely]]
						{
							std::size_t TokenIndex = SymbolTuple[Ref.FirstTokenIndex].Offset;
							throw Exception::UnacceptableEbnf{ Exception::UnacceptableEbnf::TypeT::UnsetProductionHead, TokenIndex};
						}
						auto List = Ref[1].Consume<std::vector<DLr::ProductionBuilderElement>>();
						auto Mask = Ref[2].Consume<std::size_t>();
						Builder.push_back({
							*LastProductionStartSymbol,
							std::move(List),
							Mask,
							});
						return {};
					}
					case 23:
					{
						if(StartSymbol.has_value()) [[unlikely]]
						{
							std::size_t TokenIndex = SymbolTuple[Ref.FirstTokenIndex].Offset;
							throw Exception::UnacceptableEbnf{ Exception::UnacceptableEbnf::TypeT::StartSymbolAreadySet, TokenIndex};
						}
						StartSymbol = Ref[2].Consume<DLr::Symbol>();
						return {};
					}
					default:
						break;
					}
				}
				else
				{
					auto Ref = Ele.AsTerminal();
					T TValue = static_cast<T>(Ref.Value.Value);
					switch (TValue)
					{
					case T::NoProductionNoTerminal:
					case T::Number:
					{
						std::size_t Index = 0;
						StrFormat::DirectScan(SymbolTuple[Ref.TokenIndex].Str, Index);
						return Index;
					}
					case T::Terminal:
					{
						auto Name = SymbolTuple[Ref.TokenIndex].Str;
						for (std::size_t Index = 0; Index < DefineTerminalStart; ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, SymbolTuple[Ref.TokenIndex].Offset};
					}
					case T::NoTerminal:
					{
						auto Name = SymbolTuple[Ref.TokenIndex].Str;
						auto Index = FindOrAddSymbol(std::move(Name), NoTermialMapping);
						return DLr::Symbol::AsNoTerminal(Index);
					}
					case T::Rex:
					{
						auto Name = SymbolTuple[Ref.TokenIndex].Str;
						for (std::size_t Index = DefineTerminalStart; Index < TerminalMappings.size(); ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						std::size_t Index = TerminalMappings.size();
						TerminalMappings.push_back(std::u32string{Name});
						Creator.AddRegex(Name, {0}, static_cast<Reg::SerilizeT>(Index + 1), true);
						return DLr::Symbol::AsTerminal(Index);
					}
					default:
						break;
					}
				}
				return std::any{};
			});

			if (!StartSymbol.has_value())
			{
				std::size_t Location = 0;
				if(!SymbolTuple.empty())
					Location = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
				throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnfindedStartSymbol, Location };
			}
		}
		catch (DLr::Exception::UnaccableSymbol const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].Offset;
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;

			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongLr, Token };
		}
		catch (...)
		{
			throw;
		}

		std::vector<DLr::OpePriority> Priority;
		// Step3
		try {
			auto Steps = DLr::ProcessSymbol(EbnfStep3DLr(), StepIndexs[2].Begin(), [&](std::size_t Index) -> DLr::Symbol {
				if (Index < StepIndexs[2].End())
					return SymbolTuple[Index].Value;
				else
					return DLr::Symbol::EndOfFile();
				});
			
			DLr::ProcessStep(Steps, [&](DLr::StepElement Ele) -> std::any{
				if (Ele.IsNoTerminal())
				{
					auto NT = Ele.AsNoTerminal();
					switch (NT.Mask)
					{
					case 1:
						return NT[0].Consume();
					case 2:
					{
						std::vector<DLr::Symbol> Symbols;
						Symbols.push_back(NT[0].Consume<DLr::Symbol>());
						return Symbols;
					}
					case 3:
					{
						auto Symbols = NT[0].Consume<std::vector<DLr::Symbol>>();
						Symbols.push_back(NT[1].Consume<DLr::Symbol>());
						return Symbols;
					}
					case 4:
					{
						auto Symbols = NT[2].Consume<std::vector<DLr::Symbol>>();
						Priority.push_back({std::move(Symbols), DLr::Associativity::Right});
						return {};
					}
					case 5:
					{
						auto Symbols = NT[2].Consume<std::vector<DLr::Symbol>>();
						Priority.push_back({ std::move(Symbols), DLr::Associativity::Left });
						return {};
					}
					default:
						return {};
					}
				}
				else {
					auto TRef = Ele.AsTerminal();
					T Enum = static_cast<T>(TRef.Value.Value);
					switch (Enum)
					{
					case T::Terminal:
					{
						auto Name = SymbolTuple[TRef.TokenIndex].Str;
						for (std::size_t Index = 0; Index < DefineTerminalStart; ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, SymbolTuple[TRef.TokenIndex].Offset };
					}
					case T::Rex:
					{
						auto Name = SymbolTuple[TRef.TokenIndex].Str;
						for (std::size_t Index = DefineTerminalStart; Index < TerminalMappings.size(); ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableRex, SymbolTuple[TRef.TokenIndex].Offset};
					}
						break;
					}
				}
				return {};
			});
		}
		catch (DLr::Exception::UnaccableSymbol const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].Offset;
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongPriority, Token };
		}
		catch (...)
		{
			throw;
		}

		try
		{

			std::size_t MaxNoTerminal = NoTermialMapping.size();
			for (auto& Ite : Builder)
			{
				assert(Ite.ProductionValue.IsNoTerminal());
				if (Ite.ProductionValue.Value > MaxNoTerminal)
				{
					Ite.ProductionValue.Value = Ite.ProductionValue.Value - std::numeric_limits<std::size_t>::max() + MaxNoTerminal;
				}
				for (auto& Ite2 : Ite.Element)
				{
					if (!Ite2.IsMask && Ite2.ProductionValue.IsNoTerminal() && Ite2.ProductionValue.Value > MaxNoTerminal)
					{
						Ite2.ProductionValue.Value = Ite2.ProductionValue.Value - std::numeric_limits<std::size_t>::max() + MaxNoTerminal;
					}
				}
			}

			RegTable = Creator.Generate();
			DLrTable = DLr::TableWrapper::Create(
				*StartSymbol,
				std::move(Builder),
				std::move(Priority)
			);
		}
		catch (DLr::Exception::UnaccableSymbol const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].Offset;
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongLr, Token };
		}
		catch (Reg::Exception::RegexOutOfRange const&)
		{
			std::size_t Token = 0;
			if(!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw OutofRange{ OutofRange::TypeT::Regex, Token };
		}
		catch (DLr::Exception::OutOfRange const&)
		{
			std::size_t Token = 0;
			if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw OutofRange{ OutofRange::TypeT::DLr, Token };
		}
		catch (...)
		{
			throw;
		}
	}

	auto TableWrapper::Create(UnserilizeTable const& Table) ->std::vector<StorageT>
	{
		using namespace Misc::SerilizerHelper;

		std::size_t NameStrCount = 0;
		std::size_t NameIndexCount = Table.NoTermialMapping.size() + Table.TerminalMappings.size();
		for (auto& Ite : Table.TerminalMappings)
			NameStrCount += Ite.size();
		for(auto& Ite : Table.NoTermialMapping)
			NameStrCount += Ite.size();
		std::u32string NameStorage;
		NameStorage.reserve(NameStrCount);
		std::vector<Misc::IndexSpan<SubStorageT>> Index;
		Index.reserve(NameIndexCount);
		for (auto& Ite : Table.TerminalMappings)
		{
			Misc::IndexSpan<SubStorageT> Temp;
			TryCrossTypeSet(Temp.Offset, NameStorage.size(), OutofRange{ OutofRange ::TypeT::Header, NameStorage.size() });
			TryCrossTypeSet(Temp.Length, Ite.size(), OutofRange{ OutofRange::TypeT::Header, Ite.size() });
			NameStorage.insert(NameStorage.end(), Ite.begin(), Ite.end());
			Index.push_back(Temp);
		}
		for (auto& Ite : Table.NoTermialMapping)
		{
			Misc::IndexSpan<SubStorageT> Temp;
			TryCrossTypeSet(Temp.Offset, NameStorage.size(), OutofRange{ OutofRange::TypeT::Header, NameStorage.size() });
			TryCrossTypeSet(Temp.Length, Ite.size(), OutofRange{ OutofRange::TypeT::Header, Ite.size() });
			NameStorage.insert(NameStorage.end(), Ite.begin(), Ite.end());
			Index.push_back(Temp);
		}
		std::vector<StorageT> Result;
		auto Head = WriteObject(Result, ZipHeader{});
		WriteObjectArray(Result, std::span(NameStorage));
		auto InfoLast = WriteObjectArray(Result, std::span(Index));
		auto RegLast = WriteObjectArray(Result, std::span(Table.RegTable));
		auto DLrLast = WriteObjectArray(Result, std::span(Table.DLrTable));
		auto HeadPtr = ReadObject<ZipHeader>(std::span(Result));
		TryCrossTypeSet(HeadPtr->TerminalNameCount, Table.TerminalMappings.size(), OutofRange{ OutofRange::TypeT::Header, Table.TerminalMappings.size() });
		TryCrossTypeSet(HeadPtr->NameIndexCount, Index.size(), OutofRange{ OutofRange::TypeT::Header, Index.size() });
		TryCrossTypeSet(HeadPtr->NameStringCount, NameStorage.size(), OutofRange{ OutofRange::TypeT::Header, NameStorage.size() });
		TryCrossTypeSet(HeadPtr->RegCount, RegLast.WriteLength, OutofRange{ OutofRange::TypeT::Header, RegLast.WriteLength });
		TryCrossTypeSet(HeadPtr->DLrCount, DLrLast.WriteLength, OutofRange{ OutofRange::TypeT::Header, DLrLast.WriteLength });
		return Result;
	}

	TableWrapper::TableWrapper(std::span<StorageT const> Input)
		: Wrapper(Input)
	{
		using namespace Misc::SerilizerHelper;
		if (!Input.empty())
		{
			auto Head = ReadObject<ZipHeader const>(Input);
			auto Name = ReadObjectArray<char32_t const>(Head.LastSpan, Head->NameStringCount);
			auto Index = ReadObjectArray<Misc::IndexSpan<SubStorageT> const>(Name.LastSpan, Head->NameIndexCount);
			auto Reg = ReadObjectArray<Reg::TableWrapper::StorageT const>(Index.LastSpan, Head->RegCount);
			auto DLr = ReadObjectArray<Reg::TableWrapper::StorageT const>(Reg.LastSpan, Head->DLrCount);
			TotalName = std::u32string_view(Name->data(), Name->size());
			StrIndex = *Index;
			TerminalCount = Head->TerminalNameCount;
			RegWrapper = *Reg;
			DLrWrapper = *DLr;
		}
	}

	std::u32string_view TableWrapper::FindSymbolName(DLr::Symbol Symbol) const
	{
		if (!Symbol.IsEndOfFile() && !Symbol.IsStartSymbol())
		{
			if (Symbol.IsTerminal())
			{
				auto Index = Symbol.Value;
				assert(Index < TerminalCount);
				assert(Index < StrIndex.size());
				auto Indexs = StrIndex[Index];
				return TotalName.substr(Indexs.Begin(), Indexs.Count());
			}
			else {
				auto Index = Symbol.Value + TerminalCount;
				if (Index < StrIndex.size())
				{
					auto Indexs = StrIndex[Index];
					return TotalName.substr(Indexs.Begin(), Indexs.Count());
				}
			}
		}
		return {};
	}

	auto SymbolProcessor::ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex) -> std::optional<Result>
	{
		auto Re = Processor.ConsumeTokenInput(Token, NextTokenIndex);
		if (Re.has_value())
		{
			Result Re1;
			Re1.IsEndOfFile = Re->IsEndOfFile;
			if (Re->Accept.has_value())
			{
				if (Re->Accept->UniqueID != 0)
				{
					EbnfSymbolTuple Re2;
					Re2.Value = DLr::Symbol::AsTerminal(Re->Accept->UniqueID - 1);
					Re2.Mask = Re->Accept->AcceptData.Mask;
					auto Capture = Re->Accept->GetCaptureWrapper();
					if (Capture.HasSubCapture())
					{
						Re2.StrIndex = Capture.GetTopSubCapture().GetCapture();
					}
					else {
						Re2.StrIndex = Re->Accept->MainCapture;
					}
					Re2.CaptureMain = Re->Accept->MainCapture;
					Re1.Accept = Re2;
				}
				if(Re1.IsEndOfFile)
					return Re1;
				Processor.Reset(Re->Accept->MainCapture.End());
			}
			return Re1;
		}
		else {
			return {};
		}
	}

	std::vector<EbnfSymbolTuple> ProcessSymbol(TableWrapper Wrapper, std::u32string_view StrView)
	{
		std::vector<EbnfSymbolTuple> Tuples;

		SymbolProcessor Processor(Wrapper);

		while (true)
		{
			auto RI = Processor.GetReuqireTokenIndex();
			char32_t Token = (RI >= StrView.size() ? Reg::EndOfFile() : StrView[RI]);
			auto Re = Processor.ConsumeTokenInput(Token, RI + 1);
			if (Re.has_value())
			{
				if (Re->Accept.has_value())
				{
					Tuples.push_back(std::move(*Re->Accept));
				}
				if(Re->IsEndOfFile)
					return Tuples;
			}
			else if(Token == Reg::EndOfFile())
			{
				break;
			}else{
				throw UnacceptableSymbol{ UnacceptableSymbol::TypeT::Reg, RI };
			}
		}

		return Tuples;
	}

	Misc::IndexSpan<> TranslateIndex(DLr::NTElement ELe, std::span<EbnfSymbolTuple const> InputSymbol)
	{
		if (ELe.Datas.empty())
		{
			if(ELe.FirstTokenIndex < InputSymbol.size())
				return { InputSymbol[ELe.FirstTokenIndex].StrIndex.End(), 0};
			else if(!InputSymbol.empty())
				return { InputSymbol.rbegin()->StrIndex.End(), 0};
			else
				return {0, 0};
		}
		else {
			auto Begin = InputSymbol[ELe.Datas.begin()->FirstTokenIndex].StrIndex;
			auto End = InputSymbol[ELe.Datas.rbegin()->FirstTokenIndex].StrIndex;
			return { Begin.Begin(), End.End() - Begin.Begin()};
		}
	}

	std::any ProcessStep(TableWrapper Wrapper, std::span<EbnfSymbolTuple const> InputSymbol, std::any(*F)(StepElement Element, void* Data), void* Data)
	{
		try {
			auto Steps = DLr::ProcessSymbol(Wrapper.AsDLrWrapper(), 0, [=](std::size_t Index)->DLr::Symbol {
				if (Index < InputSymbol.size())
					return InputSymbol[Index].Value;
				else
					return DLr::Symbol::EndOfFile();
				});
			return DLr::ProcessStep(Steps, [=](DLr::StepElement Ele)->std::any {
				if (Ele.IsNoTerminal())
				{
					auto NT = Ele.AsNoTerminal();
					Misc::IndexSpan<> Index = TranslateIndex(NT, InputSymbol);
					switch (NT.Mask)
					{
					case UnserilizeTable::SmallBrace():
					{
						if (NT.Datas.size() == 1)
						{
							auto TempCondition = NT[0].TryConsume<Condition>();
							if (TempCondition.has_value())
								return *TempCondition;
						}
						std::vector<DLr::NTElementData> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::Parentheses, 0, Index, std::move(TemBuffer) };
					}
					case UnserilizeTable::MiddleBrace():
					{
						std::vector<DLr::NTElementData> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::SquareBrackets, 0, Index, std::move(TemBuffer) };
					}
					case UnserilizeTable::BigBrace():
					{
						if (NT.Datas.empty())
						{
							return Condition{ Condition::TypeT::CurlyBrackets, 0, Index, {} };
						}
						else {
							auto Temp = NT[0].Consume<Condition>();
							auto Span = std::span(NT.Datas).subspan(1);
							Temp.Datas.insert(Temp.Datas.end(), std::move_iterator(Span.begin()), std::move_iterator(Span.end()));
							Temp.AppendData += 1;
							return std::move(Temp);
						}
					}
					case UnserilizeTable::OrLeft():
					{
						std::vector<DLr::NTElementData> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::Or, 0, Index, std::move(TemBuffer) };
					}
					case UnserilizeTable::OrRight():
					{
						if (NT.Datas.size() == 1)
						{
							auto Temp = NT[0].TryConsume<Condition>();
							if (Temp.has_value())
							{
								if (Temp->Type == Condition::TypeT::Or)
								{
									Temp->AppendData += 1;
									return std::move(Temp);
								}
								else
									NT[0].Data = std::move(Temp);
							}
						}
						std::vector<DLr::NTElementData> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::Or, 1, Index, std::move(TemBuffer) };
					}
					default:
					{
						NTElement NEle;
						NEle.Value = NT.Value;
						NEle.Mask = NT.Mask - UnserilizeTable::MinMask();
						NEle.Datas = NT.Datas;
						NEle.Index = Index;
						return F(StepElement{ NEle }, Data);
					}
					}
				}
				else if (Ele.IsTerminal())
				{
					auto T = Ele.AsTerminal();
					TElement Ele;
					Ele.Value = T.Value;
					auto& Sym = InputSymbol[T.TokenIndex];
					Ele.Mask = Sym.Mask;
					Ele.Index = Sym.StrIndex;
					return F(StepElement{ Ele }, Data);
				}
				return {};
				});
		}
		catch (DLr::Exception::UnaccableSymbol const& Symbol)
		{
			std::size_t Token = 0;
			if(Symbol.TokenIndex < InputSymbol.size())
				Token = InputSymbol[Symbol.TokenIndex].StrIndex.Begin();
			else if(!InputSymbol.empty())
				Token = InputSymbol.rbegin()->StrIndex.End();
			throw UnacceptableInput{ Token };
		}
	}

}

namespace Potato::Ebnf::Exception
{
	char const* Interface::what() const { return "Ebnf Exception"; }
	char const* UnacceptableEbnf::what() const { return "UnacceptableEbnf Exception"; }
	char const* OutofRange::what() const { return "OutofRange Exception"; }
	char const* UnacceptableSymbol::what() const { return "UnacceptableSymbol Exception"; }
	char const* UnacceptableInput::what() const { return "UnacceptableInput Exception"; }
}