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

	Reg::TableWrapper EbnfStepReg() {
		static auto List = []() {
			Reg::MulityRegexCreator Creator;
			Creator.PushRegex(UR"(%%%%)", { Creator.Count(), static_cast<std::size_t>(T::Barrier)});
			Creator.PushRegex(UR"([0-9]+)", { Creator.Count(), static_cast<std::size_t>(T::Number)});
			Creator.PushRegex(UR"([0-9a-zA-Z_\z]+)", { Creator.Count(), static_cast<std::size_t>(T::Terminal)});
			Creator.PushRegex(UR"(<[0-9a-zA-Z_\z]+>)", { Creator.Count(), static_cast<std::size_t>(T::NoTerminal)});
			Creator.PushRegex(UR"(<\$([0-9]+)>)", { Creator.Count(), static_cast<std::size_t>(T::NoProductionNoTerminal) });
			Creator.PushRegex(UR"(:=)", { Creator.Count(), static_cast<std::size_t>(T::Equal)});
			Creator.PushRegex(UR"(\'([^\s]+)\')", { Creator.Count(), static_cast<std::size_t>(T::Rex)});
			Creator.PushRegex(UR"(;)", { Creator.Count(), static_cast<std::size_t>(T::Semicolon)});
			Creator.PushRegex(UR"(:)", { Creator.Count(), static_cast<std::size_t>(T::Colon) });
			Creator.PushRegex(UR"(\s+)", { Creator.Count(), static_cast<std::size_t>(T::Empty)});
			Creator.PushRegex(UR"(\$)", { Creator.Count(), static_cast<std::size_t>(T::Start)});
			Creator.PushRegex(UR"(\|)", { Creator.Count(), static_cast<std::size_t>(T::Or)});
			Creator.PushRegex(UR"(\[)", { Creator.Count(), static_cast<std::size_t>(T::LM_Brace) });
			Creator.PushRegex(UR"(\])", { Creator.Count(), static_cast<std::size_t>(T::RM_Brace) });
			Creator.PushRegex(UR"(\{)", { Creator.Count(), static_cast<std::size_t>(T::LB_Brace) });
			Creator.PushRegex(UR"(\})", { Creator.Count(), static_cast<std::size_t>(T::RB_Brace) });
			Creator.PushRegex(UR"(\()", { Creator.Count(), static_cast<std::size_t>(T::LS_Brace) });
			Creator.PushRegex(UR"(\))", { Creator.Count(), static_cast<std::size_t>(T::RS_Brace) });
			Creator.PushRegex(UR"(\+\()", { Creator.Count(), static_cast<std::size_t>(T::LeftPriority) });
			Creator.PushRegex(UR"(\)\+)", { Creator.Count(), static_cast<std::size_t>(T::RightPriority) });
			Creator.PushRegex(UR"(/\*.*?\*/)", { Creator.Count(), static_cast<std::size_t>(T::Command) });
			Creator.PushRegex(UR"(//[^\n]*\n)", { Creator.Count(), static_cast<std::size_t>(T::Command) });
			return Creator.Generate();
		}();
		return Reg::TableWrapper(List);
	}

	std::u32string Translate(Misc::IndexSpan<> Index, Reg::CodePoint(*F1)(std::size_t Index, void* Data), void* Data)
	{
		std::u32string Result;
		for (std::size_t Ite = Index.Begin(); Ite < Index.End();)
		{
			auto Cur = F1(Ite, Data);
			Result.push_back(Cur.UnicodeCodePoint);
			Ite += Cur.NextUnicodeCodePointOffset;
		}
		return Result;
	}

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

	std::size_t FindOrAddSymbol(std::u32string Source, std::vector<std::u32string>& Output)
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
		Output.push_back(std::move(Source));
		return OldIndex;
	}

	struct LexicalTuple
	{
		DLr::Symbol Value;
		Misc::IndexSpan<> StrIndex;
	};

	std::array<Misc::IndexSpan<>, 3> ProcessLexical(std::vector<LexicalTuple>& OutputTuple, std::size_t Offset, Reg::CodePoint(*F1)(std::size_t Index, void* Data), void* Data)
	{
		std::size_t State = 0;
		std::array<Misc::IndexSpan<>, 3> Indexs;
		auto StepReg = EbnfStepReg();
		while (true)
		{
			auto Re = Reg::ProcessFrontMarch(StepReg, Offset, F1, Data);
			if (Re.has_value())
			{
				Offset = Re->MainCapture.End();
				auto Enum = static_cast<T>(Re->AcceptData.Mask);
				switch (Enum)
				{
				case T::Command:
				case T::Empty:
					break;
				case T::Number:
				case T::NoTerminal:
				case T::Terminal:
					OutputTuple.push_back({*Enum, Re->MainCapture});
					break;
				case T::NoProductionNoTerminal:
				case T::Rex:
				{
					assert(Re->Captures.size() == 2);
					assert(Re->Captures[0].IsBegin && !Re->Captures[1].IsBegin);
					OutputTuple.push_back({ *Enum, { Re->Captures[0].Index, Re->Captures[1].Index - Re->Captures[0].Index} });
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
					OutputTuple.push_back({ *Enum, Re->MainCapture });
					break;
				}
				if (Re->IsEndOfFile)
				{
					Indexs[State].Length = OutputTuple.size() - Indexs[State].Offset;
					break;
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
	try 
		: UnserilizeTable(0, [](std::size_t Index, void* Data)-> Reg::CodePoint {
			auto Str = *reinterpret_cast<std::u32string_view*>(Data);
			if(Index < Str.size())
				return Reg::CodePoint{Str[Index], 1};
			else
				return Reg::CodePoint::EndOfFile();
		}, & Str)
	{
	}
	catch (UnacceptableEbnf const& Ebnf)
	{
	#if _DEBUG
		auto NStr = Str.substr(Ebnf.TokenIndex);
	#endif
		throw Ebnf;
	}

	UnserilizeTable::UnserilizeTable(std::size_t Offset, Reg::CodePoint(*F1)(std::size_t Index, void* Data), void* Data)
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
		auto StepIndexs = ProcessLexical(SymbolTuple, Offset, F1, Data);
		if(StepIndexs[1].Count() == 0) [[unlikely]]
			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnf, StepIndexs[0].End()};
#if _DEBUG
		{
			std::vector<std::u32string> Strs;
			for (auto& Ite : SymbolTuple)
			{
				Strs.push_back(Translate(Ite.StrIndex, F1, Data));
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
							auto Name = Translate(NT[1].Consume<Misc::IndexSpan<>>(), F1, Data);
							auto Index = FindOrAddSymbol(std::move(Name), TerminalMappings);
							auto Index2 = NT[3].Consume<Misc::IndexSpan<>>();
							Creator.PushRegex(Index2.Begin(), Index2.End(), F1, Data, { Index + 1, 0});
							return {};
						}
						case 3:
						{
							auto Name = Translate(NT[1].Consume<Misc::IndexSpan<>>(), F1, Data);
							auto Index = FindOrAddSymbol(std::move(Name), TerminalMappings);
							auto Index2 = NT[3].Consume<Misc::IndexSpan<>>();
							std::size_t Mask = NT[6].Consume<std::size_t>();
							Creator.PushRegex(Index2.Begin(), Index2.End(), F1, Data, { Index + 1, Mask });
							return {};
						}
						case 4:
						{
							auto Index2 = NT[3].Consume<Misc::IndexSpan<>>();
							Creator.PushRegex(Index2.Begin(), Index2.End(), F1, Data, { 0, 0});
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
							return std::move(SymbolTuple[TRef.TokenIndex].StrIndex);
						case T::Number:
						{
							std::size_t Index = 0;
							auto Buffer = Translate(SymbolTuple[TRef.TokenIndex].StrIndex, F1, Data);
							StrFormat::DirectScan(Buffer, Index);
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
				Token = SymbolTuple[US.TokenIndex].StrIndex.Begin();
			else if(!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();
			throw UnacceptableEbnf{ UnacceptableEbnf ::TypeT::WrongRegex, Token };
		}
		catch (Reg::Exception::UnaccaptableRegex const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].StrIndex.Begin();
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();
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
							std::size_t TokenIndex = SymbolTuple[Ref.FirstTokenIndex].StrIndex.Begin();
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
							std::size_t TokenIndex = SymbolTuple[Ref.FirstTokenIndex].StrIndex.Begin();
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
						auto Buffer = Translate(SymbolTuple[Ref.TokenIndex].StrIndex, F1, Data);
						StrFormat::DirectScan(Buffer, Index);
						return Index;
					}
					case T::Terminal:
					{
						auto Name = Translate(SymbolTuple[Ref.TokenIndex].StrIndex, F1, Data);
						for (std::size_t Index = 0; Index < DefineTerminalStart; ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, SymbolTuple[Ref.TokenIndex].StrIndex .Begin()};
					}
					case T::NoTerminal:
					{
						auto Name = Translate(SymbolTuple[Ref.TokenIndex].StrIndex, F1, Data);
						auto Index = FindOrAddSymbol(std::move(Name), NoTermialMapping);
						return DLr::Symbol::AsNoTerminal(Index);
					}
					case T::Rex:
					{
						auto IndexSpan = SymbolTuple[Ref.TokenIndex].StrIndex;
						auto Name = Translate(IndexSpan, F1, Data);
						for (std::size_t Index = DefineTerminalStart; Index < TerminalMappings.size(); ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						std::size_t Index = TerminalMappings.size();
						TerminalMappings.push_back(std::move(Name));
						Creator.PushRawString(IndexSpan.Begin(), IndexSpan.End(), F1, Data, { Index + 1, 0});
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
					Location = SymbolTuple.rbegin()->StrIndex.End();
				throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnfindedStartSymbol, Location };
			}
		}
		catch (DLr::Exception::UnaccableSymbol const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].StrIndex.Begin();
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();

#if _DEBUG
			{
				std::u32string Str;
				std::size_t TokenIndex = Token;
				while (true)
				{
					auto Sym = F1(TokenIndex, Data);
					if(Sym.IsEndOfFile())
						break;
					else {
						TokenIndex += Sym.NextUnicodeCodePointOffset;
						Str.push_back(Sym.UnicodeCodePoint);
					}

				}
				
				volatile int i  =0;
			}
#endif
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
						auto Name = Translate(SymbolTuple[TRef.TokenIndex].StrIndex, F1, Data);
						for (std::size_t Index = 0; Index < DefineTerminalStart; ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, SymbolTuple[TRef.TokenIndex].StrIndex.Begin() };
					}
					case T::Rex:
					{
						auto Name = Translate(SymbolTuple[TRef.TokenIndex].StrIndex, F1, Data);
						for (std::size_t Index = DefineTerminalStart; Index < TerminalMappings.size(); ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								return DLr::Symbol::AsTerminal(Index);
							}
						}
						throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableRex, SymbolTuple[TRef.TokenIndex].StrIndex.Begin()};
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
				Token = SymbolTuple[US.TokenIndex].StrIndex.Begin();
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();
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
				Token = SymbolTuple[US.TokenIndex].StrIndex.Begin();
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();
			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongLr, Token };
		}
		catch (Reg::Exception::RegexOutOfRange const&)
		{
			std::size_t Token = 0;
			if(!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();
			throw OutofRange{ OutofRange::TypeT::Regex, Token };
		}
		catch (DLr::Exception::OutOfRange const&)
		{
			std::size_t Token = 0;
			if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->StrIndex.End();
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

	std::vector<EbnfSymbolTuple> ProcessSymbol(TableWrapper Wrapper, std::size_t Start, Reg::CodePoint(*F)(std::size_t Index, void* Data), void* Data)
	{
		auto RegWrapper = Wrapper.AsRegWrapper();
		std::vector<EbnfSymbolTuple> Tuples;
		while (true)
		{
			auto Result = Reg::ProcessFrontMarch(RegWrapper, Start, F, Data);
			if (Result.has_value())
			{
				Start = Result->MainCapture.End();
				if (Result->AcceptData.Index != 0)
				{
					Misc::IndexSpan<> Span = Result->MainCapture;
					if (!Result->Captures.empty())
					{
						assert(Result->Captures[0].IsBegin);
						Span.Offset = Result->Captures[0].Index;
						Span.Length = 0;
						std::size_t Stack = 0;
						for (auto& Ite : Result->Captures)
						{
							if(Ite.IsBegin)
								++Stack;
							else
								--Stack;
							if (Stack == 0)
							{
								Span.Length = Ite.Index - Span.Offset;
								break;
							}
						}
					}
					Tuples.push_back({ DLr::Symbol::AsTerminal(Result->AcceptData.Index - 1), Span, Result->AcceptData.Mask });
				}
				if(Result->IsEndOfFile)
					break;
			}
			else {
				throw UnacceptableSymbol{ UnacceptableSymbol::TypeT::Reg, Start };
			}
		}
		return Tuples;
	}

	Misc::IndexSpan<> TranslateIndex(DLr::NTElement ELe, std::span<EbnfSymbolTuple const> InputSymbol)
	{
		if (ELe.Datas.empty())
		{
			if(ELe.FirstTokenIndex < InputSymbol.size())
				return { InputSymbol[ELe.FirstTokenIndex].Input.End(), 0};
			else if(!InputSymbol.empty())
				return { InputSymbol.rbegin()->Input.End(), 0};
			else
				return {0, 0};
		}
		else {
			auto Begin = InputSymbol[ELe.Datas.begin()->FirstTokenIndex].Input;
			auto End = InputSymbol[ELe.Datas.rbegin()->FirstTokenIndex].Input;
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
					Ele.Index = Sym.Input;
					return F(StepElement{ Ele }, Data);
				}
				return {};
				});
		}
		catch (DLr::Exception::UnaccableSymbol const& Symbol)
		{
			std::size_t Token = 0;
			if(Symbol.TokenIndex < InputSymbol.size())
				Token = InputSymbol[Symbol.TokenIndex].Input.Begin();
			else if(!InputSymbol.empty())
				Token = InputSymbol.rbegin()->Input.End();
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