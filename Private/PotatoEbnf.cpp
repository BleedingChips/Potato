#include <assert.h>
#include <vector>
#include "../Public/PotatoEbnf.h"
#include "../Public/PotatoStrFormat.h"

namespace Potato::Ebnf
{


	using namespace Exception;

	static constexpr SLRX::StandardT SmallBrace = 0;
	static constexpr SLRX::StandardT MiddleBrace = 1;
	static constexpr SLRX::StandardT BigBrace = 2;
	static constexpr SLRX::StandardT OrLeft = 3;
	static constexpr SLRX::StandardT OrRight = 4;
	static constexpr SLRX::StandardT MinMask = 5;

	using SLRX::Symbol;

	enum class T : SLRX::StandardT
	{
		Empty = 0,
		Terminal,
		Equal,
		Rex,
		NoTerminal,
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
		ItSelf,
	};

	constexpr Symbol operator*(T sym) { return Symbol::AsTerminal(static_cast<SLRX::StandardT>(sym)); };

	enum class NT : SLRX::StandardT
	{
		Statement,
		FunctionEnum,
		Expression,
		NTExpression,
		ExpressionStatement,
		ExpressionList,
		//LeftOrStatement,
		RightOrStatement,
		OrStatement,
	};

	constexpr Symbol operator*(NT sym) { return Symbol::AsNoTerminal(static_cast<SLRX::StandardT>(sym)); };

	void AddRegex(Reg::MulityRegCreater& Creator, std::u8string_view Str, T Enum)
	{
		Creator.LowPriorityLink(Str, false, {static_cast<SLRX::StandardT>(Enum)});
	}

	Reg::TableWrapper EbnfStepReg() {
			static auto List = []() {
				Reg::MulityRegCreater Creator;
				AddRegex(Creator, u8R"(%%%%)", T::Barrier);
				AddRegex(Creator, u8R"([0-9]+)", T::Number);
				AddRegex(Creator, u8R"([0-9a-zA-Z_\z]+)", T::Terminal);
				AddRegex(Creator, u8R"(<[0-9a-zA-Z_\z]+>)", T::NoTerminal);
				AddRegex(Creator, u8R"(:=)", T::Equal);
				AddRegex(Creator, u8R"(\'([^\s]+)\')", T::Rex);
				AddRegex(Creator, u8R"(;)", T::Semicolon);
				AddRegex(Creator, u8R"(:)", T::Colon);
				AddRegex(Creator, u8R"(\s+)", T::Empty);
				AddRegex(Creator, u8R"(\$)", T::Start);
				AddRegex(Creator, u8R"(\|)", T::Or);
				AddRegex(Creator, u8R"(\&)", T::ItSelf);
				AddRegex(Creator, u8R"(\[)", T::LM_Brace);
				AddRegex(Creator, u8R"(\])", T::RM_Brace);
				AddRegex(Creator, u8R"(\{)", T::LB_Brace);
				AddRegex(Creator, u8R"(\})", T::RB_Brace);
				AddRegex(Creator, u8R"(\()", T::LS_Brace);
				AddRegex(Creator, u8R"(\))", T::RS_Brace);
				AddRegex(Creator, u8R"(\+\()", T::LeftPriority);
				AddRegex(Creator, u8R"(\)\+)", T::RightPriority);
				AddRegex(Creator, u8R"(/\*.*?\*/)", T::Command);
				AddRegex(Creator, u8R"(//[^\n]*\n)", T::Command);
				return *Creator.GenerateTableBuffer();
			}();

			return Reg::TableWrapper(List);
	}

	std::size_t FindOrAddSymbol(std::u8string_view Source, std::vector<std::u8string>& Output)
	{
		auto Ite = std::find_if(Output.begin(), Output.end(), [=](std::u8string const& Ref){
			return Ref == Source;
		});
		if(Ite == Output.end())
		{
			auto Index = Output.size();
			Output.push_back(std::u8string{Source});
			return Index;
			
		}else{
			return static_cast<std::size_t>(std::distance(Output.begin(), Ite));
		}
	}

	struct LexicalTuple
	{
		SLRX::Symbol Value;
		std::u8string_view Str;
		Misc::IndexSpan<> StrIndex;
	};

	struct LexicalResult
	{
		std::vector<LexicalTuple> Datas;
		bool Accept = false;
	};


	LexicalResult ProcessLexical(std::u8string_view Str)
	{
		LexicalResult Output;
		auto StepReg = EbnfStepReg();
		std::size_t Offset = 0;
		std::u8string_view StrIte = Str;
		while (!StrIte.empty())
		{
			auto Re = Reg::HeadMatch(StepReg, StrIte, true);
			if (Re)
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
					Output.Datas.push_back({*Enum, StrIte.substr(0, Re->MainCapture.Count()), {Offset, Re->MainCapture.Count()}});
					break;
				case T::Rex:
				{
					auto Wrapper = Re->GetCaptureWrapper();
					assert(Wrapper.HasSubCapture());
					auto SpanIndex = Wrapper.GetTopSubCapture().GetCapture();
					Output.Datas.push_back({ *Enum,
						StrIte.substr(SpanIndex.Begin(), SpanIndex.Count()),
						Offset
						});
				}
					break;
				case T::Barrier:
					Output.Datas.push_back({ *Enum,
							{},
							Offset
						});
					break;
				default:
					Output.Datas.push_back({ *Enum, StrIte.substr(0, Re->MainCapture.Count()), {Offset, Re->MainCapture.Count()} });
					break;
				}
				Offset += Re->MainCapture.Count();
				StrIte = StrIte.substr(Re->MainCapture.Count());
			}
			else {
				return Output;
			}
		}
		Output.Accept = true;
		return Output;
	}

	SLRX::TableWrapper EbnfStep1SLRX() {
		static SLRX::Table Table(
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

	SLRX::TableWrapper EbnfStep2SLRX() {
		static SLRX::Table Table(
			*NT::Statement,
			{
				{*NT::Expression, {*T::Terminal}, 1},
				{*NT::Expression, {*T::NoTerminal}, 1},
				{*NT::Expression, {*T::Rex}, 4},
				{*NT::Expression, {*T::Number}, 5},
				{*NT::Expression, {*T::ItSelf}, 50},

				{*NT::FunctionEnum, {*T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace}, 6},
				{*NT::FunctionEnum, {*T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace, *T::ItSelf}, 6},
				{*NT::FunctionEnum, {}, 7},

				{*NT::ExpressionStatement, {*NT::ExpressionStatement, 31, *NT::Expression}, 8},
				{*NT::ExpressionStatement, {*NT::Expression}, 9},

				{*NT::Expression, {*T::LS_Brace, *NT::ExpressionStatement, *T::RS_Brace}, 10},
				{*NT::Expression, {*T::LB_Brace, *NT::ExpressionStatement, *T::RB_Brace}, 11},
				{*NT::Expression, {*T::LM_Brace, *NT::ExpressionStatement, *T::RM_Brace}, 12},

				{*NT::Expression, {*T::LS_Brace, *NT::OrStatement, *T::RS_Brace}, 10},
				{*NT::Expression, {*T::LB_Brace, *NT::OrStatement, *T::RB_Brace}, 11},
				{*NT::Expression, {*T::LM_Brace, *NT::OrStatement, *T::RM_Brace}, 12},

				//{*NT::LeftOrStatement, {*NT::LeftOrStatement, *NT::Expression}, 8},
				//{*NT::LeftOrStatement, {*NT::Expression}, 9},
				{*NT::RightOrStatement, {*NT::Expression, *NT::RightOrStatement}, 15},
				{*NT::RightOrStatement, {*NT::Expression}, 9},
				{*NT::OrStatement, {*NT::ExpressionStatement, *T::Or, *NT::RightOrStatement}, 17},
				{*NT::OrStatement, {*NT::OrStatement, *T::Or, *NT::RightOrStatement}, 30},
				//{*NT::ExpressionStatement, {*NT::OrStatement}, 31},

				{*NT::ExpressionList, {*NT::ExpressionStatement}, 18},
				{*NT::ExpressionList, {*NT::OrStatement}, 18},
				{*NT::ExpressionList, {}, 19},

				{*NT::Statement, {*NT::Statement, *NT::Statement, 20}, 20},
				{*NT::Statement, {*T::NoTerminal, *T::Equal, *NT::ExpressionList, *NT::FunctionEnum, *T::Semicolon}, 21},
				{*NT::Statement, {*T::Equal, *NT::ExpressionList, *NT::FunctionEnum, *T::Semicolon}, 22},
				{*NT::Statement, {*T::Start, *T::Equal, *T::NoTerminal, *T::Semicolon}, 23},
				{*NT::Statement, {*T::Start, *T::Equal, *T::NoTerminal, *T::Number, *T::Semicolon}, 23},
			},
			{}
		);
		return Table.Wrapper;
	};

	SLRX::TableWrapper EbnfStep3SLRX() {
		static SLRX::Table Table(
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

	EBNFX EBNFX::Create(std::u8string_view Str)
	{

		SLRX::StandardT TempNoTerminalCount = std::numeric_limits<SLRX::StandardT>::max();
		Reg::MulityRegCreater Creator;
		std::vector<std::u8string> TerminalMapping;
		std::vector<std::u8string> NoTerminalMapping;
		std::vector<SLRX::ProductionBuilder> Builders;
		std::optional<SLRX::Symbol> StartSymbol;
		std::size_t MaxForwardDetected = 3;

		try{

			auto SymbolTuple = ProcessLexical(Str);

			if (!SymbolTuple.Accept)
			{
				std::size_t LastTokenIndex = SymbolTuple.Datas.empty() ? 0 : SymbolTuple.Datas.rbegin()->StrIndex.End();
				throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfLexical, Str, LastTokenIndex };
			}

			auto InputSpan = std::span(SymbolTuple.Datas);

			std::size_t SpanSize = InputSpan.size();

			// Step1
			{

				SLRX::SymbolProcessor Pro(EbnfStep1SLRX());

				while(!InputSpan.empty())
				{
					auto& Cur = *InputSpan.begin();
					if (Cur.Value == *T::Barrier)
					{
						InputSpan = InputSpan.subspan(1);
						break;
					}
					else {
						if (!Pro.Consume(Cur.Value, SpanSize - InputSpan.size()))
						{
							throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfSyntax, Str, Cur.StrIndex.Begin() };
						}
					}
					InputSpan = InputSpan.subspan(1);
				}
				if (!Pro.EndOfFile())
				{
					throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfSyntax, Str, InputSpan.empty() ? InputSpan.rbegin()->StrIndex.Begin() : SpanSize };
				}

				SLRX::ProcessParsingStep(Pro.GetSteps(), [&](SLRX::VariantElement Ele) -> std::any {
					if (Ele.IsTerminal())
					{
						auto TE = Ele.AsTerminal();
						auto Enum = static_cast<T>(TE.Value.Value);
						switch (Enum)
						{
						case T::Terminal:
						case T::Rex:
							return std::u8string_view{ SymbolTuple.Datas[TE.Shift.TokenIndex].Str };
						case T::Number:
						{
							Reg::StandardT Num;
							StrFormat::DirectScan(SymbolTuple.Datas[TE.Shift.TokenIndex].Str, Num);
							return Num;
						}
						default:
							return {};
						}
					}
					else {
						auto NTE = Ele.AsNoTerminal();
						switch (NTE.Reduce.Mask)
						{
						case 2:
						{
							auto Name = NTE[1].Consume<std::u8string_view>();
							auto Sym = static_cast<Reg::StandardT>(FindOrAddSymbol(Name, TerminalMapping));
							auto Reg = NTE[3].Consume<std::u8string_view>();
							Creator.LowPriorityLink(Reg, false, { Sym, 0 });
							return {};
						}
						case 3:
						{
							auto Name = NTE[1].Consume<std::u8string_view>();
							auto Sym = static_cast<Reg::StandardT>(FindOrAddSymbol(Name, TerminalMapping));
							auto Reg = NTE[3].Consume<std::u8string_view>();
							auto Mask = NTE[6].Consume<Reg::StandardT>();
							Creator.LowPriorityLink(Reg, false, { Sym, Mask });
							return {};
						}
						case 4:
						{
							auto Reg = NTE[3].Consume<std::u8string_view>();
							Creator.LowPriorityLink(Reg, false, { std::numeric_limits<Reg::StandardT>::max(),  std::numeric_limits<Reg::StandardT>::max() });
							return {};
						}
						default:
							return {};
						}
					}
					return {};
					});
			}

			// Step2
			{

				std::optional<SLRX::Symbol> LastProductionStartSymbol;

				SLRX::SymbolProcessor Pro(EbnfStep2SLRX());

				struct FunctionEnmu
				{
					SLRX::StandardT Mask = 0;
					bool NeedPredict = false;
				};

				while (!InputSpan.empty())
				{
					auto& Cur = *InputSpan.begin();
					if (Cur.Value == *T::Barrier)
					{
						InputSpan = InputSpan.subspan(1);
						break;
					}
					else {
						if (!Pro.Consume(Cur.Value, SpanSize - InputSpan.size()))
						{
							throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfSyntax, Str, Cur.StrIndex.Begin() };
						}
					}
					InputSpan = InputSpan.subspan(1);
				}
				if (!Pro.EndOfFile())
				{
					throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfSyntax, Str, InputSpan.empty() ? InputSpan.rbegin()->StrIndex.Begin() : SpanSize };
				}

				SLRX::ProcessParsingStep(Pro.GetSteps(), [&](SLRX::VariantElement Ele) -> std::any {
					if (Ele.IsNoTerminal())
					{
						auto Ref = Ele.AsNoTerminal();
						switch (Ref.Reduce.Mask)
						{
						case 1:
						case 4:
							return SLRX::ProductionBuilderElement{ Ref[0].Consume<SLRX::Symbol>() };
						case 5:
							return SLRX::ProductionBuilderElement{ Ref[0].Consume<SLRX::StandardT>() };
						case 50:
							return SLRX::ProductionBuilderElement{ SLRX::ItSelf{} };
						case 6:
						{
							FunctionEnmu Enum;
							Enum.Mask = Ref[2].Consume<SLRX::StandardT>();
							if(Ref.Datas.size() >= 5)
								Enum.NeedPredict = true;
							return Enum;
						}
						case 7:
						{
							FunctionEnmu Enum;
							return Enum;
						}
						case 8:
						{
							auto Last = Ref[0].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							Last.push_back(Ref[1].Consume<SLRX::ProductionBuilderElement>());
							return std::move(Last);
						}
						case 9:
						{
							std::vector<SLRX::ProductionBuilderElement> Temp;
							Temp.push_back(Ref[0].Consume<SLRX::ProductionBuilderElement>());
							return std::move(Temp);
						}
						case 10:
						{
							auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
							Builders.push_back({
								CurSymbol,
								std::move(Exp),
								Ebnf::SmallBrace
								});
							return SLRX::ProductionBuilderElement{ CurSymbol };
						}
						case 11:
						{
							auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
							Exp.insert(Exp.begin(), SLRX::ProductionBuilderElement{ CurSymbol });
							Builders.push_back({
								CurSymbol,
								std::move(Exp),
								Ebnf::BigBrace
								});
							Builders.push_back({
								CurSymbol,
								{},
								Ebnf::BigBrace
								});
							return SLRX::ProductionBuilderElement{ CurSymbol };
						}
						case 12:
						{
							auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
							Builders.push_back({
								CurSymbol,
								std::move(Exp),
								Ebnf::MiddleBrace
								});
							Builders.push_back({
								CurSymbol,
								{},
								Ebnf::MiddleBrace
								});
							return SLRX::ProductionBuilderElement{ CurSymbol };
						}
						case 15:
						{
							auto Last = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							Last.push_back(Ref[0].Consume<SLRX::ProductionBuilderElement>());
							return std::move(Last);
						}
						case 17:
						{
							auto Last1 = Ref[0].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto Last2 = Ref[2].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							std::reverse(Last2.begin(), Last2.end());
							std::vector<std::vector<SLRX::ProductionBuilderElement>> Temp;
							Temp.push_back(std::move(Last1));
							Temp.push_back(std::move(Last2));
							return std::move(Temp);
						}
						case 30:
						{
							auto Last1 = Ref[0].Consume<std::vector<std::vector<SLRX::ProductionBuilderElement>>>();
							auto Last2 = Ref[2].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							std::reverse(Last2.begin(), Last2.end());
							Last1.push_back(std::move(Last2));
							return std::move(Last1);
						}
						case 31:
						{
							auto Last = Ref[0].Consume<std::vector<std::vector<SLRX::ProductionBuilderElement>>>();
							while (Last.size() >= 2)
							{
								auto L1 = std::move(*Last.rbegin());
								Last.pop_back();
								auto L2 = std::move(*Last.rbegin());
								Last.pop_back();
								auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
								Builders.push_back({
									CurSymbol,
									std::move(L1),
									Ebnf::OrLeft
									});
								Builders.push_back({
									CurSymbol,
									std::move(L2),
									Ebnf::OrRight
									});
								Last.push_back({ CurSymbol });
							}
							return std::move(Last[0]);
						}
						case 18:
							return Ref[0].Consume();
						case 19:
							return std::vector<SLRX::ProductionBuilderElement>{};
						case 20:
							return {};
						case 21:
						{
							LastProductionStartSymbol = Ref[0].Consume<SLRX::Symbol>();
							auto List = Ref[2].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto Mask = Ref[3].Consume<FunctionEnmu>();
							Builders.push_back({
								*LastProductionStartSymbol,
								std::move(List),
								Mask.Mask,
								Mask.NeedPredict
								});
							return {};
						}
						case 22:
						{
							if (!LastProductionStartSymbol.has_value()) [[unlikely]]
							{
								std::size_t TokenIndex = SymbolTuple.Datas[Ref.TokenIndex.Begin()].StrIndex.Begin();
								throw Exception::UnacceptableEbnf{ Exception::UnacceptableEbnf::TypeT::UnsetProductionHead, Str, TokenIndex };
							}
							auto List = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto Mask = Ref[2].Consume<FunctionEnmu>();
							Builders.push_back({
								*LastProductionStartSymbol,
								std::move(List),
								Mask.Mask,
								Mask.NeedPredict
								});
							return {};
						}
						case 23:
						{
							if (StartSymbol.has_value()) [[unlikely]]
							{
								std::size_t TokenIndex = SymbolTuple.Datas[Ref.TokenIndex.Begin()].StrIndex.Begin();
								throw Exception::UnacceptableEbnf{ Exception::UnacceptableEbnf::TypeT::StartSymbolAreadySet, Str, TokenIndex };
							}
							StartSymbol = Ref[2].Consume<SLRX::Symbol>();
							if (Ref.Datas.size() == 5)
								MaxForwardDetected = Ref.Datas[3].Consume<SLRX::StandardT>();
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
						case T::Number:
						{
							SLRX::StandardT Index = 0;
							StrFormat::DirectScan(SymbolTuple.Datas[Ref.Shift.TokenIndex].Str, Index);
							return Index;
						}
						case T::Terminal:
						{
							auto Name = SymbolTuple.Datas[Ref.Shift.TokenIndex].Str;
							auto Find = std::find(TerminalMapping.begin(), TerminalMapping.end(), Name);
							if(Find != TerminalMapping.end())
							{
								auto Dis = static_cast<std::size_t>(std::distance(TerminalMapping.begin(), Find));
								SLRX::StandardT Value = 0;
								Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Dis, OutofRange::TypeT::TerminalCount, Dis);
								return SLRX::Symbol::AsTerminal(Value);
							}
							throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, Str, SymbolTuple.Datas[Ref.Shift.TokenIndex].StrIndex.Begin()};
						}
						case T::NoTerminal:
						{
							auto Name = SymbolTuple.Datas[Ref.Shift.TokenIndex].Str;
							auto Index = FindOrAddSymbol(Name, NoTerminalMapping);
							SLRX::StandardT Value = 0;
							Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Index, OutofRange::TypeT::SymbolCount, Index);
							return SLRX::Symbol::AsNoTerminal(Value);
						}
						case T::Rex:
						{
							auto Name = SymbolTuple.Datas[Ref.Shift.TokenIndex].Str;
							std::size_t OldSize = TerminalMapping.size();
							auto Index = FindOrAddSymbol(Name, TerminalMapping);
							SLRX::StandardT Value;
							Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Index, OutofRange::TypeT::TerminalCount, Index);
							if(OldSize != TerminalMapping.size())
								Creator.LowPriorityLink(Name, true, {Value, 0});
							return SLRX::Symbol::AsTerminal(Value);
						}
						default:
							break;
						}
					}
					return {};
					}
				);
			}

			// Step3

			std::vector<SLRX::OpePriority> Priority;

			{
				

				SLRX::SymbolProcessor Pro(EbnfStep3SLRX());

				while(!InputSpan.empty())
				{
					auto& Cur = *InputSpan.begin();
					if (!Pro.Consume(Cur.Value, SpanSize - InputSpan.size()))
					{
						throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfSyntax, Str, Cur.StrIndex.Begin() };
					}
					InputSpan = InputSpan.subspan(1);
				}

				if(!Pro.EndOfFile())
				{
					throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnfSyntax, Str, InputSpan.empty() ? InputSpan.rbegin()->StrIndex.Begin() : SpanSize };
				}

				SLRX::ProcessParsingStep(Pro.GetSteps(), [&](SLRX::VariantElement Ele) -> std::any {
					if (Ele.IsNoTerminal())
					{
						auto NT = Ele.AsNoTerminal();
						switch (NT.Reduce.Mask)
						{
						case 1:
							return NT[0].Consume();
						case 2:
						{
							std::vector<SLRX::Symbol> Symbols;
							Symbols.push_back(NT[0].Consume<SLRX::Symbol>());
							return Symbols;
						}
						case 3:
						{
							auto Symbols = NT[0].Consume<std::vector<SLRX::Symbol>>();
							Symbols.push_back(NT[1].Consume<SLRX::Symbol>());
							return Symbols;
						}
						case 4:
						{
							auto Symbols = NT[2].Consume<std::vector<SLRX::Symbol>>();
							Priority.push_back({ std::move(Symbols), SLRX::Associativity::Right });
							return {};
						}
						case 5:
						{
							auto Symbols = NT[2].Consume<std::vector<SLRX::Symbol>>();
							Priority.push_back({ std::move(Symbols), SLRX::Associativity::Left });
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
						case T::Rex:
						{
							auto Name = SymbolTuple.Datas[TRef.Shift.TokenIndex].Str;
							auto FindIte = std::find(TerminalMapping.begin(), TerminalMapping.end(), Name);
							if(FindIte != TerminalMapping.end())
							{
								std::size_t Index = static_cast<std::size_t>(std::distance(TerminalMapping.begin(), FindIte));
								SLRX::StandardT Value;
								Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Index, OutofRange::TypeT::TerminalCount, Index);
								return SLRX::Symbol::AsTerminal(Value);
							}
							throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, Str, SymbolTuple.Datas[TRef.Shift.TokenIndex].StrIndex.Begin()};
						}
						break;
						default:
							return {};
						break;
						}
					}
					return {};
					}
				);
			}

			Reg::DFA Dfe = *Creator.GenerateDFA();
			SLRX::LRX Lrx {
				*StartSymbol,
				Builders,
				Priority,
				MaxForwardDetected
			};

			return EBNFX{ std::move(TerminalMapping), std::move(NoTerminalMapping), std::move(Dfe), std::move(Lrx)};

		}catch(Reg::Exception::UnaccaptableRegex const& R)
		{	
			throw UnacceptableRegex{R.TotalString};
		}catch(...)
		{
			throw;
		}
	}

	std::optional<std::u8string_view> EBNFX::ReadSymbol(SLRX::Symbol Value) const
	{
		if (Value.IsTerminal())
		{
			assert(Value.Value < MappingTerminalSymbols.size());
			return MappingTerminalSymbols[Value.Value];
		}
		else {
			if (Value.Value < MappingNoterminalSymbols.size())
			{
				return MappingNoterminalSymbols[Value.Value];
			}
			else {
				return {};
			}
		}
	}

	bool EBNFX::IsNoName(SLRX::Symbol Value) const
	{
		if (Value.IsTerminal())
		{
			return Value.Value >= MappingTerminalSymbols.size();
		}
		else {
			return Value.Value >= MappingNoterminalSymbols.size();
		}
	}

	std::size_t TableWrapper::CalculateRequireSpace(EBNFX const& Ref)
	{
		Misc::SerilizerHelper::Predicter<StandardT> Pre;
		Pre.WriteObjectArray<Misc::IndexSpan<StandardT>>(5);
		std::size_t CharCount = 0;
		for (auto& Ite : Ref.MappingTerminalSymbols)
		{
			CharCount += Ite.size();
		}
		Pre.WriteObjectArray<Misc::IndexSpan<StandardT>>(Ref.MappingTerminalSymbols.size());
		for (auto& Ite : Ref.MappingNoterminalSymbols)
		{
			CharCount += Ite.size();
		}
		Pre.WriteObjectArray<Misc::IndexSpan<StandardT>>(Ref.MappingNoterminalSymbols.size());
		Pre.WriteObjectArray<char8_t>(CharCount);
		Pre.WriteObjectArray<Reg::StandardT>(
			Reg::TableWrapper::CalculateRequireSpaceWithStanderT(Ref.Lexical)
		);
		Pre.WriteObjectArray<Reg::StandardT>(
			SLRX::TableWrapper::CalculateRequireSpace(Ref.Syntax)
			);
		return Pre.RequireSpace();
	}

	std::size_t TableWrapper::SerilizeTo(std::span<StandardT> OutputBuffer, EBNFX const& Ref)
	{
		assert(OutputBuffer.size() >= CalculateRequireSpace(Ref));
		Misc::SerilizerHelper::SpanReader<StandardT> Reader(OutputBuffer);

		auto ES = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(5);

		ES[0] = {static_cast<StandardT>(Reader.GetIteSpacePositon()), static_cast<StandardT>(Ref.MappingTerminalSymbols.size())};

		auto Span1 = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(Ref.MappingTerminalSymbols.size());
		
		ES[1] = {static_cast<StandardT>(Reader.GetIteSpacePositon()), static_cast<StandardT>(Ref.MappingNoterminalSymbols.size())};

		auto Span2 = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(Ref.MappingNoterminalSymbols.size());

		std::size_t TotalSize = 0;

		for (auto& Ite : Ref.MappingTerminalSymbols)
		{
			Span1[0] = {static_cast<StandardT>(TotalSize), static_cast<StandardT>(Ite.size())};
			TotalSize += Ite.size();
			Span1 = Span1.subspan(1);
		}

		for (auto& Ite : Ref.MappingNoterminalSymbols)
		{
			Span2[0] = { static_cast<StandardT>(TotalSize), static_cast<StandardT>(Ite.size()) };
			TotalSize += Ite.size();
			Span2 = Span2.subspan(1);
		}

		ES[2] = {static_cast<StandardT>(Reader.GetIteSpacePositon()), static_cast<StandardT>(TotalSize)};

		auto Span3 = *Reader.ReadObjectArray<char8_t>(TotalSize);

		for (auto& Ite : Ref.MappingTerminalSymbols)
		{
			std::memcpy(Span3.data(), Ite.data(), Ite.size() * sizeof(char8_t));
			Span3 = Span3.subspan(Ite.size());
		}
			

		for (auto& Ite : Ref.MappingNoterminalSymbols)
		{
			std::memcpy(Span3.data(), Ite.data(), Ite.size() * sizeof(char8_t));
			Span3 = Span3.subspan(Ite.size());
		}
			

		

		{
			ES[3].Offset = static_cast<StandardT>(Reader.GetIteSpacePositon());

			auto Index = Reg::TableWrapper::CalculateRequireSpaceWithStanderT(Ref.Lexical);

			auto RegSpan = *Reader.ReadObjectArray<Reg::StandardT>(Index);

			Reg::TableWrapper::SerializeTo(Ref.Lexical, RegSpan);

			ES[3].Length = static_cast<StandardT>(Index);
		}

		{
			ES[4].Offset = static_cast<StandardT>(Reader.GetIteSpacePositon());

			auto Index = SLRX::TableWrapper::CalculateRequireSpace(Ref.Syntax);

			auto Span = *Reader.ReadObjectArray<SLRX::StandardT>(Index);

			SLRX::TableWrapper::SerilizeTo(Span, Ref.Syntax);

			ES[3].Length = static_cast<StandardT>(Index);
		}

		return Reader.GetIteSpacePositon();

	}

	std::vector<StandardT> TableWrapper::Create(EBNFX const& Le)
	{
		std::vector<StandardT> Buffer;
		Buffer.resize(CalculateRequireSpace(Le));
		SerilizeTo(std::span(Buffer), Le);
		return Buffer;
	}

	SLRX::TableWrapper TableWrapper::GetSyntaxWrapper() const
	{
		assert(*this);
		Misc::SerilizerHelper::SpanReader<StandardT const> Reader(Wrapper);
		auto Span = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(5);
		Reader.ResetIteSpan(Span[4].Begin());
		return SLRX::TableWrapper{ *Reader.ReadObjectArray<SLRX::StandardT>(Span[4].Count())};
	}

	Reg::TableWrapper TableWrapper::GetLexicalWrapper() const
	{
		assert(*this);
		Misc::SerilizerHelper::SpanReader<StandardT const> Reader(Wrapper);
		auto Span = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(5);
		Reader.ResetIteSpan(Span[3].Begin());
		return Reg::TableWrapper{ *Reader.ReadObjectArray<Reg::StandardT>(Span[3].Count()) };
	}

	std::optional<std::u8string_view> TableWrapper::ReadSymbol(SLRX::Symbol Value) const
	{
		Misc::SerilizerHelper::SpanReader<StandardT const> Reader(Wrapper);
		auto Span = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(5);
		if (Value.IsTerminal())
		{
			Reader.ResetIteSpan(Span[0].Begin());
			auto Mapping = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(Span[0].Count());
			assert(Value.Value < Mapping.size());
			auto In = Mapping[Value.Value];
			Reader.ResetIteSpan(Span[2].Begin());
			auto Str = *Reader.ReadObjectArray<char8_t>(Span[2].Count());
			auto Str2 = In.Slice(Str);
			return std::u8string_view{ Str2 };
		}
		else {
			Reader.ResetIteSpan(Span[1].Begin());
			auto Mapping = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(Span[1].Count());
			if (Value.Value < Mapping.size())
			{
				auto In = Mapping[Value.Value];
				Reader.ResetIteSpan(Span[2].Begin());
				auto Str = *Reader.ReadObjectArray<char8_t>(Span[2].Count());
				auto Str2 = In.Slice(Str);
				return std::u8string_view{ Str2 };
			}
			else {
				return {};
			}
		}
	}

	bool TableWrapper::IsNoName(SLRX::Symbol Value) const
	{
		Misc::SerilizerHelper::SpanReader<StandardT const> Reader(Wrapper);
		auto Span = *Reader.ReadObjectArray<Misc::IndexSpan<StandardT>>(5);
		if (Value.IsTerminal())
		{
			return Value.Value >= Span[0].Count();
		}
		else {
			return Value.Value >= Span[1].Count();
		}
	}

	std::optional<std::u8string_view> LexicalProcessor::Consume(std::u8string_view Str, std::size_t StrOffset) {
		auto Re = Reg::HeadMatch(Pro, Str);
		Pro.Clear();
		if (Re)
		{
			if (Re.SuccessdResult->AcceptData.SubMask != std::numeric_limits<Reg::StandardT>::max())
			{
				LexicalElement NewElement;
				auto Capture = Re.SuccessdResult->GetCaptureWrapper();
				if (Capture.HasSubCapture())
				{
					auto Cap = Capture.GetTopSubCapture().GetCapture();
					NewElement.CaptureValue = Str.substr(Cap.Begin(), Cap.Count());
				}
				else {
					NewElement.CaptureValue = Str.substr(0, Re->MainCapture.Count());
				}
				NewElement.Mask = Re->AcceptData.SubMask;
				NewElement.Value = SLRX::Symbol::AsTerminal(Re->AcceptData.Mask);
				if (NewElement.Mask != std::numeric_limits<decltype(Re->AcceptData.SubMask)>::max())
				{
					if (std::holds_alternative<std::reference_wrapper<EBNFX const>>(Table))
					{
						NewElement.SymbolStr = *std::get<std::reference_wrapper<EBNFX const>>(Table).get().ReadSymbol(Symbol::AsTerminal(Re->AcceptData.Mask));
					}
					else {
						NewElement.SymbolStr = *std::get<TableWrapper>(Table).ReadSymbol(Symbol::AsTerminal(Re->AcceptData.Mask));
					}
				}
				NewElement.StrIndex = { StrOffset, Re.LastTokenIndex };
				Elements.push_back(NewElement);
			}
			StrOffset += Re.LastTokenIndex;
			Str = Str.substr(Re.LastTokenIndex);
			return Str;
		}
		else {
			return { };
		}
	}

	std::tuple<ParsingStep, std::size_t> SyntaxProcessor::Translate(SLRX::ParsingStep In, std::span<LexicalElement const> Buffer) const
	{
		std::optional<std::u8string_view> Sym;
		if (std::holds_alternative<std::reference_wrapper<EBNFX const>>(Table))
		{
			Sym = std::get<std::reference_wrapper<EBNFX const>>(Table).get().ReadSymbol(In.Value);
		}
		else {
			Sym = std::get<TableWrapper>(Table).ReadSymbol(In.Value);
		}
		if (In.IsTerminal())
		{
			assert(Sym.has_value());
			auto Ele = Buffer[In.Shift.TokenIndex];
			ParsingStep Step;
			Step.Symbol = *Sym;
			ParsingStep::ShiftT Shift{ Ele.CaptureValue, Ele.Mask, Ele.StrIndex };
			Step.Data = Shift;
			return { Step , In.Shift.TokenIndex + 1 };
		}
		else {
			ParsingStep Step;
			Step.IsPredict = In.IsPredict;
			if (Sym.has_value())
			{
				Step.Symbol = *Sym;
			}
			else {
				switch (In.Reduce.Mask)
				{
				case SmallBrace:
					Step.Symbol = u8"(...)";
					break;
				case BigBrace:
					Step.Symbol = u8"[...]";
					break;
				case MiddleBrace:
					Step.Symbol = u8"{...}";
					break;
				case OrLeft:
				case OrRight:
					Step.Symbol = u8"...|...";
					break;
				default:
					assert(false);
				}
			}
			ParsingStep::ReduceT Reduce;
			Reduce.Mask = In.Reduce.Mask;
			Reduce.ProductionElementCount = In.Reduce.ElementCount;
			Reduce.IsNoNameReduce = !Sym.has_value();
			Step.Data = Reduce;
			return { Step , 0 };
		}
	}

	bool SyntaxProcessor::Consume(LexicalElement Element)
	{
		auto Re = Pro.Consume(Element.Value, TemporaryElementBuffer.size());
		if (Re)
		{
			TemporaryElementBuffer.push_back(Element);
			std::size_t Max = 0;
			for (auto Ite : Pro.GetSteps())
			{
				auto [Step, Ind] = Translate(Ite, TemporaryElementBuffer);
				Max = std::max(Max, Ind);
				ParsingSteps.push_back(Step);
			}
			Pro.ClearSteps();
			if (Max >= TemporaryElementBuffer.size())
				TemporaryElementBuffer.clear();
		}
		return Re;
	}

	bool SyntaxProcessor::EndOfFile()
	{
		if (Pro.EndOfFile())
		{
			std::size_t Max = 0;
			for (auto Ite : Pro.GetSteps())
			{
				auto [Step, Ind] = Translate(Ite, TemporaryElementBuffer);
				Max = std::max(Max, Ind);
				ParsingSteps.push_back(Step);
			}
			Pro.Clear();
			if (Max >= TemporaryElementBuffer.size())
				TemporaryElementBuffer.clear();
			assert(TemporaryElementBuffer.empty());
			return true;
		}
		return false;
	}

	Result<std::vector<ParsingStep>> SyntaxProcessor::Process(SyntaxProcessor& Pro, std::span<LexicalElement const> Eles)
	{

		for (std::size_t I = 0; I < Eles.size(); ++I)
		{
			if (!Pro.Pro.Consume(Eles[I].Value, I))
				return { {}, I };
		}
		if (!Pro.Pro.EndOfFile())
		{
			return { {}, Eles.size() };
		}

		std::vector<ParsingStep> Temp;
		Temp.reserve(Pro.GetSteps().size());
		for (auto Ite : Pro.Pro.GetSteps())
		{
			Temp.push_back(std::get<0>(Pro.Translate(Ite, Eles)));
		}
		return { Temp, Eles.size() };
	}

	auto PasringStepProcessor::Consume(ParsingStep Input) ->std::optional<Result>
	{
		if (Input.IsTerminal())
		{
			auto Shift = std::get<ParsingStep::ShiftT>(Input.Data);
			TElement Ele{ Input.Symbol, Shift };
			return Result{ VariantElement{Ele}, NTElement::MetaData{Input.Symbol, Shift.StrIndex} };
		}
		else {
			auto Reduce = std::get<ParsingStep::ReduceT>(Input.Data);
			if(Input.IsPredict)
			{
				assert(!Reduce.IsNoNameReduce);
				PredictElement Ele {Input.Symbol, Reduce};
				return Result{ VariantElement{Ele}, {}};
			}else{
				TemporaryDatas.clear();
				std::size_t AdressSize = Datas.size() - Reduce.ProductionElementCount;
				TemporaryDatas.insert(TemporaryDatas.end(), 
					std::move_iterator(Datas.begin() + AdressSize),
					std::move_iterator(Datas.end())
				);
				Datas.resize(AdressSize);
				Misc::IndexSpan<> TokenIndex;
				if (!TemporaryDatas.empty())
				{
					auto Start = TemporaryDatas.begin()->Meta.StrIndex.Begin();
					TokenIndex = { Start, TemporaryDatas.rbegin()->Meta.StrIndex.End() - Start };
				}
				else {
					TokenIndex = { Datas.empty() ? 0 : Datas.rbegin()->Meta.StrIndex.End(), 0 };
				}
				if(Reduce.IsNoNameReduce)
				{
					switch (Reduce.Mask)
					{
					case Ebnf::SmallBrace:
					{
						if (Reduce.ProductionElementCount == 1)
						{
							auto TempCondition = TemporaryDatas[0].TryConsume<Condition>();
							if (TempCondition.has_value())
							{
								Push({Input.Symbol, TemporaryDatas[0].Meta.StrIndex}, std::move(*TempCondition));
								return Result{{}, {}};
							}
						}

						std::vector<NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(TemporaryDatas.begin()), std::move_iterator(TemporaryDatas.end()));
						Condition Con{ Condition::TypeT::Parentheses, 0, std::move(TemBuffer)};
						Push({Input.Symbol, TokenIndex }, std::move(Con));
						return Result{{}, {}};
					}
					case Ebnf::MiddleBrace:
					{
						std::vector<NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(TemporaryDatas.begin()), std::move_iterator(TemporaryDatas.end()));
						Condition Con{ Condition::TypeT::SquareBrackets, 0, std::move(TemBuffer) };
						Push({ Input.Symbol, TokenIndex }, std::move(Con));
						return Result{ {}, {} };
					}
					case Ebnf::BigBrace:
					{
						if (TemporaryDatas.empty())
						{
							Push({ Input.Symbol, TokenIndex }, Condition{ Condition::TypeT::CurlyBrackets, 0, {} });
							return Result{ {}, {} };
						}
						else {
							auto Temp = TemporaryDatas[0].Consume<Condition>();
							Temp.Datas.insert(Temp.Datas.end(), std::move_iterator(TemporaryDatas.begin() + 1), std::move_iterator(TemporaryDatas.end()));
							Temp.AppendData += 1;
							Push({ Input.Symbol, TokenIndex }, std::move(Temp));
							return Result{ {}, {} };
						}
					}
					case Ebnf::OrLeft:
					{
						std::vector<NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(TemporaryDatas.begin()), std::move_iterator(TemporaryDatas.end()));
						Condition Con{ Condition::TypeT::Or, 0, std::move(TemBuffer) };
						Push({ Input.Symbol, TokenIndex }, std::move(Con));
						return Result{ {}, {} };
					}
					case Ebnf::OrRight:
					{
						if (TemporaryDatas.size() == 1)
						{
							auto Temp = TemporaryDatas[0].TryConsume<Condition>();
							if (Temp.has_value())
							{
								if (Temp->Type == Condition::TypeT::Or)
								{
									Temp->AppendData += 1;
									Push({ Input.Symbol, TokenIndex }, std::move(Temp));
									return Result{ {}, {} };
								}
								else
									TemporaryDatas[0].Data.Data = std::move(Temp);
							}
						}
						std::vector<NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(TemporaryDatas.begin()), std::move_iterator(TemporaryDatas.end()));
						Condition Con{ Condition::TypeT::Or, 1, std::move(TemBuffer) };
						Push({ Input.Symbol, TokenIndex }, std::move(Con));
						return Result{ {}, {} };
					}
					default:
						assert(false);
						return {};
						break;
					}
				}
				else{
					NTElement Ele{ Input.Symbol, Reduce, std::span(TemporaryDatas)};
					return Result{ VariantElement{Ele}, NTElement::MetaData{Input.Symbol, TokenIndex} };
				}
			}
		}
	}


	std::optional<std::any> PasringStepProcessor::EndOfFile() 
	{
		if (Datas.size() == 1)
		{
			return Datas[0].Consume();
		}
		return {};
	}
}

namespace Potato::Ebnf::Exception
{
	char const* Interface::what() const { return "Ebnf Exception"; }
	char const* UnacceptableRegex::what() const { return "UnacceptableRegex Exception"; }
	char const* UnacceptableEbnf::what() const { return "UnacceptableEbnf Exception"; }
	char const* OutofRange::what() const { return "OutofRange Exception"; }
	char const* IllegalEbnfProduction::what() const { return "IllegalEbnfProduction Exception"; }

	auto IllegalEbnfProduction::Translate(std::span<SLRX::ParsingStep const> Steps, std::vector<std::u32string> const& TMapping, std::vector<std::u32string> const& NTMapping) -> std::vector<std::variant<TMap, NTMap>>
	{
		std::vector<std::variant<TMap, NTMap>> Result;
		Result.reserve(Steps.size());
		for(auto Ite : Steps)
		{
			if (Ite.IsTerminal())
			{
				
				if (Ite.Value.IsEndOfFile())
				{
					Result.push_back({ TMap{UR"($EndOfFile)"}});
				}
				else {
					assert(Ite.Value.Value < TMapping.size());
					Result.push_back({ TMap{TMapping[Ite.Value.Value]} });
				}
			}
			else if(Ite.IsNoTerminal()) 
			{
				if (Ite.Value.Value < NTMapping.size())
				{
					assert(Ite.Reduce.Mask >= MinMask);
					Result.push_back({ NTMap{NTMapping[Ite.Value.Value], Ite.Reduce.ElementCount, Ite.Reduce.Mask - MinMask} });
				}
				else {
					static constexpr SLRX::StandardT SmallBrace = 0;
					static constexpr SLRX::StandardT MiddleBrace = 1;
					static constexpr SLRX::StandardT BigBrace = 2;
					static constexpr SLRX::StandardT OrLeft = 3;
					static constexpr SLRX::StandardT OrRight = 4;
					static constexpr SLRX::StandardT MinMask = 5;
					switch (Ite.Reduce.Mask)
					{
						
					case SmallBrace:
						Result.push_back({ NTMap{UR"((...))", 0, 0}});
						break;
					case MiddleBrace:
						Result.push_back({ NTMap{UR"([...])", 0, 0} });
						break;
					case BigBrace:
						Result.push_back({ NTMap{UR"({...})", 0, 0} });
						break;
					case OrLeft:
						Result.push_back({ NTMap{UR"(...|)", 0, 0} });
						break;
					case OrRight:
						Result.push_back({ NTMap{UR"(|...)", 0, 0} });
						break;
					default:
						assert(false);
						break;
					}
				}
			}
		}
		return Result;
	}

	IllegalEbnfProduction::IllegalEbnfProduction(SLRX::Exception::IllegalSLRXProduction const& IS, std::vector<std::u32string> const& TMapping, std::vector<std::u32string> const& NTMapping)
		: Type(IS.Type), MaxForwardDetectNum(IS.MaxForwardDetectNum), DetectNum(IS.MaxForwardDetectNum)
		, Steps1(Translate(IS.Steps1, TMapping, NTMapping)), Steps2(Translate(IS.Steps2, TMapping, NTMapping))
	{
	}

}