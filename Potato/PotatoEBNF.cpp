module;

#include <cassert>

module Potato.EBNF;

namespace Potato::EBNF
{

	using namespace Exception;

	static constexpr std::size_t SmallBrace = 0;
	static constexpr std::size_t MiddleBrace = 1;
	static constexpr std::size_t BigBrace = 2;
	static constexpr std::size_t OrLeft = 3;
	static constexpr std::size_t OrRight = 4;
	static constexpr std::size_t MinMask = 5;

	using SLRX::Symbol;

	enum class T
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
		Line
	};

	constexpr Symbol operator*(T sym) { return Symbol::AsTerminal(static_cast<std::size_t>(sym)); };

	enum class NT
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

	constexpr Symbol operator*(NT sym) { return Symbol::AsNoTerminal(static_cast<std::size_t>(sym)); };

	Reg::DfaBinaryTableWrapper GetRegTable() {
		static auto List = []() {

			Reg::Nfa StartupTable{ std::basic_string_view{UR"(%%%%)"}, false, static_cast<std::size_t>(T::Barrier) };

			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"([0-9]+)"}, false, static_cast<std::size_t>(T::Number) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"([0-9a-zA-Z_\z]+)"},false, static_cast<std::size_t>(T::Terminal) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(<[0-9a-zA-Z_\z]+>)"},false, static_cast<std::size_t>(T::NoTerminal) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(:=)"}, false,static_cast<std::size_t>(T::Equal) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\'([^\s]+)\')"}, false,static_cast<std::size_t>(T::Rex) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(;)"}, false,static_cast<std::size_t>(T::Semicolon) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(:)"}, false,static_cast<std::size_t>(T::Colon) });
			//StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\r?\n)"}, false,static_cast<std::size_t>(T::Line) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\s+)"}, false,static_cast<std::size_t>(T::Empty) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\$)"}, false,static_cast<std::size_t>(T::Start) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\|)"}, false,static_cast<std::size_t>(T::Or) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\[)"}, false,static_cast<std::size_t>(T::LM_Brace) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\])"}, false,static_cast<std::size_t>(T::RM_Brace) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\{)"}, false,static_cast<std::size_t>(T::LB_Brace) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\})"}, false,static_cast<std::size_t>(T::RB_Brace) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\()"}, false, static_cast<std::size_t>(T::LS_Brace) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\))"}, false,static_cast<std::size_t>(T::RS_Brace) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\+\()"}, false,static_cast<std::size_t>(T::LeftPriority) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\)\+)"}, false,static_cast<std::size_t>(T::RightPriority) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(/\*.*?\*/)"}, false,static_cast<std::size_t>(T::Command) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(//[^\n]*\n)"}, false,static_cast<std::size_t>(T::Command) });

			Reg::Dfa FinalTable(Reg::Dfa::FormatE::GreedyHeadMatch, StartupTable);
			return Reg::CreateDfaBinaryTable(FinalTable);
		}();

		return Reg::DfaBinaryTableWrapper(List);
	}

	SLRX::LRXBinaryTableWrapper EbnfStep1SLRX() {
		static SLRX::LRXBinaryTable Table(
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

	SLRX::LRXBinaryTableWrapper EbnfStep2SLRX() {
		static SLRX::LRXBinaryTable Table(
			*NT::Statement,
			{
				{*NT::Expression, {*T::Terminal}, 1},
				{*NT::Expression, {*T::NoTerminal}, 2},
				{*NT::Expression, {*T::Rex}, 4},
				{*NT::Expression, {*T::Number}, 5},
				{*NT::Expression, {*T::Start}, 50},

				{*NT::FunctionEnum, {*T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace}, 6},
				//{*NT::FunctionEnum, {*T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace, *T::Start}, 6},
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

	SLRX::LRXBinaryTableWrapper EbnfStep3SLRX() {
		static SLRX::LRXBinaryTable Table(
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

	std::any EbnfBuilder::BuilderStep1::operator()(SLRX::SymbolInfo Value, std::size_t Index)
	{
		return {};
	}

	std::any EbnfBuilder::BuilderStep1::operator()(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
	{
		switch (Pros.UserMask)
		{
		case 4:
		case 2:
		{
			Ref.RegMappings.push_back({
				Pros[1].Value.TokenIndex,
				(Pros.UserMask == 4 ? RegTypeE::Empty : RegTypeE::Terminal),
				Pros[3].Value.TokenIndex,
				{}
				});
			return {};
		}
		case 3:
		{
			Ref.RegMappings.push_back({
					Pros[1].Value.TokenIndex,
					RegTypeE::Terminal,
					Pros[3].Value.TokenIndex,
					Pros[6].Value.TokenIndex
				});
			return {};
		}
		case 1:
			return {};
		default:
			assert(false);
			return {};
		}
		return {};
	}

	std::any EbnfBuilder::BuilderStep2::operator()(SLRX::SymbolInfo Value, std::size_t Index)
	{
		T TValue = static_cast<T>(Value.Value.Value);
		if (TValue == T::Rex)
		{
			Ref.RegMappings.push_back({
				Value.TokenIndex,
				RegTypeE::Reg,
				Value.TokenIndex,
				{}
			});
		}
		return {};
	}

	std::any EbnfBuilder::BuilderStep2::operator()(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
	{
		switch (Pros.UserMask)
		{
		case 1:
		case 4:
			return ElementT{ ElementTypeE::Terminal, Pros[0].Value.TokenIndex };
		case 2:
			return ElementT{ ElementTypeE::NoTerminal, Pros[0].Value.TokenIndex };
		case 5:
			return ElementT{ ElementTypeE::Mask, Pros[0].Value.TokenIndex };
		case 50:
			return ElementT{ ElementTypeE::Self, {} };
		case 6:
		{
			return Pros[2].Value.TokenIndex;
		}
		case 7:
		{
			return Misc::IndexSpan<>{};
		}
		case 8:
		{
			auto Last = Pros[0].Consume<std::vector<ElementT>>();
			Last.push_back(Pros[1].Consume<ElementT>());
			return std::move(Last);
		}
		case 9:
		{
			std::vector<ElementT> Temp;
			Temp.push_back(Pros[0].Consume<ElementT>());
			return std::move(Temp);
		}
		case 10:
		{
			auto Exp = Pros[1].Consume<std::vector<ElementT>>();
			auto CurSymbol = ElementT{ ElementTypeE::Temporary, {Ref.TerminalProductionIndex, Ref.TerminalProductionIndex} };
			++Ref.TerminalProductionIndex;
			Ref.Builder.push_back({
				CurSymbol,
				std::move(Exp),
				{EBNF::SmallBrace, EBNF::SmallBrace}
			});
			return CurSymbol;
		}
		case 11:
		{
			auto Exp = Pros[1].Consume<std::vector<ElementT>>();
			auto CurSymbol = ElementT{ ElementTypeE::Temporary, {Ref.TerminalProductionIndex, Ref.TerminalProductionIndex} };
			++Ref.TerminalProductionIndex;
			Exp.insert(Exp.begin(), CurSymbol);
			Ref.Builder.push_back({
				CurSymbol,
				std::move(Exp),
				{EBNF::BigBrace, EBNF::BigBrace}
				});
			Ref.Builder.push_back({
				CurSymbol,
				{},
				{EBNF::BigBrace, EBNF::BigBrace}
				});
			return CurSymbol;
		}
		case 12:
		{
			auto Exp = Pros[1].Consume<std::vector<ElementT>>();
			auto CurSymbol = ElementT{ ElementTypeE::Temporary, {Ref.TerminalProductionIndex, Ref.TerminalProductionIndex} };
			++Ref.TerminalProductionIndex;
			Ref.Builder.push_back({
				CurSymbol,
				std::move(Exp),
				{EBNF::MiddleBrace, EBNF::MiddleBrace}
			});
			Ref.Builder.push_back({
				CurSymbol,
				{},
				{EBNF::MiddleBrace, EBNF::MiddleBrace}
			});
			return CurSymbol;
		}
		case 15:
		{
			auto Last = Pros[1].Consume<std::vector<ElementT>>();
			Last.push_back(Pros[0].Consume<ElementT>());
			return std::move(Last);
		}
		case 17:
		{
			auto Last1 = Pros[0].Consume<std::vector<ElementT>>();
			auto Last2 = Pros[2].Consume<std::vector<ElementT>>();
			std::reverse(Last2.begin(), Last2.end());
			Ref.OrMaskIte = EBNF::MinMask;

			auto OrSymbol = ElementT{ ElementTypeE::Temporary, {Ref.TerminalProductionIndex, Ref.TerminalProductionIndex} };
			++Ref.TerminalProductionIndex;

			Ref.Builder.push_back({
					OrSymbol,
					std::move(Last1),
					{Ref.OrMaskIte, Ref.OrMaskIte}
				});

			++Ref.OrMaskIte;

			Ref.Builder.push_back({
					OrSymbol,
					std::move(Last2),
					{Ref.OrMaskIte, Ref.OrMaskIte}
				});

			++Ref.OrMaskIte;

			std::vector<ElementT> Temp;
			Temp.push_back(OrSymbol);
			return std::move(Temp);
		}
		case 30:
		{
			auto Last1 = Pros[0].Consume<std::vector<ElementT>>();
			auto Last2 = Pros[2].Consume<std::vector<ElementT>>();
			std::reverse(Last2.begin(), Last2.end());
			assert(Last1.size() == 0);
			//assert(Last1[0].ProductionValue.IsNoTerminal());
			Ref.Builder.push_back({
					Last1[0],
					std::move(Last2),
					{Ref.OrMaskIte, Ref.OrMaskIte}
				});
			++Ref.OrMaskIte;
			return std::move(Last1);
		}
		case 18:
			return Pros[0].Consume();
		case 19:
			return std::vector<ElementT>{};
		case 20:
			return {};
		case 21:
		{
			Ref.LastProductionStartSymbol = ElementT{ ElementTypeE::NoTerminal, Pros[0].Value.TokenIndex };
			auto List = Pros[2].Consume<std::vector<ElementT>>();
			auto Mask = Pros[3].Consume<Misc::IndexSpan<>>();
			Ref.Builder.push_back({
				*Ref.LastProductionStartSymbol,
				std::move(List),
				Mask
			});
			return {};
		}
		case 22:
		{
			if (!Ref.LastProductionStartSymbol.has_value()) [[unlikely]]
			{
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::UnsetProductionHead, Value.TokenIndex };
			}
			auto List = Pros[1].Consume<std::vector<ElementT>>();
			auto Mask = Pros[2].Consume<Misc::IndexSpan<>>();
			Ref.Builder.push_back({
				*Ref.LastProductionStartSymbol,
				std::move(List),
				Mask
			});
			return {};
		}
		case 23:
		{
			if (Ref.StartSymbol.has_value()) [[unlikely]]
			{
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::StartSymbolAreadySet, Value.TokenIndex };
			}
			Ref.StartSymbol = ElementT{ ElementTypeE::NoTerminal, Pros[2].Value.TokenIndex};
			if (Pros.GetElementCount() == 5)
			{
				Ref.MaxForwardDetect = ElementT{ ElementTypeE::Mask, Pros[3].Value.TokenIndex };
			}

			return {};
		}
		default:
			break;
		}
		return {};
	}

	std::any EbnfBuilder::BuilderStep3::operator()(SLRX::SymbolInfo Value, std::size_t Index) { return {}; }

	std::any EbnfBuilder::BuilderStep3::operator()(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
	{
		switch (Pros.UserMask)
		{
		case 1:
			return ElementT{ ElementTypeE::Terminal, Pros[0].Value.TokenIndex };
		case 2:
		{
			std::vector<ElementT> Symbols;
			Symbols.push_back(Pros[0].Consume<ElementT>());
			return Symbols;
		}
		case 3:
		{
			auto Symbols = Pros[0].Consume<std::vector<ElementT>>();
			Symbols.push_back(Pros[1].Consume<ElementT>());
			return Symbols;
		}
		case 4:
		{
			auto Symbols = Pros[2].Consume<std::vector<ElementT>>();
			Ref.OpePriority.push_back({ std::move(Symbols), SLRX::OpePriority::Associativity::Right });
			return {};
		}
		case 5:
		{
			auto Symbols = Pros[2].Consume<std::vector<ElementT>>();
			Ref.OpePriority.push_back({ std::move(Symbols), SLRX::OpePriority::Associativity::Left });
			return {};
		}
		default:
			return {};
		}
	}


	EbnfBuilder::EbnfBuilder(std::size_t StartupTokenIndex)
		: RequireTokenIndex(StartupTokenIndex), LastSymbolToken(StartupTokenIndex), Processor(GetRegTable())
	{
		BuilderStep1 Step1{*this};
		SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep1> Pro{ EbnfStep1SLRX(), Context, Step1 };
	}

	bool EbnfBuilder::Consume(char32_t InputValue, std::size_t NextTokenIndex)
	{
		if (Processor.Consume(InputValue, RequireTokenIndex))
		{
			RequireTokenIndex = NextTokenIndex;
			return true;
		}
		else {
			auto Accept = Processor.GetAccept();
			if (Accept)
			{
				auto Ter = static_cast<T>(Accept.GetMask());
				auto TokenIndex = Accept.MainCapture;
				if (AddSymbol(*Ter, TokenIndex))
				{
					LastSymbolToken = TokenIndex.End();
					RequireTokenIndex = LastSymbolToken;
					Processor.Clear();
					return true;
				}
				else {
					RequireTokenIndex = LastSymbolToken;
					Processor.Clear();
					return false;
				}
			}
			return false;
		}
	}

	bool EbnfBuilder::EndOfFile()
	{
		auto Re = Processor.EndOfFile(RequireTokenIndex);
		auto Accept = Processor.GetAccept();

		if (Accept)
		{
			auto Symbol = SLRX::Symbol::AsTerminal(Accept.GetMask());
			auto TokenIndex = Accept.MainCapture;

			if (AddSymbol(Symbol, TokenIndex))
			{
				if (RequireTokenIndex == TokenIndex.End())
				{
					if (AddEndOfFile())
					{
						RequireTokenIndex = RequireTokenIndex + 1;
						LastSymbolToken = TokenIndex.End();
						Processor.Clear();
						return true;
					}
					else {
						RequireTokenIndex = LastSymbolToken;
						Processor.Clear();
						return false;
					}
				}
				else {
					LastSymbolToken = TokenIndex.End();
					RequireTokenIndex = LastSymbolToken;
					Processor.Clear();
					return true;
				}
			}
			else {
				RequireTokenIndex = LastSymbolToken;
				Processor.Clear();
				return false;
			}
		}
		return false;
	}

	bool EbnfBuilder::AddEndOfFile()
	{
		switch (State)
		{
		case StateE::Step1:
			return false;
		case StateE::Step2:
		{
			BuilderStep2 Step2{ *this };
			SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep2> Pro(SLRX::IgnoreClear{}, EbnfStep2SLRX(), Context, Step2);
			return Pro.EndOfFile();
		}
		case StateE::Step3:
		{
			BuilderStep3 Step3{ *this };
			SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep3> Pro(SLRX::IgnoreClear{}, EbnfStep3SLRX(), Context, Step3);
			return Pro.EndOfFile();
		}
		default:
			assert(false);
			return false;
		}
	}

	bool EbnfBuilder::AddSymbol(SLRX::Symbol Symbol, Misc::IndexSpan<> TokenIndex)
	{
		if(Symbol == *T::Empty)
			return true;
		switch (State)
		{
		case StateE::Step1:
		{
			BuilderStep1 Step1{*this};
			SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep1> Pro(SLRX::IgnoreClear{}, EbnfStep1SLRX(), Context, Step1);
			if (Symbol == *T::Barrier)
			{
				if(!Pro.EndOfFile())
					return false;
				BuilderStep2 Step2{ *this };
				SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep2> Pro2(EbnfStep2SLRX(), Context, Step2);
				State = StateE::Step2;
				return true;
			}
			else {
				return Pro.Consume(Symbol, TokenIndex, 0);
			}
		}
		case StateE::Step2:
		{
			BuilderStep2 Step2{ *this };
			SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep2> Pro(SLRX::IgnoreClear{}, EbnfStep2SLRX(), Context, Step2);
			if (Symbol == *T::Barrier)
			{
				if (!Pro.EndOfFile())
					return false;
				BuilderStep3 Step3{ *this };
				SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep3> Pro2(EbnfStep3SLRX(), Context, Step3);
				State = StateE::Step3;
				return true;
			}else
				return Pro.Consume(Symbol, TokenIndex, 1);
		}
		case StateE::Step3:
		{
			BuilderStep3 Step3{ *this };
			SLRX::LRXBinaryTableProcessor<std::size_t, BuilderStep3> Pro(SLRX::IgnoreClear{}, EbnfStep3SLRX(), Context, Step3);
			if (Symbol == *T::Barrier)
			{
				return false;
			}
			auto Re = Pro.Consume(Symbol, TokenIndex, 2);
			if(!Re)
				volatile int i = 0;
			return Re;
		}
		}
		return true;
	}


	bool CoreProcessor::Consume(char32_t TokenIndex, std::size_t NextTokenIndex)
	{
		if (LexicalConsume(TokenIndex, RequireTokenIndex))
		{
			RequireTokenIndex = NextTokenIndex;
			return true;
		}
		else {
			auto Accept = LexicalGetAccept();
			if (Accept)
			{
				auto Ter = SLRX::Symbol::AsTerminal(Accept.GetMask());
				auto TokenIndex = Accept.MainCapture;
				if (SyntaxConsume(Ter, TokenIndex))
				{
					LastSymbolToken = TokenIndex.End();
					RequireTokenIndex = LastSymbolToken;
					LexicalClear();
					return true;
				}
				else {
					RequireTokenIndex = LastSymbolToken;
					LexicalClear();
					return false;
				}
			}
			return false;
		}
	}

	bool CoreProcessor::EndOfFile()
	{
		auto Re = LexicalEndOfFile(RequireTokenIndex);
		auto Accept = LexicalGetAccept();

		if (Accept)
		{
			auto Symbol = SLRX::Symbol::AsTerminal(Accept.GetMask());
			auto TokenIndex = Accept.MainCapture;

			if (SyntaxConsume(Symbol, TokenIndex))
			{
				if (RequireTokenIndex == TokenIndex.End())
				{
					if (SyntaxEndOfFile())
					{
						RequireTokenIndex = RequireTokenIndex + 1;
						LastSymbolToken = TokenIndex.End();
						LexicalClear();
						return true;
					}
					else {
						RequireTokenIndex = LastSymbolToken;
						LexicalClear();
						return false;
					}
				}
				else {
					LastSymbolToken = TokenIndex.End();
					RequireTokenIndex = LastSymbolToken;
					LexicalClear();
					return true;
				}
			}
			else {
				RequireTokenIndex = LastSymbolToken;
				LexicalClear();
				return false;
			}
		}
		return false;
	}






	/*
	void EbnfT::Parsing(
		EbnfLexerT const& Input,
		std::vector<RegMappingT>& RegMapping,
		SLRX::Symbol& OutputStartSymbol,
		bool& NeedTokenIndexRef,
		std::size_t& MaxFormatDetect,
		std::vector<SLRX::ProductionBuilder>& Builders,
		std::vector<SLRX::OpePriority>& OpePriority
	)
	{
		auto ElementSpan = std::span(Input.Elements);
		
		std::size_t TempNoTerminalCount = ElementSpan.size();
		std::size_t TokenUsed = 0;
		NeedTokenIndexRef = false;
		MaxFormatDetect = 3;

		auto Locate = std::find_if(ElementSpan.begin(), ElementSpan.end(), [](ElementT Ele){
			return Ele.Symbol == T::Barrier;
		});
		auto Step1 = std::span(ElementSpan.begin(), Locate);
		ElementSpan = std::span(Locate + 1, ElementSpan.end());

		// Step1
		{
			SLRX::SymbolProcessor Pro(EbnfStep1SLRX());

			while (!Step1.empty())
			{
				auto Cur = *Step1.begin();
				if (!Pro.Consume(*Cur.Symbol, TokenUsed))
				{
					throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
				}
				Step1 = Step1.subspan(1);
				TokenUsed += 1;
			}
			if (!Pro.EndOfFile())
			{
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
			}

			SLRX::ProcessParsingStep(Pro.GetSteps(), [&](SLRX::VariantElement Ele) -> std::any {
				if (Ele.IsTerminal())
				{
					auto TE = Ele.AsTerminal();
					auto Enum = static_cast<T>(TE.Value.Value);
					return TE.Shift.TokenIndex;
				}
				else {
					auto NTE = Ele.AsNoTerminal();
					switch (NTE.Reduce.Mask)
					{
					case 4:
					case 2:
					{
						RegMapping.push_back({
							NTE[1].Consume<std::size_t>(),
							NTE[3].Consume<std::size_t>(),
							{}
						});
						return {};
					}
					case 3:
					{
						RegMapping.push_back({
								NTE[1].Consume<std::size_t>(),
								NTE[3].Consume<std::size_t>(),
								NTE[6].Consume<std::size_t>()
						});
						return {};
					}
					case 1:
						return {};
					default:
						assert(false);
						return {};
					}
				}
				return {};
			});
		}

		Locate = std::find_if(ElementSpan.begin(), ElementSpan.end(), [](ElementT Ele) {
			return Ele.Symbol == T::Barrier;
		});

		auto Step2 = std::span(ElementSpan.begin(), Locate);

		if (Step2.empty())
		{
			throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
		}

		TokenUsed += 1;

		if(Locate == ElementSpan.end())
			ElementSpan = {};
		else {
			ElementSpan = std::span(Locate + 1, ElementSpan.end());
		}

		// Step2
		{
			
			std::optional<SLRX::Symbol> LastProductionStartSymbol;
			std::optional<SLRX::Symbol> StartSymbol;

			SLRX::SymbolProcessor Pro(EbnfStep2SLRX());

			struct FunctionEnmu
			{
				std::size_t Mask = 0;
				bool NeedPredict = false;
			};

			while (!Step2.empty())
			{
				auto& Cur = *Step2.begin();
				if (!Pro.Consume(*Cur.Symbol, TokenUsed))
				{
					throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
				}
				Step2 = Step2.subspan(1);
				TokenUsed += 1;
			}
			if (!Pro.EndOfFile())
			{
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
			}

			std::size_t OrMaskIte = EBNF::MinMask;

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
						return SLRX::ProductionBuilderElement{ Ref[0].Consume<std::size_t>() };
					case 50:
						return SLRX::ProductionBuilderElement{ SLRX::ItSelf{} };
					case 6:
					{
						FunctionEnmu Enum;
						Enum.Mask = Ref[2].Consume<std::size_t>();
						if (Ref.Datas.size() >= 5)
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
						auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount++);
						Builders.push_back({
							CurSymbol,
							std::move(Exp),
							EBNF::SmallBrace
						});
						return SLRX::ProductionBuilderElement{ CurSymbol };
					}
					case 11:
					{
						auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount++);
						Exp.insert(Exp.begin(), SLRX::ProductionBuilderElement{ CurSymbol });
						Builders.push_back({
							CurSymbol,
							std::move(Exp),
							EBNF::BigBrace
							});
						Builders.push_back({
							CurSymbol,
							{},
							EBNF::BigBrace
							});
						return SLRX::ProductionBuilderElement{ CurSymbol };
					}
					case 12:
					{
						auto Exp = Ref[1].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						auto CurSymbol = SLRX::Symbol::AsNoTerminal(TempNoTerminalCount++);
						Builders.push_back({
							CurSymbol,
							std::move(Exp),
							EBNF::MiddleBrace
							});
						Builders.push_back({
							CurSymbol,
							{},
							EBNF::MiddleBrace
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
						OrMaskIte = EBNF::MinMask;

						Symbol OrSymbol = Symbol::AsNoTerminal(TempNoTerminalCount++);

						Builders.push_back({
								OrSymbol,
								std::move(Last1),
								OrMaskIte++
							});

						Builders.push_back({
								OrSymbol,
								std::move(Last2),
								OrMaskIte++
							});

						std::vector<SLRX::ProductionBuilderElement> Temp;
						Temp.push_back(OrSymbol);
						return std::move(Temp);
					}
					case 30:
					{
						auto Last1 = Ref[0].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						auto Last2 = Ref[2].Consume<std::vector<SLRX::ProductionBuilderElement>>();
						std::reverse(Last2.begin(), Last2.end());
						assert(Last1.size() == 0);
						assert(Last1[0].ProductionValue.IsNoTerminal());
						Builders.push_back({
								Last1[0].ProductionValue,
								std::move(Last2),
								OrMaskIte++
							});
						return std::move(Last1);
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
							throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::UnsetProductionHead, Ref.TokenIndex.Begin() };
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
							throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::StartSymbolAreadySet, Ref.TokenIndex.Begin() };
						}
						StartSymbol = Ref[2].Consume<SLRX::Symbol>();
						if (Ref.Datas.size() == 5)
						{
							MaxFormatDetect = Ref.Datas[3].Consume<std::size_t>();
							NeedTokenIndexRef = true;
						}
							
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
						return Ref.Shift.TokenIndex;
					}
					case T::Rex:
					{
						RegMapping.push_back({ Ref.Shift.TokenIndex, Ref.Shift.TokenIndex, {} });
						return SLRX::Symbol::AsTerminal(Ref.Shift.TokenIndex);
					}
					case T::Terminal:
					{
						return SLRX::Symbol::AsTerminal(Ref.Shift.TokenIndex);
					}
					case T::NoTerminal:
					{
						return SLRX::Symbol::AsNoTerminal(Ref.Shift.TokenIndex);
					}
					default:
						break;
					}
				}
				return {};
				}
				);
				OutputStartSymbol = *StartSymbol;
		}

		Locate = std::find_if(ElementSpan.begin(), ElementSpan.end(), [](ElementT Ele) {
			return Ele.Symbol == T::Barrier;
		});

		auto Step3 = std::span(ElementSpan.begin(), Locate);

		if(!Step3.empty())
			TokenUsed += 1;

		if (Locate == ElementSpan.end())
			ElementSpan = {};
		else {
			ElementSpan = std::span(Locate + 1, ElementSpan.end());
		}

		assert(ElementSpan.empty());

		// Step3
		{
			SLRX::SymbolProcessor Pro(EbnfStep3SLRX());

			while (!Step3.empty())
			{
				auto& Cur = *Step3.begin();
				if (!Pro.Consume(*Cur.Symbol, TokenUsed))
				{
					throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
				}
				Step3 = Step3.subspan(1);
				TokenUsed += 1;
			}

			if (!Pro.EndOfFile())
			{
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::WrongEbnfSyntax, TokenUsed };
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
						OpePriority.push_back({ std::move(Symbols), SLRX::Associativity::Right });
						return {};
					}
					case 5:
					{
						auto Symbols = NT[2].Consume<std::vector<SLRX::Symbol>>();
						OpePriority.push_back({ std::move(Symbols), SLRX::Associativity::Left });
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
						return Symbol::AsTerminal(TRef.Shift.TokenIndex);
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
	}*/

	/*
	void EbnfLexicalProcessor::Reset() {
		RequireStrTokenIndex = StartupTokenIndex;
		DfaProcessor.Reset();
	}

	bool EbnfLexicalProcessor::Consume(char32_t Input, std::size_t NextTokenIndex)
	{
		if (DfaProcessor.Consume(Input, RequireStrTokenIndex))
		{
			RequireStrTokenIndex = NextTokenIndex;
			return true;
		}
		else {
			auto Accept = DfaProcessor.GetAccept();
			if (Accept.has_value())
			{
				DfaProcessor.Reset();
				auto& RegMapping = TableRef.RegScriptions[Accept->Mask];
				if (RegMapping.SymbolValue != 0)
				{
					auto ElementTokenIndex = LexicalElementT.size();
					auto Symbol = SLRX::Symbol::AsTerminal(RegMapping.SymbolValue);
					LexicalElementT.push_back({
						RegMapping,
						Accept->MainCapture
					});
				}
				RequireStrTokenIndex = Accept->MainCapture.End();
				return true;
			}
			return false;
		}
	}
	*/


	/*
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
						{Offset, SpanIndex.Count()}
						});
				}
					break;
				case T::Barrier:
					Output.Datas.push_back({ *Enum,
							{u8"%%%%"},
							{Offset, Re->MainCapture.Count()}
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
							Format::DirectScan(SymbolTuple.Datas[TE.Shift.TokenIndex].Str, Num);
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

				SLRX::StandardT OrMaskIte = EBNF::OrMaskBase;

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
								EBNF::SmallBrace
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
								EBNF::BigBrace
								});
							Builders.push_back({
								CurSymbol,
								{},
								EBNF::BigBrace
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
								EBNF::MiddleBrace
								});
							Builders.push_back({
								CurSymbol,
								{},
								EBNF::MiddleBrace
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
							OrMaskIte = EBNF::OrMaskBase;

							Symbol OrSymbol = Symbol::AsNoTerminal(TempNoTerminalCount--);

							Builders.push_back({
									OrSymbol,
									std::move(Last1),
									OrMaskIte++
								});

							Builders.push_back({
									OrSymbol,
									std::move(Last2),
									OrMaskIte++
								});

							std::vector<SLRX::ProductionBuilderElement> Temp;
							Temp.push_back(OrSymbol);
							return std::move(Temp);
						}
						case 30:
						{
							auto Last1 = Ref[0].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							auto Last2 = Ref[2].Consume<std::vector<SLRX::ProductionBuilderElement>>();
							std::reverse(Last2.begin(), Last2.end());
							assert(Last1.size() == 0);
							assert(Last1[0].ProductionValue.IsNoTerminal());
							Builders.push_back({
									Last1[0].ProductionValue,
									std::move(Last2),
									OrMaskIte++
								});
							return std::move(Last1);
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
							Format::DirectScan(SymbolTuple.Datas[Ref.Shift.TokenIndex].Str, Index);
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

			ES[4].Length = static_cast<StandardT>(Index);
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
				default:
					Step.Symbol = u8"...|...";
					break;
				}
			}
			ParsingStep::ReduceT Reduce;
			Reduce.Mask = In.Reduce.Mask;
			Reduce.ElementCount = In.Reduce.ElementCount;
			Reduce.IsNoNameReduce = !Sym.has_value();
			Reduce.UniqueReduceID = In.Reduce.ProductionIndex;
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

	auto ParsingStepProcessor::Consume(ParsingStep Input) ->std::optional<Result>
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
				std::size_t AdressSize = Datas.size() - Reduce.ElementCount;
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
					case EBNF::SmallBrace:
					{
						if (Reduce.ElementCount == 1)
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
					case EBNF::MiddleBrace:
					{
						std::vector<NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(TemporaryDatas.begin()), std::move_iterator(TemporaryDatas.end()));
						Condition Con{ Condition::TypeT::SquareBrackets, 0, std::move(TemBuffer) };
						Push({ Input.Symbol, TokenIndex }, std::move(Con));
						return Result{ {}, {} };
					}
					case EBNF::BigBrace:
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
					default:
					{
						std::vector<NTElement::DataT> TemBuffer;
						TemBuffer.insert(TemBuffer.end(), std::move_iterator(TemporaryDatas.begin()), std::move_iterator(TemporaryDatas.end()));
						Condition Con{ Condition::TypeT::Or, Reduce.Mask - EBNF::OrMaskBase, std::move(TemBuffer) };
						Push({ Input.Symbol, TokenIndex }, std::move(Con));
						return Result{ {}, {} };
					}
					}
				}
				else{
					NTElement Ele{ Input.Symbol, Reduce, std::span(TemporaryDatas)};
					return Result{ VariantElement{Ele}, NTElement::MetaData{Input.Symbol, TokenIndex} };
				}
			}
		}
	}


	std::optional<std::any> ParsingStepProcessor::EndOfFile() 
	{
		if (Datas.size() == 1)
		{
			return Datas[0].Consume();
		}
		return {};
	}
	*/
}

namespace Potato::EBNF::Exception
{
	char const* Interface::what() const { return "EBNF Exception"; }
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
					Result.push_back({ NTMap{NTMapping[Ite.Value.Value], Ite.Reduce.ElementCount, Ite.Reduce.Mask} });
				}
				else {
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