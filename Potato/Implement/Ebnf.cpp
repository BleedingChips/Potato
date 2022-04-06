#include <assert.h>
#include <vector>
#include "../Include/PotatoEbnf.h"

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

	constexpr DLr::Symbol operator*(T sym) { return DLr::Symbol{ DLr::Symbol::ValueType::TERMINAL, static_cast<std::size_t>(sym)}; };

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

	constexpr DLr::Symbol operator*(NT sym) { return DLr::Symbol{ DLr::Symbol::ValueType::NOTERMIAL, static_cast<std::size_t>(sym) };};

	/*
	std::u32string_view EbnfTable::FindSymbolString(size_t input, bool IsTerminal) const noexcept {
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

	std::optional<size_t> EbnfTable::FindSymbolState(std::u32string_view sym, bool& IsTerminal) const noexcept
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

	std::tuple<std::vector<LrSymbol>, std::vector<LexicalMarch>> DefaultLexer(LexicalTable const& table, std::u32string_view input)
	{
		std::vector<LrSymbol> R1;
		std::vector<LexicalMarch> R2 = table.Process(input);
		for(auto& ite : R2)
			R1.push_back(LrSymbol(ite.mask, LrTerminalT{}));
		return { std::move(R1), std::move(R2) };
	}

	EbnfHistory Translate(EbnfTable const& Tab, LrHistory const& Steps, std::vector<LexicalMarch> const& Datas)
	{
		std::vector<std::u32string_view> Names;
		Names.reserve(Steps.steps.size());
		size_t NewStepSize = 0;
		for (auto& Ite : Steps.steps)
		{
			auto Name = Tab.FindSymbolString(Ite.value.Index(), Ite.IsTerminal());
			if (Ite.IsTerminal() || Name.empty())
				++NewStepSize;
			else
				NewStepSize += 2;
			Names.push_back(Name);
		}
		std::vector<EbnfStep> AllStep;
		AllStep.reserve(NewStepSize);
		Section LastSection;
		std::vector<size_t> AvailableElement;
		for (size_t index = 0; index < Steps.steps.size(); ++index)
		{
			auto& Ite = Steps.steps[index];
			EbnfStep Result{};
			Result.state = Ite.value.Index();
			Result.category = Ite.IsTerminal() ? EbnfStepCategory::TERMINAL : EbnfStepCategory::NOTERMINAL;
			Result.string = Names[index];
			if (Result.IsTerminal())
			{
				auto& DatasRef = Datas[Ite.shift.token_index];
				Result.section = DatasRef.section;
				LastSection = DatasRef.section;
				Result.shift.capture = DatasRef.capture;
				Result.shift.mask = Tab.state_to_mask[DatasRef.acception];
				AvailableElement.push_back(AllStep.size());
				AllStep.push_back(Result);
			}
			else
			{
				assert(Result.IsNoterminal());
				Result.reduce.mask = Ite.reduce.mask;
				Result.reduce.production_count = Ite.reduce.production_count;
				Result.section = LastSection;
				if (!Result.string.empty())
				{
					EbnfStep PreDefine = Result;
					PreDefine.category = EbnfStepCategory::PREDEFINETERMINAL;
					AllStep.insert(AllStep.begin() + AllStep.size() - Result.reduce.production_count, PreDefine);
				}
				AvailableElement.resize(AvailableElement.size() - Result.reduce.production_count);
				AvailableElement.push_back(AllStep.size());
				AllStep.push_back(Result);
			}
		}
		return EbnfHistory{ std::move(AllStep) };
	}

	EbnfHistory Process(EbnfTable const& Tab, std::u32string_view Code)
	{
		auto [Symbols, Datas] = DefaultLexer(Tab.lexical_table, Code);

		try{
			auto Steps = Process(Tab.lr0_table, Symbols.data(), Symbols.size());
			return Translate(Tab, Steps, Datas);
		}
		catch (Exception::LrUnaccableSymbol const& Symbol)
		{
			auto his = Translate(Tab, Symbol.backup_step, Datas);
			auto Str = Tab.FindSymbolString(Symbol.symbol.Index(), Symbol.symbol.IsTerminal());
			if (Str.empty())
			{
				Section loc = (Symbol.index > 0) ? Datas[Symbol.index - 1].section : Section{};
				throw Exception::MakeExceptionTuple(Exception::EbnfUnacceptableSyntax{ U"$_Eof", U"$_Eof", loc, his.Expand() });
			}
			auto loc = Datas[Symbol.symbol.Index()].section;
			throw Exception::MakeExceptionTuple(Exception::EbnfUnacceptableSyntax{ std::u32string(Str), std::u32string(Datas[Symbol.index].capture),Datas[Symbol.index].section, his.Expand() });
		}
	}

	std::any HandleTemplateElement(EbnfNTElement& Element)
	{
		assert(Element.string.empty());
		assert(!Element.IsPredefine());
		switch (Element.mask)
		{
		case *EbnfSequencerType::OR:
		{
			if (Element.production.size() == 1)
			{
				auto Ptr = Element[0].TryConsume<EbnfSequencer>();
				if(Ptr)
					return std::move(*Ptr);
			}
			EbnfSequencer Se{ EbnfSequencerType::OR, {std::move_iterator(Element.production.begin()), std::move_iterator(Element.production.end())}};
			return std::move(Se);
		}
		case * EbnfSequencerType::OPTIONAL:
		{
			EbnfSequencer Se{ EbnfSequencerType::OPTIONAL, {std::move_iterator(Element.production.begin()), std::move_iterator(Element.production.end())} };
			return std::move(Se);
		}
		case * EbnfSequencerType::REPEAT:
		{
			if(Element.production.size() == 0)
				return EbnfSequencer{ EbnfSequencerType::REPEAT, {} };
			else {
				auto Ptr = Element[0].Consume<EbnfSequencer>();
				Ptr.datas.insert(Ptr.datas.end(), std::move_iterator(Element.production.begin() + 1), std::move_iterator(Element.production.end()));
				return std::move(Ptr);
			}
		}
		default:
			assert(false);
			return {};
			break;
		}
	}

	std::any EbnfHistory::operator()(std::function<std::any(EbnfNTElement&)> NTFunc, std::function<std::any(EbnfTElement&)> TFunc) const
	{
		if(NTFunc && TFunc)
		{
			std::vector<EbnfNTElement::Property> Storage;
			for (auto& ite : steps)
			{
				if (ite.IsTerminal())
				{
					EbnfTElement Re(ite);
					auto Result = TFunc(Re);
					Storage.push_back({ ite, std::move(Result) });
				}
				else {
					if (ite.IsNoterminal())
					{
						size_t TotalUsed = ite.reduce.production_count;
						size_t CurrentAdress = Storage.size() - TotalUsed;
						EbnfNTElement Re(ite, Storage.data() + CurrentAdress);
						if (TotalUsed >= 1)
						{
							Re.section = { Re[0].section.start, Re[TotalUsed - 1].section.end };
						}
						else {
							Re.section = ite.section;
						}
						auto Result = Re.string.empty() ? HandleTemplateElement(Re) : NTFunc(Re);
						Storage.resize(CurrentAdress);
						Storage.push_back({ ite, std::move(Result) });
					}
					else if (ite.IsPreDefineNoterminal())
					{
						EbnfNTElement Re(ite, nullptr);
						NTFunc(Re);
					}
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
		for (auto& ite : ts)
		{
			result += DirectFormat(U"", ite);
		}
		return result;
	}

	std::vector<std::u32string> EbnfHistory::Expand() const
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

	LexicalTable CreateLexicalTable(std::map<T, std::u32string_view> const& mapping, PreEbnfInitTuple const* init_tuple, size_t length)
	{
		std::vector<LexicalRegexInitTuple> true_tuple;
		true_tuple.reserve(length);
		for(size_t i =0; i < length; ++i)
		{
			auto& ref = init_tuple[i];
			auto Find = mapping.find(ref.symbol);
			assert(Find != mapping.end());
			true_tuple.push_back({Find->second, ref.ignore ? LexicalDefaultIgnoreMask() : static_cast<size_t>(Find->first)});
		}
		return CreateLexicalFromRegexs(true_tuple.data(), true_tuple.size());
	}

	std::tuple<std::vector<LrSymbol>, std::vector<LexicalMarch>> EbnfLexer(LexicalTable const& table, std::u32string_view input, Section& Loc)
	{
		std::vector<LrSymbol> R1;
		std::vector<LexicalMarch> R2 = table.Process(input);
		R1.reserve(R2.size());
		for(auto& ite : R2)
			R1.push_back(LrSymbol(ite.mask, LrTerminalT{}));
		if(!R2.empty())
		{
			Loc.start = Loc.end;
			Loc.end = Loc.end + R2.rbegin()->section.end;
		}
		return { std::move(R1), std::move(R2) };
	}


	std::u32string_view Sperate(std::u32string_view& input)
	{
		size_t state = 0;
		std::optional<size_t> last_index;
		for (size_t index = 0; index < input.size(); ++index)
		{
			auto cur = input[index];
			switch (state)
			{
			case 0:
				switch (cur)
				{
				case U'%':
					state = 1;
				case U'\f':
				case U'\t':
				case U'\v':
				case U'\r':
					if (!last_index)
						last_index = index;
					break;
				case U'\n':
					last_index = {};
					break;
				default:
					state = 4;
					break;
				}
				break;
			case 1:
			case 2:
				switch (cur)
				{
				case U'%':
					state = state + 1;
					break;
				default:
					state = 4;
					break;
				}
				break;
			case 3:
				switch (cur)
				{
				case U'\f':
				case U'\t':
				case U'\v':
				case U'\r':
					break;
				case U'\n':
				{
					auto P = input.substr(0, *last_index);
					input = input.substr(index + 1);
					return P;
				}
					break;
				default:
					state = 4;
					break;
				}
				break;
			case 4:
				if(cur == U'\n')
					state = 0;
				break;
			default:
				assert(false);
				break;
			}
		}
		auto old = input;
		input = {};
		return old;
	}

	EbnfTable CreateEbnfTable(std::u32string_view code)
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

		static UnfaSerilizedTable sperator = CreateUnfaTableFromRegex(UR"((.*?(?:\r\n|\n))[\f\t\v\r]*?%%%[\s]*?\n|()[\f\t\v\r]*?%%%[\s]*?\n)").Simplify().Serilized();
		auto end_point = CalculateSectionPoint(code);
		struct SperatedCode
		{
			std::u32string_view code;
			Section section;
		};
		SperatedCode sperated_code[3];
		{
			size_t index = 0;
			SectionPoint last_point;
			while (index < 3)
			{
				auto re = Sperate(code);
				if (!re.empty())
				{
					auto current_point = CalculateSectionPoint(re);
					SperatedCode code{re, {last_point, current_point} };
					sperated_code[index] = code;
					++index;
					last_point = current_point;
				}else
					break;
			}
			if(index < 2 || ( index > 3 && !code.empty()))
				throw Exception::MakeExceptionTuple(Exception::EbnfUncompleteEbnf{ index });
			/*
			size_t used = 0;
			SectionPoint Point;
			while (!code.empty() && used < 3)
			{
				auto P = sperator.Mark(code, false);
				
				if (P)
				{
					auto new_point = CalculateSectionPoint(P->sub_capture[0].capture);
					auto cur_end_point = Point + new_point;
					sperated_code[used] = { P->sub_capture[0].capture, {Point, cur_end_point} };
					code = { code.begin() + P->capture.size(), code.end() };
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
				throw Exception::MakeExceptionTuple(Exception::EbnfUncompleteEbnf{ used });
			*/
		}
		
		std::vector<uint32_t> state_to_mask;
		std::map<std::u32string, uint32_t> symbol_to_mask;
		std::vector<std::tuple<std::u32string, uint32_t>> symbol_rex;
		//std::vector<Lexical::LexicalRegexInitTuple> symbol_rex;

		// step1
		{

			static LexicalTable nfa_table = ([]() -> LexicalTable {
				PreEbnfInitTuple RequireList[] = {
					{T::LM_Brace}, {T::RM_Brace}, {T::Number}, {T::Colon}, {T::Terminal},
					{T::Equal}, {T::Rex}, {T::Line}, {T::Command, true}, {T::Empty, true }, {T::Ignore}
				};
				return CreateLexicalTable(Rexs, RequireList, std::size(RequireList));
			}());

			static Lr0Table lr0_instance = CreateLr0Table(
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
				auto History = Process(lr0_instance, Symbols.data(), Symbols.size());
				Process(History, [&](LrNTElement& input) -> std::any {
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
							symbol_rex.push_back({ std::u32string(Rex), LexicalDefaultIgnoreMask() });
							state_to_mask.push_back(LexicalDefaultMask());
						} break;
						case 5: {
							return input[2].Consume<uint32_t>();
						}break;
						case 6: {
							return LexicalDefaultMask();
						} break;
						case 4:
							return false;
						case 3:
							break;
						default: assert(false); break;
					}
					return {};
				},
				[&](LrTElement& input) -> std::any
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
			catch (Exception::LrUnaccableSymbol const& US)
			{
				assert(Elements.size() > US.index);
				auto P = Elements[US.index];
				throw Exception::MakeExceptionTuple(Exception::EbnfUnacceptableToken{ std::u32string(P.capture), P.section});
			}
		}

		std::map<std::u32string, size_t> noterminal_symbol_to_index;
		std::vector<LrProductionInput> productions;
		std::optional<LrSymbol> start_symbol;
		size_t noterminal_temporary = lr_symbol_storage_max - 1;


		struct Token
		{
			LrSymbol sym;
			LexicalMarch march;
		};

		//std::map<lr1::storage_t, std::tuple<std::variant<OrRelationShift, MBraceRelationShift, BBraceRelationShift>, size_t>> temporary_noterminal_production_debug;

		// step2
		{

			static LexicalTable nfa_instance = ([]() -> LexicalTable {
				PreEbnfInitTuple RequireList[] = {
					{T::Or}, {T::StartSymbol}, {T::Colon}, {T::Terminal}, {T::Equal}, {T::Number}, {T::NoTerminal}, {T::Rex}, {T::Line},
					{T::LS_Brace}, {T::RS_Brace}, {T::LM_Brace}, {T::RM_Brace}, {T::LB_Brace}, {T::RB_Brace}, {T::Command, true}, {T::Empty, true}
				};
				return CreateLexicalTable(Rexs, RequireList, std::size(RequireList));
			}());


			static Lr0Table imp = CreateLr0Table(
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

			std::optional<LrSymbol> LastHead;
			size_t LastTemporaryNoTerminal = lr_symbol_storage_min - 1;

			using SymbolList = std::vector<LrSymbol>;

			Section Loc = sperated_code[0].section;
			
			auto [Symbols, Elements] = EbnfLexer(nfa_instance, sperated_code[1].code, Loc);


			try {
				auto History = Process(imp, Symbols.data(), Symbols.size());
				auto Total = Process(History, [&](LrNTElement & tra) -> std::any {
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
								throw Exception::MakeExceptionTuple(Exception::EbnfRedefinedStartSymbol{ P1.march.section });
							return std::vector<LrProductionInput>{};
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
							auto TemSym = LrSymbol(noterminal_temporary--, LrNoTerminalT{});
							auto P1 = tra[0].Consume<SymbolList>();
							auto P2 = tra[2].Consume<SymbolList>();
							P1.insert(P1.begin(), TemSym);
							P2.insert(P2.begin(), TemSym);
							productions.push_back({ std::move(P1), *EbnfSequencerType::OR });
							productions.push_back({ std::move(P2), *EbnfSequencerType::OR });
							return SymbolList{ TemSym };
						} break;
						case 6: {
							auto P1 = tra[0].Consume<Token>();
							LastHead = P1.sym;
							return P1;
						}
						case 7: {
							if (!LastHead)
								throw Exception::MakeExceptionTuple(Exception::EbnfUnsetDefaultProductionHead{});
							return Token{ *LastHead, {} };
						}
						case 8: {
							auto Head = tra[1].Consume<Token>();
							auto Expression = tra[3].Consume<SymbolList>();
							auto [RemoveRe, Enum] = tra[4].Consume<std::tuple<std::set<LrSymbol>, size_t>>();
							Expression.insert(Expression.begin(), Head.sym);
							LrProductionInput re(std::move(Expression), std::move(RemoveRe), Enum);
							productions.push_back(std::move(re));
							return {};
						}break;
						case 19: {
							auto Head = tra[1].Consume<Token>();
							auto [RemoveRe, Enum] = std::move(tra[3].Consume<std::tuple<std::set<LrSymbol>, size_t>>());
							SymbolList Expression({ Head.sym });
							LrProductionInput re(std::move(Expression), std::move(RemoveRe), Enum);
							productions.push_back(std::move(re));
							return {};
						}break;
						case 9: {
							return tra[1].Consume();
						}break;
						case 10: {
							auto TemSym = LrSymbol(noterminal_temporary--, LrNoTerminalT{});
							auto P = tra[1].Consume<SymbolList>();
							P.insert(P.begin(), TemSym);
							productions.push_back({std::move(P), *EbnfSequencerType::OPTIONAL});
							productions.push_back(LrProductionInput({ TemSym }, * EbnfSequencerType::OPTIONAL));
							return SymbolList{ TemSym };
						}break;
						case 11: {
							auto TemSym = LrSymbol(noterminal_temporary--, LrNoTerminalT{});
							auto P = tra[1].Consume<SymbolList>();
							auto List = { TemSym, TemSym };
							P.insert(P.begin(), List.begin(), List.end());
							productions.push_back({std::move(P), *EbnfSequencerType::REPEAT});
							productions.push_back(LrProductionInput({ TemSym }, * EbnfSequencerType::REPEAT));
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
							return size_t(LrProductionInput::default_mask());
						} break;
						case 15: {
							return std::set<LrSymbol>{};
						}break;
						case 16: {
							auto P = tra[0].Consume<std::set<LrSymbol>>();
							auto P2 = tra[1].Consume<Token>();
							P.insert(P2.sym);
							return std::move(P);
						}break;
						case 17: {
							return std::tuple<std::set<LrSymbol>, size_t>{tra[1].Consume<std::set<LrSymbol>>(), tra[2].Consume<size_t>()};
						} break;
						case 18: {
							return std::tuple<std::set<LrSymbol>, size_t>{ std::set<LrSymbol>{}, LrProductionInput::default_mask()};
						} break;
						}
					return {};
				},
				[&](LrTElement& tra) -> std::any
				{
					auto& element = Elements[tra.token_index];
					auto string = std::u32string(element.capture);
					switch (tra.value)
					{
						case* T::Terminal: {
							auto Find = symbol_to_mask.find(string);
							if (Find != symbol_to_mask.end())
								return Token{ LrSymbol(Find->second, LrTerminalT{}), element };
							else
								throw Exception::MakeExceptionTuple(Exception::EbnfUndefinedTerminal{ string, element.section });
						}break;
						case* T::NoTerminal: {
							auto Find = noterminal_symbol_to_index.insert({ string, noterminal_symbol_to_index.size() });
							return Token{ LrSymbol(Find.first->second, LrNoTerminalT{}), element };
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
								state_to_mask.push_back(LexicalDefaultMask());
							}
							return Token{ LrSymbol(re.first->second, LrTerminalT{}), element };
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
			catch (Exception::LrUnaccableSymbol const& US)
			{
				auto P = Elements[US.index];
				throw Exception::MakeExceptionTuple(Exception::EbnfUnacceptableToken{ std::u32string(P.capture), P.section });
			}

		}

		std::vector<LrOpePriority> operator_priority;
		// step3
		{
			static LexicalTable nfa_instance = ([]() -> LexicalTable {
				PreEbnfInitTuple RequireList[] = { {T::Terminal}, {T::Rex}, {T::Command, true},
					{T::LS_Brace}, {T::RS_Brace}, {T::LM_Brace}, {T::RM_Brace}, {T::Empty, true} };
				return CreateLexicalTable(Rexs, RequireList, std::size(RequireList));
			}());

			static Lr0Table lr0_instance = CreateLr0Table(
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
				auto History = Process(lr0_instance, Symbols.data(), Symbols.size());
				Process(History, [&](LrNTElement& step) -> std::any {
					switch (step.mask)
						{
						case 1: {
							std::vector<LrSymbol> List;
							List.push_back(step[0].Consume<Token>().sym);
							return std::move(List);
						}break;
						case 2: {
							auto P1 = step[0].Consume<std::vector<LrSymbol>>();
							auto P2 = step[1].Consume<std::vector<LrSymbol>>();
							P1.insert(P1.end(), P2.begin(), P2.end());
							return std::move(P1);
						} break;
						case 3: {
							auto P = step[1].Consume<Token>().sym;
							operator_priority.push_back({ {P}, LrAssociativity::Left });
							return {};
						} break;
						case 4: {
							auto P = step[2].Consume<std::vector<LrSymbol>>();
							operator_priority.push_back({ std::move(P), LrAssociativity::Left });
							return {};
						} break;
						case 5: {
							auto P = step[2].Consume<std::vector<LrSymbol>>();
							operator_priority.push_back({ std::move(P), LrAssociativity::Right });
							return {};
						} break;
						}
					return {};
				},
				[&](LrTElement& step)->std::any
				{
					if (step.value == *T::Terminal || step.value == *T::Rex)
					{
						auto element = Elements[step.token_index];
						auto Find = symbol_to_mask.find(std::u32string(element.capture));
						if (Find != symbol_to_mask.end())
							return Token{ LrSymbol(Find->second, LrTerminalT{}) };
						else
							throw Exception::MakeExceptionTuple(Exception::EbnfUndefinedTerminal{std::u32string(element.capture), element.section });
					}
					return {};
				}
				);
			}
			catch (Exception::LrUnaccableSymbol const& US)
			{
				auto P = Elements[US.index];
				throw Exception::MakeExceptionTuple(Exception::EbnfUnacceptableToken{ std::u32string(P.capture), P.section });
			}
		}

		if (!start_symbol)
			throw Exception::MakeExceptionTuple(Exception::EbnfMissingStartSymbol{});

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
			std::vector<LexicalRegexInitTuple> Rexs;
			Rexs.reserve(symbol_rex.size());
			for(auto& ite : symbol_rex)
			{
				auto& [str, mask] = ite;
				Rexs.push_back({std::u32string_view(str), mask});
			}
			LexicalTable lexical_table = CreateLexicalFromRegexsReverse(Rexs.data(), Rexs.size());
			Lr0Table Lr0Table = CreateLr0Table(
				*start_symbol, productions, std::move(operator_priority)
			);
			return { std::move(table),
				std::move(state_to_mask),
				std::move(lexical_table),
				std::move(symbol_map), TerminalCount, std::move(Lr0Table) };
		}
		catch (Exception::UnfaUnaccaptableRexgex const& ref)
		{
			throw Exception::MakeExceptionTuple(Exception::EbnfUnacceptableRegex{ref.regex, ref.accepetable_mask});
		}
		catch (Exception::LrNoterminalUndefined const NU)
		{
			for(auto& ite : noterminal_symbol_to_index)
				if(ite.second == NU.value.Index())
					throw Exception::MakeExceptionTuple(Exception::EbnfUndefinedNoterminal{std::u32string(ite.first)});
			throw Exception::MakeExceptionTuple(Exception::EbnfUndefinedNoterminal{ std::u32string(U"$UnknowSymbol") });
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