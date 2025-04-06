module;

#include <cassert>

module PotatoEBNF;

namespace Potato::EBNF
{

	using namespace Exception;

	static constexpr std::size_t SmallBrace = 1;
	static constexpr std::size_t MiddleBrace = 2;
	static constexpr std::size_t BigBrace = 3;
	static constexpr std::size_t MinMask = 4;

	using SLRX::Symbol;

	std::wstring_view OrStatementRegName() {
		return L"$Or";
	}
	static std::wstring_view RoundBracketStatementRegName() {
		return L"$RoundBracket";
	}
	static std::wstring_view SquareBracketStatementRegName() {
		return L"$SquareBracket";
	}
	static std::wstring_view CurlyBracketStatementRegName() {
		return L"$CurlyBracket";
	}

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
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\'(.*?)[^\\]\')"}, false,static_cast<std::size_t>(T::Rex) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(;)"}, false,static_cast<std::size_t>(T::Semicolon) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(:)"}, false,static_cast<std::size_t>(T::Colon) });
			//StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"(\r?\n)"}, false,static_cast<std::size_t>(T::Line) });
			StartupTable.Link(Reg::Nfa{ std::basic_string_view{UR"([\s\t]+)"}, false,static_cast<std::size_t>(T::Empty) });
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

		return Reg::DfaBinaryTableWrapper{List};
	}

	SLRX::LRXBinaryTableWrapper EbnfStep1SLRX() {
		static SLRX::LRXBinaryTable Table(
			*NT::Statement,
			{
				{*NT::Statement, {}, 1},
				{*NT::Statement, {*NT::Statement, *T::Terminal, *T::Equal, *T::Rex, *T::Semicolon}, 2},
				{*NT::Statement, {*NT::Statement, *T::Terminal, *T::Equal, *T::Rex, *T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace, *T::Semicolon}, 3},
				{*NT::Statement, {*NT::Statement, *T::Start, *T::Equal, *T::Rex, *T::Semicolon}, 4},
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
				{*NT::Statement, {*NT::Statement, *T::LS_Brace, *NT::ExpressionStatement, *T::RightPriority, *T::Semicolon}, 4},
				{*NT::Statement, {*NT::Statement, *T::LeftPriority, *NT::ExpressionStatement, *T::RS_Brace, *T::Semicolon }, 5},
			},
			{}
		);
		return Table.Wrapper;
	};

	std::any EbnfBuilder::HandleSymbol(SLRX::SymbolInfo Value)
	{
		switch (State)
		{
		case StateE::Step1:
			return HandleSymbolStep1(Value);
		case StateE::Step2:
			return HandleSymbolStep2(Value);
		default:
			return HandleSymbolStep3(Value);
		}
	}

	std::any EbnfBuilder::HandleReduce(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
	{
		switch (State)
		{
		case StateE::Step1:
			return HandleReduceStep1(Value, Pros);
		case StateE::Step2:
			return HandleReduceStep2(Value, Pros);
		default:
			return HandleReduceStep3(Value, Pros);
		}
	}

	std::any EbnfBuilder::HandleSymbolStep1(SLRX::SymbolInfo Value)
	{
		return {};
	}

	std::any EbnfBuilder::HandleReduceStep1(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
	{
		switch (Pros.UserMask)
		{
		case 4:
		case 2:
		{
			RegMappings.push_back({
				Pros[1].Value.TokenIndex,
				(Pros.UserMask == 4 ? RegTypeE::Empty : RegTypeE::Terminal),
				Pros[3].Value.TokenIndex,
				{}
				});
			return {};
		}
		case 3:
		{
			RegMappings.push_back({
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

	std::any EbnfBuilder::HandleSymbolStep2(SLRX::SymbolInfo Value)
	{
		T TValue = static_cast<T>(Value.Value.symbol);
		if (TValue == T::Rex)
		{
			RegMappings.push_back({
				Value.TokenIndex,
				RegTypeE::Reg,
				Value.TokenIndex,
				{}
			});
		}
		return {};
	}

	std::any EbnfBuilder::HandleReduceStep2(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
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
			auto CurSymbol = ElementT{ ElementTypeE::Temporary, {TerminalProductionIndex, TerminalProductionIndex} };
			++TerminalProductionIndex;
			Builder.push_back({
				CurSymbol,
				std::move(Exp),
				{EBNF::SmallBrace, EBNF::SmallBrace}
			});
			return CurSymbol;
		}
		case 11:
		{
			auto Exp = Pros[1].Consume<std::vector<ElementT>>();
			auto CurSymbol = ElementT{ ElementTypeE::Temporary, {TerminalProductionIndex, TerminalProductionIndex} };
			++TerminalProductionIndex;
			Exp.insert(Exp.begin(), CurSymbol);
			Builder.push_back({
				CurSymbol,
				std::move(Exp),
				{EBNF::BigBrace, EBNF::BigBrace}
				});
			Builder.push_back({
				CurSymbol,
				{},
				{EBNF::BigBrace, EBNF::BigBrace}
				});
			return CurSymbol;
		}
		case 12:
		{
			auto Exp = Pros[1].Consume<std::vector<ElementT>>();
			auto CurSymbol = ElementT{ ElementTypeE::Temporary, {TerminalProductionIndex, TerminalProductionIndex} };
			++TerminalProductionIndex;
			Builder.push_back({
				CurSymbol,
				std::move(Exp),
				{EBNF::MiddleBrace, EBNF::MiddleBrace}
			});
			Builder.push_back({
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
			OrMaskIte = EBNF::MinMask;
			auto LastOrStatementTokenIndex = TerminalProductionIndex;
			++TerminalProductionIndex;
			auto OrSymbol = ElementT{ ElementTypeE::Temporary, {LastOrStatementTokenIndex, LastOrStatementTokenIndex} };

			Builder.push_back({
					OrSymbol,
					std::move(Last1),
					{OrMaskIte, OrMaskIte}
				});

			++OrMaskIte;

			Builder.push_back({
					OrSymbol,
					std::move(Last2),
					{OrMaskIte, OrMaskIte}
				});

			++OrMaskIte;

			std::vector<ElementT> Temp;
			Temp.push_back(OrSymbol);
			return std::move(Temp);
		}
		case 30:
		{
			auto Last1 = Pros[0].Consume<std::vector<ElementT>>();
			auto Last2 = Pros[2].Consume<std::vector<ElementT>>();
			std::reverse(Last2.begin(), Last2.end());
			assert(Last1.size() == 1);
			//assert(Last1[0].ProductionValue.IsNoTerminal());
			Builder.push_back({
					Last1[0],
					std::move(Last2),
					{OrMaskIte, OrMaskIte}
				});
			++OrMaskIte;
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
			LastProductionStartSymbol = ElementT{ ElementTypeE::NoTerminal, Pros[0].Value.TokenIndex };
			auto List = Pros[2].Consume<std::vector<ElementT>>();
			auto Mask = Pros[3].Consume<Misc::IndexSpan<>>();
			Builder.push_back({
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
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::UnsetProductionHead, Value.TokenIndex };
			}
			auto List = Pros[1].Consume<std::vector<ElementT>>();
			auto Mask = Pros[2].Consume<Misc::IndexSpan<>>();
			Builder.push_back({
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
				throw BuildInUnacceptableEbnf{ BuildInUnacceptableEbnf::TypeE::StartSymbolAreadySet, Value.TokenIndex };
			}
			StartSymbol = ElementT{ ElementTypeE::NoTerminal, Pros[2].Value.TokenIndex};
			if (Pros.Size() == 5)
			{
				MaxForwardDetect = ElementT{ ElementTypeE::Mask, Pros[3].Value.TokenIndex };
			}

			return {};
		}
		default:
			break;
		}
		return {};
	}

	std::any EbnfBuilder::HandleSymbolStep3(SLRX::SymbolInfo Value)
	{
		return {};
	}
	
	std::any EbnfBuilder::HandleReduceStep3(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pros)
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
			OpePriority.push_back({ std::move(Symbols), SLRX::OpePriority::Associativity::Right });
			return {};
		}
		case 5:
		{
			auto Symbols = Pros[2].Consume<std::vector<ElementT>>();
			OpePriority.push_back({ std::move(Symbols), SLRX::OpePriority::Associativity::Left });
			return {};
		}
		default:
			return {};
		}
	}


	EbnfBuilder::EbnfBuilder(std::size_t StartupTokenIndex)
		: RequireTokenIndex(StartupTokenIndex), LastSymbolToken(StartupTokenIndex)
	{
		Processor.SetObserverTable(GetRegTable());
		LRXProcessor.SetObserverTable(EbnfStep1SLRX(), this);
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
			return LRXProcessor.EndOfFile();
		}
		case StateE::Step3:
		{
			return LRXProcessor.EndOfFile();
		}
		default:
			assert(false);
			return false;
		}
	}

	bool EbnfBuilder::AddSymbol(SLRX::Symbol Symbol, Misc::IndexSpan<> TokenIndex)
	{
		if(Symbol == *T::Empty || Symbol == *T::Command)
			return true;
		switch (State)
		{
		case StateE::Step1:
		{
			if (Symbol == *T::Barrier)
			{
				if(!LRXProcessor.EndOfFile())
					return false;
				State = StateE::Step2;
				LRXProcessor.SetObserverTable(EbnfStep2SLRX(), this);
				return true;
			}
			else {
				return LRXProcessor.Consume(Symbol, TokenIndex, HandleSymbol({Symbol, TokenIndex }));
			}
		}
		case StateE::Step2:
		{
			if (Symbol == *T::Barrier)
			{
				if (!LRXProcessor.EndOfFile())
					return false;
				State = StateE::Step3;
				LRXProcessor.SetObserverTable(EbnfStep3SLRX(), this);
				return true;
			}
			else {
				return LRXProcessor.Consume(Symbol, TokenIndex, HandleSymbol({ Symbol, TokenIndex }));
			}
		}
		case StateE::Step3:
		{
			if (Symbol == *T::Barrier)
			{
				return false;
			}
			auto Re = LRXProcessor.Consume(Symbol, TokenIndex, HandleSymbol({ Symbol, TokenIndex }));
			return Re;
		}
		}
		return true;
	}

	std::wstring_view Ebnf::GetRegName(std::size_t Index) const
	{
		if(Index < SymbolMap.size())
			return SymbolMap[Index].StrIndex.Slice(std::wstring_view{ TotalString });
		return {};
	};


	EbnfBinaryTableWrapper::HeadT const* EbnfBinaryTableWrapper::GetHead() const {
		return reinterpret_cast<HeadT const*>(Buffer.data());
	}

	Ebnf::RegInfoT EbnfBinaryTableWrapper::GetRgeInfo(std::size_t Index) const {
		auto Head = GetHead();
		Misc::StructedSerilizerReader<StandardT const> Reader{Buffer};
		Reader.SetPointer(Head->RegMapOffset);
		auto Span = Reader.ReadObjectArray<RegInfoT>(Head->RegMapCount);
		Ebnf::RegInfoT Info;
		Info.MapSymbolValue = Span[Index].MapSymbolValue;
		Info.UserMask = Span[Index].UserMask;
		return Info;
	}

	std::wstring_view EbnfBinaryTableWrapper::GetRegName(std::size_t Index) const 
	{
		auto Head = GetHead();
		if (Index < Head->SymbolMapCount)
		{
			Misc::StructedSerilizerReader<StandardT const> Reader{Buffer};
			Reader.SetPointer(Head->SymbolMapOffset);
			auto Span = Reader.ReadObjectArray<SymbolMapT>(Head->SymbolMapCount);
			auto NameIndex = Span[Index].StrIndex;
			Reader.SetPointer(Head->TotalNameOffset);
			auto Str = Reader.ReadObjectArray<wchar_t>(Head->TotalNameCount);
			return std::wstring_view{NameIndex.Slice(Str)};
		}
		else {
			return {};
		}
	}

	Reg::DfaBinaryTableWrapper EbnfBinaryTableWrapper::GetLexicalTable() const
	{
		auto Head = GetHead();
		return Reg::DfaBinaryTableWrapper{
			Buffer.subspan(Head->LexicalOffset, Head->LexicalCount)
		};
	}

	SLRX::LRXBinaryTableWrapper EbnfBinaryTableWrapper::GetSyntaxTable() const {
		auto Head = GetHead();
		return SLRX::LRXBinaryTableWrapper{
			Buffer.subspan(Head->SyntaxOffset, Head->SyntaxCount)
		};
	}

	void EbnfBinaryTableWrapper::Serilize(Misc::StructedSerilizerWritter<StandardT>& Write, Ebnf const& Table)
	{
		auto OldMark = Write.PushMark();
		HeadT Head;
		auto StartOffset = Write.WriteObject(Head);
		Head.TotalNameOffset = static_cast<StandardT>(Write.WriteObjectArray(std::span(Table.TotalString)));
		Head.TotalNameCount = static_cast<StandardT>(Table.TotalString.size());

		Head.SymbolMapOffset = static_cast<StandardT>(Write.NewObjectArray<SymbolMapT>(Table.SymbolMap.size()));
		Head.SymbolMapCount = static_cast<StandardT>(Table.SymbolMap.size());

		if (Write.IsWritting())
		{
			auto Reader = *Write.GetReader();
			Reader.SetPointer(Head.SymbolMapOffset);
			auto Span = Reader.ReadObjectArray<SymbolMapT>(Head.SymbolMapCount);
			for (std::size_t I = 0; I < Span.size(); ++I)
			{
				auto Tar = Table.SymbolMap[I];
				Span[I].StrIndex = Misc::IndexSpan<StandardT>(
					static_cast<StandardT>(Tar.StrIndex.Begin()),
					static_cast<StandardT>(Tar.StrIndex.End())
				);
			}
		}

		Head.RegMapOffset = static_cast<StandardT>(Write.NewObjectArray<RegInfoT>(Table.RegMap.size()));
		Head.RegMapCount = static_cast<StandardT>(Table.RegMap.size());

		if (Write.IsWritting())
		{
			auto Reader = *Write.GetReader();
			Reader.SetPointer(Head.RegMapOffset);
			auto Span = Reader.ReadObjectArray<RegInfoT>(Head.RegMapCount);
			for (std::size_t I = 0; I < Span.size(); ++I)
			{
				auto Tar = Table.RegMap[I];
				auto& Sor = Span[I];
				Sor.MapSymbolValue = static_cast<StandardT>(Tar.MapSymbolValue);
				Sor.UserMask = Tar.UserMask.has_value() ? static_cast<StandardT>(*Tar.UserMask) : static_cast<StandardT>(0);
			}
		}

		Head.LexicalOffset = static_cast<StandardT>(Write.GetWritedSize());

		Reg::DfaBinaryTableWrapper::Serilize(Write, Table.Lexical);

		Head.LexicalCount = static_cast<StandardT>(Write.GetWritedSize() - Head.LexicalOffset);

		Head.SyntaxOffset = static_cast<StandardT>(Write.GetWritedSize());

		SLRX::LRXBinaryTableWrapper::Serilize(Write, Table.Syntax);

		Head.SyntaxCount = static_cast<StandardT>(Write.GetWritedSize() - Head.SyntaxOffset);

		if (Write.IsWritting())
		{
			auto Reader = *Write.GetReader();
			Reader.SetPointer(StartOffset);
			auto PHead = Reader.ReadObject<HeadT>();
			*PHead = Head;
		}

		Write.PopMark(OldMark);
	}

	std::vector<EbnfBinaryTableWrapper::StandardT> CreateEbnfBinaryTable(Ebnf const& Table)
	{
		Misc::StructedSerilizerWritter<EbnfBinaryTableWrapper::StandardT> Pre;
		EbnfBinaryTableWrapper::Serilize(Pre, Table);
		std::vector<EbnfBinaryTableWrapper::StandardT> Buffer;
		Buffer.resize(Pre.GetWritedSize());
		Misc::StructedSerilizerWritter<EbnfBinaryTableWrapper::StandardT> Write(std::span{Buffer});
		EbnfBinaryTableWrapper::Serilize(Write, Table);
		return Buffer;
	}


	std::tuple<SymbolInfo, std::size_t> EbnfProcessor::Tranlate(std::size_t Mask, Misc::IndexSpan<> TokenIndex) const
	{
		assert(!std::holds_alternative<std::monostate>(TableWrapper));

		if (std::holds_alternative<Pointer::ObserverPtr<Ebnf const>>(TableWrapper))
		{
			auto Inf = std::get<Pointer::ObserverPtr<Ebnf const>>(TableWrapper)->GetRgeInfo(Mask);
			return {
				{SLRX::Symbol::AsTerminal(Inf.MapSymbolValue), std::get<Pointer::ObserverPtr<Ebnf const>>(TableWrapper)->GetRegName(Inf.MapSymbolValue), TokenIndex},
				Inf.UserMask.has_value() ? *Inf.UserMask : 0,
			};
		}
		else {
			auto Inf = std::get<EbnfBinaryTableWrapper>(TableWrapper).GetRgeInfo(Mask);
			return {
				{SLRX::Symbol::AsTerminal(Inf.MapSymbolValue), std::get<EbnfBinaryTableWrapper>(TableWrapper).GetRegName(Inf.MapSymbolValue), TokenIndex},
				Inf.UserMask.has_value() ? *Inf.UserMask : 0,
			};
		}
	}

	SymbolInfo EbnfProcessor::Tranlate(SLRX::Symbol Symbol, Misc::IndexSpan<> TokenIndex) const
	{
		assert(!std::holds_alternative<std::monostate>(TableWrapper));
		if (std::holds_alternative<Pointer::ObserverPtr<Ebnf const>>(TableWrapper))
		{
			return {Symbol, std::get<Pointer::ObserverPtr<Ebnf const>>(TableWrapper)->GetRegName(Symbol.symbol), TokenIndex };
		}
		else {
			return { Symbol, std::get<EbnfBinaryTableWrapper>(TableWrapper).GetRegName(Symbol.symbol), TokenIndex };
		}
	}

	void EbnfProcessor::SetObserverTable(Ebnf const& Table, Pointer::ObserverPtr<EbnfOperator> Ope, std::size_t StartupTokenIndex)
	{
		Operator = std::move(Ope);
		TableWrapper = &Table;
		LexicalProcessor.SetObserverTable(Table.Lexical);
		SyntaxProcessor.SetObserverTable(Table.Syntax, this);
		LastSymbolTokenIndex = StartupTokenIndex;
		RequireTokenIndex = StartupTokenIndex;
	}

	void EbnfProcessor::SetObserverTable(EbnfBinaryTableWrapper Table, Pointer::ObserverPtr<EbnfOperator> Ope, std::size_t StartupTokenIndex)
	{
		Operator = std::move(Ope);
		TableWrapper = Table;
		LexicalProcessor.SetObserverTable(Table.GetLexicalTable());
		SyntaxProcessor.SetObserverTable(Table.GetSyntaxTable(), this);
		StartupTokenIndex = StartupTokenIndex;
		RequireTokenIndex = StartupTokenIndex;
	}

	void EbnfProcessor::Clear(std::size_t Startup) {
		LastSymbolTokenIndex = Startup;
		RequireTokenIndex = Startup;
		LexicalProcessor.Clear();
		SyntaxProcessor.Clear();
	}

	bool EbnfProcessor::Consume(char32_t Value, std::size_t NextTokenIndex)
	{
		assert(!std::holds_alternative<std::monostate>(TableWrapper));
		assert(Operator);
		if (LexicalProcessor.Consume(Value, GetRequireTokenIndex()))
		{
			RequireTokenIndex = NextTokenIndex;
			return true;
		}
		else {
			auto Accept = LexicalProcessor.GetAccept();
			if (Accept)
			{
				auto MainCapture = Accept.GetMainCapture();
				auto Capture = MainCapture;
				if(Accept.GetCaptureSize() >= 1)
					Capture = Accept[0];
				bool Re = AddTerminalSymbol(*Accept.Mask, Capture);
				if (Re)
				{
					RequireTokenIndex = Accept.GetMainCapture().End();
					LastSymbolTokenIndex = RequireTokenIndex;
					LexicalProcessor.Clear();
					return true;
				}
			}
			RequireTokenIndex = LastSymbolTokenIndex;
			LexicalProcessor.Clear();
			return false;
		}
	}

	bool EbnfProcessor::EndOfFile()
	{
		assert(!std::holds_alternative<std::monostate>(TableWrapper));
		assert(Operator);
		if (LastSymbolTokenIndex != RequireTokenIndex)
		{
			LexicalProcessor.EndOfFile(RequireTokenIndex);
			auto Accept = LexicalProcessor.GetAccept();
			if (Accept)
			{
				auto MainCapture = Accept.GetMainCapture();
				auto Capture = MainCapture;
				if (Accept.GetCaptureSize() >= 1)
					Capture = Accept[0];
				bool Re = AddTerminalSymbol(*Accept.Mask, Capture);
				if (Re)
				{
					if (MainCapture.End() == RequireTokenIndex)
					{
						if (TerminalEndOfFile())
						{
							RequireTokenIndex = RequireTokenIndex + 1;
							LastSymbolTokenIndex = RequireTokenIndex;
							LexicalProcessor.Clear();
							return true;
						}
					}
					else {
						RequireTokenIndex = Accept.GetMainCapture().End();
						LastSymbolTokenIndex = RequireTokenIndex;
						LexicalProcessor.Clear();
						return true;
					}
				}
			}
		}
		else {
			if (TerminalEndOfFile())
			{
				RequireTokenIndex = RequireTokenIndex + 1;
				LastSymbolTokenIndex = RequireTokenIndex;
				LexicalProcessor.Clear();
				return true;
			}
		}
		LexicalProcessor.Clear();
		RequireTokenIndex = LastSymbolTokenIndex;
		return false;
	}

	bool EbnfProcessor::AddTerminalSymbol(std::size_t RegIndex, Misc::IndexSpan<> TokenIndex)
	{
		auto [Sym, UserMask] = Tranlate(RegIndex, TokenIndex);
		if (Sym.Symbol.symbol == 0)
		{
			return true;
		}
		else {
			auto AppendData = Operator->HandleSymbol(Sym, UserMask);
			auto Re = SyntaxProcessor.Consume(Sym.Symbol, Sym.TokenIndex, std::move(AppendData));
			return Re;
		}
	}

	bool EbnfProcessor::TerminalEndOfFile()
	{
		return SyntaxProcessor.EndOfFile();
	}

	std::any EbnfProcessor::HandleReduce(SLRX::SymbolInfo Value, SLRX::ReduceProduction Desc)
	{
		auto Sym = Tranlate(Value.Value, Value.TokenIndex);
		TempElement.clear();

		for (auto& Ite : Desc.Elements)
		{
			ReduceProduction::Element Ele;
			Ele.AppendData = std::move(Ite.AppendData);
			static_cast<SymbolInfo&>(Ele) = Tranlate(Ite.Value.Value, Ite.Value.TokenIndex);
			if (Ele.SymbolName.empty())
			{
				assert(Ite.Reduce.has_value());
				switch (Ite.Reduce->UserMask)
				{
				case SmallBrace:
					Ele.SymbolName = RoundBracketStatementRegName();
					break;
				case MiddleBrace:
					Ele.SymbolName = SquareBracketStatementRegName();
					break;
				case BigBrace:
					Ele.SymbolName = CurlyBracketStatementRegName();
					break;
				case 0:
					assert(false);
					break;
				default:
					Ele.SymbolName = OrStatementRegName();
					break;
				}
			}
			TempElement.push_back(std::move(Ele));
		}

		if (!Sym.SymbolName.empty())
		{
			ReduceProduction Pro;
			Pro.UserMask = Desc.UserMask;
			Pro.Elements = std::span(TempElement);
			auto Re = Operator->HandleReduce(Sym, Pro);
			TempElement.clear();
			return Re;
		}
		else {
			BuilInStatement NewStatement;
			NewStatement.Info = Sym;
			NewStatement.UserMask = Desc.UserMask;
			switch (Desc.UserMask)
			{
			case SmallBrace:
				NewStatement.Info.SymbolName = RoundBracketStatementRegName();
				NewStatement.ProductionElements.insert(
					NewStatement.ProductionElements.end(),
					std::move_iterator(TempElement.begin()),
					std::move_iterator(TempElement.end())
				);
				break;
			case MiddleBrace:
				NewStatement.Info.SymbolName = SquareBracketStatementRegName();
				NewStatement.ProductionElements.insert(
					NewStatement.ProductionElements.end(),
					std::move_iterator(TempElement.begin()),
					std::move_iterator(TempElement.end())
				);
				break;
			case BigBrace:
				NewStatement.Info.SymbolName = CurlyBracketStatementRegName();
				if (TempElement.size() >= 1)
				{
					auto OldElement = TempElement[0].Consume<BuilInStatement>();
					NewStatement.ProductionElements.insert(
						NewStatement.ProductionElements.end(),
						std::move_iterator(OldElement.ProductionElements.begin()),
						std::move_iterator(OldElement.ProductionElements.end())
					);
					NewStatement.ProductionElements.insert(
						NewStatement.ProductionElements.end(),
						std::move_iterator(TempElement.begin() + 1),
						std::move_iterator(TempElement.end())
					);
				}
				else {
					NewStatement.ProductionElements.insert(
						NewStatement.ProductionElements.end(),
						std::move_iterator(TempElement.begin()),
						std::move_iterator(TempElement.end())
					);
				}
				break;
			case 0:
				assert(false);
				break;
			default:
				NewStatement.Info.SymbolName = OrStatementRegName();
				NewStatement.UserMask -= MinMask;
				NewStatement.ProductionElements.insert(
					NewStatement.ProductionElements.end(),
					std::move_iterator(TempElement.begin()),
					std::move_iterator(TempElement.end())
				);
				break;
			}
			TempElement.clear();
			return std::move(NewStatement);
		}
	}
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
					assert(Ite.Value.symbol < TMapping.size());
					Result.push_back({ TMap{TMapping[Ite.Value.symbol]} });
				}
			}
			else if(Ite.IsNoTerminal()) 
			{
				if (Ite.Value.symbol < NTMapping.size())
				{
					Result.push_back({ NTMap{NTMapping[Ite.Value.symbol], Ite.Reduce.ElementCount, Ite.Reduce.Mask} });
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
					default:
						Result.push_back({ NTMap{UR"(...|...)", 0, 0} });
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