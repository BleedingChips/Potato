#include "../Public/Ebnf.h"
#include "../Public/StrFormat.h"
#include <assert.h>
#include <vector>

namespace Potato::Ebnf
{
	enum class T
	{
		Empty = 0,
		Line,
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
		Or,
		Number,
		Colon,
		Command,
		TokenMax,
		Ignore, // ^
	};

	constexpr Lr0::Symbol operator*(T sym) { return Lr0::Symbol{ static_cast<size_t>(sym), Lr0::TerminalT{} }; };

	enum class NT
	{
		Statement,
		FunctionEnum,
		ProductionHead,
		RemoveElement,
		RemoveExpression,
		AppendExpression,
		Expression,
		ExpressionStatement,
		LeftOrStatement,
		RightOrStatement,
		OrStatement,
	};

	constexpr Lr0::Symbol operator*(NT sym) { return Lr0::Symbol{ static_cast<size_t>(sym), Lr0::NoTerminalT{} }; };

	std::u32string_view Table::FindSymbolString(size_t input, bool IsTerminal) const noexcept {
		if (IsTerminal)
		{
			if (input < ter_count)
			{
				auto [s, size] = symbol_map[input];
				return std::u32string_view{ symbol_table.data() + s, size };
			}
		}
		else {
			input += ter_count;
			if (input < symbol_map.size())
			{
				auto [s, size] = symbol_map[input];
				return std::u32string_view{ symbol_table.data() + s, size };
			}
		}
		return {};
	}

	std::optional<size_t> Table::FindSymbolState(std::u32string_view sym, bool& IsTerminal) const noexcept
	{
		for (size_t i = 0; i < symbol_map.size(); ++i)
		{
			auto [s, size] = symbol_map[i];
			auto str = std::u32string_view{ symbol_table.data() + s, size };
			if (str == sym)
			{
				if (i > ter_count) {
					IsTerminal = false;
					return i;
				}
				else {
					IsTerminal = true;
					return i;
				}
			}
		}
		return std::nullopt;
	}

	std::tuple<std::vector<Symbol>, std::vector<Lexical::March>> DefaultLexer(Lexical::Table const& table, std::u32string_view input)
	{
		std::vector<Symbol> R1;
		std::vector<Lexical::March> R2 = table.Process(input);
		for(auto& ite : R2)
			R1.push_back(Symbol(ite.mask, Lr0::TerminalT{}));
		return { std::move(R1), std::move(R2) };
	}

	History Translate(Table const& Tab, Lr0::History const& Steps, std::vector<Lexical::March> const& Datas)
	{
		std::vector<Step> AllStep;
		AllStep.reserve(Steps.steps.size());
		Section LastSection;
		std::vector<size_t> TemporaryNoTerminalList;
		std::vector<std::optional<size_t>> SimulateProduction;
		for (auto& Ite : Steps.steps)
		{
			Step Result{};
			Result.state = Ite.value.Index();
			Result.is_terminal = Ite.IsTerminal();
			Result.string = Tab.FindSymbolString(Result.state, Result.is_terminal);
			if(!Result.string.empty() && Result.is_terminal)
			{
				auto& DatasRef = Datas[Ite.shift.token_index];
				Result.section = DatasRef.section;
				LastSection = DatasRef.section;
				Result.shift.capture = DatasRef.capture;
				Result.shift.mask = Tab.state_to_mask[DatasRef.acception];
				AllStep.push_back(Result);
				SimulateProduction.push_back(std::nullopt);
			}else
			{
				assert(Result.IsNoterminal());
				size_t ProCount = Ite.reduce.production_count;
				size_t Used = 0;
				size_t UsedSimulateCount = 0;
				size_t StartPosition = SimulateProduction.size() - ProCount;
				for (size_t i = SimulateProduction.size(); i > StartPosition; --i)
				{
					if (!SimulateProduction[i - 1].has_value())
					{
						++Used;
					}
					else
					{
						assert(!TemporaryNoTerminalList.empty());
						Used += *TemporaryNoTerminalList.rbegin();
						TemporaryNoTerminalList.pop_back();
					}
					++UsedSimulateCount;
				}
				SimulateProduction.resize(SimulateProduction.size() - UsedSimulateCount);
				if(Result.string.empty())
				{
					TemporaryNoTerminalList.push_back(Used);
					SimulateProduction.push_back(TemporaryNoTerminalList.size());
				}else
				{
					Result.reduce.mask = Ite.reduce.mask;
					Result.reduce.production_count = Used;
					Result.section = LastSection;
					SimulateProduction.push_back(std::nullopt);
					AllStep.push_back(Result);
				}
			}
		}
		return History{ std::move(AllStep) };
	}

	History Process(Table const& Tab, std::u32string_view Code)
	{
		auto [Symbols, Datas] = DefaultLexer(Tab.lexical_table, Code);

		try{
			auto Steps = Lr0::Process(Tab.lr0_table, Symbols.data(), Symbols.size());
			return Translate(Tab, Steps, Datas);
		}
		catch (Exception::Lr::UnaccableSymbol const& Symbol)
		{
			auto his = Translate(Tab, Symbol.backup_step, Datas);
			auto Str = Tab.FindSymbolString(Symbol.symbol.Index(), Symbol.symbol.IsTerminal());
			if (Str.empty())
			{
				Section loc = (Symbol.index > 0) ? Datas[Symbol.index - 1].section : Section{};
				throw Exception::MakeExceptionTuple(Exception::Ebnf::UnacceptableSyntax{ U"$_Eof", U"$_Eof", loc, his.Expand() });
			}
			auto loc = Datas[Symbol.symbol.Index()].section;
			throw Exception::MakeExceptionTuple(Exception::Ebnf::UnacceptableSyntax{ std::u32string(Str), std::u32string(Datas[Symbol.index].capture),Datas[Symbol.index].section, his.Expand() });
		}
	}

	std::any History::operator()(std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFunc) const
	{
		if(NTFunc && TFunc)
		{
			std::vector<NTElement::Property> Storage;
			for (auto& ite : steps)
			{
				if (ite.IsTerminal())
				{
					TElement Re(ite);
					auto Result = TFunc(Re);
					Storage.push_back({ ite, std::move(Result) });
				}
				else {
					
					size_t TotalUsed = ite.reduce.production_count;
					size_t CurrentAdress = Storage.size() - TotalUsed;
					NTElement Re(ite, Storage.data() + CurrentAdress);
					if (TotalUsed >= 1)
					{
						Re.section = {Re[0].section.start, Re[TotalUsed - 1].section.end};
					}
					else {
						Re.section = ite.section;
					}
					auto Result = NTFunc(Re);
					Storage.resize(CurrentAdress);
					Storage.push_back({ ite, std::move(Result) });
				}
			}
			assert(Storage.size() == 1);
			return std::move(Storage[0].data);
		}
		return {};
	}

	std::u32string ExpandExe(std::vector<std::u32string_view> ts)
	{
		std::u32string result;
		static auto pattern = StrFormat::CreatePatternRef(U"{} ");
		for (auto& ite : ts)
		{
			result += StrFormat::Process(pattern, ite);
		}
		return result;
	}

	std::vector<std::u32string> History::Expand() const
	{
		std::vector<std::u32string> result;
		std::vector<std::u32string_view> all_token;
		bool InputTerminal = false;
		for (auto& ite : steps)
		{
			if (ite.IsTerminal())
			{
				all_token.push_back(ite.shift.capture);
				InputTerminal = true;
			}
			else {
				if (InputTerminal)
				{
					auto re = ExpandExe(all_token);
					result.push_back(std::move(re));
					InputTerminal = false;
				}
				assert(all_token.size() >= ite.reduce.production_count);
				all_token.resize(all_token.size() - ite.reduce.production_count);
				all_token.push_back(ite.string);
				auto re = ExpandExe(all_token);
				result.push_back(std::move(re));
			}
		}
		if (!all_token.empty())
			result.push_back(ExpandExe(all_token));
		return result;
	}

	struct PreEbnfInitTuple
	{
		T symbol;
		bool ignore = false;
	};

	Lexical::Table CreateLexicalTable(std::map<T, std::u32string_view> const& mapping, PreEbnfInitTuple const* init_tuple, size_t length)
	{
		std::vector<Lexical::RegexInitTuple> true_tuple;
		true_tuple.reserve(length);
		for(size_t i =0; i < length; ++i)
		{
			auto& ref = init_tuple[i];
			auto Find = mapping.find(ref.symbol);
			assert(Find != mapping.end());
			true_tuple.push_back({Find->second, ref.ignore ? Lexical::DefaultIgnoreMask() : static_cast<size_t>(Find->first)});
		}
		return Lexical::CreateLexicalFromRegexs(true_tuple.data(), true_tuple.size());
	}

	std::tuple<std::vector<Symbol>, std::vector<Lexical::March>> EbnfLexer(Lexical::Table const& table, std::u32string_view input, Section& Loc)
	{
		std::vector<Symbol> R1;
		std::vector<Lexical::March> R2 = table.Process(input);
		R1.reserve(R2.size());
		for(auto& ite : R2)
			R1.push_back(Symbol(ite.mask, Lr0::TerminalT{}));
		if(!R2.empty())
		{
			Loc.start = Loc.end;
			Loc.end = Loc.end + R2.rbegin()->section.end;
		}
		return { std::move(R1), std::move(R2) };
	}

	Table CreateTable(std::u32string_view code)
	{

		static std::map<T, std::u32string_view> Rexs = {
			{T::Empty, UR"(\s)" },
			{T::Line, UR"(\n)"},
			{T::Terminal, UR"([a-zA-Z_][a-zA-Z_0-9]*)"},
			{T::Equal, UR"(:=)"},
			{T::Rex, UR"('([^\']|(\\\'))*?')"},
			{T::NoTerminal, UR"(\<[_a-zA-Z][_a-zA-Z0-9]*\>)"},
			{T::StartSymbol, UR"(\$)"},
			{T::LB_Brace, UR"(\{)"},
			{T::RB_Brace, UR"(\})"},
			{T::LM_Brace, UR"(\[)"},
			{T::RM_Brace, UR"(\])"},
			{T::LS_Brace, UR"(\()"},
			{T::RS_Brace, UR"(\))"},
			{T::Colon, UR"(:)"},
			{T::Or, UR"(\|)"},
			{T::Ignore, UR"(\^)"},
			{T::Number, UR"([1-9][0-9]*|0)"},
			{T::Command, UR"(/\*.*?\*/|//.*?\n)"},
		};

		static Unfa::SerilizedTable sperator = Unfa::CreateUnfaTableFromRegex(UR"((.*?(?:\r\n|\n))[\f\t\v\r]*?%%%[\s]*?\n|()[\f\t\v\r]*?%%%[\s]*?\n)").Simplify();
		auto end_point = CalculateSectionPoint(code);
		struct SperatedCode
		{
			std::u32string_view code;
			Section section;
		};
		SperatedCode sperated_code[3];
		{
			size_t used = 0;
			SectionPoint Point;
			while (!code.empty() && used < 3)
			{
				auto P = sperator.Mark(code, false);
				
				if (P)
				{
					auto new_point = CalculateSectionPoint(P->sub_capture[0].string);
					auto cur_end_point = Point + new_point;
					sperated_code[used] = { P->sub_capture[0].string, {Point, cur_end_point} };
					code = { code.begin() + P->capture.string.size(), code.end() };
					Point = cur_end_point;
				}
				else
				{
					sperated_code[used] = { code, {Point, end_point} };
					code = {};
				}
				
				++used;
			}

			if (used < 2)
				throw Exception::MakeExceptionTuple(Exception::Ebnf::UncompleteEbnf{ used });
		}
		
		std::vector<uint32_t> state_to_mask;
		std::map<std::u32string, uint32_t> symbol_to_mask;
		std::vector<std::tuple<std::u32string, uint32_t>> symbol_rex;
		//std::vector<Lexical::LexicalRegexInitTuple> symbol_rex;

		// step1
		{

			static Lexical::Table nfa_table = ([]() -> Lexical::Table {
				PreEbnfInitTuple RequireList[] = {
					{T::LM_Brace}, {T::RM_Brace}, {T::Number}, {T::Colon}, {T::Terminal},
					{T::Equal}, {T::Rex}, {T::Line}, {T::Command, true}, {T::Empty, true }, {T::Ignore}
				};
				return CreateLexicalTable(Rexs, RequireList, std::size(RequireList));
			}());

			static Lr0::Table lr0_instance = Lr0::CreateTable(
				*NT::Statement, {
					{{*NT::Statement, *NT::Statement, *T::Terminal, *T::Equal, *T::Rex, *NT::FunctionEnum, *T::Line}, 1},
					{{*NT::Statement, *NT::Statement, *T::Ignore, *T::Equal, *T::Rex, *T::Line}, 7},
					{{*NT::Statement}, 3},
					{{*NT::FunctionEnum, *T::Colon, *T::LM_Brace, *T::Number, *T::RM_Brace}, 5},
					{{*NT::FunctionEnum}, 6},
					//{{*SYM::Statement,*SYM::Statement,  *SYM::Mask}},
					{{*NT::Statement, *NT::Statement,  *T::Line}, 4}
				}, {}
			);
			Section Loc;
			auto [Symbols, Elements] = EbnfLexer(nfa_table, sperated_code[0].code, Loc);
			try {
				auto History = Lr0::Process(lr0_instance, Symbols.data(), Symbols.size());
				Lr::Process(History, [&](Lr0::NTElement& input) -> std::any {
					switch (input.mask)
					{
						case 1: {
							auto Token = input[1].Consume<std::u32string_view>();
							auto Rex = input[3].Consume<std::u32string_view>();
							auto re = symbol_to_mask.insert({ std::u32string(Token), static_cast<uint32_t>(symbol_to_mask.size()) });
							state_to_mask.push_back(input[4].Consume<uint32_t>());
							symbol_rex.push_back({ std::u32string(Rex), re.first->second });
						}break;
						case 7: {
							auto Rex = input[3].Consume<std::u32string_view>();
							symbol_rex.push_back({ std::u32string(Rex), Lexical::DefaultIgnoreMask() });
							state_to_mask.push_back(Lexical::DefaultMask());
						} break;
						case 5: {
							return input[2].Consume<uint32_t>();
						}break;
						case 6: {
							return Lexical::DefaultMask();
						} break;
						case 4:
							return false;
						case 3:
							break;
						default: assert(false); break;
					}
					return {};
				},
				[&](Lr0::TElement& input) -> std::any
				{
					switch (input.value)
					{
					case* T::Terminal: {
						return Elements[input.token_index].capture;
					}break;
					case* T::Rex: {
						auto re = Elements[input.token_index].capture;
						return  std::u32string_view(re.data() + 1, re.size() - 2);
					}break;
					case* T::Number: {
						uint32_t Number = 0;
						for (auto ite : Elements[input.token_index].capture)
							Number = Number * 10 + ite - U'0';
						return Number;
					} break;
					default:
						return {};
					}
					return {};
				}
				);
			}
			catch (Exception::Lr::UnaccableSymbol const& US)
			{
				assert(Elements.size() > US.index);
				auto P = Elements[US.index];
				throw Exception::MakeExceptionTuple(Exception::Ebnf::UnacceptableToken{ std::u32string(P.capture), P.section});
			}
		}

		std::map<std::u32string, size_t> noterminal_symbol_to_index;
		std::vector<Lr0::ProductionInput> productions;
		std::optional<Symbol> start_symbol;
		size_t noterminal_temporary = Lr::symbol_storage_max - 1;


		struct Token
		{
			Symbol sym;
			Lexical::March march;
		};

		//std::map<lr1::storage_t, std::tuple<std::variant<OrRelationShift, MBraceRelationShift, BBraceRelationShift>, size_t>> temporary_noterminal_production_debug;

		// step2
		{

			static Lexical::Table nfa_instance = ([]() -> Lexical::Table {
				PreEbnfInitTuple RequireList[] = {
					{T::Or}, {T::StartSymbol}, {T::Colon}, {T::Terminal}, {T::Equal}, {T::Number}, {T::NoTerminal}, {T::Rex}, {T::Line},
					{T::LS_Brace}, {T::RS_Brace}, {T::LM_Brace}, {T::RM_Brace}, {T::LB_Brace}, {T::RB_Brace}, {T::Command, true}, {T::Empty, true}
				};
				return CreateLexicalTable(Rexs, RequireList, std::size(RequireList));
			}());


			static Lr0::Table imp = Lr0::CreateTable(
				*NT::Statement, {
				{{*NT::Statement}, 0},
				{{*NT::Statement, *NT::Statement, *T::Line}, 0},
				{{*NT::Statement, *NT::Statement, *T::StartSymbol, *T::Equal, *T::NoTerminal, *T::Line}, 1},

				{{*NT::LeftOrStatement, *NT::Expression}, 2},
				{{*NT::LeftOrStatement, *NT::LeftOrStatement, *NT::Expression}, 3},
				{{*NT::RightOrStatement, *NT::Expression}, 2},
				{{*NT::RightOrStatement, *NT::Expression, *NT::RightOrStatement}, 3},
				{{*NT::OrStatement, *NT::LeftOrStatement, *T::Or, *NT::RightOrStatement}, 5},
				{{*NT::OrStatement, *NT::OrStatement, *T::Or, *NT::RightOrStatement}, 5},

				{{*NT::ProductionHead, *T::NoTerminal}, 6},
				{{*NT::ProductionHead}, 7},

				{{*NT::Statement, *NT::Statement, *NT::ProductionHead, *T::Equal, *NT::OrStatement, *NT::AppendExpression, *T::Line}, 8},
				{{*NT::Statement, *NT::Statement, *NT::ProductionHead, *T::Equal, *NT::ExpressionStatement, *NT::AppendExpression, *T::Line}, 8},
				{{*NT::Statement, *NT::Statement, *NT::ProductionHead, *T::Equal, *NT::AppendExpression, *T::Line}, 19},

				{{*NT::ExpressionStatement, *NT::Expression}, 2},
				{{*NT::ExpressionStatement, *NT::ExpressionStatement, *NT::Expression}, 3},

				{{*NT::Expression, *T::LS_Brace, *NT::OrStatement, *T::RS_Brace}, 9},
				{{*NT::Expression, *T::LS_Brace, *NT::ExpressionStatement, *T::RS_Brace}, 9},
				{{*NT::Expression, *T::LM_Brace, *NT::OrStatement, *T::RM_Brace}, 10},
				{{*NT::Expression, *T::LM_Brace, *NT::ExpressionStatement, *T::RM_Brace}, 10},
				{{*NT::Expression, *T::LB_Brace, *NT::OrStatement, *T::RB_Brace}, 11},
				{{*NT::Expression, *T::LB_Brace, *NT::ExpressionStatement, *T::RB_Brace}, 11},
				{{*NT::Expression, *T::NoTerminal}, 12},
				{{*NT::Expression, *T::Terminal}, 12},
				{{*NT::Expression, *T::Rex}, 12},

				{{*NT::FunctionEnum, *T::LM_Brace, *T::Number, *T::RM_Brace}, 13},
				{{*NT::FunctionEnum}, 14},
				{{*NT::RemoveElement, *NT::RemoveElement, *T::Terminal}, 16},
				{{*NT::RemoveElement, *NT::RemoveElement, *T::Rex}, 16},
				{{*NT::RemoveElement}, 15},

				{{*NT::AppendExpression, *T::Colon, *NT::RemoveElement, *NT::FunctionEnum}, 17},
				{{*NT::AppendExpression}, 18},
				},
				{}
			);

			std::optional<Symbol> LastHead;
			size_t LastTemporaryNoTerminal = Lr0::symbol_storage_max - 1;

			using SymbolList = std::vector<Symbol>;

			Section Loc = sperated_code[0].section;
			
			auto [Symbols, Elements] = EbnfLexer(nfa_instance, sperated_code[1].code, Loc);


			try {
				auto History = Lr0::Process(imp, Symbols.data(), Symbols.size());
				auto Total = Lr::Process(History, [&](Lr0::NTElement & tra) -> std::any {
					switch (tra.mask)
						{
						case 0: {
						} break;
						case 1: {
							LastHead = std::nullopt;
							auto P1 = tra[3].Consume<Token>();
							if (!start_symbol)
								start_symbol = P1.sym;
							else
								throw Exception::MakeExceptionTuple(Exception::Ebnf::RedefinedStartSymbol{ P1.march.section });
							return std::vector<Lr0::ProductionInput>{};
						}break;
						case 2: {
							return tra[0].Consume();
						} break;
						case 3: {
							auto P1 = tra[0].Consume<SymbolList>();
							auto P2 = tra[1].Consume<SymbolList>();
							P1.insert(P1.end(), P2.begin(), P2.end());
							return std::move(P1);
						}break;
						case 5: {
							auto TemSym = Symbol(noterminal_temporary--, Lr0::NoTerminalT{});
							auto P1 = tra[0].Consume<SymbolList>();
							auto P2 = tra[2].Consume<SymbolList>();
							P1.insert(P1.begin(), TemSym);
							P2.insert(P2.begin(), TemSym);
							productions.push_back({ std::move(P1) });
							productions.push_back({ std::move(P2) });
							return SymbolList{ TemSym };
						} break;
						case 6: {
							auto P1 = tra[0].Consume<Token>();
							LastHead = P1.sym;
							return P1;
						}
						case 7: {
							if (!LastHead)
								throw Exception::MakeExceptionTuple(Exception::Ebnf::UnsetDefaultProductionHead{});
							return Token{ *LastHead, {} };
						}
						case 8: {
							auto Head = tra[1].Consume<Token>();
							auto Expression = tra[3].Consume<SymbolList>();
							auto [RemoveRe, Enum] = tra[4].Consume<std::tuple<std::set<Symbol>, size_t>>();
							Expression.insert(Expression.begin(), Head.sym);
							Lr0::ProductionInput re(std::move(Expression), std::move(RemoveRe), Enum);
							productions.push_back(std::move(re));
							return {};
						}break;
						case 19: {
							auto Head = tra[1].Consume<Token>();
							auto [RemoveRe, Enum] = std::move(tra[3].Consume<std::tuple<std::set<Symbol>, size_t>>());
							SymbolList Expression({ Head.sym });
							Lr0::ProductionInput re(std::move(Expression), std::move(RemoveRe), Enum);
							productions.push_back(std::move(re));
							return {};
						}break;
						case 9: {
							return tra[1].Consume();
						}break;
						case 10: {
							auto TemSym = Symbol(noterminal_temporary--, Lr0::NoTerminalT{});
							auto P = tra[1].Consume<SymbolList>();
							P.insert(P.begin(), TemSym);
							productions.push_back(std::move(P));
							productions.push_back(Lr0::ProductionInput({ TemSym }));
							return SymbolList{ TemSym };
						}break;
						case 11: {
							auto TemSym = Symbol(noterminal_temporary--, Lr0::NoTerminalT{});
							auto P = tra[1].Consume<SymbolList>();
							auto List = { TemSym, TemSym };
							P.insert(P.begin(), List.begin(), List.end());
							productions.push_back(std::move(P));
							productions.push_back(Lr0::ProductionInput({ TemSym }));
							return SymbolList{ TemSym };
						} break;
						case 12: {
							auto P = tra[0].Consume<Token>();
							SymbolList SL({ P.sym });
							return std::move(SL);
						} break;
						case 13: {
							return tra[1].Consume();
						} break;
						case 14: {
							return size_t(Lr0::ProductionInput::default_mask());
						} break;
						case 15: {
							return std::set<Symbol>{};
						}break;
						case 16: {
							auto P = tra[0].Consume<std::set<Symbol>>();
							auto P2 = tra[1].Consume<Token>();
							P.insert(P2.sym);
							return std::move(P);
						}break;
						case 17: {
							return std::tuple<std::set<Symbol>, size_t>{tra[1].Consume<std::set<Symbol>>(), tra[2].Consume<size_t>()};
						} break;
						case 18: {
							return std::tuple<std::set<Symbol>, size_t>{ std::set<Symbol>{}, Lr0::ProductionInput::default_mask()};
						} break;
						}
					return {};
				},
				[&](Lr0::TElement& tra) -> std::any
				{
					auto& element = Elements[tra.token_index];
					auto string = std::u32string(element.capture);
					switch (tra.value)
					{
						case* T::Terminal: {
							auto Find = symbol_to_mask.find(string);
							if (Find != symbol_to_mask.end())
								return Token{ Symbol(Find->second, Lr0::TerminalT{}), element };
							else
								throw Exception::MakeExceptionTuple(Exception::Ebnf::UndefinedTerminal{ string, element.section });
						}break;
						case* T::NoTerminal: {
							auto Find = noterminal_symbol_to_index.insert({ string, noterminal_symbol_to_index.size() });
							return Token{ Symbol(Find.first->second, Lr0::NoTerminalT{}), element };
						}break;
						case* T::Rex: {
							static const std::u32string SpecialChar = UR"($()*+.[]?\^{}|,\)";
							assert(string.size() >= 2);
							auto re = symbol_to_mask.insert({ string, static_cast<uint32_t>(symbol_to_mask.size()) });
							if (re.second)
							{
								std::u32string rex;
								for (size_t i = 1; i < string.size() - 1; ++i)
								{
									for (auto& ite : SpecialChar)
										if (ite == string[i])
										{
											rex.push_back(U'\\');
											break;
										}
									rex.push_back(string[i]);
								}
								symbol_rex.push_back({ std::move(rex),  re.first->second });
								state_to_mask.push_back(Lexical::DefaultMask());
							}
							return Token{ Symbol(re.first->second, Lr0::TerminalT{}), element };
						}break;
						case* T::Number: {
							size_t Number = 0;
							for (auto ite : string)
								Number = Number * 10 + ite - U'0';
							return Number;
						}break;
						default:
							break;
					}
					return {};
				}
				);
			}
			catch (Exception::Lr::UnaccableSymbol const& US)
			{
				auto P = Elements[US.index];
				throw Exception::MakeExceptionTuple(Exception::Ebnf::UnacceptableToken{ std::u32string(P.capture), P.section });
			}

		}

		std::vector<Lr0::OpePriority> operator_priority;
		// step3
		{
			static Lexical::Table nfa_instance = ([]() -> Lexical::Table {
				PreEbnfInitTuple RequireList[] = { {T::Terminal}, {T::Rex}, {T::Command, true},
					{T::LS_Brace}, {T::RS_Brace}, {T::LM_Brace}, {T::RM_Brace}, {T::Empty, true} };
				return CreateLexicalTable(Rexs, RequireList, std::size(RequireList));
			}());

			static Lr0::Table lr0_instance = Lr0::CreateTable(
				*NT::Statement, {
					{{*NT::Expression, *T::Terminal}, 1},
					{{*NT::Expression, *T::Rex}, 1},
					{{*NT::Expression, *NT::Expression, *NT::Expression}, 2},
					{{*NT::Statement, *NT::Statement, *T::Terminal}, 3},
					{{*NT::Statement, *NT::Statement, *T::Rex}, 3},
					{{*NT::Statement, *NT::Statement, *T::LS_Brace, *NT::Expression, *T::RS_Brace}, 4},
					{{*NT::Statement, *NT::Statement, *T::LM_Brace, *NT::Expression, *T::RM_Brace}, 5},
					{{*NT::Statement}},
				}, {}
			);

			Section Loc = sperated_code[2].section;

			auto [Symbols, Elements] = EbnfLexer(nfa_instance, sperated_code[2].code, Loc);
			try {
				auto History = Lr0::Process(lr0_instance, Symbols.data(), Symbols.size());
				Lr::Process(History, [&](Lr0::NTElement& step) -> std::any {
					switch (step.mask)
						{
						case 1: {
							std::vector<Symbol> List;
							List.push_back(step[0].Consume<Token>().sym);
							return std::move(List);
						}break;
						case 2: {
							auto P1 = step[0].Consume<std::vector<Symbol>>();
							auto P2 = step[1].Consume<std::vector<Symbol>>();
							P1.insert(P1.end(), P2.begin(), P2.end());
							return std::move(P1);
						} break;
						case 3: {
							auto P = step[1].Consume<Token>().sym;
							operator_priority.push_back({ {P}, Lr0::Associativity::Left });
							return {};
						} break;
						case 4: {
							auto P = step[2].Consume<std::vector<Symbol>>();
							operator_priority.push_back({ std::move(P), Lr0::Associativity::Left });
							return {};
						} break;
						case 5: {
							auto P = step[2].Consume<std::vector<Symbol>>();
							operator_priority.push_back({ std::move(P), Lr0::Associativity::Right });
							return {};
						} break;
						}
					return {};
				},
				[&](Lr0::TElement& step)->std::any
				{
					if (step.value == *T::Terminal || step.value == *T::Rex)
					{
						auto element = Elements[step.token_index];
						auto Find = symbol_to_mask.find(std::u32string(element.capture));
						if (Find != symbol_to_mask.end())
							return Token{ Symbol(Find->second, Lr0::TerminalT{}) };
						else
							throw Exception::MakeExceptionTuple(Exception::Ebnf::UndefinedTerminal{std::u32string(element.capture), element.section });
					}
					return {};
				}
				);
			}
			catch (Exception::Lr::UnaccableSymbol const& US)
			{
				auto P = Elements[US.index];
				throw Exception::MakeExceptionTuple(Exception::Ebnf::UnacceptableToken{ std::u32string(P.capture), P.section });
			}
		}

		if (!start_symbol)
			throw Exception::MakeExceptionTuple(Exception::Ebnf::MissingStartSymbol{});

		size_t DefineProduction_count = productions.size();
		//productions.insert(productions.end(), std::move_iterator(productions_for_temporary.begin()), std::move_iterator(productions_for_temporary.end()));
		std::u32string table;
		std::vector<std::tuple<size_t, size_t>> symbol_map;
		symbol_map.resize(symbol_to_mask.size() + noterminal_symbol_to_index.size(), {0, 0});
		for (auto ite : symbol_to_mask)
		{
			auto start = table.size();
			table += ite.first;
			symbol_map[ite.second] = {start, ite.first.size()};
		}
		size_t TerminalCount = symbol_to_mask.size();
		for (auto ite : noterminal_symbol_to_index)
		{
			auto start = table.size();
			table += ite.first;
			symbol_map[ite.second + TerminalCount] = { start, ite.first.size() };
		}
		
		try {
			std::vector<Lexical::RegexInitTuple> Rexs;
			Rexs.reserve(symbol_rex.size());
			for(auto& ite : symbol_rex)
			{
				auto& [str, mask] = ite;
				Rexs.push_back({std::u32string_view(str), mask});
			}
			Lexical::Table lexical_table = Lexical::CreateLexicalFromRegexsReverse(Rexs.data(), Rexs.size());
			Lr0::Table Lr0Table = Lr0::CreateTable(
				*start_symbol, productions, std::move(operator_priority)
			);
			return { std::move(table),
				std::move(state_to_mask),
				std::move(lexical_table),
				std::move(symbol_map), TerminalCount, std::move(Lr0Table) };
		}
		catch (Exception::Unfa::UnaccaptableRexgex const& ref)
		{
			throw Exception::MakeExceptionTuple(Exception::Ebnf::UnacceptableRegex{ref.regex, ref.accepetable_mask});
		}
		catch (Exception::Lr::NoterminalUndefined const NU)
		{
			for(auto& ite : noterminal_symbol_to_index)
				if(ite.second == NU.value.Index())
					throw Exception::MakeExceptionTuple(Exception::Ebnf::UndefinedNoterminal{std::u32string(ite.first)});
			throw Exception::MakeExceptionTuple(Exception::Ebnf::UndefinedNoterminal{ std::u32string(U"$UnknowSymbol") });
		}
	}
}

namespace PineApple::StrFormat
{

	/*
	struct Wrap { std::u32string const& ref; };

	template<> struct Formatter<Wrap>
	{
		std::u32string operator()(std::u32string_view, Wrap const& ref)
		{
			static auto pat = CreatePatternRef(U"\\x{-hex}");
			std::u32string W = U"U\"";
			for (auto& ite : ref.ref)
				W += Process(pat, static_cast<uint32_t>(ite));
			W += U"\"";
			return W;
		}
	};

	template<> struct Formatter<std::vector<std::tuple<std::size_t, std::size_t>>>
	{
		std::u32string operator()(std::u32string_view, std::vector<std::tuple<std::size_t, std::size_t>> const& ref)
		{
			static auto pat = CreatePatternRef(U"{{{}, {}}}, ");
			std::u32string W = U"{";
			for (auto [i1, i2] : ref)
				W += Process(pat, i1, i2);
			W += U"}";
			return W;
		}
	};

	std::u32string Formatter<Ebnf::Table>::operator()(std::u32string_view, Ebnf::Table const& ref)
	{
		static auto pat = CreatePatternRef(U"{{{}, {}, {}, {}, {}}}");
		return Process(pat, Wrap{ ref.symbol_table }, ref.symbol_map, ref.ter_count, ref.nfa_table, ref.lr0_table);
	}
	*/
}