module;

#include <cassert>

module Potato.Reg;
import Potato.Encode;

namespace Potato::Reg
{

	IntervalT const& MaxInterval() {
		static IntervalT Temp{{ 1, MaxChar() }};
		return Temp;
	};

	RegLexerT::RegLexerT(bool IsRaw) : 
		CurrentState(IsRaw ? StateT::Raw : StateT::Normal) {}

	bool RegLexerT::Consume(char32_t InputSymbol, Misc::IndexSpan<> TokenIndex)
	{
		if (InputSymbol < MaxChar())
		{
			switch (CurrentState)
			{
			case StateT::Normal:
			{
				switch (InputSymbol)
				{
				case U'-':
					StoragedSymbol.push_back({ ElementEnumT::Min, InputSymbol, TokenIndex });
					break;
				case U'[':
					StoragedSymbol.push_back({ ElementEnumT::BracketsLeft, InputSymbol, TokenIndex });
					break;
				case U']':
					StoragedSymbol.push_back({ ElementEnumT::BracketsRight, InputSymbol, TokenIndex });
					break;
				case U'{':
					StoragedSymbol.push_back({ ElementEnumT::CurlyBracketsLeft, InputSymbol, TokenIndex });
					break;
				case U'}':
					StoragedSymbol.push_back({ ElementEnumT::CurlyBracketsRight, InputSymbol, TokenIndex });
					break;
				case U',':
					StoragedSymbol.push_back({ ElementEnumT::Comma, InputSymbol, TokenIndex });
					break;
				case U'(':
					StoragedSymbol.push_back({ ElementEnumT::ParenthesesLeft, InputSymbol, TokenIndex });
					break;
				case U')':
					StoragedSymbol.push_back({ ElementEnumT::ParenthesesRight, InputSymbol, TokenIndex });
					break;
				case U'*':
					StoragedSymbol.push_back({ ElementEnumT::Mulity, InputSymbol, TokenIndex });
					break;
				case U'?':
					StoragedSymbol.push_back({ ElementEnumT::Question, InputSymbol, TokenIndex });
					break;
				case U'.':
					StoragedSymbol.push_back({ ElementEnumT::CharSet, MaxInterval(), TokenIndex });
					break;
				case U'|':
					StoragedSymbol.push_back({ ElementEnumT::Or, InputSymbol, TokenIndex });
					break;
				case U'+':
					StoragedSymbol.push_back({ ElementEnumT::Add, InputSymbol, TokenIndex });
					break;
				case U'^':
					StoragedSymbol.push_back({ ElementEnumT::Not, InputSymbol, TokenIndex });
					break;
				case U':':
					StoragedSymbol.push_back({ ElementEnumT::Colon, InputSymbol, TokenIndex });
					break;
				case U'\\':
					CurrentState = StateT::Transfer;
					RecordSymbol = InputSymbol;
					break;
				default:
					if (InputSymbol >= U'0' && InputSymbol <= U'9')
						StoragedSymbol.push_back({ ElementEnumT::Num, InputSymbol, TokenIndex });
					else
						StoragedSymbol.push_back({ ElementEnumT::SingleChar, InputSymbol, TokenIndex });
					break;
				}
				break;
			}
			case StateT::Transfer:
			{
				switch (InputSymbol)
				{
				case U'd':
					StoragedSymbol.push_back({ ElementEnumT::CharSet, {{U'0', U'9' + 1}}, TokenIndex });
					CurrentState = StateT::Normal;
					break;
				case U'D':
				{
					IntervalT Tem{ {{1, U'0'}, {U'9' + 1, MaxChar()} } };
					StoragedSymbol.push_back({ ElementEnumT::CharSet, std::move(Tem), TokenIndex });
					CurrentState = StateT::Normal;
					break;
				}
				case U'f':
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, U'\f', TokenIndex });
					CurrentState = StateT::Normal;
					break;
				case U'n':
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, U'\n', TokenIndex });
					CurrentState = StateT::Normal;
					break;
				case U'r':
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, U'\r', TokenIndex });
					CurrentState = StateT::Normal;
					break;
				case U't':
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, U'\t', TokenIndex });
					CurrentState = StateT::Normal;
					break;
				case U'v':
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, U'\v', TokenIndex });
					CurrentState = StateT::Normal;
					break;
				case U's':
				{
					IntervalT tem({{ 1, 33 },{127, 128} });
					StoragedSymbol.push_back({ ElementEnumT::CharSet, std::move(tem), TokenIndex });
					CurrentState = StateT::Normal;
					break;
				}
				case U'S':
				{
					IntervalT tem({{33, 127}, {128, MaxChar()} });
					StoragedSymbol.push_back({ ElementEnumT::CharSet, std::move(tem), TokenIndex });
					CurrentState = StateT::Normal;
					break;
				}
				case U'w':
				{
					IntervalT tem({{ U'A', U'Z' + 1 },{ U'_'}, { U'a', U'z' + 1 }});
					StoragedSymbol.push_back({ ElementEnumT::CharSet, std::move(tem), TokenIndex });
					CurrentState = StateT::Normal;
					break;
				}
				case U'W':
				{
					IntervalT tem({{ U'A', U'Z' + 1 },{ U'_'}, { U'a', U'z' + 1 } });
					StoragedSymbol.push_back({ ElementEnumT::CharSet, MaxInterval() - tem, TokenIndex});
					CurrentState = StateT::Normal;
					break;
				}
				case U'z':
				{
					IntervalT tem({{ 256, MaxChar() }});
					StoragedSymbol.push_back({ ElementEnumT::CharSet, std::move(tem), TokenIndex });
					CurrentState = StateT::Normal;
					break;
				}
				case U'u':
				{
					CurrentState = StateT::Number;
					NumberChar = 0;
					Number = 0;
					NumberIsBig = false;
					break;
				}
				case U'U':
				{
					CurrentState = StateT::Number;
					NumberChar = 0;
					Number = 0;
					NumberIsBig = true;
					break;
				}
				default:
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, InputSymbol, TokenIndex });
					CurrentState = StateT::Normal;
					break;
				}
				break;
			}
			case StateT::Number:
			{
				Number += 1;
				if (InputSymbol >= U'0' && InputSymbol <= U'9')
				{
					NumberChar *= 16;
					NumberChar += InputSymbol - U'0';
				}
				else if (InputSymbol >= U'a' && InputSymbol <= U'f')
				{
					NumberChar *= 16;
					NumberChar += InputSymbol - U'a' + 10;
				}
				else if (InputSymbol >= U'A' && InputSymbol <= U'F')
				{
					NumberChar *= 16;
					NumberChar += InputSymbol - U'A' + 10;
				}
				else {
					return false;
				}
				if ((Number == 4 && !NumberIsBig) || (NumberIsBig && Number == 6))
				{
					StoragedSymbol.push_back({ ElementEnumT::SingleChar, NumberChar, TokenIndex });
					CurrentState = StateT::Normal;
				}
				break;
			}
			case StateT::Raw:
				StoragedSymbol.push_back({ ElementEnumT::SingleChar, InputSymbol, TokenIndex });
				break;
			default:
				assert(false);
			}
			return true;
		}
		else {
			return false;
		}
	}

	bool RegLexerT::EndOfFile()
	{
		if (CurrentState == StateT::Normal || CurrentState == StateT::Raw)
		{
			CurrentState = StateT::Done;
			return true;
		}
		else
			return false;
	}

	
	using namespace Exception;

	using SLRX::Symbol;

	using T = RegLexerT::ElementEnumT;

	constexpr Symbol operator*(T Input) { return Symbol::AsTerminal(static_cast<SLRX::StandardT>(Input)); };

	enum class NT : StandardT
	{
		AcceptableSingleChar,
		CharListAcceptableSingleChar,
		Statement,
		CharList,
		Expression,
		ExpressionStatement,
		CharListSingleChar,
		Number,
		FinalCharList,
	};

	constexpr Symbol operator*(NT Input) { return Symbol::AsNoTerminal(static_cast<SLRX::StandardT>(Input)); };


	const SLRX::TableWrapper RexSLRXWrapper()
	{
#ifdef _DEBUG
		try {
#endif
			static SLRX::Table Table(
				*NT::ExpressionStatement,
				{

					{*NT::AcceptableSingleChar, {*T::SingleChar}, 40},
					{*NT::AcceptableSingleChar, {*T::Num}, 40},
					{*NT::AcceptableSingleChar, {*T::Min}, 40},
					{*NT::AcceptableSingleChar, {*T::Comma}, 40},
					{*NT::AcceptableSingleChar, {*T::Colon}, 40},
					{*NT::CharListSingleChar, {*T::SingleChar}, 40},
					{*NT::CharListSingleChar, {*T::Num}, 40},
					{*NT::CharListSingleChar, {*T::Comma}, 40},
					{*NT::CharListSingleChar, {*T::Colon}, 40},
					{*NT::CharList, {*NT::CharListSingleChar}, 41},
					{*NT::CharList, {*NT::CharListSingleChar, *T::Min, *NT::CharListSingleChar}, 42},
					{*NT::CharList, {*T::CharSet}, 1},
					{*NT::CharList, {*NT::CharList, *NT::CharList, SLRX::ItSelf{}}, 3},

					{*NT::FinalCharList, {*T::Min}, 60},
					{*NT::FinalCharList, {*T::Min, *NT::CharList}, 61},
					{*NT::FinalCharList, {*NT::CharList, *T::Min}, 62},
					{*NT::FinalCharList, {*NT::CharList}, 63},

					{*NT::Expression, {*T::BracketsLeft, *NT::FinalCharList, *T::BracketsRight}, 4},
					{*NT::Expression, {*T::BracketsLeft, *T::Not, *NT::FinalCharList, *T::BracketsRight}, 5},
					{*NT::Expression, {*T::ParenthesesLeft, *T::Question, *T::Colon, *NT::ExpressionStatement, *T::ParenthesesRight}, 6},
					{*NT::Expression, {*T::ParenthesesLeft, *NT::ExpressionStatement, *T::ParenthesesRight}, 7 },
					{*NT::ExpressionStatement, {*NT::ExpressionStatement,  *T::Or, *NT::ExpressionStatement, 8}, 8},
					{*NT::Expression, {*NT::AcceptableSingleChar}, 9},
					{*NT::Expression, {*T::CharSet}, 50},
					{*NT::ExpressionStatement, {*NT::Expression}, 10},
					{*NT::ExpressionStatement, {*NT::ExpressionStatement, 8, *NT::Expression}, 11},
					{*NT::Expression, {*NT::Expression, *T::Mulity}, 12},
					{*NT::Expression, {*NT::Expression, *T::Add}, 13},
					{*NT::Expression, {*NT::Expression, *T::Mulity, *T::Question}, 14},
					{*NT::Expression, {*NT::Expression, *T::Add, *T::Question}, 15},
					{*NT::Expression, {*NT::Expression, 12, 13, 20, 21, 22, 23, SLRX::ItSelf{}, *T::Question}, 16},
					{*NT::Expression, {*NT::Expression, 12, 13, 14, 15, 16, 20, 21, 22, 23, *T::Question, *T::Question}, 17},
					{*NT::Number, {*T::Num}, 18},
					{*NT::Number, {*NT::Number, *T::Num}, 19},
					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::CurlyBracketsRight}, 20},
					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *T::Comma, *NT::Number, *T::CurlyBracketsRight}, 21},
					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *T::CurlyBracketsRight}, 22},
					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *NT::Number, *T::CurlyBracketsRight}, 23},

					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *T::Comma, *NT::Number, *T::CurlyBracketsRight, *T::Question}, 25},
					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *T::CurlyBracketsRight, *T::Question}, 26},
					{*NT::Expression, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *NT::Number, *T::CurlyBracketsRight, *T::Question}, 27},
				}, {}
			);
			return Table.Wrapper;
#ifdef _DEBUG
		}
		catch (SLRX::Exception::IllegalSLRXProduction const& Pro)
		{
			volatile int i = 0;

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
					Symbols.push_back(NTer{ static_cast<NT>(Ite.Value.Value), Ite.Reduce.ElementCount, Ite.Reduce.Mask });
				}
			}

			for (auto& Ite : Pro.Steps2)
			{
				if (Ite.IsTerminal())
				{
					Symbols2.push_back(Ter{ static_cast<T>(Ite.Value.Value) });
				}
				else {
					Symbols2.push_back(NTer{ static_cast<NT>(Ite.Value.Value), Ite.Reduce.ElementCount, Ite.Reduce.Mask });
				}
			}


			throw;
		}
#endif

	}

	struct BadRegexRef
	{
		std::size_t Index;
	};

	bool NfaT::EdgeT::HasCapture() const
	{
		for (auto& Ite : Propertys)
		{
			if (Ite.Type == EdgePropertyE::CaptureBegin || Ite.Type == EdgePropertyE::CaptureEnd)
			{
				return true;
			}
		}
		return false;
	}
	bool NfaT::EdgeT::HasCounter() const
	{
		for (auto& Ite : Propertys)
		{
			if (
				Ite.Type == EdgePropertyE::OneCounter
				|| Ite.Type == EdgePropertyE::AddCounter
				|| Ite.Type == EdgePropertyE::LessCounter
				|| Ite.Type == EdgePropertyE::BiggerCounter
			)
			{
				return true;
			}
		}
		return false;

	}


	NfaT NfaT::Create(std::u32string_view Str, bool IsRaw, std::size_t Mask)
	{
		RegLexerT Lex(IsRaw);

		std::size_t Index = 0;
		for (auto& Ite : Str)
		{
			if (!Lex.Consume(Ite, { Index, Index + 1 }))
				throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {Index, Str.size()} };
			++Index;
		}
		if(!Lex.EndOfFile())
			throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {Index, Str.size()} };
		try {
			return NfaT{ Lex.GetSpan(), Mask };
		}
		catch (BadRegexRef EIndex)
		{
			throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {EIndex.Index , Str.size()} };
		}
		catch (UnaccaptableRegexTokenIndex const& EIndex)
		{
			throw Exception::UnaccaptableRegex{ EIndex.Type, Str, EIndex.BadIndex };
		}
	}

	std::size_t NfaT::AddNode()
	{
		NodeT Node;
		auto OldSize = Nodes.size();
		Node.CurIndex = OldSize;
		Nodes.push_back(std::move(Node));
		return OldSize;
	}

	Misc::IndexSpan<> Translate(Misc::IndexSpan<> TokenIndex, std::span<RegLexerT::ElementT const> Tokens)
	{
		Misc::IndexSpan<> Result;
		for (auto Index = TokenIndex.Begin() + 1; Index < TokenIndex.End(); ++Index)
		{
			if(Result.End() == 0)
				Result = Tokens[Index].Token;
			else
				Result = Result.Expand(Tokens[Index].Token);
		}
		return Result;
	}

	void NfaT::AddConsume(NodeSetT Set, IntervalT Chars, ContentT Content)
	{
		EdgeT Edge;
		Edge.ToNode = Set.Out;
		Edge.CharSets = std::move(Chars);
		Edge.TokenIndex = Translate(Content.TokenIndex, Content.Tokens);
		Nodes[Set.In].Edges.push_back(std::move(Edge));
	}

	Misc::IndexSpan<> NfaT::CollectTokenIndexFromNodePath(std::span<NodeT const> NodeView, Misc::IndexSpan<> Default, std::span<std::size_t const> NodeStateView)
	{
		for (auto Ite1 = NodeStateView.begin(); Ite1 != NodeStateView.end(); ++Ite1)
		{
			auto Next = Ite1 + 1;
			if (Next != NodeStateView.end())
			{
				auto& Edges = NodeView[*Ite1].Edges;
				for (auto& Ite : Edges)
				{
					if (Ite.ToNode == *Next)
					{
						Default = Default.Expand(Ite.TokenIndex);
						break;
					}
				}
			}
		}
		return Default;
	}
	
	NfaT::NfaT(std::span<RegLexerT::ElementT const> InputSpan, std::size_t Mask)
	{
		SLRX::SymbolProcessor SymPro(RexSLRXWrapper());
		auto IteSpan = InputSpan;
		while (!IteSpan.empty())
		{
			auto Top = *IteSpan.begin();
			if (!SymPro.Consume(*Top.Value, InputSpan.size() - IteSpan.size()))
				throw BadRegexRef{ InputSpan.size() - IteSpan.size() };
			IteSpan = IteSpan.subspan(1);
		}
		if (!SymPro.EndOfFile())
		{
			throw BadRegexRef{ InputSpan.size() - IteSpan.size() };
		}

		auto StepSpan = SymPro.GetSteps();

		auto Top = AddNode();

		std::size_t CaptureCount = 0;
		std::size_t CounterCount = 0;

		auto Re = ProcessParsingStepWithOutputType<NodeSetT>(StepSpan, [&, this](SLRX::VariantElement Var) -> std::any
		{
			if (Var.IsTerminal())
			{
				auto Ele = Var.AsTerminal();
				if (Ele.Value == *T::ParenthesesLeft)
				{
					return CaptureCount++;
				}
				else if (Ele.Value == *T::CurlyBracketsLeft)
				{
					return CounterCount++;
				}
				return InputSpan[Ele.Shift.TokenIndex].Chars;
			}
			else
			{

				assert(Var.IsNoTerminal());
				auto NT = Var.AsNoTerminal();

				ContentT Content{NT.TokenIndex, InputSpan };

				switch (NT.Reduce.Mask)
				{
				case 40:
					return NT[0].Consume();
				case 60:
				case 41:
				{
					return NT[0].Consume<IntervalT>();
				}
				case 42:
				{
					auto Cur = NT[0].Consume<IntervalT>();
					auto Cur2 = NT[2].Consume<IntervalT>();
					return IntervalT{ Cur[0].Expand(Cur2[0])};
				}
				case 1:
					return NT[0].Consume();
				case 3:
				{
					auto T1 = NT[0].Consume<IntervalT>();
					auto T2 = NT[1].Consume<IntervalT>();
					return T1 + T2;
				}
				case 61:
				{
					auto T1 = NT[0].Consume<IntervalT>();
					auto T2 = NT[1].Consume<IntervalT>();
					return T1 + T2;
				}
				case 62:
				{
					auto T1 = NT[1].Consume<IntervalT>();
					auto T2 = NT[0].Consume<IntervalT>();
					return T1 + T2;
				}
				case 63:
				{
					return NT[0].Consume();
				}
				case 4:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					NodeSetT Set{ T1, T2 };
					AddConsume(Set, NT[1].Consume<IntervalT>(), Content);
					return Set;
				}
				case 5:
				{
					auto Tar = NT[2].Consume<IntervalT>();
					auto P = MaxInterval() - Tar;
					auto T1 = AddNode();
					auto T2 = AddNode();
					NodeSetT Set{ T1, T2 };
					AddConsume(Set, P, Content);
					return Set;
				}
				case 6:
				{
					return NT[3].Consume();
				}
				case 7:
				{
					auto InDex = NT.Datas[0].Consume<std::size_t>();
					NodeSetT Last = NT.Datas[1].Consume<NodeSetT>();
					return AddCapture(Last, Content, InDex);
				}
				case 8:
				{
					NodeSetT Last1 = NT[0].Consume<NodeSetT>();
					NodeSetT Last2 = NT[2].Consume<NodeSetT>();
					auto T1 = AddNode();
					auto T2 = AddNode();
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({T1, Last2.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					AddConsume({Last2.Out, T2}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 9:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Tar = NT[0].Consume<IntervalT>();
					NodeSetT Set{ T1, T2 };
					AddConsume({T1, T2}, std::move(Tar), Content);
					return Set;
				}
				case 50:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					NodeSetT Set{ T1, T2 };
					AddConsume(Set, NT[0].Consume<IntervalT>(), Content);
					return Set;
				}
				case 10:
				{
					return NT[0].Consume();
				}
				case 11:
				{
					auto Last1 = NT[0].Consume<NodeSetT>();
					auto Last2 = NT[1].Consume<NodeSetT>();
					NodeSetT Set{ Last1.Out, Last2.In };
					AddConsume(Set, {}, Content);
					return NodeSetT{Last1.In, Last2.Out};
				}
				case 12:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Last1 = NT[0].Consume<NodeSetT>();
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({T1, T2}, {}, Content);
					AddConsume({Last1.Out, Last1.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 13:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Last1 = NT[0].Consume<NodeSetT>();
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({Last1.Out, Last1.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 14:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Last1 = NT[0].Consume<NodeSetT>();
					AddConsume({T1, T2}, {}, Content);
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					AddConsume({Last1.Out, Last1.In}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 15:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Last1 = NT[0].Consume<NodeSetT>();
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					AddConsume({Last1.Out, Last1.In}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 16:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Last1 = NT[0].Consume<NodeSetT>();
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					AddConsume({T1, T2}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 17:
				{
					auto T1 = AddNode();
					auto T2 = AddNode();
					auto Last1 = NT[0].Consume<NodeSetT>();
					AddConsume({T1, T2}, {}, Content);
					AddConsume({T1, Last1.In}, {}, Content);
					AddConsume({Last1.Out, T2}, {}, Content);
					return NodeSetT{ T1, T2 };
				}
				case 18:
				{
					char32_t Te = NT[0].Consume<IntervalT>()[0].Start;
					std::size_t Count = Te - U'0';
					return Count;
				}
				case 19:
				{
					auto Te = NT[0].Consume<std::size_t>();
					Te *= 10;
					auto Te2 = NT[1].Consume<IntervalT>()[0].Start;
					Te += Te2 - U'0';
					return Te;
				}
				case 20: // {num}
					return AddCounter(NT[0].Consume<NodeSetT>(), NT[2].Consume<std::size_t>(), {}, true, Content, NT[0].Consume<std::size_t>());
				case 25: // {,N}?
					return AddCounter(NT[0].Consume<NodeSetT>(), {}, NT[3].Consume<std::size_t>(), false, Content, NT[0].Consume<std::size_t>());
				case 21: // {,N}
					return AddCounter(NT[0].Consume<NodeSetT>(), {}, NT[3].Consume<std::size_t>(), true, Content, NT[0].Consume<std::size_t>());
				case 26: // {N,} ?
					return AddCounter(NT[0].Consume<NodeSetT>(), NT[2].Consume<std::size_t>(), {}, false, Content, NT[0].Consume<std::size_t>());
				case 22: // {N,}
					return AddCounter(NT[0].Consume<NodeSetT>(), NT[2].Consume<std::size_t>(), {}, true, Content, NT[0].Consume<std::size_t>());
				case 27: // {N, N} ?
					return AddCounter(NT[0].Consume<NodeSetT>(), NT[2].Consume<std::size_t>(), NT[4].Consume<std::size_t>(), false, Content, NT[1].Consume<std::size_t>());
				case 23: // {N, N}
					return AddCounter(NT[0].Consume<NodeSetT>(), NT[2].Consume<std::size_t>(), NT[4].Consume<std::size_t>(), true, Content, NT[1].Consume<std::size_t>());
				default:
					assert(false);
					break;
				}
				return {};
			}
		});

		{
			EdgeT Tem;
			Tem.ToNode = Re.In;
			Nodes[Top].Edges.push_back(Tem);
		}

		{
			auto Last = AddNode();
			AddConsume({Re.Out, Last}, EndOfFile(), {});
			Nodes[Last].Accept = AcceptT{Mask, 0};
		}

		auto TargetNode = std::move(Nodes);
		Nodes.clear();

		{
			std::map<std::size_t, std::size_t> NodeMapping;

			std::vector<decltype(NodeMapping)::const_iterator> SearchingStack;

			{
				auto CT = AddNode();
				auto [Ite, B] = NodeMapping.insert({ 0, CT });
				SearchingStack.push_back(Ite);
			}

			std::vector<std::size_t> StateStack;
			std::vector<PropertyT> Pros;

			struct StackRecord
			{
				std::size_t StateSize;
				std::size_t PropertySize;
				std::size_t EdgeSize;
				std::optional<Misc::IndexSpan<>> TokenIndex;
			};

			std::vector<StackRecord> RecordStack;
			

			while (!SearchingStack.empty())
			{
				auto Top = *SearchingStack.rbegin();
				SearchingStack.pop_back();
				
				StateStack.clear();
				Pros.clear();
				RecordStack.clear();
				StateStack.push_back(Top->first);
				std::optional<Misc::IndexSpan<>> LastRecordTokenIndex;
				RecordStack.push_back({ 1, 0, 0, LastRecordTokenIndex });
				
				while (!RecordStack.empty())
				{
					auto TopRecord = *RecordStack.rbegin();
					RecordStack.pop_back();
					StateStack.resize(TopRecord.StateSize);
					Pros.resize(TopRecord.PropertySize);
					LastRecordTokenIndex = TopRecord.TokenIndex;
					auto& CurNode = TargetNode[*StateStack.rbegin()];
					if (TopRecord.EdgeSize < CurNode.Edges.size())
					{

						auto& CurEdge = CurNode.Edges[TopRecord.EdgeSize];
						RecordStack.push_back({ TopRecord.StateSize, TopRecord.PropertySize, TopRecord.EdgeSize + 1, LastRecordTokenIndex });
						
						if (LastRecordTokenIndex.has_value())
						{
							LastRecordTokenIndex = LastRecordTokenIndex->Expand(CurEdge.TokenIndex);
						}
						else {
							LastRecordTokenIndex = CurEdge.TokenIndex;
						}
						
						Pros.insert(Pros.end(), CurEdge.Propertys.begin(), CurEdge.Propertys.end());
						if (CurEdge.IsEpsilonEdge())
						{
							auto SameStateStackIte = std::find(StateStack.begin(), StateStack.end(), CurEdge.ToNode);
							if (SameStateStackIte != StateStack.end())
							{
								StateStack.push_back(CurEdge.ToNode);
								auto ErrorTokenIndex = CollectTokenIndexFromNodePath(
									std::span(TargetNode),
									CurEdge.TokenIndex,
									std::span(StateStack).subspan(std::distance(StateStack.begin(), SameStateStackIte))
								);
								throw UnaccaptableRegexTokenIndex{
									UnaccaptableRegexTokenIndex::TypeT::BadRegex,
									ErrorTokenIndex };
							}
							StateStack.push_back(CurEdge.ToNode);

							RecordStack.push_back({
								StateStack.size(),
								Pros.size(),
								0,
								LastRecordTokenIndex
								});
						}
						else {
							StateStack.push_back(CurEdge.ToNode);
							std::size_t FinnalToNode = 0;
							auto [MIte, B] = NodeMapping.insert({ CurEdge.ToNode, FinnalToNode });
							auto Sets = CurEdge.CharSets;
							if (B)
							{
								FinnalToNode = AddNode();

								{
									auto& RefToNode = TargetNode[CurEdge.ToNode];
									if (RefToNode.Accept.has_value())
									{
										Nodes[FinnalToNode].Accept = RefToNode.Accept;
									}
								}

								MIte->second = FinnalToNode;
								//if(Sets.Size() != 0)
								SearchingStack.push_back(MIte);
							}
							else {
								FinnalToNode = MIte->second;
							}

							auto& CurEdges = Nodes[Top->second].Edges;

							for (auto& Ite : CurEdges)
							{
								if (Ite.ToNode == FinnalToNode)
								{
									throw 
										UnaccaptableRegexTokenIndex{
											UnaccaptableRegexTokenIndex::TypeT::BadRegex,
											CurEdge.TokenIndex 
									};
								}
							}

							Nodes[Top->second].Edges.push_back({
									Pros,
									FinnalToNode,
									std::move(Sets),
									(LastRecordTokenIndex.has_value() ? *LastRecordTokenIndex : Misc::IndexSpan<>{0, 0}),
									CurEdge.MaskIndex
								}
							);
						}
					}
				}
			}

			bool Change = true;
			while (Change)
			{
				Change = false;
				for (std::size_t I1 = 0; I1 < Nodes.size(); ++I1)
				{
					for (std::size_t I2 = I1 + 1; I2 < Nodes.size(); )
					{
						auto& N1 = Nodes[I1];
						auto& N2 = Nodes[I2];
						if (N1 == N2)
						{
							for (auto& Ite3 : Nodes)
							{
								if (Ite3.CurIndex > N2.CurIndex)
									--Ite3.CurIndex;
								for (auto& Ite4 : Ite3.Edges)
								{
									if (Ite4.ToNode == N2.CurIndex)
										Ite4.ToNode = N1.CurIndex;
									else if (Ite4.ToNode > N2.CurIndex)
										--Ite4.ToNode;
								}
							}
							Nodes.erase(Nodes.begin() + I2);
							Change = true;
						}
						else {
							++I2;
						}
					}
				}
			}
		}
	}

	auto NfaT::AddCapture(NodeSetT Inside, ContentT Content, std::size_t CaptureIndex) -> NodeSetT
	{
		auto T1 = AddNode();
		auto T2 = AddNode();

		auto Tk = Translate(Content.TokenIndex, Content.Tokens);

		EdgeT Ege;
		Ege.Propertys.push_back(
			{ EdgePropertyE::CaptureBegin, CaptureIndex, 0}
		);
		Ege.ToNode = Inside.In;
		Ege.TokenIndex = Tk;

		Nodes[T1].Edges.push_back(Ege);

		EdgeT Ege2;

		Ege2.Propertys.push_back(
			{ EdgePropertyE::CaptureEnd, CaptureIndex, 0 }
		);
		Ege2.ToNode = T2;
		Ege2.TokenIndex = Tk;

		Nodes[Inside.Out].Edges.push_back(Ege2);

		return {T1, T2};
	}

	auto NfaT::AddCounter(NodeSetT Inside, std::optional<std::size_t> Min, std::optional<std::size_t> Max, bool Greedy, ContentT Content, std::size_t CounterIndex) -> NodeSetT
	{
		auto Tk = Translate(Content.TokenIndex, Content.Tokens);
		assert(static_cast<bool>(Min) || static_cast<bool>(Max));
		auto T1 = AddNode();
		auto T2 = AddNode();

		std::size_t IMin = 0;

		if (Min.has_value())
		{
			IMin = *Min;
		}

		std::size_t IMax = std::numeric_limits<std::size_t>::max();

		if (Max.has_value())
		{
			IMax = *Max;
		}

		{
			auto& Ref = Nodes[T1];
			EdgeT Ege;
			Ege.Propertys.push_back(
				{ EdgePropertyE::OneCounter, CounterIndex, 0 }
			);
			Ege.ToNode = Inside.In;
			Ege.TokenIndex = Tk;
			Ref.Edges.push_back(std::move(Ege));

			if (IMin == 0)
			{
				EdgeT Ege2;
				Ege2.ToNode = T2;
				Ege2.TokenIndex = Tk;
				Ref.Edges.push_back(std::move(Ege2));
			}

			if (IMin == 0 && !Greedy)
				std::swap(Ref.Edges[0], Ref.Edges[1]);
		}

		{
			EdgeT Ege;
			Ege.ToNode = Inside.In;
			Ege.TokenIndex = Tk;
			if (IMax != 0)
			{
				Ege.Propertys.push_back(
					{ EdgePropertyE::LessCounter, CounterIndex, IMax - 1 }
				);
			}
			Ege.Propertys.push_back(
				{ EdgePropertyE::AddCounter, CounterIndex, 0 }
			);
			EdgeT Ege2;
			Ege2.ToNode = T2;
			Ege2.TokenIndex = Tk;
			if (IMin != 0)
			{
				Ege2.Propertys.push_back(
					{ EdgePropertyE::BiggerCounter, CounterIndex, IMin }
				);
			}
			
			if (IMax != 0)
			{
				Ege2.Propertys.push_back(
					{ EdgePropertyE::LessCounter, CounterIndex, IMax }
				);
			}

			auto& Ref = Nodes[Inside.Out];
			if (Greedy)
			{
				Ref.Edges.push_back(std::move(Ege));
				Ref.Edges.push_back(std::move(Ege2));
			}
			else {
				Ref.Edges.push_back(std::move(Ege2));
				Ref.Edges.push_back(std::move(Ege));
			}
			
		}
		return {T1, T2};
	}

	void NfaT::Link(NfaT const& Input)
	{
		assert(Input.Nodes.size() >= 1);
		Nodes.reserve(Nodes.size() + Input.Nodes.size() - 1);
		auto Last = Nodes.size();
		Nodes.insert(Nodes.end(), Input.Nodes.begin() + 1, Input.Nodes.end());
		auto Span = std::span(Nodes).subspan(Last);
		auto IndexOffset = Last - 1;
		for (auto& Ite : Span)
		{
			Ite.CurIndex += IndexOffset;
			for (auto& Ite2 : Ite.Edges)
			{
				Ite2.MaskIndex += MaskIndex;
				if(Ite2.ToNode != 0)
					Ite2.ToNode += IndexOffset;
			}
			if (Ite.Accept.has_value())
				Ite.Accept->MaskIndex += MaskIndex;
		}

		auto& CTopNodeEdge = Nodes[0].Edges;
		auto& TTopNodeEdge = Input.Nodes[0].Edges;

		auto OldEdgeSize = CTopNodeEdge.size();
		CTopNodeEdge.insert(CTopNodeEdge.end(), TTopNodeEdge.begin(), TTopNodeEdge.end());

		for (auto& Ite : std::span(CTopNodeEdge).subspan(OldEdgeSize))
		{
			Ite.MaskIndex += MaskIndex;
			if (Ite.ToNode != 0)
				Ite.ToNode += IndexOffset;
		}

		MaskIndex += Input.MaskIndex;
	}


	void MartixStateT::ResetRowCount(std::size_t RowCount)
	{
		States.clear();
		assert(RowCount >= 1);
		this->RowCount = RowCount;
		LineCount = 1;
		States.insert(States.end(), RowCount, StateE::UnSet);
	}

	MartixStateT::StateE& MartixStateT::Get(std::size_t IRowIndex, std::size_t ILineIndex) {
		assert(IRowIndex < RowCount && ILineIndex < LineCount);
		return States[IRowIndex + ILineIndex * RowCount];
	}

	std::size_t MartixStateT::CopyLine(std::size_t SourceLine)
	{
		assert(SourceLine < LineCount);
		auto OldLineCount = LineCount;
		States.insert(States.end(), RowCount, StateE::UnSet);
		LineCount += 1;
		for (std::size_t I = 0; I < RowCount; ++I)
		{
			Get(I, OldLineCount) = Get(I, SourceLine);
		}
		return OldLineCount;
	}

	bool MartixStateT::RemoveAllFalseLine()
	{
		bool Removed = false;
		for (std::size_t Line = 0; Line < LineCount; )
		{
			bool HasTrue = false;
			for (std::size_t Row = 0; Row < RowCount; ++Row)
			{
				if (Get(Row, Line) == StateE::True)
				{
					HasTrue = true;
					break;
				}
			}
			if (!HasTrue)
			{
				States.erase(
					States.begin() + Line * RowCount,
					States.begin() + (Line + 1) * RowCount
				);
				--LineCount;
				Removed = true;
			}
			else {
				++Line;
			}
		}
		return Removed;
	}

	std::span<MartixStateT::StateE const> MartixStateT::ReadLine(std::size_t LineIndex) const
	{
		assert(LineIndex < LineCount);
		return std::span(States).subspan(
			LineIndex * RowCount,
			RowCount
		);
	}

	struct ActionIndexT
	{
		enum class CategoryE
		{
			CaptureBegin,
			CaptureEnd,
			Counter,
		};

		CategoryE Category = CategoryE::Counter;
		std::size_t Index = 0;
		std::size_t MaskIndex = 0;

		bool operator==(ActionIndexT const& T1) const = default;
		bool operator<(ActionIndexT const& T1) const {
			if (Category != T1.Category)
			{
				if (Category == CategoryE::Counter)
					return false;
				else if (T1.Category == CategoryE::Counter)
					return true;
			}

			if (MaskIndex < T1.MaskIndex)
				return true;
			else if (MaskIndex == T1.MaskIndex)
			{
				if (Index < T1.Index)
					return true;
				else if (Index == T1.Index)
					return static_cast<std::size_t>(Category) < static_cast<std::size_t>(T1.Category);
			}
			return false;
		}
	};

	struct ActionIndexWithSubIndexT
	{
		ActionIndexT Original;
		std::size_t SubIndex = 0;

		bool operator==(ActionIndexWithSubIndexT const& T1) const = default;
		bool operator<(ActionIndexWithSubIndexT const& T1) const {

			if(SubIndex < T1.SubIndex)
				return true;
			else if (SubIndex == T1.SubIndex)
			{
				return Original < T1.Original;
			}
			return false;
		}
	};

	DfaT::DfaT(FormatE Format, std::u32string_view Str, bool IsRaw, std::size_t Mask)
		try : DfaT(Format, NfaT{ Str, IsRaw, Mask })
	{

	}
	catch (UnaccaptableRegexTokenIndex const& Error)
	{
		throw UnaccaptableRegex(Error.Type, Str, Error.BadIndex);
	}

	DfaT::DfaT(FormatE Format, NfaT const& T1)
		: Format(Format)
	{

		std::map<NfaEdgeKeyT, NfaEdgePropertyT> EdgeMapping;

		{
			for (std::size_t From = 0; From < T1.Nodes.size(); ++From)
			{
				auto& CurNode = T1.Nodes[From];
				for (std::size_t EdgeIndex = 0; EdgeIndex < CurNode.Edges.size(); ++EdgeIndex)
				{
					auto& CrEdge = CurNode.Edges[EdgeIndex];
					auto Key = NfaEdgeKeyT{From, EdgeIndex};
					NfaEdgePropertyT Property;
					Property.ToNode = CrEdge.ToNode;
					Property.ToAccept = T1.Nodes[CrEdge.ToNode].Accept.has_value();
					Property.MaskIndex = CrEdge.MaskIndex;
					for (auto& Ite : CrEdge.Propertys)
					{
						switch (Ite.Type)
						{
						case NfaT::EdgePropertyE::CaptureBegin:
						case NfaT::EdgePropertyE::CaptureEnd:
							Property.HasCapture = true;
							break;
						case NfaT::EdgePropertyE::OneCounter:
						case NfaT::EdgePropertyE::AddCounter:
							Property.HasCounter = true;
							break;
						case NfaT::EdgePropertyE::LessCounter:
						{
							if (Property.Ranges.empty() || Property.Ranges.rbegin()->Index != Ite.Index)
							{
								Property.Ranges.push_back({
									Ite.Index,
									0,
									Ite.Par
									}
								);
							}
							else {
								Property.Ranges.rbegin()->Max = Ite.Par;
							}
							Property.HasCounter = true;
							break;
						}
						case NfaT::EdgePropertyE::BiggerCounter:
						{
							if (Property.Ranges.empty() || Property.Ranges.rbegin()->Index != Ite.Index)
							{
								Property.Ranges.push_back({
									Ite.Index,
									Ite.Par,
									std::numeric_limits<std::size_t>::max()
								});
							}
							else {
								Property.Ranges.rbegin()->Min = Ite.Par;
							}
							Property.HasCounter = true;
						}
						break;
						default:
							break;
						}
					}
					EdgeMapping.insert({ Key, std::move(Property)});
				}
			}
		}

		std::map<std::vector<std::size_t>, std::size_t> Mapping;

		std::vector<decltype(Mapping)::const_iterator> SearchingStack;

		std::vector<TempNodeT> TempNode;

		{
			std::vector<std::size_t> StartupNode = {0};
			auto [Ite, B] = Mapping.insert({ StartupNode, TempNode.size() });

			TempNodeT Startup;

			for (auto Ite : StartupNode)
			{
				Startup.Accept = T1.Nodes[Ite].Accept;
				if (Startup.Accept.has_value())
					break;
			}

			Startup.OriginalToNode = std::move(StartupNode);
			
			TempNode.push_back(std::move(Startup));
			SearchingStack.push_back(Ite);
		}

		std::vector<TempEdgeT> TempEdges;

		MartixStateT MartixState;

		while (!SearchingStack.empty())
		{
			TempEdges.clear();
			auto Top = *SearchingStack.rbegin();
			SearchingStack.pop_back();

			for (auto Ite : Top->first)
			{
				auto& EdgeRef = T1.Nodes[Ite].Edges;
				std::size_t EdgeIndex = 0;

				for (auto& Ite2 : EdgeRef)
				{

					auto EdgeIndexIte = EdgeIndex++;

					auto Key = EdgeMapping.find({ Ite, EdgeIndexIte });

					TempPropertyT Property{Key, {}};

					auto TempCharSets = Ite2.CharSets;

					if (
						(Format == FormatE::HeadMarch || Format == FormatE::GreedyHeadMarch)
						&& Key->second.ToAccept
					)
					{
						TempCharSets = IntervalT{
							{1, MaxChar()},
							{EndOfFile(), EndOfFile() + 1}
						};
					}

					TempEdgeT Temp{
						std::move(TempCharSets),
						{Property},
						{},
					};
					
					for (std::size_t I = 0; I < TempEdges.size(); ++I)
					{
						auto& Ite3 = TempEdges[I];
						auto Middle = (Temp.CharSets & Ite3.CharSets);
						if (Middle.Size() != 0)
						{
							TempEdgeT NewEdge;
							NewEdge.Propertys.insert(NewEdge.Propertys.end(), Ite3.Propertys.begin(), Ite3.Propertys.end());
							NewEdge.Propertys.insert(NewEdge.Propertys.end(), Temp.Propertys.begin(), Temp.Propertys.end());
							Temp.CharSets = Temp.CharSets - Middle;
							Ite3.CharSets = Ite3.CharSets - Middle;
							NewEdge.CharSets = std::move(Middle);
							TempEdges.push_back(std::move(NewEdge));
							if (Temp.CharSets.Size() == 0)
								break;
						}
					}

					if (!Temp.CharSets.Size() == 0)
					{
						TempEdges.push_back(std::move(Temp));
					}
						

					TempEdges.erase(
						std::remove_if(TempEdges.begin(), TempEdges.end(), [](TempEdgeT const& T) { return T.CharSets.Size() == 0; }),
						TempEdges.end()
					);
				}
			}

			for (std::size_t I = 0; I < TempEdges.size(); ++I)
			{
				auto& Cur = TempEdges[I];
				for (std::size_t I2 = I + 1; I2 < TempEdges.size(); ++I2)
				{
					auto& Last = TempEdges[I2];
					auto NewCharSets = (Cur.CharSets + Last.CharSets);
					if (NewCharSets.Size() <= Last.CharSets.Size())
						Last.CharSets = std::move(NewCharSets);
				}
			}
			
			for (auto& Ite : TempEdges)
			{
				if (Format == FormatE::HeadMarch || Format == FormatE::March)
				{
					for (auto Ite2 = Ite.Propertys.begin(); Ite2 != Ite.Propertys.end(); ++Ite2)
					{
						auto& EdgePro = Ite2->Key->second;
						if (EdgePro.Ranges.empty() && EdgePro.ToAccept)
						{
							Ite.Propertys.erase(Ite2 + 1, Ite.Propertys.end());
							break;
						}
					}
				}
				else if (Format == FormatE::GreedyHeadMarch)
				{
					bool HasAccept = false;
					for (std::size_t I = 0; I < Ite.Propertys.size(); )
					{
						auto& EdgePro = Ite.Propertys[I].Key->second;
						if (EdgePro.Ranges.empty() && EdgePro.ToAccept)
						{
							std::size_t TarMaskIndex = EdgePro.MaskIndex;

							Ite.Propertys.erase(
								std::remove_if(Ite.Propertys.begin() + I + 1, Ite.Propertys.end(), [=](TempPropertyT const& T) {
									return T.Key->second.MaskIndex == TarMaskIndex;
								}),
								Ite.Propertys.end()
							);

							if (HasAccept)
							{
								Ite.Propertys.erase(Ite.Propertys.begin() + I);
								continue;
							}
						}
						++I;
					}
				}

				assert(!Ite.Propertys.empty());

				MartixState.ResetRowCount(Ite.Propertys.size());

				for (std::size_t I = 0; I < Ite.Propertys.size(); ++I)
				{
					auto& RefProperty = Ite.Propertys[I];
					auto& EdgePro = RefProperty.Key->second;
					for (std::size_t I2 = 0; I2 < I; ++I2)
					{
						auto& Tar = Ite.Propertys[I2].Key->second;
						
						bool SameTrue = false;
						bool SameFalse = false;

						if (Tar.MaskIndex == EdgePro.MaskIndex)
						{
							SameTrue = true;
							for (auto& Ite2 : EdgePro.Ranges)
							{
								auto F1 = std::find_if(
									Tar.Ranges.begin(),
									Tar.Ranges.end(),
									[&](NfaEdgePropertyT::RangeT const& Reg) { return Reg.Index == Ite2.Index; }
								);
								if (
									F1 == Tar.Ranges.end()
									|| F1->Min < Ite2.Min || F1->Max > Ite2.Max
									)
								{
									SameTrue = false;
									break;
								}
							}

							SameFalse = true;
							for (auto& Ite2 : Tar.Ranges)
							{
								auto F1 = std::find_if(
									EdgePro.Ranges.begin(),
									EdgePro.Ranges.end(),
									[&](NfaEdgePropertyT::RangeT const& Reg) { return Reg.Index == Ite2.Index; }
								);
								if (
									F1 == EdgePro.Ranges.end()
									|| F1->Min > Ite2.Min || F1->Max < Ite2.Max
									)
								{
									SameFalse = false;
									break;
								}
							}
						}

						if (Tar.ToAccept)
						{
							if (Format == FormatE::March || Format == FormatE::HeadMarch)
							{
								RefProperty.Constraints.push_back({
									I2,
									ActionE::False,
									SameFalse ? ActionE::False : ActionE::Ignore
								});
							}
							else {
								if (Tar.MaskIndex == EdgePro.MaskIndex)
								{
									RefProperty.Constraints.push_back({
										I2,
										ActionE::False,
										SameFalse ? ActionE::False : ActionE::Ignore
									});
								}
								else if(SameTrue || SameFalse){
									RefProperty.Constraints.push_back({
										I2,
										SameTrue ? ActionE::True : ActionE::Ignore,
										SameFalse ? ActionE::False : ActionE::Ignore
									});
								}
							}
						}
						else {
							if (SameTrue || SameFalse)
							{
								RefProperty.Constraints.push_back({
									I2,
									SameTrue ? ActionE::True : ActionE::Ignore,
									SameFalse ? ActionE::False : ActionE::Ignore
								});
							}
						}
					}

					std::size_t OldLineSize = MartixState.GetLineCount();
					for (std::size_t LC = 0; LC < OldLineSize; ++LC)
					{
						ActionE CurAction = ActionE::Ignore;

						for (auto& Ite6 : RefProperty.Constraints)
						{
							auto RCState = MartixState.Get(Ite6.Source, LC);
							CurAction = (RCState == MartixStateT::StateE::True ? Ite6.PassAction : Ite6.UnpassAction);
							if(CurAction != ActionE::Ignore)
								break;
						}

						switch (CurAction)
						{
						case ActionE::True:
							MartixState.Get(I, LC) = MartixStateT::StateE::True;
							break;
						case ActionE::False:
							MartixState.Get(I, LC) = MartixStateT::StateE::False;
							break;
						case ActionE::Ignore:
						{
							if (EdgePro.Ranges.empty())
							{
								MartixState.Get(I, LC) = MartixStateT::StateE::True;
							}
							else {
								MartixState.Get(I, LC) = MartixStateT::StateE::False;
								auto NewLine = MartixState.CopyLine(LC);
								MartixState.Get(I, NewLine) = MartixStateT::StateE::True;
							}
							break;
						}
						default:
							assert(false);
							break;
						}
					}
				}

				Ite.ToNode.reserve(MartixState.GetLineCount());

				std::vector<std::size_t> CacheNode;

				CacheNode.reserve(Ite.Propertys.size());

				MartixState.RemoveAllFalseLine();

				assert(MartixState.GetLineCount() != 0);

				for (std::size_t Line = 0; Line < MartixState.GetLineCount(); ++Line)
				{
					CacheNode.clear();
					for (std::size_t Row = 0; Row < MartixState.GetRowCount(); ++Row)
					{
						if (MartixState.Get(Row, Line) == MartixStateT::StateE::True)
						{
							std::size_t TNode = Ite.Propertys[Row].Key->second.ToNode;
							auto F = std::find(CacheNode.begin(), CacheNode.end(), TNode);
							if(F == CacheNode.end())
								CacheNode.push_back(TNode);
							else {
								auto Key = Ite.Propertys[Row].Key->first;
								throw UnaccaptableRegexTokenIndex{
									UnaccaptableRegexTokenIndex::TypeT::BadRegex,
									T1.Nodes[Key.From].Edges[Key.EdgeIndex].TokenIndex
								};
							}
						}
					}

					assert(!CacheNode.empty());

					auto [MapIte, Bool] = Mapping.insert({ CacheNode, TempNode.size() });

					if (Bool)
					{
						TempNodeT TemNode;
						for (auto Ite3 : CacheNode)
						{
							TemNode.Accept = T1.Nodes[Ite3].Accept;
							if (TemNode.Accept.has_value())
								break;
						}
						TemNode.OriginalToNode = CacheNode;
						TempNode.push_back(std::move(TemNode));
						SearchingStack.push_back(MapIte);
					}

					auto Re = MartixState.ReadLine(Line);

					TempToNodeT NewToNode;

					for (auto Ite : Re)
					{
						switch (Ite)
						{
						case MartixStateT::StateE::True:
							NewToNode.Actions.push_back(ActionE::True);
							break;
						case MartixStateT::StateE::False:
							NewToNode.Actions.push_back(ActionE::False);
							break;
						default:
							assert(false);
							break;
						}
					}

					NewToNode.ToNode = MapIte->second;

					Ite.ToNode.push_back(std::move(NewToNode));
				}

			}

			TempNode[Top->second].TempEdge = TempEdges;
		}

		struct ActionIndexMappingNodeT
		{
			std::map<ActionIndexT, std::size_t> Indexs;
			bool HasAccept = false;
		};

		std::vector<ActionIndexMappingNodeT> ActionIndexNodes;
		std::vector<ActionIndexWithSubIndexT> ActionIndexLocate;

		auto LocateActionIndex = [&](ActionIndexT Index, std::size_t NodeIndex) -> std::size_t {

			std::size_t SubIndex = 0;
			auto F1 = ActionIndexNodes[NodeIndex].Indexs.find(Index);
			assert(F1 != ActionIndexNodes[NodeIndex].Indexs.end());
			SubIndex = F1->second;
			ActionIndexWithSubIndexT AIWSB {Index, SubIndex};
			for (std::size_t I = 0; I < ActionIndexLocate.size(); ++I)
			{
				if(ActionIndexLocate[I] == AIWSB)
					return I;
			}
			assert(false);
			return ActionIndexLocate.size();
		};

		{

			ActionIndexNodes.resize(T1.Nodes.size());

			std::set<NfaEdgeKeyT> UsedKeys;

			struct SerarchStackT
			{
				ActionIndexT Index;
				std::size_t NodeIndex;
			};

			std::vector<SerarchStackT> SearchStackT;

			for (auto& Ite : TempNode)
			{
				for (auto& Ite2 : Ite.TempEdge)
				{
					for (auto Ite3 : Ite2.Propertys)
					{
						auto NfaEdgeKey = Ite3.Key->first;
						auto [I, B] = UsedKeys.insert(NfaEdgeKey);
						if(B)
						{
							auto& FromNode = ActionIndexNodes[Ite3.Key->first.From];
							auto& NfaEdgeProperty = Ite3.Key->second;
							if (NfaEdgeProperty.ToAccept || NfaEdgeProperty.HasCounter || NfaEdgeProperty.HasCapture)
							{
								auto& Ref = ActionIndexNodes[NfaEdgeProperty.ToNode];
								Ref.HasAccept = NfaEdgeProperty.ToAccept;

								if (NfaEdgeProperty.HasCounter || NfaEdgeProperty.HasCapture)
								{
									auto& Edge = T1.Nodes[NfaEdgeKey.From].Edges[NfaEdgeKey.EdgeIndex];
									for (auto& Ite4 : Edge.Propertys)
									{
										switch (Ite4.Type)
										{
										case NfaT::EdgePropertyE::CaptureBegin:
										{
											ActionIndexT Index{
												ActionIndexT::CategoryE::CaptureBegin, Ite4.Index, Edge.MaskIndex
											};
											FromNode.Indexs.insert({Index, 0});
											SearchStackT.push_back({Index, NfaEdgeProperty.ToNode });
											break;
										}
										case NfaT::EdgePropertyE::CaptureEnd:
										{
											ActionIndexT Index{
												ActionIndexT::CategoryE::CaptureEnd, Ite4.Index, Edge.MaskIndex
											};
											FromNode.Indexs.insert({Index, 0});
											SearchStackT.push_back({ Index, NfaEdgeProperty.ToNode });
											break;
										}
										case NfaT::EdgePropertyE::OneCounter:
										case NfaT::EdgePropertyE::AddCounter:
										case NfaT::EdgePropertyE::LessCounter:
										case NfaT::EdgePropertyE::BiggerCounter:
										{
											ActionIndexT Index{
												ActionIndexT::CategoryE::Counter, Ite4.Index, Edge.MaskIndex
											};
											FromNode.Indexs.insert({Index, 0});
											Ref.Indexs.insert({Index, 0});
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

			while (!SearchStackT.empty())
			{
				auto Top = *SearchStackT.rbegin();
				SearchStackT.pop_back();
				auto& Ref = ActionIndexNodes[Top.NodeIndex];
				auto [Ite, B] = Ref.Indexs.insert({Top.Index, 0});
				if (B)
				{
					for (auto& Ite2 : UsedKeys)
					{
						if (Ite2.From == Top.NodeIndex)
						{
							auto& EdgeRef = T1.Nodes[Ite2.From].Edges[Ite2.EdgeIndex];
							SearchStackT.push_back({ Top.Index, EdgeRef.ToNode });
						}
					}
				}
			}

			bool Change = true;

			while (Change)
			{
				Change = false;

				for (auto& Ite : TempNode)
				{
					if (Ite.OriginalToNode.size() <= 1)
						continue;
					for (std::size_t I = 0; I < Ite.OriginalToNode.size(); ++I)
					{
						auto& Cur = ActionIndexNodes[Ite.OriginalToNode[I]];
						for (std::size_t I2 = I + 1; I2 < Ite.OriginalToNode.size(); ++I2)
						{
							auto& Cur2 = ActionIndexNodes[Ite.OriginalToNode[I2]];
							for (auto& Ite2 : Cur.Indexs)
							{
								auto Find = Cur2.Indexs.find(Ite2.first);
								if (Find != Cur2.Indexs.end() && Ite2.second == Find->second)
								{
									assert(!(Cur.HasAccept && Cur2.HasAccept));
									if (Cur2.HasAccept)
										Ite2.second += 1;
									else
										Find->second += 1;
									Change = true;
								}
							}
						}
					}
				}
			}

			for (auto& Ite : ActionIndexNodes)
			{
				for (auto Ite2 : Ite.Indexs)
				{
					auto Action = ActionIndexWithSubIndexT{ Ite2.first, Ite2.second };
					auto F = std::find(ActionIndexLocate.begin(), ActionIndexLocate.end(), Action);
					if (F == ActionIndexLocate.end())
					{
						ActionIndexLocate.push_back(Action);
					}
				}
			}

			std::sort(ActionIndexLocate.begin(), ActionIndexLocate.end());

		}

		CacheRecordCount = ActionIndexLocate.size();

		Nodes.reserve(TempNode.size());

		for (auto& Ite : TempNode)
		{
			NodeT NewNode;

			if (Ite.Accept.has_value())
			{
				std::optional<std::size_t> Begin;
				std::size_t Last = 0;

				for (; Last < ActionIndexLocate.size(); ++Last)
				{
					auto& Ref = ActionIndexLocate[Last];
					if (Ref.SubIndex == 0 
						&& (Ref.Original.Category == ActionIndexT::CategoryE::CaptureBegin 
							|| Ref.Original.Category == ActionIndexT::CategoryE::CaptureEnd
							)
						&& Ref.Original.MaskIndex == Ite.Accept->MaskIndex)
					{
						if (!Begin.has_value())
							Begin = Last;
					}else if(Begin.has_value())
						break;
				}

				AcceptT NewAccept{
					Ite.Accept->Mask,
					Begin.has_value() ? Misc::IndexSpan<>{ 
						*Begin, 
						Last
					} : Misc::IndexSpan<>{0, 0}
				};
				NewNode.Accept = NewAccept;
			}

			for (auto& Ite2 : Ite.TempEdge)
			{
				EdgeT NewEdge;
				NewEdge.CharSets = Ite2.CharSets;
				auto ToNode = Ite2.ToNode;

				bool LastHasNoRange = false;

				std::vector<std::size_t> ConstraintsMapping;

				ConstraintsMapping.resize(Ite2.Propertys.size());

				for (std::size_t I = 0; I < Ite2.Propertys.size(); ++I)
					ConstraintsMapping[I] = I;

				for (std::size_t I = 0; I < Ite2.Propertys.size(); ++I)
				{
					auto& Pro = Ite2.Propertys[I];
					auto& Edge = T1.Nodes[Pro.Key->first.From].Edges[Pro.Key->first.EdgeIndex];
					bool NeedMerge = false;
					if (LastHasNoRange && Pro.Key->second.Ranges.empty())
					{
						NeedMerge = true;

						#ifdef _DEBUG

						for (std::size_t LC = 0; LC < Ite2.ToNode.size(); ++LC)
						{
							if (Ite2.ToNode[LC].Actions[I - 1] != Ite2.ToNode[LC].Actions[I])
							{
								assert(false);
							}
						}

						#endif

						for (auto& Ite3 : ToNode)
						{
							Ite3.Actions[I] = ActionE::Ignore;
							//Ite3.Actions.erase(Ite3.Actions.begin() + I);
						}

						for(std::size_t I2 = I; I2 < Ite2.Propertys.size(); ++I2)
							ConstraintsMapping[I2] -= 1;
					}

					LastHasNoRange = Pro.Key->second.Ranges.empty();

					if (I != 0 && !NeedMerge)
						NewEdge.Propertys.push_back({ PropertyActioE::NewContext });
					
					if (!NeedMerge)
					{
						for (auto& Ite3 : Pro.Constraints)
						{
							std::size_t Tar = ConstraintsMapping[Ite3.Source];

							if (Ite3.PassAction == ActionE::True)
							{
								NewEdge.Propertys.push_back({ PropertyActioE::ConstraintsTrueTrue, Tar });
							}
							else if (Ite3.PassAction == ActionE::False)
							{
								NewEdge.Propertys.push_back({ PropertyActioE::ConstraintsTrueFalse, Tar });
							}

							if (Ite3.UnpassAction == ActionE::True)
							{
								NewEdge.Propertys.push_back({ PropertyActioE::ConstraintsFalseTrue, Tar });
							}
							else if (Ite3.UnpassAction == ActionE::False)
							{
								NewEdge.Propertys.push_back({ PropertyActioE::ConstraintsFalseFalse, Tar });
							}
						}
					}

					struct CopyRecord
					{
						std::size_t FromSolt;
						std::size_t ToSolt;
						std::strong_ordering operator<=>(CopyRecord const& CR) const = default;
					};

					std::set<CopyRecord> HasCounter;

					std::set<std::size_t> OverwritedCapture;

					if (Pro.Key->second.HasCapture)
					{
						for (auto Ite3 : Edge.Propertys)
						{
							switch (Ite3.Type)
							{
							case NfaT::EdgePropertyE::CaptureBegin:
							{
								ActionIndexT Index{
									ActionIndexT::CategoryE::CaptureBegin,
									Ite3.Index,
									Edge.MaskIndex
								};
								OverwritedCapture.insert(LocateActionIndex(Index, Edge.ToNode));
								break;
							}
							case NfaT::EdgePropertyE::CaptureEnd:
							{
								ActionIndexT Index{
									ActionIndexT::CategoryE::CaptureEnd,
									Ite3.Index,
									Edge.MaskIndex
								};
								OverwritedCapture.insert(LocateActionIndex(Index, Edge.ToNode));
								break;
							}
							default:
								break;
							}
						}
					}
					

					auto& FromSubIndex = ActionIndexNodes[Pro.Key->first.From].Indexs;
					auto& ToSubIndex = ActionIndexNodes[Edge.ToNode].Indexs;

					for (auto& Ite3 : FromSubIndex)
					{
						if (Ite3.first.Category == ActionIndexT::CategoryE::CaptureBegin || Ite3.first.Category == ActionIndexT::CategoryE::CaptureEnd)
						{
							auto F1 = ToSubIndex.find(Ite3.first);
							if (F1 != ToSubIndex.end() && Ite3.second != F1->second)
							{
								auto Target = LocateActionIndex(Ite3.first, Edge.ToNode);
								if (OverwritedCapture.find(Target) == OverwritedCapture.end())
								{
									NewEdge.Propertys.push_back({
										PropertyActioE::CopyValue,
										LocateActionIndex(Ite3.first, Pro.Key->first.From),
										LocateActionIndex(Ite3.first, Edge.ToNode),
									});
								}
							}
						}
					}

					for (auto Ite3 : Edge.Propertys)
					{
						switch (Ite3.Type)
						{
						case NfaT::EdgePropertyE::CaptureBegin:
						{
							ActionIndexT Index{
								ActionIndexT::CategoryE::CaptureBegin,
								Ite3.Index,
								Edge.MaskIndex
							};
							NewEdge.Propertys.push_back({
								PropertyActioE::RecordLocation,
								LocateActionIndex(Index, Edge.ToNode)
							});

							break;
						}
						case NfaT::EdgePropertyE::CaptureEnd:
						{
							ActionIndexT Index{
								ActionIndexT::CategoryE::CaptureEnd,
								Ite3.Index,
								Edge.MaskIndex
							};
							NewEdge.Propertys.push_back({
								PropertyActioE::RecordLocation,
								LocateActionIndex(Index, Edge.ToNode)
							});
							break;
						}
						case NfaT::EdgePropertyE::OneCounter:
						{
							ActionIndexT Index{
								ActionIndexT::CategoryE::Counter,
								Ite3.Index,
								Edge.MaskIndex
							};
							NewEdge.Propertys.push_back({
								PropertyActioE::OneCounter,
								LocateActionIndex(Index, Edge.ToNode)
							});
							break;
						}
						case NfaT::EdgePropertyE::AddCounter:
						{
							ActionIndexT Index{
								ActionIndexT::CategoryE::Counter,
								Ite3.Index,
								Edge.MaskIndex
							};
							NewEdge.Propertys.push_back({
								PropertyActioE::AddCounter,
								LocateActionIndex(Index, Edge.ToNode)
							});
							break;
						}
						case NfaT::EdgePropertyE::LessCounter:
						{
							ActionIndexT Index{
								ActionIndexT::CategoryE::Counter,
								Ite3.Index,
								Edge.MaskIndex
							};
							std::size_t FormSolt = LocateActionIndex(Index, Pro.Key->first.From);
							std::size_t ToSolt = LocateActionIndex(Index, Edge.ToNode);
							NewEdge.Propertys.push_back({
								PropertyActioE::LessCounter,
								FormSolt,
								Ite3.Par
							});
							HasCounter.insert({ FormSolt, ToSolt });
							break;
						}
						case NfaT::EdgePropertyE::BiggerCounter:
						{
							ActionIndexT Index{
								ActionIndexT::CategoryE::Counter,
								Ite3.Index,
								Edge.MaskIndex
							};
							std::size_t FormSolt = LocateActionIndex(Index, Pro.Key->first.From);
							std::size_t ToSolt = LocateActionIndex(Index, Edge.ToNode);
							NewEdge.Propertys.push_back({
								PropertyActioE::BiggerCounter,
								FormSolt,
								Ite3.Par
							});
							HasCounter.insert({ FormSolt, ToSolt });
							break;
						}
						default:
							assert(false);
							break;
						}
					}

					for (auto& Ite3 : HasCounter)
					{
						if (Ite3.FromSolt != Ite3.ToSolt)
						{
							NewEdge.Propertys.push_back({
								PropertyActioE::CopyValue,
								Ite3.FromSolt,
								Ite3.ToSolt
							});
						}
					}
				}

				for (auto& Ite3 : ToNode)
				{
					Ite3.Actions.erase(
						std::remove_if(Ite3.Actions.begin(), Ite3.Actions.end(), [](ActionE I){ return I == ActionE::Ignore; }),
						Ite3.Actions.end()
					);
				}

				std::vector<ConditionT> Conditions;

				for (auto& Ite3 : ToNode)
				{
					std::size_t NextIndex = 0;

					for (auto Ite4 : Ite3.Actions)
					{
						if (NextIndex >= Conditions.size())
						{
							ConditionT NewCondi;

							if (Ite4 == ActionE::True)
							{
								NewCondi.PassCommand = ConditionT::CommandE::Next;
								NewCondi.Pass = Conditions.size() + 1;
								NextIndex = NewCondi.Pass;
							}
							else {
								NewCondi.UnpassCommand = ConditionT::CommandE::Next;
								NewCondi.Unpass = Conditions.size() + 1;
								NextIndex = NewCondi.Unpass;
							}

							Conditions.push_back(NewCondi);
						}
						else {
							auto& Cur = Conditions[NextIndex];
							if (Ite4 == ActionE::True)
							{
								if(Cur.PassCommand == ConditionT::CommandE::Fail)
								{
									Cur.PassCommand = ConditionT::CommandE::Next;
									Cur.Pass = Conditions.size();
								}
								NextIndex = Cur.Pass;
							}
							else if (Ite4 == ActionE::False)
							{
								if (Cur.UnpassCommand == ConditionT::CommandE::Fail);
								{
									Cur.UnpassCommand = ConditionT::CommandE::Next;
									Cur.Unpass = Conditions.size();
								}
								NextIndex = Conditions.size();
							}
						}
					}

					for (auto& Ite4 : Conditions)
					{
						if (Ite4.PassCommand == ConditionT::CommandE::Next && Ite4.Pass == Conditions.size())
						{
							Ite4.PassCommand = ConditionT::CommandE::ToNode;
							Ite4.Pass = Ite3.ToNode;
						}
						if (Ite4.UnpassCommand == ConditionT::CommandE::Next && Ite4.Unpass == Conditions.size())
						{
							Ite4.UnpassCommand = ConditionT::CommandE::ToNode;
							Ite4.Unpass = Ite3.ToNode;
						}
					}
				}

				NewEdge.Conditions = std::move(Conditions);
				NewNode.Edges.push_back(NewEdge);
			}


			Nodes.push_back(std::move(NewNode));
		}
	}

	RegProcessor::RegProcessor(DfaT& Table)
		: Table(Table), CurNodeIndex(Table.GetStartupNodeIndex())
	{
		CacheIndex.resize(Table.GetCacheCounterCount());
	}

	void RegProcessor::Reset()
	{
		CurNodeIndex = Table.GetStartupNodeIndex();
		CacheIndex.clear();
		CacheIndex.resize(Table.GetCacheCounterCount(), 0);
		auto& NodeRef = Table.Nodes[CurNodeIndex];
		Accept = NodeRef.Accept;
		CurMainCapture.reset();
	}

	auto RegProcessor::Consume(char32_t Token, std::size_t TokenIndex) -> bool
	{
		auto& NodeRef = Table.Nodes[CurNodeIndex];
		for (auto& Ite : NodeRef.Edges)
		{
			if (Ite.CharSets.IsInclude(Token))
			{
				TempResult.clear();

				auto DetectReuslt = DetectResultE::True;
				for (auto& Ite2 : Ite.Propertys)
				{
					if (DetectReuslt == DetectResultE::True)
					{
						switch (Ite2.Action)
						{
						case DfaT::PropertyActioE::CopyValue:
							CacheIndex[Ite2.Par] = CacheIndex[Ite2.Solt];
							break;
						case DfaT::PropertyActioE::RecordLocation:
							CacheIndex[Ite2.Solt] = TokenIndex;
							break;
						case DfaT::PropertyActioE::NewContext:
							break;
						case DfaT::PropertyActioE::ConstraintsTrueTrue:
							if(TempResult[Ite2.Solt] == DetectResultE::True)
								DetectReuslt = DetectResultE::True;
							break;
						case DfaT::PropertyActioE::ConstraintsFalseFalse:
							if (TempResult[Ite2.Solt] == DetectResultE::False)
								DetectReuslt = DetectResultE::False;
							break;
						case DfaT::PropertyActioE::ConstraintsFalseTrue:
							if (TempResult[Ite2.Solt] == DetectResultE::False)
								DetectReuslt = DetectResultE::True;
							break;
						case DfaT::PropertyActioE::ConstraintsTrueFalse:
							if (TempResult[Ite2.Solt] == DetectResultE::True)
								DetectReuslt = DetectResultE::False;
							break;
						case DfaT::PropertyActioE::OneCounter:
							CacheIndex[Ite2.Solt] = 1;
							break;
						case DfaT::PropertyActioE::AddCounter:
							CacheIndex[Ite2.Solt] += 1;
							break;
						case DfaT::PropertyActioE::LessCounter:
							if(CacheIndex[Ite2.Solt] > Ite2.Par)
								DetectReuslt = DetectResultE::False;
							break;
						case DfaT::PropertyActioE::BiggerCounter:
							if (CacheIndex[Ite2.Solt] < Ite2.Par)
								DetectReuslt = DetectResultE::False;
							break;
						default:
							assert(false);
							break;
						}
					}
					if (Ite2.Action == DfaT::PropertyActioE::NewContext)
					{
						TempResult.push_back(DetectReuslt);
						DetectReuslt = DetectResultE::True;
					}
				}

				TempResult.push_back(DetectReuslt);

				std::size_t NextIte = 0;
				std::optional<std::size_t> ToNode;
				assert(!Ite.Conditions.empty());

				for (auto Ite2 : TempResult)
				{
					assert(!ToNode.has_value());
					auto& Cond = Ite.Conditions[NextIte];
					if (Ite2 == DetectResultE::True)
					{
						switch (Cond.PassCommand)
						{
						case DfaT::ConditionT::CommandE::Next:
							NextIte = Cond.Pass;
							break;
						case DfaT::ConditionT::CommandE::Fail:
							return false;
						case DfaT::ConditionT::CommandE::ToNode:
							ToNode = Cond.Pass;
							break;
						default:
							assert(false);
							break;
						}
					}
					else if (Ite2 == DetectResultE::False)
					{
						switch (Cond.UnpassCommand)
						{
						case DfaT::ConditionT::CommandE::Next:
							NextIte = Cond.Unpass;
							break;
						case DfaT::ConditionT::CommandE::Fail:
							return false;
						case DfaT::ConditionT::CommandE::ToNode:
							ToNode = Cond.Unpass;
							break;
						default:
							assert(false);
							break;
						}
					}
				}

				assert(ToNode.has_value());

				CurNodeIndex = *ToNode;
				Accept = Table.Nodes[CurNodeIndex].Accept;
				if (!CurMainCapture.has_value())
				{
					CurMainCapture = {TokenIndex, TokenIndex};
				}
				else {
					CurMainCapture = { CurMainCapture->Begin(), TokenIndex };
				}
				return true;
			}
		}
		return false;
	}

	std::optional<ProcessorAcceptT> RegProcessor::GetAccept() const
	{
		if (Accept.has_value())
		{
			ProcessorAcceptT NewAccept;
			NewAccept.Mask = Accept->Mask;
			for (std::size_t I = Accept->CaptureIndex.Begin(); I + 1 < Accept->CaptureIndex.End(); I += 2)
			{
				NewAccept.Capture.push_back(Misc::IndexSpan<>(CacheIndex[I], CacheIndex[I + 1]));
			}
			if (CurMainCapture.has_value())
				NewAccept.MainCapture = *CurMainCapture;
			return NewAccept;
		}
		return {};
	}


	static void DfaBinaryTable::SerilizeToExe(Misc::StructedSerilizerWriter<StandardT>& Writer, DfaT const& RefTable)
	{
		std::array<StandardT, 6> Array;

		Writer.WriteObjectArray(std::span(Array));

		std::vector<StandardT> NodeIndexOffset;

		for (auto& Ite : RefTable.Nodes)
		{
			NodeT NewNode;

		}
	}




	namespace Exception
	{
		char const* Interface::what() const
		{
			return "PotatoRegException";
		}

		char const* UnaccaptableRegexTokenIndex::what() const
		{
			return "PotatoRegException::UnaccaptableRegexTokenIndex";
		}

		UnaccaptableRegex::UnaccaptableRegex(TypeT Type, std::u8string_view Str, Misc::IndexSpan<> BadIndex)
			: UnaccaptableRegexTokenIndex(Type, BadIndex)
		{
			TotalString.resize(Encode::StrEncoder<char8_t, wchar_t>::RequireSpace(Str).TargetSpace);
			Encode::StrEncoder<char8_t, wchar_t>::EncodeUnSafe(Str, TotalString);
		}

		UnaccaptableRegex::UnaccaptableRegex(TypeT Type, std::wstring_view Str, Misc::IndexSpan<> BadIndex)
			: UnaccaptableRegexTokenIndex(Type, BadIndex), TotalString(Str)
		{
		}

		UnaccaptableRegex::UnaccaptableRegex(TypeT Type, std::u16string_view Str, Misc::IndexSpan<> BadIndex)
			: UnaccaptableRegexTokenIndex(Type, BadIndex)
		{
			TotalString.resize(Encode::StrEncoder<char16_t, wchar_t>::RequireSpace(Str).TargetSpace);
			Encode::StrEncoder<char16_t, wchar_t>::EncodeUnSafe(Str, TotalString);
		}

		UnaccaptableRegex::UnaccaptableRegex(TypeT Type, std::u32string_view Str, Misc::IndexSpan<> BadIndex)
			: UnaccaptableRegexTokenIndex(Type, BadIndex)
		{
			TotalString.resize(Encode::StrEncoder<char32_t, wchar_t>::RequireSpace(Str).TargetSpace);
			Encode::StrEncoder<char32_t, wchar_t>::EncodeUnSafe(Str, TotalString);
		}

		char const* UnaccaptableRegex::what() const
		{
			return "PotatoRegException::UnaccaptableRegex";
		}

		char const* RegexOutOfRange::what() const
		{
			return "PotatoRegException::RegexOutOfRange";
		}

		char const* CircleShifting::what() const
		{
			return "PotatoRegException::CircleShifting";
		}
	}
}