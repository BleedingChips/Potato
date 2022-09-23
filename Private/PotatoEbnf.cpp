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
				AddRegex(Creator, u8R"(<\$([0-9]+)>)", T::NoProductionNoTerminal);
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
				case T::NoProductionNoTerminal:
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
				{*NT::Expression, {*T::NoProductionNoTerminal}, 40},

				{*NT::FunctionEnum, {*T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace}, 6},
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

				SLRX::TableProcessor Pro(EbnfStep1SLRX());

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
							return std::u8string_view{ SymbolTuple.Datas[TE.TokenIndex].Str };
						case T::Number:
						{
							Reg::StandardT Num;
							StrFormat::DirectScan(SymbolTuple.Datas[TE.TokenIndex].Str, Num);
							return Num;
						}
						default:
							return {};
						}
					}
					else {
						auto NTE = Ele.AsNoTerminal();
						switch (NTE.Mask)
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

				SLRX::TableProcessor Pro(EbnfStep2SLRX());

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
						switch (Ref.Mask)
						{
						case 1:
						case 4:
							return SLRX::ProductionBuilderElement{ Ref[0].Consume<SLRX::Symbol>() };
						case 5:
							return SLRX::ProductionBuilderElement{ Ref[0].Consume<SLRX::StandardT>() + Ebnf::MinMask };
						case 50:
							return SLRX::ProductionBuilderElement{ SLRX::ItSelf{} };
						case 40:
						{
							auto Exp = Ref[0].Consume<SLRX::StandardT>();
							auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
							Builders.push_back({
								CurSymbol,
								Exp + Ebnf::MinMask
								});
							return SLRX::ProductionBuilderElement{ CurSymbol };
						}
						case 6:
							return Ref[2].Consume<SLRX::StandardT>() + Ebnf::MinMask;
						case 7:
							return Ebnf::MinMask;
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
							auto Mask = Ref[3].Consume<SLRX::StandardT>();
							Builders.push_back({
								*LastProductionStartSymbol,
								std::move(List),
								Mask
								});
							return {};
						}
						case 22:
						{
							if (!LastProductionStartSymbol.has_value()) [[unlikely]]
							{
								std::size_t TokenIndex = SymbolTuple.Datas[Ref.FirstTokenIndex].StrIndex.Begin();
								throw Exception::UnacceptableEbnf{ Exception::UnacceptableEbnf::TypeT::UnsetProductionHead, Str, TokenIndex };
							}
							auto List = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto Mask = Ref[2].Consume<SLRX::StandardT>();
							Builders.push_back({
								*LastProductionStartSymbol,
								std::move(List),
								Mask
								});
							return {};
						}
						case 23:
						{
							if (StartSymbol.has_value()) [[unlikely]]
							{
								std::size_t TokenIndex = SymbolTuple.Datas[Ref.FirstTokenIndex].StrIndex.Begin();
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
						case T::NoProductionNoTerminal:
						case T::Number:
						{
							SLRX::StandardT Index = 0;
							StrFormat::DirectScan(SymbolTuple.Datas[Ref.TokenIndex].Str, Index);
							return Index;
						}
						case T::Terminal:
						{
							auto Name = SymbolTuple.Datas[Ref.TokenIndex].Str;
							auto Find = std::find(TerminalMapping.begin(), TerminalMapping.end(), Name);
							if(Find != TerminalMapping.end())
							{
								auto Dis = static_cast<std::size_t>(std::distance(TerminalMapping.begin(), Find));
								SLRX::StandardT Value = 0;
								Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Dis, OutofRange::TypeT::TerminalCount, Dis);
								return SLRX::Symbol::AsTerminal(Value);
							}
							throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, Str, SymbolTuple.Datas[Ref.TokenIndex].StrIndex.Begin()};
						}
						case T::NoTerminal:
						{
							auto Name = SymbolTuple.Datas[Ref.TokenIndex].Str;
							auto Index = FindOrAddSymbol(Name, NoTerminalMapping);
							SLRX::StandardT Value = 0;
							Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Index, OutofRange::TypeT::SymbolCount, Index);
							return SLRX::Symbol::AsNoTerminal(Value);
						}
						case T::Rex:
						{
							auto Name = SymbolTuple.Datas[Ref.TokenIndex].Str;
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
				

				SLRX::TableProcessor Pro(EbnfStep3SLRX());

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
						switch (NT.Mask)
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
							auto Name = SymbolTuple.Datas[TRef.TokenIndex].Str;
							auto FindIte = std::find(TerminalMapping.begin(), TerminalMapping.end(), Name);
							if(FindIte != TerminalMapping.end())
							{
								std::size_t Index = static_cast<std::size_t>(std::distance(TerminalMapping.begin(), FindIte));
								SLRX::StandardT Value;
								Misc::SerilizerHelper::CrossTypeSetThrow<OutofRange>(Value, Index, OutofRange::TypeT::TerminalCount, Index);
								return SLRX::Symbol::AsTerminal(Value);
							}
							throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, Str, SymbolTuple.Datas[TRef.TokenIndex].StrIndex.Begin()};
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
				Priority
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

	template<typename CharT>
	auto TempCoreProConsume(EBNFX const& Table, std::basic_string_view<CharT> Str) ->CoreProcessor::Result<LexicalElement>
	{
		auto Re = Reg::HeadMatch(Table.Lexical, Str, true);
		if (Re)
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
			return {NewElement, Re.LastTokenIndex};
		}
		else {
			return {{}, Re.LastTokenIndex};
		}
	}

	auto CoreProcessor::LexicalConsumeOnce(EBNFX const& Table, std::u8string_view Str) -> Result<LexicalElement>
	{
		return TempCoreProConsume(Table, Str);
	}

	auto CoreProcessor::LexicalProcess(EBNFX const& Table, std::u8string_view Str) ->Result<std::vector<LexicalElementTuple>>
	{
		std::vector<LexicalElementTuple> Output;
		std::size_t TotalSize = Str.size();
		while (!Str.empty())
		{
			auto Re = CoreProcessor::LexicalConsumeOnce(Table, Str);
			if (Re)
			{
				//if(Re.Element->)
				if (Re.Element->Mask != std::numeric_limits<Reg::StandardT>::max())
				{
					Output.push_back({ *Re.Element, {TotalSize - Str.size(), Re.Length} });
				}
				Str = Str.substr(Re.Length);
			}
			else {
				return {{}, TotalSize - Str.size()};
			}
		}
		return { std::move(Output), TotalSize - Str.size()};
	}

	Condition::TypeT Translate(SLRX::StandardT Mask)
	{
		switch (Mask)
		{
		case SmallBrace:
			return Condition::TypeT::Parentheses;
		case MiddleBrace:
			return Condition::TypeT::SquareBrackets;
		case BigBrace:
			return Condition::TypeT::CurlyBrackets;
		case OrLeft:
		case OrRight:
			return Condition::TypeT::Or;
		default:
			return Condition::TypeT::Normal;
		}
	}

	auto CoreProcessor::SynatxProcess(EBNFX const& Table, std::span<LexicalElementTuple const> Tuples) ->Result<std::vector<ParsingStep>>
	{
		std::size_t TotalSize = Tuples.size();
		SLRX::LRXProcessor Pro(Table.Syntax);
		auto TuplesIte = Tuples;
		while (!TuplesIte.empty())
		{
			auto Cur = TuplesIte[0];
			if (!Pro.Consume(Cur.Ele.Value, TotalSize - TuplesIte.size()))
			{
				return {{}, TotalSize - TuplesIte.size() };
			}
			TuplesIte = TuplesIte.subspan(1);
		}
		if (!Pro.EndOfFile())
		{
			return { {}, TotalSize - TuplesIte.size() };
		}
		std::vector<ParsingStep> Steps;
		
		auto StepSpan = Pro.GetSteps();
		std::size_t StepTotalSize = StepSpan.size();
		Steps.reserve(StepTotalSize);
		while (!StepSpan.empty())
		{
			auto Cur = StepSpan[0];
			if (Cur.Value.IsTerminal())
			{
				assert(Cur.Shift.TokenIndex < Tuples.size());
				auto Ele = Tuples[Cur.Shift.TokenIndex];
				ParsingStep::ShiftT Shift{
					Ele.Ele.CaptureValue,
					Ele.Ele.Mask,
					Cur.Shift.TokenIndex
				};
				Steps.push_back({Cur.Value, Shift});
			}
			else {

				ParsingStep::ReduceT Reduce{
					Cur.Reduce.ProductionIndex,
					Cur.Reduce.ProductionCount,
					Cur.Reduce.Mask
				};

				Steps.push_back({ Cur.Value, Reduce });
			}
			StepSpan = StepSpan.subspan(1);
		}
		return { std::move(Steps), TotalSize };

	}

	/*
	std::any StepProcess(std::span<ParsingStep const> Tuples, std::function<std::any(VariantElement Ele)> Func)
	{
		assert(Func);
		std::vector<SLRX::NTElement::DataT> Datas;
		while (!Tuples.empty())
		{
			auto Input = Tuples[0];

			if (Input.Value.IsTerminal())
			{
				TElement Ele{ Input.Value, std::get<ParsingStep::ShiftT>(Input.Datas) };
				
				auto Result = Func(VariantElement{ std::move(Ele)});
				Datas.push_back({ Ele.Value, 0, std::move(Result) });
			}
			else {
				auto Ref = std::get<ParsingStep::ReduceT>(Input.Datas);
				assert(Datas.size() >= Ref.ProductionCount);
				auto CurDatasSpan = std::span(Datas).subspan(Datas.size() - Ref.ProductionCount, Ref.ProductionCount);
				switch (Ref.Mask)
				{
				case Ebnf::SmallBrace:
				{
					if (Ref.ProductionCount == 1)
					{
						auto TempCondition = CurDatasSpan[0].TryConsume<Condition>();
						if (TempCondition.has_value())
							return *TempCondition;
					}
					std::vector<SLRX::NTElement::DataT> TemBuffer;
					TemBuffer.insert(TemBuffer.end(), std::move_iterator(CurDatasSpan.begin()), std::move_iterator(CurDatasSpan.end()));
					return Condition{ Condition::TypeT::Parentheses, 0, std::move(TemBuffer) };
				}
				case Ebnf::MiddleBrace:
				{
					std::vector<SLRX::NTElement::DataT> TemBuffer;
					TemBuffer.insert(TemBuffer.end(), std::move_iterator(CurDatasSpan.begin()), std::move_iterator(CurDatasSpan.end()));
					return Condition{ Condition::TypeT::SquareBrackets, 0, std::move(TemBuffer) };
				}
				case Ebnf::BigBrace:
				{
					if (NT.Datas.empty())
					{
						return Condition{ Condition::TypeT::CurlyBrackets, 0, {} };
					}
					else {
						auto Temp = NT[0].Consume<Condition>();
						auto Span = std::span(NT.Datas).subspan(1);
						Temp.Datas.insert(Temp.Datas.end(), std::move_iterator(Span.begin()), std::move_iterator(Span.end()));
						Temp.AppendData += 1;
						return std::move(Temp);
					}
				}
				case Ebnf::OrLeft:
				{
					std::vector<SLRX::NTElement::DataT> TemBuffer;
					TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
					return Condition{ Condition::TypeT::Or, 0, Index, std::move(TemBuffer) };
				}
				case Ebnf::OrRight:
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
					std::vector<SLRX::NTElement::DataT> TemBuffer;
					TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
					return Condition{ Condition::TypeT::Or, 1, Index, std::move(TemBuffer) };
				}
				default:
				{
					NTElement NEle;
					NEle.Value = NT.Value;
					NEle.Mask = NT.Mask - Ebnf::MinMask;
					NEle.Datas = NT.Datas;
					NEle.Index = Index;
					return F(StepElement{ NEle }, Data);
				}
				}



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
			Tuples = Tuples.subspan(1);
		}
	}
	*/
		/*
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
		{
			SLRX::TableProcessor Pro(EbnfStep1SLRX());

			for (auto Index = StepIndexs[0].Begin(); Index < StepIndexs[0].End(); ++Index)
			{
				if (!Pro.Consume(SymbolTuple[Index].Value, Index))
				{
					throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnf, SymbolTuple[Index].Offset};
				}
			}

			if(!Pro.EndOfFile())
			{
				if(StepIndexs.size() >= StepIndexs[0].End())
				{
					auto& Ref = SymbolTuple[StepIndexs[0].End() - 1];
					throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongEbnf, Ref.Offset + Ref.Str.size()};
				}
					
			}
				
		}
		

		// Step1
		try
		{

			

			assert(Re2);

			auto Steps = Pro.EndOfFile();

			SLRX::ProcessParsingStep(Steps, [&](SLRX::VariantElement Ele) -> std::any
				{
					if (Ele.IsNoTerminal())
					{
						auto& NT = Ele.AsNoTerminal();
						switch (NT.Mask)
						{
						case 2:
						{
							auto Name = NT[1].Consume<std::u32string_view>();
							auto Index = FindOrAddSymbol(Name, TerminalMappings) + 1;
							auto SI = Misc::SerilizerHelper::CrossType<Reg::StandardT, OutofRange>(Index, OutofRange::TypeT::SymbolCount, Index);
							auto Index2 = NT[3].Consume<std::u32string_view>();
							Creator.AddRegex(Index2, { 0 }, SI);
							return {};
						}
						case 3:
						{
							auto Name = NT[1].Consume<std::u32string_view>();
							auto Index = FindOrAddSymbol(Name, TerminalMappings) + 1;
							auto SI = Misc::SerilizerHelper::CrossType<Reg::StandardT, OutofRange>(Index, OutofRange::TypeT::SymbolCount, Index);
							auto Index2 = NT[3].Consume<std::u32string_view>();
							auto Mask = NT[6].Consume<Reg::StandardT>();
							Creator.AddRegex(Index2, { Mask }, SI);
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
							Reg::StandardT Index = 0;
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
		catch (SLRX::Exception::UnaccableSymbol const& US)
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

		std::vector<SLRX::ProductionBuilder> Builder;
		std::optional<SLRX::Symbol> StartSymbol;
		std::size_t DefineTerminalStart = TerminalMappings.size();
		std::optional<SLRX::StandardT> MaxForwardDetected;
		// Step2
		try
		{

			std::optional<SLRX::Symbol> LastProductionStartSymbol;

			

			SLRX::CoreProcessor Pro(EbnfStep2SLRX());

			for (auto Index = StepIndexs[1].Begin(); Index < StepIndexs[1].End(); ++Index)
			{
				Pro.Consume(SymbolTuple[Index].Value, Index);
			}
			auto Steps = Pro.EndOfFile();

			SLRX::ProcessParsingStep(Steps, [&](SLRX::VariantElement Ele) -> std::any{
				if (Ele.IsNoTerminal())
				{
					auto Ref = Ele.AsNoTerminal();
					switch (Ref.Mask)
					{
					case 1:
					case 4:
						return SLRX::ProductionBuilderElement{Ref[0].Consume<SLRX::Symbol>()};
					case 5:
						return SLRX::ProductionBuilderElement{ Ref[0].Consume<SLRX::StandardT>() + Ebnf::MinMask };						
					case 50:
						return SLRX::ProductionBuilderElement{SLRX::ItSelf{}};
					case 40:
					{
						auto Exp = Ref[0].Consume<SLRX::StandardT>();
						auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Builder.push_back({
							CurSymbol,
							Exp + Ebnf::MinMask
							});
						return SLRX::ProductionBuilderElement{ CurSymbol };
					}
					case 6:
						return Ref[2].Consume<SLRX::StandardT>() + Ebnf::MinMask;
					case 7:
						return Ebnf::MinMask;
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
						Builder.push_back({
							CurSymbol,
							std::move(Exp),
							Ebnf::SmallBrace
							});
						return SLRX::ProductionBuilderElement{ CurSymbol};
					}
					case 11:
					{
						auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Exp.insert(Exp.begin(), SLRX::ProductionBuilderElement{ CurSymbol});
						Builder.push_back({
							CurSymbol,
							std::move(Exp),
							Ebnf::BigBrace
							});
						Builder.push_back({
							CurSymbol,
							Ebnf::BigBrace
							});
						return SLRX::ProductionBuilderElement{ CurSymbol};
					}
					case 12:
					{
						auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount--);
						Builder.push_back({
							CurSymbol,
							std::move(Exp),
							Ebnf::MiddleBrace
							});
						Builder.push_back({
							CurSymbol,
							Ebnf::MiddleBrace
							});
						return SLRX::ProductionBuilderElement{ CurSymbol};
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
							Builder.push_back({
								CurSymbol,
								std::move(L1),
								Ebnf::OrLeft
								});
							Builder.push_back({
								CurSymbol,
								std::move(L2),
								Ebnf::OrRight
								});
							Last.push_back({CurSymbol});
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
						auto Mask = Ref[3].Consume<SLRX::StandardT>();
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
						auto List = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						auto Mask = Ref[2].Consume<SLRX::StandardT>();
						Builder.push_back({
							*LastProductionStartSymbol,
							std::move(List),
							Mask
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
						StartSymbol = Ref[2].Consume<SLRX::Symbol>();
						if(Ref.Datas.size() == 5)
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
					case T::NoProductionNoTerminal:
					case T::Number:
					{
						SLRX::StandardT Index = 0;
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
								auto SI = Misc::SerilizerHelper::CrossType<SLRX::StandardT, OutofRange>(Index, OutofRange::TypeT::TerminalCount, Index);
								return SLRX::Symbol::AsTerminal(SI);
							}
						}
						throw Exception::UnacceptableEbnf{ UnacceptableEbnf::TypeT::UnrecognizableTerminal, SymbolTuple[Ref.TokenIndex].Offset};
					}
					case T::NoTerminal:
					{
						auto Name = SymbolTuple[Ref.TokenIndex].Str;
						auto Index = FindOrAddSymbol(std::move(Name), NoTermialMapping);
						auto SI = Misc::SerilizerHelper::CrossType<SLRX::StandardT, OutofRange>(Index, OutofRange::TypeT::SymbolCount, Index);
						return SLRX::Symbol::AsNoTerminal(SI);

					}
					case T::Rex:
					{
						auto Name = SymbolTuple[Ref.TokenIndex].Str;
						for (std::size_t Index = DefineTerminalStart; Index < TerminalMappings.size(); ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								auto SI = Misc::SerilizerHelper::CrossType<SLRX::StandardT, OutofRange>(Index, OutofRange::TypeT::TerminalCount, Index);
								return SLRX::Symbol::AsTerminal(SI);
							}
						}
						std::size_t Index = TerminalMappings.size();
						auto SI = Misc::SerilizerHelper::CrossType<SLRX::StandardT, OutofRange>(Index, OutofRange::TypeT::TerminalCount, Index);
						TerminalMappings.push_back(std::u32string{Name});
						Creator.AddRegex(Name, {0}, SI + 1, true);
						return SLRX::Symbol::AsTerminal(SI);
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
		catch (SLRX::Exception::UnaccableSymbol const& US)
		{
			std::size_t Token = 0;
			if (SymbolTuple.size() > US.TokenIndex)
				Token = SymbolTuple[US.TokenIndex].Offset;
			else if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;

			throw UnacceptableEbnf{ UnacceptableEbnf::TypeT::WrongLr, Token };
		}
#ifdef _DEBUG
		catch (SLRX::Exception::IllegalSLRXProduction const& Pro)
		{

			struct Ter
			{
				T Value;
			};

			struct NTer
			{
				NT Value;
				std::size_t Count;
				std::size_t Mask;
			};

			std::vector<std::variant<Ter, NTer>> Symbols;
			std::vector<std::variant<Ter, NTer>> Symbols2;

			for (auto& Ite : Pro.Steps1)
			{
				if (Ite.IsTerminal())
				{
					Symbols.push_back(Ter{ static_cast<T>(Ite.Value.Value) });
				}
				else {
					Symbols.push_back(NTer{ static_cast<NT>(Ite.Value.Value), Ite.Reduce.ProductionCount, Ite.Reduce.Mask });
				}
			}

			for (auto& Ite : Pro.Steps2)
			{
				if (Ite.IsTerminal())
				{
					Symbols2.push_back(Ter{ static_cast<T>(Ite.Value.Value) });
				}
				else {
					Symbols2.push_back(NTer{ static_cast<NT>(Ite.Value.Value), Ite.Reduce.ProductionCount, Ite.Reduce.Mask });
				}
			}


			throw;
		}
#endif
		catch (...)
		{
			throw;
		}

		std::vector<SLRX::OpePriority> Priority;
		// Step3
		try {


			SLRX::CoreProcessor Pro(EbnfStep3SLRX());

			for (auto Index = StepIndexs[2].Begin(); Index < StepIndexs[2].End(); ++Index)
			{
				Pro.Consume(SymbolTuple[Index].Value, Index);
			}

			auto Steps = Pro.EndOfFile();
			
			SLRX::ProcessParsingStep(Steps, [&](SLRX::VariantElement Ele) -> std::any{
				if (Ele.IsNoTerminal())
				{
					auto NT = Ele.AsNoTerminal();
					switch (NT.Mask)
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
						Priority.push_back({std::move(Symbols), SLRX::Associativity::Right});
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
					{
						auto Name = SymbolTuple[TRef.TokenIndex].Str;
						for (std::size_t Index = 0; Index < DefineTerminalStart; ++Index)
						{
							if (TerminalMappings[Index] == Name)
							{
								auto SI = Misc::SerilizerHelper::CrossType<SLRX::StandardT, OutofRange>(Index, OutofRange::TypeT::TerminalCount, Index);

								return SLRX::Symbol::AsTerminal(SI);
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
								auto SI = Misc::SerilizerHelper::CrossType<SLRX::StandardT, OutofRange>(Index, OutofRange::TypeT::TerminalCount, Index);
								return SLRX::Symbol::AsTerminal(SI);
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
		catch (SLRX::Exception::UnaccableSymbol const& US)
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

			RegTable = Creator.Generate();
			if (!MaxForwardDetected.has_value())
			{
				DLrTable = SLRX::TableWrapper::Create(
					*StartSymbol,
					std::move(Builder),
					std::move(Priority)
				);
			}
			else {
				DLrTable = SLRX::TableWrapper::Create(
					*StartSymbol,
					std::move(Builder),
					std::move(Priority),
					*MaxForwardDetected
				);
			}
			
		}
		catch (SLRX::Exception::UnaccableSymbol const& US)
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
			throw OutofRange{ OutofRange::TypeT::FromRegex, Token };
		}
		catch (SLRX::Exception::OutOfRange const&)
		{
			std::size_t Token = 0;
			if (!SymbolTuple.empty())
				Token = SymbolTuple.rbegin()->Str.size() + SymbolTuple.rbegin()->Offset;
			throw OutofRange{ OutofRange::TypeT::FromSLRX, Token };
		}
		catch (SLRX::Exception::IllegalSLRXProduction const& LR)
		{
			throw IllegalEbnfProduction{LR, TerminalMappings, NoTermialMapping};
		}
		catch (...)
		{
			throw;
		}
	}
	*/

	/*
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
			TryCrossTypeSet<OutofRange>(Temp.Offset, NameStorage.size(), OutofRange ::TypeT::Header, NameStorage.size());
			TryCrossTypeSet<OutofRange>(Temp.Length, Ite.size(), OutofRange::TypeT::Header, Ite.size());
			NameStorage.insert(NameStorage.end(), Ite.begin(), Ite.end());
			Index.push_back(Temp);
		}
		for (auto& Ite : Table.NoTermialMapping)
		{
			Misc::IndexSpan<SubStorageT> Temp;
			TryCrossTypeSet<OutofRange>(Temp.Offset, NameStorage.size(), OutofRange::TypeT::Header, NameStorage.size() );
			TryCrossTypeSet<OutofRange>(Temp.Length, Ite.size(), OutofRange::TypeT::Header, Ite.size() );
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
		TryCrossTypeSet<OutofRange>(HeadPtr->TerminalNameCount, Table.TerminalMappings.size(), OutofRange::TypeT::Header, Table.TerminalMappings.size());
		TryCrossTypeSet<OutofRange>(HeadPtr->NameIndexCount, Index.size(), OutofRange::TypeT::Header, Index.size());
		TryCrossTypeSet<OutofRange>(HeadPtr->NameStringCount, NameStorage.size(), OutofRange::TypeT::Header, NameStorage.size());
		TryCrossTypeSet<OutofRange>(HeadPtr->RegCount, RegLast.WriteLength, OutofRange::TypeT::Header, RegLast.WriteLength );
		TryCrossTypeSet<OutofRange>(HeadPtr->DLrCount, DLrLast.WriteLength, OutofRange::TypeT::Header, DLrLast.WriteLength );
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
			auto Reg = ReadObjectArray<Reg::TableWrapper::StandardT const>(Index.LastSpan, Head->RegCount);
			auto DLr = ReadObjectArray<Reg::TableWrapper::StandardT const>(Reg.LastSpan, Head->DLrCount);
			TotalName = std::u32string_view(Name->data(), Name->size());
			StrIndex = *Index;
			TerminalCount = Head->TerminalNameCount;
			RegWrapper = *Reg;
			SLRXWrapper = *DLr;
		}
	}

	std::u32string_view TableWrapper::FindSymbolName(SLRX::Symbol Symbol) const
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
					Re2.Value = SLRX::Symbol::AsTerminal(Re->Accept->UniqueID - 1);
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

	Misc::IndexSpan<> TranslateIndex(SLRX::NTElement ELe, std::span<EbnfSymbolTuple const> InputSymbol)
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

			SLRX::CoreProcessor Pro(Wrapper.AsSLRXWrapper());

			for(auto& Ite : InputSymbol)
				Pro.Consume(Ite.Value);

			auto Steps = Pro.EndOfFile();

			return *SLRX::ProcessParsingStep(Steps, [=](SLRX::VariantElement Ele)->std::any {
				if (Ele.IsNoTerminal())
				{
					auto NT = Ele.AsNoTerminal();
					Misc::IndexSpan<> Index = TranslateIndex(NT, InputSymbol);
					switch (NT.Mask)
					{
					case Ebnf::SmallBrace:
					{
						if (NT.Datas.size() == 1)
						{
							auto TempCondition = NT[0].TryConsume<Condition>();
							if (TempCondition.has_value())
								return *TempCondition;
						}
						std::vector<SLRX::NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::Parentheses, 0, Index, std::move(TemBuffer) };
					}
					case Ebnf::MiddleBrace:
					{
						std::vector<SLRX::NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::SquareBrackets, 0, Index, std::move(TemBuffer) };
					}
					case Ebnf::BigBrace:
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
					case Ebnf::OrLeft:
					{
						std::vector<SLRX::NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::Or, 0, Index, std::move(TemBuffer) };
					}
					case Ebnf::OrRight:
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
						std::vector<SLRX::NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(NT.Datas.begin()), std::move_iterator(NT.Datas.end()));
						return Condition{ Condition::TypeT::Or, 1, Index, std::move(TemBuffer) };
					}
					default:
					{
						NTElement NEle;
						NEle.Value = NT.Value;
						NEle.Mask = NT.Mask - Ebnf::MinMask;
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
		catch (SLRX::Exception::UnaccableSymbol const& Symbol)
		{
			std::size_t Token = 0;
			if(Symbol.TokenIndex < InputSymbol.size())
				Token = InputSymbol[Symbol.TokenIndex].StrIndex.Begin();
			else if(!InputSymbol.empty())
				Token = InputSymbol.rbegin()->StrIndex.End();
			if (Symbol.LastSymbol.IsEndOfFile())
			{
				throw UnacceptableInput{UR"($EndOfFile)", Token };
			}
			else {
				throw UnacceptableInput{ std::u32string{Wrapper.FindSymbolName(Symbol.LastSymbol)}, Token};
			}
		}
	}

}
*/
}

namespace Potato::Ebnf::Exception
{
	char const* Interface::what() const { return "Ebnf Exception"; }
	char const* UnacceptableRegex::what() const { return "UnacceptableRegex Exception"; }
	char const* UnacceptableEbnf::what() const { return "UnacceptableEbnf Exception"; }
	char const* OutofRange::what() const { return "OutofRange Exception"; }
	char const* UnacceptableSymbol::what() const { return "UnacceptableSymbol Exception"; }
	char const* UnacceptableInput::what() const { return "UnacceptableInput Exception"; }
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
			else {
				if (Ite.Value.Value < NTMapping.size())
				{
					assert(Ite.Reduce.Mask >= MinMask);
					Result.push_back({ NTMap{NTMapping[Ite.Value.Value], Ite.Reduce.ProductionCount, Ite.Reduce.Mask - MinMask} });
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