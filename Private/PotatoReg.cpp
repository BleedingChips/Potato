#include "../Public/PotatoReg.h"
#include "../Public/PotatoSLRX.h"
#include "../Public/PotatoStrEncode.h"
#include <deque>
#include <limits>

namespace Potato::Reg
{
	
	SeqIntervalT const& MaxIntervalRange() { 
		static SeqIntervalT Temp{ IntervalT{ 1, MaxChar() } };
		return Temp;
	};
	
	using namespace Exception;

	using SLRX::Symbol;

	enum class T : SLRX::StandardT
	{
		SingleChar = 0, // ���ַ�
		CharSet, // ���ַ�
		Min, // -
		BracketsLeft, //[
		BracketsRight, // ]
		ParenthesesLeft, //(
		ParenthesesRight, //)
		CurlyBracketsLeft, //{
		CurlyBracketsRight, //}
		Num, // 0 - 1
		Comma, // ,
		Mulity, //*
		Question, // ?
		Or, // |
		Add, // +
		Not, // ^
		Colon, // :
	};

	constexpr Symbol operator*(T Input) { return Symbol::AsTerminal(static_cast<SLRX::StandardT>(Input)); };

	enum class NT : SLRX::StandardT
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

	struct RegLexer
	{

		struct Element
		{
			T Value;
			std::variant<SeqIntervalT, char32_t> Acceptable;
			std::size_t TokenIndex;
		};

		std::span<Element const> GetSpan() const { return StoragedSymbol;};

		bool Consume(char32_t InputSymbol, std::size_t TokenIndex);
		bool EndOfFile();

		enum class State
		{
			Normal,
			Transfer,
			BigNumber,
			Number,
			Done,
			Raw,
		};

		RegLexer(bool IsRaw) : CurrentState(IsRaw ? State::Raw : State::Normal) {}

	protected:

		State CurrentState;
		std::size_t Number = 0;
		char32_t NumberChar = 0;
		bool NumberIsBig = false;
		char32_t RecordSymbol;
		std::size_t RecordTokenIndex = 0;
		std::size_t TokenIndexIte = 0;

		std::vector<Element> StoragedSymbol;
	};

	bool RegLexer::Consume(char32_t InputSymbol, std::size_t TokenIndex)
	{
		if (InputSymbol < MaxChar())
		{
			switch (CurrentState)
			{
			case State::Normal:
			{
				switch (InputSymbol)
				{
				case U'-':
					StoragedSymbol.push_back({T::Min,  InputSymbol, TokenIndex });
					break;
				case U'[':
					StoragedSymbol.push_back({T::BracketsLeft, InputSymbol, TokenIndex });
					break;
				case U']':
					StoragedSymbol.push_back({T::BracketsRight, InputSymbol, TokenIndex });
					break;
				case U'{':
					StoragedSymbol.push_back({T::CurlyBracketsLeft, InputSymbol, TokenIndex });
					break;
				case U'}':
					StoragedSymbol.push_back({T::CurlyBracketsRight, InputSymbol, TokenIndex });
					break;
				case U',':
					StoragedSymbol.push_back({T::Comma, InputSymbol, TokenIndex });
					break;
				case U'(':
					StoragedSymbol.push_back({T::ParenthesesLeft, InputSymbol, TokenIndex });
					break;
				case U')':
					StoragedSymbol.push_back({T::ParenthesesRight, InputSymbol, TokenIndex });
					break;
				case U'*':
					StoragedSymbol.push_back({T::Mulity, InputSymbol, TokenIndex });
					break;
				case U'?':
					StoragedSymbol.push_back({T::Question, InputSymbol, TokenIndex });
					break;
				case U'.':
					StoragedSymbol.push_back({T::CharSet, MaxIntervalRange(), TokenIndex });
					break;
				case U'|':
					StoragedSymbol.push_back({T::Or, InputSymbol, TokenIndex });
					break;
				case U'+':
					StoragedSymbol.push_back({T::Add, InputSymbol, TokenIndex });
					break;
				case U'^':
					StoragedSymbol.push_back({T::Not, InputSymbol, TokenIndex });
					break;
				case U':':
					StoragedSymbol.push_back({T::Colon, InputSymbol, TokenIndex });
					break;
				case U'\\':
					CurrentState = State::Transfer;
					RecordSymbol = InputSymbol;
					break;
				default:
					if (InputSymbol >= U'0' && InputSymbol <= U'9')
						StoragedSymbol.push_back({T::Num, InputSymbol, TokenIndex });
					else
						StoragedSymbol.push_back({T::SingleChar, InputSymbol, TokenIndex });
					break;
				}
				break;
			}
			case State::Transfer:
			{
				switch (InputSymbol)
				{
				case U'd':
					StoragedSymbol.push_back({T::CharSet, SeqIntervalT{ {U'0', U'9' + 1} }, TokenIndex });
					CurrentState = State::Normal;
					break;
				case U'D':
				{
					SeqIntervalT Tem{ { {1, U'0'},{U'9' + 1, MaxChar()} } };
					StoragedSymbol.push_back({T::CharSet, std::move(Tem), TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				case U'f':
					StoragedSymbol.push_back({T::SingleChar, U'\f', TokenIndex });
					CurrentState = State::Normal;
					break;
				case U'n':
					StoragedSymbol.push_back({T::SingleChar, U'\n', TokenIndex });
					CurrentState = State::Normal;
					break;
				case U'r':
					StoragedSymbol.push_back({T::SingleChar, U'\r', TokenIndex });
					CurrentState = State::Normal;
					break;
				case U't':
					StoragedSymbol.push_back({T::SingleChar, U'\t', TokenIndex });
					CurrentState = State::Normal;
					break;
				case U'v':
					StoragedSymbol.push_back({T::SingleChar, U'\v', TokenIndex });
					CurrentState = State::Normal;
					break;
				case U's':
				{
					SeqIntervalT tem({ IntervalT{ 1, 33 }, IntervalT{127, 128} });
					StoragedSymbol.push_back({T::CharSet, std::move(tem), TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				case U'S':
				{
					SeqIntervalT tem({ IntervalT{33, 127}, IntervalT{128, MaxChar()} });
					StoragedSymbol.push_back({T::CharSet, std::move(tem), TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				case U'w':
				{
					SeqIntervalT tem({ IntervalT{ U'a', U'z' + 1 }, IntervalT{ U'A', U'Z' + 1 }, IntervalT{ U'_', U'_' + 1} });
					StoragedSymbol.push_back({T::CharSet, std::move(tem), TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				case U'W':
				{
					SeqIntervalT tem({ IntervalT{ U'a', U'z' + 1 }, IntervalT{ U'A', U'Z' + 1 }, IntervalT{ U'_', U'_' + 1} });
					SeqIntervalT total({ 1, MaxChar() });
					StoragedSymbol.push_back({T::CharSet, tem.Complementary(MaxIntervalRange()), TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				case U'z':
				{
					SeqIntervalT tem(IntervalT{ 256, MaxChar() });
					StoragedSymbol.push_back({T::CharSet, std::move(tem), TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				case U'u':
				{
					CurrentState = State::Number;
					NumberChar = 0;
					Number = 0;
					NumberIsBig = false;
					break;
				}
				case U'U':
				{
					CurrentState = State::Number;
					NumberChar = 0;
					Number = 0;
					NumberIsBig = true;
					break;
				}
				default:
					StoragedSymbol.push_back({T::SingleChar, InputSymbol, TokenIndex });
					CurrentState = State::Normal;
					break;
				}
				break;
			}
			case State::Number:
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
					StoragedSymbol.push_back({T::SingleChar, NumberChar, TokenIndex });
					CurrentState = State::Normal;
				}
				break;
			}
			case State::Raw:
				StoragedSymbol.push_back({ T::SingleChar, {InputSymbol}, TokenIndex });
				break;
			default:
				assert(false);
			}
			return true;
		}else {
			return false;
		}
	}

	bool RegLexer::EndOfFile()
	{
		if (CurrentState == State::Normal || CurrentState == State::Raw)
		{
			CurrentState = State::Done;
			return true;
		}	
		else
			return false;
	}

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

	std::any RegNoTerminalFunction(Potato::SLRX::NTElement& NT, EpsilonNFA& Output)
	{

		using NodeSet = EpsilonNFA::NodeSet;

		switch (NT.Reduce.Mask)
		{
		case 40:
			return NT[0].Consume();
		case 41:
		{
			char32_t Cur = NT[0].Consume<char32_t>();
			SeqIntervalT Ptr({ Cur, Cur + 1 });
			return Ptr;
		}
		case 42:
		{
			char32_t Cur = NT[0].Consume<char32_t>();
			char32_t Cur2 = NT[2].Consume<char32_t>();

			if (Cur <= Cur2)
			{
				return SeqIntervalT({ Cur, Cur2 + 1 });
			}
			else {
				return SeqIntervalT({ Cur2, Cur + 1 });
			}
		}
		case 1:
			return NT[0].Consume();
		case 3:
		{
			auto T1 = NT[0].Consume<SeqIntervalT>();
			auto T2 = NT[1].Consume<SeqIntervalT>();
			return SeqIntervalT{ {T1.AsWrapper().Union(T2.AsWrapper())} };
		}
		case 60:
		{
			auto T1 = NT[0].Consume<char32_t>();
			SeqIntervalT Ptr({ T1, T1 + 1 });
			return Ptr;
		}
		case 61:
		{
			auto T1 = NT[0].Consume<char32_t>();
			SeqIntervalT Ptr2({ T1, T1 + 1 });
			auto T2 = NT[1].Consume<SeqIntervalT>();
			return T2.AsWrapper().Union(Ptr2.AsWrapper());
		}
		case 62:
		{
			auto T1 = NT[1].Consume<char32_t>();
			SeqIntervalT Ptr2({ T1, T1 + 1 });
			auto T2 = NT[0].Consume<SeqIntervalT>();
			return T2.AsWrapper().Union(Ptr2.AsWrapper());
		}
		case 63:
		{
			return NT[0].Consume();
		}
		case 4:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			NodeSet Set{ T1, T2 };
			Output.AddComsumeEdge(T1, T2, NT[1].Consume<SeqIntervalT>());
			return Set;
		}
		case 5:
		{
			auto Tar = NT[2].Consume<SeqIntervalT>();
			auto P = MaxIntervalRange().AsWrapper().Remove(Tar);
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			NodeSet Set{ T1, T2 };
			Output.AddComsumeEdge(T1, T2, P);
			return Set;
		}
		case 6:
		{
			return NT[3].Consume();
		}
		case 7:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			NodeSet Last = NT.Datas[1].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Output.AddCapture(Set, Last);
			return Set;
		}
		case 8:
		{
			NodeSet Last1 = NT[0].Consume<NodeSet>();
			NodeSet Last2 = NT[2].Consume<NodeSet>();
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(T1, Last2.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(Last2.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 9:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Tar = NT[0].Consume<char32_t>();
			NodeSet Set{ T1, T2 };
			Output.AddComsumeEdge(T1, T2, { {Tar, Tar + 1} });
			return Set;
		}
		case 50:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			NodeSet Set{ T1, T2 };
			Output.AddComsumeEdge(T1, T2, NT[0].Consume<SeqIntervalT>());
			return Set;
		}
		case 10:
		{
			return NT[0].Consume();
		}
		case 11:
		{
			auto Last1 = NT[0].Consume<NodeSet>();
			auto Last2 = NT[1].Consume<NodeSet>();
			Output.AddComsumeEdge(Last1.Out, Last2.In, {});
			return NodeSet{ Last1.In, Last2.Out };
		}
		case 12:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(T1, T2, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 13:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 14:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, T2, {});
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			return NodeSet{ T1, T2 };
		}
		case 15:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			return NodeSet{ T1, T2 };
		}
		case 16:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(T1, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 17:
		{
			auto T1 = Output.NewNode(NT.TokenIndex.Begin());
			auto T2 = Output.NewNode(NT.TokenIndex.Begin());
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, T2, {});
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 18:
		{
			char32_t Te = NT[0].Consume<char32_t>();
			std::size_t Count = Te - U'0';
			return Count;
		}
		case 19:
		{
			auto Te = NT[0].Consume<std::size_t>();
			Te *= 10;
			auto Te2 = NT[1].Consume<char32_t>();
			Te += Te2 - U'0';
			return Te;
		}
		case 20: // {num}
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), NT[2].Consume<std::size_t>(), {}, {}, false);
		case 25: // {,N}?
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), {}, {}, NT[3].Consume<std::size_t>(), false);
		case 21: // {,N}
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), {}, {}, NT[3].Consume<std::size_t>(), true);
		case 26: // {N,} ?
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), NT[2].Consume<std::size_t>(), {}, {}, false);
		case 22: // {N,}
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), NT[2].Consume<std::size_t>(), {}, {}, true);
		case 27: // {N, N} ?
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), {}, NT[2].Consume<std::size_t>(), NT[4].Consume<std::size_t>(), false);
		case 23: // {N, N}
			return Output.AddCounter(NT.TokenIndex.Begin(), NT[0].Consume<NodeSet>(), {}, NT[2].Consume<std::size_t>(), NT[4].Consume<std::size_t>(), true);
		default:
			assert(false);
			return {};
			break;
		}
	}

	std::any RegTerminalFunction(Potato::SLRX::TElement& Ele, RegLexer& Translater)
	{
		auto& Ref = Translater.GetSpan()[Ele.Shift.TokenIndex].Acceptable;
		if (std::holds_alternative<char32_t>(Ref))
		{
			return std::get<char32_t>(Ref);
		}
		else if (std::holds_alternative<SeqIntervalT>(Ref))
		{
			return std::move(std::get<SeqIntervalT>(Ref));
		}
		else
		{
			assert(false);
			return {};
		}
	}

	std::size_t EpsilonNFA::NewNode(std::size_t TokenIndex)
	{
		std::size_t Index = Nodes.size();
		Nodes.push_back({ Index, {}, TokenIndex });
		return Index;
	}

	EpsilonNFA::NodeSet EpsilonNFA::AddCounter(std::size_t TokenIndex, NodeSet InSideSet, std::optional<std::size_t> Equal, std::optional<std::size_t> Min, std::optional<std::size_t> Max, bool IsGreedy)
	{
		if (Equal.has_value() || (Min.has_value() && Max.has_value() && *Min == *Max))
		{
			std::size_t Tar = Equal.has_value() ? *Equal : *Min;
			if (Tar == 0)
			{
				auto T1 = NewNode(Nodes[InSideSet.In].TokenIndex);
				auto T2 = NewNode(TokenIndex);
				AddComsumeEdge(T1, T2, {});
				return NodeSet{T1, T2};
			}
			else if (Tar == 1)
			{
				return InSideSet;
			}
		}
		else if (Min.has_value() && !Max.has_value() && *Min <= 1)
		{
			std::size_t Index = *Min;
			auto T1 = NewNode(TokenIndex);
			auto T2 = NewNode(TokenIndex);

			if (Index == 0)
			{
				if (IsGreedy)
				{
					AddComsumeEdge(T1, InSideSet.In, {});
					AddComsumeEdge(T1, T2, {});
				}
				else {
					AddComsumeEdge(T1, T2, {});
					AddComsumeEdge(T1, InSideSet.In, {});
				}
			}
			else
				AddComsumeEdge(T1, InSideSet.In, {});
			if (IsGreedy)
			{
				AddComsumeEdge(InSideSet.Out, InSideSet.In, {});
				AddComsumeEdge(InSideSet.Out, T2, {});
			}
			else {
				AddComsumeEdge(InSideSet.Out, T2, {});
				AddComsumeEdge(InSideSet.Out, InSideSet.In, {});
			}
			return NodeSet{ T1, T2 };
		}
		else if ((!Min.has_value() || (Min.has_value() && *Min == 0)) && Max.has_value() && *Max <= 1)
		{
			if (*Max == 0)
			{
				auto T1 = NewNode(TokenIndex);
				auto T2 = NewNode(TokenIndex);
				AddComsumeEdge(T1, T2, {});
				return NodeSet{ T1, T2 };
			}
			else {
				auto T1 = NewNode(TokenIndex);
				auto T2 = NewNode(TokenIndex);
				if (IsGreedy)
				{
					AddComsumeEdge(T1, InSideSet.In, {});
					AddComsumeEdge(T1, T2, {});
				}
				else {
					AddComsumeEdge(T1, T2, {});
					AddComsumeEdge(T1, InSideSet.In, {});
				}
				AddComsumeEdge(InSideSet.Out, T2, {});
				return NodeSet{T1, T2};
			}
		}	

		auto T1 = NewNode(TokenIndex);
		auto T2 = NewNode(TokenIndex);
		auto T3 = NewNode(TokenIndex);
		auto T4 = NewNode(TokenIndex);

		{
			EpsilonNFA::Edge Edge{ {EpsilonNFA::EdgeType::CounterPush}, T2, 0 };
			AddEdge(T1, std::move(Edge));
		}

		AddComsumeEdge(T2, InSideSet.In, {});
		AddComsumeEdge(InSideSet.Out, T3, {});

		auto Start = T3;

		if (Equal.has_value() || Max.has_value())
		{
			Start = NewNode(TokenIndex);
			std::size_t Tar = Equal.has_value() ? *Equal : *Max;
			assert(Tar > 1);

			Counter Tem;
			Misc::SerilizerHelper::CrossTypeSetThrow<RegexOutOfRange>(Tem.Target, Tar - 1, RegexOutOfRange::TypeT::Counter, Tar - 1);

			EpsilonNFA::Edge Edge{ {EpsilonNFA::EdgeType::CounterSmallEqual, Tem}, Start, 0 };
			AddEdge(T3, std::move(Edge));
		}

		{
			EpsilonNFA::Edge Edge{ {EpsilonNFA::EdgeType::CounterAdd}, T2, 0 };
			AddEdge(Start, std::move(Edge));
		}

		Start = T3;

		if (Min.has_value())
		{
			Counter Tem;
			Misc::SerilizerHelper::CrossTypeSetThrow<RegexOutOfRange>(Tem.Target, *Min, RegexOutOfRange::TypeT::Counter, *Min);

			auto New = NewNode(TokenIndex);

			EpsilonNFA::Edge Edge{ {EpsilonNFA::EdgeType::CounterBigEqual, Tem}, New, 0 };
			AddEdge(Start, std::move(Edge));

			Start = New;
		}

		if (Max.has_value())
		{
			Counter Tem;
			Misc::SerilizerHelper::CrossTypeSetThrow<RegexOutOfRange>(Tem.Target, *Max, RegexOutOfRange::TypeT::Counter, *Max);

			auto New = NewNode(TokenIndex);
			EpsilonNFA::Edge Edge{ {EpsilonNFA::EdgeType::CounterSmallEqual, Tem}, New, 0 };
			AddEdge(Start, std::move(Edge));

			Start = New;
		}

		{
			EpsilonNFA::Edge Edge{ {EpsilonNFA::EdgeType::CounterPop}, T4, 0 };
			AddEdge(Start, std::move(Edge));
		}

		if (!IsGreedy)
		{
			auto& Ref = Nodes[T3].Edges;
			assert(Ref.size() == 2);
			std::swap(Ref[0], Ref[1]);
		}

		return NodeSet{ T1, T4 };
	}

	void EpsilonNFA::AddComsumeEdge(std::size_t From, std::size_t To, std::vector<IntervalT> Acceptable) {
		Edge NewEdge;
		NewEdge.Property.Type = EdgeType::Consume;
		NewEdge.Property.Datas = std::move(Acceptable);
		NewEdge.ShiftNode = To;
		AddEdge(From, std::move(NewEdge));
	}

	void EpsilonNFA::AddAcceptableEdge(std::size_t From, std::size_t To, Accept Data)
	{
		Edge NewEdge;
		NewEdge.Property.Type = EdgeType::Acceptable;
		NewEdge.ShiftNode = To;
		NewEdge.Property.Datas = std::move(Data);
		AddEdge(From, std::move(NewEdge));
	}

	void EpsilonNFA::AddCapture(NodeSet OutsideSet, NodeSet InsideSet)
	{
		Edge NewEdge;
		NewEdge.ShiftNode = InsideSet.In;
		NewEdge.Property.Type = EdgeType::CaptureBegin;
		AddEdge(OutsideSet.In, NewEdge);

		NewEdge.ShiftNode = OutsideSet.Out;
		NewEdge.Property.Type = EdgeType::CaptureEnd;
		AddEdge(InsideSet.Out, NewEdge);
	}

	void EpsilonNFA::AddEdge(std::size_t FormNodeIndex, Edge Edge)
	{
		assert(FormNodeIndex < Nodes.size());
		auto& Cur = Nodes[FormNodeIndex];
		Edge.UniqueID = 0;
		Cur.Edges.push_back(std::move(Edge));
	}

	void EpsilonNFA::Link(EpsilonNFA const& OtherTable, bool ThisHasHigherPriority)
	{
		if (!OtherTable.Nodes.empty())
		{
			for (auto& Ite : Nodes)
			{
				for (auto& Ite2 : Ite.Edges)
				{
					Ite2.UniqueID += 1;
				}
			}
			Nodes.reserve(Nodes.size() + OtherTable.Nodes.size());
			std::size_t NodeOffset = Nodes.size();
			for (auto& Ite : OtherTable.Nodes)
			{
				Node NewNode{Ite.Index + NodeOffset, {}};
				NewNode.Edges.reserve(Ite.Edges.size());
				for (auto& Ite2 : Ite.Edges)
				{
					auto NewEdges = Ite2;
					NewEdges.ShiftNode += NodeOffset;
					NewNode.Edges.push_back(NewEdges);
				}
				Nodes.push_back(std::move(NewNode));
			}
			Edge NewEdges;
			NewEdges.Property.Type = EdgeType::Consume;
			NewEdges.Property.Datas = std::vector<IntervalT>{};
			NewEdges.ShiftNode = NodeOffset;
			if(ThisHasHigherPriority)
				Nodes[0].Edges.emplace_back(NewEdges);
			else {
				auto& Ref = Nodes[0].Edges;
				Ref.emplace(Ref.begin(), NewEdges);
			}	
		}
	}

	std::optional<std::size_t> CreateUnfaTable(RegLexer& Translater, EpsilonNFA& Output, Accept AcceptData)
	{
		auto N1 = Output.NewNode(0);
		auto N2 = Output.NewNode(0);
		auto N3 = Output.NewNode(0);
		Output.AddComsumeEdge(N2, N3, { {EndOfFile(), EndOfFile() + 1} });
		auto Wrapper = RexSLRXWrapper();

		SLRX::SymbolProcessor Pro(Wrapper);
		std::size_t TokenIndex = 0;
		auto Spans = Translater.GetSpan();
		for (auto& Ite : Translater.GetSpan())
		{
			if (!Pro.Consume(*Ite.Value, TokenIndex))
				return { Ite.TokenIndex };
			TokenIndex++;
		}
		if (!Pro.EndOfFile())
			return Spans.empty() ? 0 : Spans.rbegin()->TokenIndex;

		EpsilonNFA::NodeSet Set = SLRX::ProcessParsingStepWithOutputType<EpsilonNFA::NodeSet>(Pro.GetSteps(),
			[&](SLRX::VariantElement Elements)-> std::any {
				if (Elements.IsTerminal())
					return RegTerminalFunction(Elements.AsTerminal(), Translater);
				else
					return RegNoTerminalFunction(Elements.AsNoTerminal(), Output);
			}
		);
		Output.AddComsumeEdge(N1, Set.In, {});
		Output.AddAcceptableEdge(Set.Out, N2, AcceptData);
		return {};
	}

	EpsilonNFA EpsilonNFA::Create(std::u8string_view Str, bool IsRaw, Accept AcceptData)
	{
		EpsilonNFA Result;
		RegLexer Tran(IsRaw);
		std::size_t TokenIndex = 0;
		auto Span = std::span(Str);
		char32_t Tembuffer;
		std::span<char32_t> OutSpan = {&Tembuffer, 1};
		while (!Span.empty())
		{
			auto Re = StrEncode::CharEncoder<char8_t, char32_t>::EncodeOnceUnSafe(Span, OutSpan);
			assert(Re);
			if(!Tran.Consume(Tembuffer, TokenIndex))
				throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, std::u8string{Str}, TokenIndex };
			Span = Span.subspan(Re.SourceSpace);
			TokenIndex += Re.SourceSpace;
		}
		if (!Tran.EndOfFile())
		{
			throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, std::u8string{Str}, TokenIndex };
		}

		auto Re = CreateUnfaTable(Tran, Result, AcceptData);
		if (Re.has_value())
		{
			throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, std::u8string{Str}, *Re };
		}
		return Result;
	}

	struct ExpandResult
	{
		std::vector<std::size_t> ContainNodes;
		std::vector<NFA::Edge> Edges;
	};

	struct SearchCore
	{
		std::size_t ToNode;
		
		struct Element
		{
			std::size_t From;
			std::size_t To;
			EpsilonNFA::PropertyT Propertys;
		};

		std::vector<Element> Propertys;

		struct UniqueID
		{
			bool HasUniqueID;
			std::size_t UniqueID;
		};
		std::optional<UniqueID> UniqueID;
		void AddUniqueID(std::size_t ID)
		{
			if (!UniqueID.has_value())
			{
				UniqueID = {true, ID };
			}
			else {
				if (UniqueID->HasUniqueID && UniqueID->UniqueID != ID)
					UniqueID->HasUniqueID = false;
			}
		}
	};

	bool CheckProperty(std::span<SearchCore::Element const> Ref)
	{
		std::vector<std::size_t> Temp;
		for (auto& Ite : Ref)
		{
			switch (Ite.Propertys.Type)
			{
			case EpsilonNFA::EdgeType::CounterPush:
				Temp.push_back(0);
				break;
			case EpsilonNFA::EdgeType::CounterAdd:
				if (!Temp.empty())
					*Temp.rbegin() += 1;
				break;
			case EpsilonNFA::EdgeType::CounterPop:
				if (!Temp.empty())
					Temp.pop_back();
				break;
			case EpsilonNFA::EdgeType::CounterEqual:
				if (!Temp.empty())
				{
					auto Last = *Temp.rbegin();
					return Last == std::get<Counter>(Ite.Propertys.Datas).Target;
				}
				break;
			case EpsilonNFA::EdgeType::CounterSmallEqual:
				if (!Temp.empty())
				{
					auto Last = *Temp.rbegin();
					return Last <= std::get<Counter>(Ite.Propertys.Datas).Target;
				}
				break;
			case EpsilonNFA::EdgeType::CounterBigEqual:
				if (!Temp.empty())
				{
					auto Last = *Temp.rbegin();
					return Last >= std::get<Counter>(Ite.Propertys.Datas).Target;
				}
				break;
			default:
				break;
			}
			return true;
		}
		return true;
	}

	ExpandResult SearchEpsilonNode(EpsilonNFA const& Input, std::size_t SearchNode)
	{
		ExpandResult Result;

		std::vector<SearchCore> SearchStack;
		
		SearchStack.push_back({ SearchNode, {}, {}});
		Result.ContainNodes.push_back(SearchNode);
		while (!SearchStack.empty())
		{
			auto Cur = std::move(*SearchStack.rbegin());
			SearchStack.pop_back();
			assert(Input.Nodes.size() > Cur.ToNode);
			auto& Node = Input.Nodes[Cur.ToNode];
			for (auto EdgeIte = Node.Edges.rbegin(); EdgeIte != Node.Edges.rend(); ++EdgeIte)
			{
				auto& Ite = *EdgeIte;

				bool IsLast = ( EdgeIte  + 1 == Node.Edges.rend());

				if (Ite.Property.Type == EpsilonNFA::EdgeType::Consume && !std::get<std::vector<IntervalT>>(Ite.Property.Datas).empty())
				{
					if (!CheckProperty(std::span(Cur.Propertys)))
						continue;

					NFA::Edge NewEdge;
					NewEdge.ToNode = Ite.ShiftNode;

					Cur.AddUniqueID(Ite.UniqueID);

					assert(Cur.UniqueID.has_value());

					if (Cur.UniqueID->HasUniqueID)
						NewEdge.UniqueID = Cur.UniqueID->UniqueID;
					NewEdge.ConsumeChars = std::get<std::vector<IntervalT>>(Ite.Property.Datas);
					for (auto& Ite : Cur.Propertys)
					{
						if(Ite.Propertys.Type != EpsilonNFA::EdgeType::Consume)
							NewEdge.Propertys.push_back(Ite.Propertys);
					}

					Result.Edges.push_back(std::move(NewEdge));

				}
				else {

					auto CircleDetected = std::find_if(Cur.Propertys.begin(), Cur.Propertys.end(), [&](SearchCore::Element& Ele){
						return Ele.From == Ite.ShiftNode;
					});

					if (CircleDetected != Cur.Propertys.end())
					{
						volatile int i = 0;
						throw Exception::CircleShifting{};
					}

					Result.ContainNodes.push_back(Ite.ShiftNode);

					SearchCore NewOne;
					if (IsLast)
						NewOne = std::move(Cur);
					else
						NewOne = Cur;

					NewOne.AddUniqueID(Ite.UniqueID);

					NewOne.Propertys.push_back({ NewOne.ToNode, Ite.ShiftNode, Ite.Property });
					NewOne.ToNode = Ite.ShiftNode;
					SearchStack.push_back(std::move(NewOne));
				}
			}
		}
		return Result;
	}

	struct NodeAllocator
	{
		std::vector<std::vector<std::size_t>> Mapping;
		std::tuple<std::size_t, bool> FindOrAdd(std::span<std::size_t const> Ref) {
			auto Res = Find(Ref);
			if(Res.has_value())
				return {*Res, false};
			else {
				std::size_t ID = Mapping.size();
				std::vector<std::size_t> NewSet{ Ref .begin(), Ref.end() };
				Mapping.push_back(std::move(NewSet));
				return {ID, true};
			}
		}
		std::optional<std::size_t> Find(std::span<std::size_t const> Ref) const
		{
			for (std::size_t I = 0; I < Mapping.size(); ++I)
			{
				auto TRef = std::span(Mapping[I]);
				if(TRef.size() == Ref.size() && std::equal(TRef.begin(), TRef.end(), Ref.begin(), Ref.end()))
					return I;
			}
			return {};
		}
		std::size_t Size() const { return Mapping.size(); }
		std::span<std::size_t const> Read(std::size_t Require)  const
		{
			assert(Mapping.size() > Require);
			return std::span(Mapping[Require]);
		}
	};

	NFA::NFA(EpsilonNFA const& Table)
	{

		struct NodeStorage
		{
			std::size_t FirstStartNode;
			std::vector<Edge> Edges;
		};

		std::vector<NodeStorage> NodesFastMapping;

		NodesFastMapping.push_back({0, {}});

		for (std::size_t Index = 0; Index < NodesFastMapping.size(); ++Index)
		{
			auto SearchNode = NodesFastMapping[Index].FirstStartNode;
			auto SearchResult = SearchEpsilonNode(Table, SearchNode);
			for (auto& Ite : SearchResult.Edges)
			{
				std::optional<std::size_t> FindedState;
				for (std::size_t Index2 = 0; Index2 < NodesFastMapping.size(); ++Index2)
				{
					if (Ite.ToNode == NodesFastMapping[Index2].FirstStartNode)
					{
						FindedState = Index2;
						break;
					}
				}
				if (!FindedState.has_value())
				{
					FindedState = NodesFastMapping.size();
					NodesFastMapping.push_back({ Ite.ToNode, {}});
				}
				assert(FindedState.has_value());
				Ite.ToNode = *FindedState;
			}
			NodesFastMapping[Index].Edges = std::move(SearchResult.Edges);
		}

		for (auto& Ite : NodesFastMapping)
		{
			Nodes.push_back({std::move(Ite.Edges)});
		}
	}

	struct TemporaryProperty
	{
		std::vector<EpsilonNFA::PropertyT> Propertys;
		std::size_t OriginalFrom;
		std::size_t OriginalTo;
		std::size_t UniqueID;
		bool HasCounter = false;
		bool HasAcceptable = false;
		bool operator ==(TemporaryProperty const&) const = default;
	};

	struct TemporaryEdge
	{
		std::vector<TemporaryProperty> Propertys;
		std::vector<IntervalT> ConsumeSymbols;
		std::vector<std::size_t> ToIndexs;
		bool operator ==(TemporaryEdge const&) const = default;
	};

	struct TemporaryNode
	{
		std::vector<TemporaryEdge> KeepAcceptEdges;
		std::vector<TemporaryEdge> Edges;
		bool HasAccept = false;
	};

	void CollectEdgeProperty(std::size_t SourceIndex, NFA::Edge const& Source, TemporaryProperty& Target)
	{
		Target.OriginalFrom = SourceIndex;
		Target.OriginalTo = Source.ToNode;
		Target.UniqueID = Source.UniqueID;
		Target.HasAcceptable = false;
		Target.HasCounter = false;
		Target.Propertys.reserve(Source.Propertys.size());
		for (auto& Ite : Source.Propertys)
		{
			switch (Ite.Type)
			{
			case EpsilonNFA::EdgeType::Acceptable:
				Target.HasAcceptable = true;
				break;
			case EpsilonNFA::EdgeType::CounterBigEqual:
			case EpsilonNFA::EdgeType::CounterEqual:
			case EpsilonNFA::EdgeType::CounterSmallEqual:
				Target.HasCounter = true;
				break;
			default:
				break;
			}
			Target.Propertys.push_back(Ite);
		}
	}

	void CollectTemporaryProperty(NFA const& Input, std::span<std::size_t const> Indexs, TemporaryNode& Output)
	{
		std::vector<std::size_t> RemovedUniqueID;
		bool ReadAccept = false;
		bool LastAccept = false;
		for (auto IndexIte : Indexs)
		{
			auto& NodeRef = Input.Nodes[IndexIte];
			for (auto& Ite : NodeRef.Edges)
			{

				TemporaryProperty Temporary;
				CollectEdgeProperty(IndexIte, Ite, Temporary);

				TemporaryEdge Edges;
				Edges.ConsumeSymbols = Ite.ConsumeChars;

				if (Temporary.HasAcceptable)
				{
					ReadAccept = true;
					LastAccept = true;
					Output.HasAccept = true;
					RemovedUniqueID.push_back(Ite.UniqueID);
				}
				else {
					if (LastAccept)
						Output.KeepAcceptEdges = Output.Edges;
					LastAccept = false;
					if (ReadAccept)
					{
						if (std::find(RemovedUniqueID.begin(), RemovedUniqueID.end(), Ite.UniqueID) == RemovedUniqueID.end())
						{
							TemporaryEdge Edges;
							Edges.ConsumeSymbols = Ite.ConsumeChars;
							Edges.Propertys.push_back(Temporary); 
							Output.KeepAcceptEdges.push_back(std::move(Edges));
						}
					}
				}
				Edges.Propertys.push_back(std::move(Temporary));
				Output.Edges.push_back(std::move(Edges));
			}
		}
	}

	void UnionEdges(std::vector<TemporaryEdge>& TempBuffer, std::vector<TemporaryEdge>& Output)
	{
		if (Output.size() >= 2)
		{
			TempBuffer.clear();
			bool Change = true;
			while(Change)
			{
				Change = false;
				std::span<TemporaryEdge> Span = std::span(Output);

				while (!Span.empty())
				{
					auto Top = std::move(Span[0]);
					Span = Span.subspan(1);

					if (Top.ConsumeSymbols.empty())
						continue;

					for (auto& Ite : Span)
					{
						auto Result = Misc::SequenceIntervalWrapper{ Top.ConsumeSymbols }.Collision(Ite.ConsumeSymbols);
						if (!Result.middle.empty())
						{
							TemporaryEdge New;
							New.ConsumeSymbols = Result.middle;
							New.Propertys = Top.Propertys;
							New.Propertys.insert(New.Propertys.end(), Ite.Propertys.begin(), Ite.Propertys.end());
							TempBuffer.push_back(std::move(New));

							Top.ConsumeSymbols = std::move(Result.left);
							Ite.ConsumeSymbols = std::move(Result.right);

							Change = true;

							if (Top.ConsumeSymbols.empty())
								break;
						}
					}

					if (!Top.ConsumeSymbols.empty())
						TempBuffer.push_back(std::move(Top));
				}
				std::swap(TempBuffer, Output);
				TempBuffer.clear();
			}
		}
	}

	struct DFASearchCore
	{
		std::size_t Index;
		TemporaryNode TempRawNodes;
	};

	void ClassifyNodeIndes(NFA const& Table, std::vector<TemporaryEdge>& TempraryEdges, NodeAllocator& Mapping, std::vector<DFASearchCore>& SeachStack)
	{
		if (!TempraryEdges.empty())
		{
			
			for (auto& Ite : TempraryEdges)
			{
				std::size_t ToNodeBuffer = 1;
				for (auto& Ite2 : Ite.Propertys)
				{
					if(Ite2.HasCounter)
						ToNodeBuffer *= 2;
				}
				std::vector<std::vector<std::size_t>> TemporaryMapping;
				TemporaryMapping.reserve(ToNodeBuffer);
				TemporaryMapping.push_back({});
				for (auto& Ite2 : Ite.Propertys)
				{
					if (!Ite2.HasCounter)
					{
						for (auto& Ite3 : TemporaryMapping)
						{
							if(std::find(Ite3.begin(), Ite3.end(), Ite2.OriginalTo) == Ite3.end())
								Ite3.push_back(Ite2.OriginalTo);
						}
					}
					else {
						std::size_t Cur = TemporaryMapping.size();
						for (std::size_t I = 0; I < Cur; ++I)
						{
							auto New = TemporaryMapping[I];
							if(std::find(New.begin(), New.end(), Ite2.OriginalTo) == New.end())
								New.push_back(Ite2.OriginalTo);
							TemporaryMapping.push_back({std::move(New)});
						}
					}
				}
				Ite.ToIndexs.resize(ToNodeBuffer);
				for (std::size_t I = 0; I < TemporaryMapping.size(); ++I)
				{
					assert(I < Ite.ToIndexs.size());
					auto& Ref = TemporaryMapping[I];
					if(Ref.empty())
						Ite.ToIndexs[I] = std::numeric_limits<std::size_t>::max();
					else {
						auto [ID, IsNew] = Mapping.FindOrAdd(std::span(Ref));
						if (IsNew)
						{
							DFASearchCore New;
							New.Index = ID;
							CollectTemporaryProperty(Table, std::span(Ref), New.TempRawNodes);
							SeachStack.push_back(std::move(New));
						}
						Ite.ToIndexs[I] = ID;
					}
				}
			}
		}
	}

	std::vector<DFA::Edge> CollectDFAEdge(std::vector<TemporaryEdge>& Source)
	{

		using EdgeType = EpsilonNFA::EdgeType;
		using ActionT = DFA::ActionT;

		std::vector<DFA::Edge> Target;
		Target.reserve(Source.size());
		for (auto& Ite : Source)
		{
			DFA::Edge NewOne;
			NewOne.ConsumeSymbols = std::move(Ite.ConsumeSymbols);
			NewOne.ToIndex = std::move(Ite.ToIndexs);
			NewOne.ContentNeedChange = false;
			for (auto& Ite2 : Ite.Propertys)
			{
				NewOne.List.push_back(ActionT::ContentBegin);
				StandardT From;
				StandardT To;
				Misc::SerilizerHelper::CrossTypeSetThrow<Exception::RegexOutOfRange>(From, Ite2.OriginalFrom, RegexOutOfRange::TypeT::ContentIndex, Ite2.OriginalFrom);
				Misc::SerilizerHelper::CrossTypeSetThrow<Exception::RegexOutOfRange>(To, Ite2.OriginalTo, RegexOutOfRange::TypeT::ContentIndex, Ite2.OriginalTo);
				NewOne.Parameter.push_back(From);
				NewOne.Parameter.push_back(To);
				if(From != To)
					NewOne.ContentNeedChange = true;
				for (auto& Ite3 : Ite2.Propertys)
				{
					switch (Ite3.Type)
					{
					case EdgeType::Consume:
						assert(false);
						break;
					case EdgeType::Acceptable:
					{
						auto Accr = std::get<Accept>(Ite3.Datas);
						NewOne.Parameter.push_back(Accr.Mask);
						NewOne.Parameter.push_back(Accr.SubMask);
						NewOne.List.push_back(ActionT::Accept);
						break;
					}
					case EdgeType::CaptureBegin:
						NewOne.List.push_back(ActionT::CaptureBegin);
						NewOne.ContentNeedChange = true;
						break;
					case EdgeType::CaptureEnd:
						NewOne.List.push_back(ActionT::CaptureEnd);
						NewOne.ContentNeedChange = true;
						break;
					case EdgeType::CounterAdd:
						NewOne.List.push_back(ActionT::CounterAdd);
						NewOne.ContentNeedChange = true;
						break;
					case EdgeType::CounterBigEqual:
						NewOne.List.push_back(ActionT::CounterBigEqual);
						NewOne.Parameter.push_back(std::get<Counter>(Ite3.Datas).Target);
						break;
					case EdgeType::CounterEqual:
						NewOne.List.push_back(ActionT::CounterEqual);
						NewOne.Parameter.push_back(std::get<Counter>(Ite3.Datas).Target);
						break;
					case EdgeType::CounterPop:
						NewOne.List.push_back(ActionT::CounterPop);
						NewOne.ContentNeedChange = true;
						break;
					case EdgeType::CounterPush:
						NewOne.List.push_back(ActionT::CounterPush);
						NewOne.ContentNeedChange = true;
						break;
					case EdgeType::CounterSmallEqual:
						NewOne.List.push_back(ActionT::CounterSmaller);
						NewOne.Parameter.push_back(std::get<Counter>(Ite3.Datas).Target);
						break;
					default:
						assert(false);
						break;
					}
				}
				NewOne.List.push_back(ActionT::ContentEnd);
			}
			Target.push_back(std::move(NewOne));
		}
		return Target;
	}

	DFA::DFA(NFA const& Input)
	{

		assert(!Input.Nodes.empty());

		using EdgeT = EpsilonNFA::EdgeType;

		NodeAllocator NodeAll;


		std::vector<DFASearchCore> SearchingStack;

		{
			DFASearchCore Startup;
			Startup.Index = 0;
			auto [ID, New] = NodeAll.FindOrAdd({&Startup.Index, 1});
			assert(New);
			CollectTemporaryProperty(Input, NodeAll.Read(ID), Startup.TempRawNodes);
			SearchingStack.push_back(std::move(Startup));
		}

		std::vector<TemporaryNode> TempNodes;
		std::vector<TemporaryEdge> TempBuffer;

		while (!SearchingStack.empty())
		{
			auto Top = *SearchingStack.rbegin();
			SearchingStack.pop_back();

			if(TempNodes.size() <= Top.Index)
				TempNodes.resize(Top.Index + 1);

			UnionEdges(TempBuffer, Top.TempRawNodes.KeepAcceptEdges);
			UnionEdges(TempBuffer, Top.TempRawNodes.Edges);

			ClassifyNodeIndes(Input, Top.TempRawNodes.KeepAcceptEdges, NodeAll, SearchingStack);
			ClassifyNodeIndes(Input, Top.TempRawNodes.Edges, NodeAll, SearchingStack);

			if (Top.TempRawNodes.KeepAcceptEdges == Top.TempRawNodes.Edges)
				Top.TempRawNodes.KeepAcceptEdges.clear();

			TempNodes[Top.Index] = std::move(Top.TempRawNodes);
		
		}

		{ // shared empty node with accept node
			std::optional<std::size_t> FirstEmpty;

			for (std::size_t I1 = 0; I1 < TempNodes.size(); ++I1)
			{
				auto& Ref = TempNodes[I1];
				if (Ref.Edges.empty())
				{
					FirstEmpty = I1;
					break;
				}
			}

			if (FirstEmpty.has_value())
			{
				for (std::size_t I1 = *FirstEmpty + 1; I1 < TempNodes.size();)
				{
					auto& Ref = TempNodes[I1];
					if (Ref.Edges.empty())
					{
						for (std::size_t I2 = 0; I2 < TempNodes.size(); ++I2)
						{
							auto& Tar = TempNodes[I2];
							for (auto& Ite1 : Tar.Edges)
							{
								for (auto& Ite2 : Ite1.ToIndexs)
								{
									if (Ite2 == I1)
										Ite2 = *FirstEmpty;
									else if (Ite2 > I1)
									{
										Ite2 -= 1;
									}
								}
							}
							for (auto& Ite1 : Tar.KeepAcceptEdges)
							{
								for (auto& Ite2 : Ite1.ToIndexs)
								{
									if (Ite2 == I1)
										Ite2 = *FirstEmpty;
									else if (Ite2 > I1)
									{
										Ite2 -= 1;
									}
								}
							}
						}
						TempNodes.erase(TempNodes.begin() + I1);
					}
					else {
						++I1;
					}
				}
			}
		}

		{ // remove no content change property

			struct TemPro
			{
				std::size_t Count = 0;
				bool HasContentChangedEdge = false;
				std::size_t LastNodeIndex = 0;
			};

			std::vector<TemPro> Pros;
			Pros.resize(Input.Nodes.size());
			for(std::size_t I = 0; I < Input.Nodes.size(); ++I)
			{
				auto& Cur = Input.Nodes[I];
				for (auto& Ite : Cur.Edges)
				{
					bool HasContentChangedEdge = false;
					for (auto& Ite2 : Ite.Propertys)
					{
						switch (Ite2.Type)
						{
						case EdgeT::CaptureBegin:
						case EdgeT::CaptureEnd:
						case EdgeT::CounterPush:
						case EdgeT::CounterPop:
						case EdgeT::CounterAdd:
						case EdgeT::Acceptable:
							HasContentChangedEdge = true;
							break;
						default:
							break;
						}
						if(HasContentChangedEdge)
							break;
					}
					auto& Ref = Pros[Ite.ToNode];
					Ref.Count += 1;
					Ref.HasContentChangedEdge |= HasContentChangedEdge;
					Ref.LastNodeIndex = I;
				}
			}

			std::vector<std::tuple<std::size_t, std::size_t>> NoContentChangeNode;

			for (std::size_t I = 0; I < Pros.size(); ++I)
			{
				auto& Cur = Pros[I];
				if(Cur.Count == 1 && !Cur.HasContentChangedEdge)
					NoContentChangeNode.push_back({I, Cur.LastNodeIndex});
			}

			if (!NoContentChangeNode.empty())
			{
				bool Change = true;
				while (Change)
				{
					Change = false;
					for (auto Ite = NoContentChangeNode.begin(); Ite != NoContentChangeNode.end() && !Change; ++Ite)
					{
						auto& [T1, F1] = *Ite;
						for (auto Ite2 = Ite + 1; Ite2 != NoContentChangeNode.end(); ++Ite2)
						{
							auto [T2, F2] = *Ite2;
							if (F1 == T2)
							{
								F1 = F2;
								Change = true;
								break;
							}
						}
					}
				}

				for (auto& Ite : TempNodes)
				{
					for (auto& Ite2 : Ite.Edges)
					{
						for (auto& Ite3 : Ite2.Propertys)
						{
							auto FindIte = std::find_if(NoContentChangeNode.begin(), NoContentChangeNode.end(), [&](auto Value){
								return std::get<0>(Value) == Ite3.OriginalTo;
							});

							if(FindIte != NoContentChangeNode.end())
								Ite3.OriginalTo = std::get<1>(*FindIte);

							auto FindIte2 = std::find_if(NoContentChangeNode.begin(), NoContentChangeNode.end(), [&](auto Value) {
								return std::get<0>(Value) == Ite3.OriginalFrom;
								});

							if (FindIte2 != NoContentChangeNode.end())
								Ite3.OriginalFrom = std::get<1>(*FindIte2);
						}
					}
				}
			}

		}

		Nodes.reserve(TempNodes.size());

		for (auto& Ite : TempNodes)
		{
			Node NewNode;
			NewNode.Edges = CollectDFAEdge(Ite.Edges);
			NewNode.KeepAcceptableEdges = CollectDFAEdge(Ite.KeepAcceptEdges);
			Nodes.push_back(std::move(NewNode));
		}
	}

	std::size_t CalConsumeSymbolSize(std::span<IntervalT const> Symbols)
	{
		std::size_t Count = 0;
		for (auto& Ite : Symbols)
		{
			if(Ite.end - Ite.start == 1)
				Count += 1;
			else
				Count += 2;
		}
		return Count;
	}

	void CalEdgeSize(Misc::SerilizerHelper::Predicter<StandardT>& Pre, std::span<DFA::Edge const> Symbols)
	{
		for (auto& Ite2 : Symbols)
		{
			Pre.WriteObject<TableWrapper::ZipEdge>();
			Pre.WriteObjectArray(std::span(Ite2.List));
			Pre.WriteObjectArray(std::span(Ite2.Parameter));
			Pre.WriteObjectArray<StandardT>(Ite2.ToIndex.size());
			Pre.WriteObjectArray<TableWrapper::ZipChar>(CalConsumeSymbolSize(Ite2.ConsumeSymbols));
		}
	}

	std::size_t TranslateIntervalT(std::span<IntervalT const> Source, std::vector<TableWrapper::ZipChar>& Output)
	{
		auto OldSize = Output.size();
		for (auto& Ite : Source)
		{
			if (Ite.start + 1 == Ite.end)
				Output.push_back({ true, Ite.start });
			else {
				Output.push_back({ false, Ite.start });
				Output.push_back({ false, Ite.end });
			}
		}
		return Output.size() - OldSize;
	}

	std::vector<std::size_t> Records; 

	std::size_t TableWrapper::CalculateRequireSpaceWithStanderT(DFA const& Tab)
	{
		Misc::SerilizerHelper::Predicter<StandardT> Pred;
		Pred.WriteElement();
		for (auto& Ite : Tab.Nodes)
		{
			Records.push_back(Pred.RequireSpace());
			Pred.WriteObject<ZipNode>();
			CalEdgeSize(Pred, std::span(Ite.Edges));
			CalEdgeSize(Pred, std::span(Ite.KeepAcceptableEdges));
		}
		return Pred.RequireSpace();
	}

	std::size_t TableWrapper::SerializeTo(DFA const& Tab, std::span<StandardT> Source)
	{

		std::vector<std::size_t> NodeOffset;


		assert(CalculateRequireSpaceWithStanderT(Tab) <= Source.size());
		std::vector<std::size_t> NodeOffetMapping;
		NodeOffetMapping.reserve(Tab.Nodes.size());

		struct ToIndexRecord
		{
			std::span<StandardT> Target;
			std::span<std::size_t const> Source;
		};

		std::vector<ToIndexRecord> Records;

		std::vector<ZipChar> TempZipBuffer;
		{
			std::size_t Count = 0;
			std::size_t Record = 0;
			for (auto& Ite : Tab.Nodes)
			{
				for (auto& Ite2 : Ite.Edges)
				{
					Count += Ite2.ToIndex.size();
					Record += 1;
				}
				for (auto& Ite2 : Ite.KeepAcceptableEdges)
				{
					Count += Ite2.ToIndex.size();
					Record += 1;
				}
			}
			Records.reserve(Count);
		}

		Misc::SerilizerHelper::SpanReader<StandardT> Serilizer(Source);
		auto NodeNumber = Serilizer.NewElement();
		Serilizer.CrossTypeSetThrow<Exception::RegexOutOfRange>(*NodeNumber, Tab.Nodes.size(), RegexOutOfRange::TypeT::NodeCount, Tab.Nodes.size());
		int NodeIndex =  0;
		for (auto& Ite : Tab.Nodes)
		{
			NodeOffset.push_back(Serilizer.GetIteSpacePositon());
			NodeOffetMapping.push_back(Serilizer.GetIteSpacePositon());
			auto Node = Serilizer.NewObject<ZipNode>();
			Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Node->EdgeCount, Ite.Edges.size(), RegexOutOfRange::TypeT::EdgeCount, Ite.Edges.size());
			std::size_t EdgeStartOffset = Serilizer.GetIteSpacePositon();
			int EdgeIndex = 0;
			auto SerilizeEdge = [&](std::span<DFA::Edge const> Edges)
			{
				EdgeIndex += 1;
				ZipEdge* LastEdge = nullptr;
				std::size_t LastEdgeBegin = 0;
				for (auto& Ite : Edges)
				{
					std::size_t EdgeOffset  = Serilizer.GetIteSpacePositon();
					auto Edge = Serilizer.NewObject<ZipEdge>();
					assert(Edge != nullptr);
					if (LastEdge != nullptr)
					{
						Serilizer.CrossTypeSetThrow<RegexOutOfRange>(LastEdge->NextEdgeOffset, EdgeOffset - LastEdgeBegin, RegexOutOfRange::TypeT::EdgeOffset, EdgeOffset - LastEdgeBegin);
					}
					LastEdge = Edge;
					LastEdgeBegin = Serilizer.GetIteSpacePositon();
					Edge->NeedContentChange = Ite.ContentNeedChange;
					Edge->NextEdgeOffset = 0;
					
					TempZipBuffer.clear();
					TranslateIntervalT(std::span(Ite.ConsumeSymbols), TempZipBuffer);
					Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Edge->ConsumeSymbolCount, TempZipBuffer.size(), RegexOutOfRange::TypeT::AcceptableCharCount, TempZipBuffer.size());
					Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Edge->ToIndexCount, Ite.ToIndex.size(), RegexOutOfRange::TypeT::ToIndeCount, Ite.ToIndex.size());
					Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Edge->ActionCount, Ite.List.size(), RegexOutOfRange::TypeT::ActionCount, Ite.List.size());
					Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Edge->ParmaterCount, Ite.Parameter.size(), RegexOutOfRange::TypeT::ActionParameterCount, Ite.Parameter.size());
					Serilizer.NewObjectArray(std::span(TempZipBuffer));
					Serilizer.NewObjectArray(std::span(Ite.List));
					Serilizer.NewObjectArray(std::span(Ite.Parameter));
					auto Span = Serilizer.NewObjectArray<StandardT>(Ite.ToIndex.size());
					Records.push_back({*Span, std::span(Ite.ToIndex)});
				}
			};
			NodeIndex += 1;

			SerilizeEdge(Ite.Edges);
			

			if (Ite.KeepAcceptableEdges.empty())
			{
				Node->AcceptEdgeOffset = 0;
			}
			else {
				std::size_t Offset = Serilizer.GetIteSpacePositon();
				Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Node->AcceptEdgeOffset, Offset - EdgeStartOffset, RegexOutOfRange::TypeT::EdgeOffset, Offset - EdgeStartOffset);
				EdgeIndex = 0;
				SerilizeEdge(Ite.KeepAcceptableEdges);
			}
		}

		for (auto& Ite : Records)
		{
			assert(Ite.Source.size() == Ite.Target.size());
			for (std::size_t I1 = 0; I1 < Ite.Source.size(); ++I1)
			{
				auto ID = Ite.Source[I1];
				if (ID == std::numeric_limits<std::size_t>::max())
				{
					Ite.Target[I1] = std::numeric_limits<StandardT>::max();
				}
				else {
					Serilizer.CrossTypeSetThrow<RegexOutOfRange>(Ite.Target[I1], NodeOffset[ID], RegexOutOfRange::TypeT::NodeOffset, ID);
				}
			}
		}

		return Serilizer.GetIteSpacePositon();
	}

	std::vector<StandardT> TableWrapper::Create(DFA const& Tab)
	{
		std::vector<StandardT> Buffer;
		Buffer.resize(CalculateRequireSpaceWithStanderT(Tab));
		SerializeTo(Tab, std::span(Buffer));
		return Buffer;
	}

	template<typename Tar> Misc::IndexSpan<> FindBlockIndex(std::span<Tar const> Blocks, std::size_t RequireBlockID) 
		requires(std::is_same_v<Tar, ProcessorContent::CaptureBlock> || std::is_same_v<Tar, ProcessorContent::CounterBlock>)
	{
		std::size_t Start = 0;
		for (; Start != Blocks.size(); ++Start)
		{
			if(Blocks[Start].BlockIndex == RequireBlockID)
				break;
		}

		std::size_t End = Start;
		for (; End < Blocks.size(); ++End)
		{
			if (Blocks[End].BlockIndex != RequireBlockID)
				break;
		}

		return {Start, End - Start};
	}

	Misc::IndexSpan<> ProcessorContent::FindCaptureIndex(std::size_t BlockIndex) const
	{
		return FindBlockIndex(std::span(CaptureBlocks), BlockIndex);
	}

	Misc::IndexSpan<> ProcessorContent::FindCounterIndex(std::size_t BlockIndex) const
	{
		return FindBlockIndex(std::span(CounterBlocks), BlockIndex);
	}

	std::optional<Misc::IndexSpan<>> CaptureWrapper::FindFirstCapture(std::span<Capture const> Captures)
	{
		std::size_t Stack = 0;
		Misc::IndexSpan<> First;
		for (std::size_t Index = 0; Index < Captures.size(); ++Index)
		{
			auto Cur = Captures[Index];
			if (Cur.IsBegin)
			{
				if (Stack == 0)
					First.Offset = Index;
				++Stack;
			}
			else {
				--Stack;
				if (Stack == 0)
				{
					First.Length = Index - First.Offset + 1;
					return First;
				}
			}
		}
		return First;
	}

	CaptureWrapper CaptureWrapper::GetNextCapture() const
	{
		auto First = FindFirstCapture(NextCaptures);
		if (First.has_value())
		{
			CaptureWrapper Result;
			Result.SubCaptures = First->Sub(1, First->Length - 2).Slice(NextCaptures);
			Result.NextCaptures = NextCaptures.subspan(First->End());
			auto Start = NextCaptures[First->Begin()];
			auto End = NextCaptures[First->End() - 1];
			Result.CurrentCapture = { Start.Index, End.Index - Start.Index };
			return Result;
		}
		return {};
	}

	CaptureWrapper CaptureWrapper::GetTopSubCapture() const
	{
		auto First = FindFirstCapture(SubCaptures);
		if (First.has_value())
		{
			auto Start = SubCaptures[First->Begin()];
			auto End = SubCaptures[First->End() - 1];
			CaptureWrapper Result;
			Result.SubCaptures = First->Sub(1, First->Length - 2).Slice(SubCaptures);
			Result.NextCaptures = SubCaptures.subspan(First->End());
			Result.CurrentCapture = { Start.Index, End.Index - Start.Index };
			return Result;
		}
		return {};
	}
	
	struct ApplyActionResult
	{
		std::size_t NodeArrayIndex = 0;
		CoreProcessor::AcceptResult AcceptData;
	};

	ApplyActionResult ApplyAction(std::span<DFA::ActionT const> Actions, std::span<StandardT const> Parameter, ProcessorContent& Target, ProcessorContent const& Source,  std::size_t TokenIndex, bool ContentNeedChange)
	{
		using ActionT = DFA::ActionT;
		std::size_t CounterDeep = 1;
		std::optional<std::size_t> CorrentBlockIndex;
		bool CounterTestPass = true;
		bool MeetCounter = false;
		ApplyActionResult Result;

		if (ContentNeedChange)
		{
			std::size_t CounterOffset = 0;
			std::size_t CaptureOffset = 0;

			for (auto TopAction : Actions)
			{
				switch (TopAction)
				{
				case ActionT::CaptureBegin:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						Target.CaptureBlocks.push_back({ true, TokenIndex, *CorrentBlockIndex });
					}
					break;
				case ActionT::CaptureEnd:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						Target.CaptureBlocks.push_back({ false, TokenIndex, *CorrentBlockIndex });
					}
					break;
				case ActionT::ContentBegin:
					assert(Parameter.size() >= 2);
					CorrentBlockIndex = Parameter[1];
					CounterOffset = Target.CounterBlocks.size();
					CaptureOffset = Target.CaptureBlocks.size();

					{
						auto SourceCounter = Source.FindCounterSpan(Parameter[0]);
						for(auto Ite2 : SourceCounter)
							Target.CounterBlocks.push_back({ Ite2.CountNumber, *CorrentBlockIndex });
						auto SourceCapture = Source.FindCaptureSpan(Parameter[0]);
						for (auto Ite2 : SourceCapture)
							Target.CaptureBlocks.push_back({ Ite2.CaptureData, *CorrentBlockIndex });
					}
					
					Parameter = Parameter.subspan(2);
					break;
				case ActionT::ContentEnd:
					if (MeetCounter)
					{
						if (CounterTestPass)
						{
							Result.NodeArrayIndex += CounterDeep;
						}
						else {
							Target.CaptureBlocks.resize(CaptureOffset);
							Target.CounterBlocks.resize(CounterOffset);
						}
						CounterDeep *= 2;
					}
					CounterTestPass = true;
					MeetCounter = false;
					CorrentBlockIndex.reset();
					break;
				case ActionT::CounterAdd:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Target.CounterBlocks.size() > CounterOffset);
						assert(Target.CounterBlocks.rbegin()->BlockIndex == *CorrentBlockIndex);
						++Target.CounterBlocks.rbegin()->CountNumber;
					}
					break;
				case ActionT::CounterBigEqual:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Target.CounterBlocks.size() > CounterOffset);
						assert(Target.CounterBlocks.rbegin()->BlockIndex == *CorrentBlockIndex);
						assert(Parameter.size() >= 1);
						if (Target.CounterBlocks.rbegin()->CountNumber < Parameter[0])
						{
							CounterTestPass = false;
						}
					}
					MeetCounter = true;
					Parameter = Parameter.subspan(1);
					break;
				case ActionT::CounterSmaller:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Target.CounterBlocks.size() > CounterOffset);
						assert(Target.CounterBlocks.rbegin()->BlockIndex == *CorrentBlockIndex);
						assert(Parameter.size() >= 1);
						if (Target.CounterBlocks.rbegin()->CountNumber >= Parameter[0])
						{
							CounterTestPass = false;
						}
					}
					MeetCounter = true;
					Parameter = Parameter.subspan(1);
					break;
				case ActionT::CounterEqual:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Target.CounterBlocks.size() > CounterOffset);
						assert(Target.CounterBlocks.rbegin()->BlockIndex == *CorrentBlockIndex);
						assert(Parameter.size() >= 1);
						if (Target.CounterBlocks.rbegin()->CountNumber != Parameter[0])
						{
							CounterTestPass = false;
						}
					}
					MeetCounter = true;
					Parameter = Parameter.subspan(1);
					break;
				case ActionT::CounterPush:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						Target.CounterBlocks.push_back({ 0, *CorrentBlockIndex });
					}
					break;
				case ActionT::CounterPop:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Target.CounterBlocks.size() > CounterOffset);
						assert(Target.CounterBlocks.rbegin()->BlockIndex == *CorrentBlockIndex);
						Target.CounterBlocks.pop_back();
					}
					break;
				case ActionT::Accept:
					if (CounterTestPass && !Result.AcceptData)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Parameter.size() >= 2);
						Result.AcceptData.AcceptData = { Parameter[0], Parameter[1]};
						Result.AcceptData.CaptureSpan = {CaptureOffset, Target.CaptureBlocks.size() - CaptureOffset};
					}
					Parameter = Parameter.subspan(2);
					break;
				default:
					assert(false);
					break;
				}
			}
		}
		else {
			for (auto TopAction : Actions)
			{
				std::span<ProcessorContent::CaptureBlock const> CacheCapture;
				std::span<ProcessorContent::CounterBlock const> CacheCounter;
				Misc::IndexSpan<> CacheCaptureIndex;
				switch (TopAction)
				{
				case ActionT::ContentBegin:
					assert(Parameter.size() >= 2);
					CorrentBlockIndex = Parameter[1];
					CacheCaptureIndex = Source.FindCaptureIndex(*CorrentBlockIndex);
					CacheCapture = CacheCaptureIndex.Slice(Source.CaptureBlocks);
					Parameter = Parameter.subspan(2);
					break;
				case ActionT::ContentEnd:
					if (MeetCounter)
					{
						if (CounterTestPass)
						{
							Result.NodeArrayIndex += CounterDeep;
						}
						CounterDeep *= 2;
					}
					CounterTestPass = true;
					MeetCounter = false;
					CorrentBlockIndex.reset();
					break;
				case ActionT::CounterBigEqual:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(!CacheCounter.empty());
						assert(CacheCounter.rbegin()->BlockIndex == *CorrentBlockIndex);
						assert(Parameter.size() >= 1);
						if (CacheCounter.rbegin()->CountNumber < Parameter[0])
						{
							CounterTestPass = false;
						}
					}
					MeetCounter = true;
					Parameter = Parameter.subspan(1);
					break;
				case ActionT::CounterSmaller:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(!CacheCounter.empty());
						assert(CacheCounter.rbegin()->BlockIndex == *CorrentBlockIndex);
						assert(Parameter.size() >= 1);
						if (CacheCounter.rbegin()->CountNumber >= Parameter[0])
						{
							CounterTestPass = false;
						}
					}
					MeetCounter = true;
					Parameter = Parameter.subspan(1);
					break;
				case ActionT::CounterEqual:
					if (CounterTestPass)
					{
						assert(CorrentBlockIndex.has_value());
						assert(!CacheCounter.empty());
						assert(CacheCounter.rbegin()->BlockIndex == *CorrentBlockIndex);
						assert(Parameter.size() >= 1);
						if (CacheCounter.rbegin()->CountNumber != Parameter[0])
						{
							CounterTestPass = false;
						}
					}
					MeetCounter = true;
					Parameter = Parameter.subspan(1);
					break;
				case ActionT::Accept:
					if (CounterTestPass && !Result.AcceptData)
					{
						assert(CorrentBlockIndex.has_value());
						assert(Parameter.size() >= 2);
						Result.AcceptData.CaptureSpan = CacheCaptureIndex;
						Result.AcceptData.AcceptData = { Parameter[0], Parameter[1]};
					}
					Parameter = Parameter.subspan(2);
					break;
				default:
					assert(false);
					break;
				}
			}
		}
		return Result;
		
	}

	auto CoreProcessor::ConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeIndex, DFA const& Table, char32_t Symbol, std::size_t TokenIndex, bool KeepAccept) -> std::optional<Result>
	{
		assert(CurNodeIndex < Table.Nodes.size());

		auto& CurNode = Table.Nodes[CurNodeIndex];

		decltype(CurNode.Edges)::const_iterator Begin;
		decltype(CurNode.Edges)::const_iterator End;

		if (KeepAccept && !CurNode.KeepAcceptableEdges.empty())
		{
			Begin = CurNode.KeepAcceptableEdges.begin();
			End = CurNode.KeepAcceptableEdges.end();
		}
		else {
			Begin = CurNode.Edges.begin();
			End = CurNode.Edges.end();
		}

		for (auto Ite = Begin; Ite != End; ++Ite)
		{
			if (Misc::SequenceIntervalWrapper{ Ite->ConsumeSymbols }.IsInclude(Symbol))
			{
				auto Pars = std::span{ Ite->Parameter };
				auto Actions = std::span{ Ite->List };
				auto AR = ApplyAction(Actions, Pars, Target, Source, TokenIndex, Ite->ContentNeedChange);
				auto NodexIndex = Ite->ToIndex[AR.NodeArrayIndex];
				if (NodexIndex != std::numeric_limits<std::size_t>::max())
				{
					Result Re;
					Re.NextNodeIndex = NodexIndex;
					Re.ContentNeedChange = Ite->ContentNeedChange;
					Re.AcceptData = AR.AcceptData;
					return Re;
				}
				else {
					return {};
				}
			}
		}

		return {};
	}

	bool AcceptConsumeSymbol(std::span<TableWrapper::ZipChar const> Source, char32_t Symbol)
	{
		while (!Source.empty())
		{
			auto Cur = Source[0];
			if (Cur.IsSingleChar)
			{
				if(Symbol == Cur.Char)
					return true;
				Source = Source.subspan(1);
			}
			else {
				assert(Source.size() >= 2);
				assert(!Source[1].IsSingleChar);
				if (Symbol >= Cur.Char && Symbol < Source[1].Char)
					return true;
				Source = Source.subspan(2);
			}
		}
		return false;
	}

	auto CoreProcessor::ConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeOffset, TableWrapper Wrapper, char32_t Symbol, std::size_t TokenIndex, bool KeepAccept) ->std::optional<Result>
	{
		assert(CurNodeOffset < Wrapper.BufferSize());

		auto IteSpan = Wrapper.Wrapper.subspan(CurNodeOffset);

		Misc::SerilizerHelper::SpanReader<StandardT const> Reader(IteSpan);

		auto CurNode = Reader.ReadObject<TableWrapper::ZipNode const>();
		if (CurNode->EdgeCount != 0)
		{
			TableWrapper::ZipEdge const* Edge = nullptr;
			if (KeepAccept)
				Reader = Reader.SubSpan(CurNode->AcceptEdgeOffset);
			Edge = Reader.ReadObject<TableWrapper::ZipEdge const>();
			auto EdgeReader = Reader;
			assert(Edge != nullptr);

			while (Edge != nullptr)
			{
				auto Symbols = EdgeReader.ReadObjectArray<TableWrapper::ZipChar const>(Edge->ConsumeSymbolCount);
				assert(Symbols.has_value());
				if (AcceptConsumeSymbol(*Symbols, Symbol))
				{
					auto List = EdgeReader.ReadObjectArray<DFA::ActionT const>(Edge->ActionCount);
					auto Par = EdgeReader.ReadObjectArray<StandardT const>(Edge->ParmaterCount);
					auto ToIndex = EdgeReader.ReadObjectArray<StandardT const>(Edge->ToIndexCount);
					assert(List.has_value() && Par.has_value() && ToIndex.has_value());
					auto AR = ApplyAction(*List, *Par, Target, Source, TokenIndex, Edge->NeedContentChange);
					auto NodexIndex = (*ToIndex)[AR.NodeArrayIndex];
					if (NodexIndex != std::numeric_limits<StandardT>::max())
					{
						Result Re;
						Re.NextNodeIndex = NodexIndex;
						Re.ContentNeedChange = Edge->NeedContentChange;
						Re.AcceptData = AR.AcceptData;
						return Re;
					}
					else {
						return {};
					}
				}
				if (Edge->NextEdgeOffset == 0)
					Edge = nullptr;
				else
				{
					Reader = Reader.SubSpan(Edge->NextEdgeOffset);
					Edge = Reader.ReadObject<TableWrapper::ZipEdge const>();
					EdgeReader = Reader;
				}
			}
		}
		return {};
	}


	auto CoreProcessor::KeepAcceptConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeIndex, DFA const& Table, char32_t Symbol, std::size_t TokenIndex) ->KeepAcceptResult
	{
		assert(CurNodeIndex < Table.Nodes.size());

		auto& CurNode = Table.Nodes[CurNodeIndex];

		decltype(CurNode.Edges)::const_iterator Begin;
		decltype(CurNode.Edges)::const_iterator End;

		if (!CurNode.KeepAcceptableEdges.empty())
		{
			Begin = CurNode.KeepAcceptableEdges.begin();
			End = CurNode.KeepAcceptableEdges.end();
		}
		else {
			Begin = CurNode.Edges.begin();
			End = CurNode.Edges.end();
		}

		KeepAcceptResult KAResult;

		for (auto Ite = Begin; Ite != End; ++Ite)
		{
			if (!KAResult.MeetAcceptRequireConsume && Misc::SequenceIntervalWrapper{ Ite->ConsumeSymbols }.IsInclude(Symbol))
			{
				KAResult.MeetAcceptRequireConsume = true;
			}	
			else if (Misc::SequenceIntervalWrapper{ Ite->ConsumeSymbols }.IsInclude(Reg::EndOfFile()))
			{
				auto Pars = std::span{ Ite->Parameter };
				auto Actions = std::span{ Ite->List };
				auto AR = ApplyAction(Actions, Pars, Target, Source, TokenIndex, Ite->ContentNeedChange);
				auto NodexIndex = Ite->ToIndex[AR.NodeArrayIndex];
				
				if (NodexIndex != std::numeric_limits<std::size_t>::max())
				{
					KAResult.ContentNeedChange = Ite->ContentNeedChange;
					KAResult.AcceptData = AR.AcceptData;
				}
				return KAResult;
			}
		}

		return KAResult;
	}

	auto CoreProcessor::KeepAcceptConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeOffset, TableWrapper Wrapper, char32_t Symbol, std::size_t TokenIndex) ->KeepAcceptResult
	{
		assert(CurNodeOffset < Wrapper.BufferSize());

		auto IteSpan = Wrapper.Wrapper.subspan(CurNodeOffset);

		Misc::SerilizerHelper::SpanReader<StandardT const> Reader(IteSpan);

		auto CurNode = Reader.ReadObject<TableWrapper::ZipNode const>();

		KeepAcceptResult KAResult;


		if (CurNode->EdgeCount != 0)
		{
			Reader = Reader.SubSpan(CurNode->AcceptEdgeOffset);
			auto Edge = Reader.ReadObject<TableWrapper::ZipEdge const>();
			auto EdgeReader = Reader;
			assert(Edge != nullptr);

			while (Edge != nullptr)
			{
				auto Symbols = EdgeReader.ReadObjectArray<TableWrapper::ZipChar const>(Edge->ConsumeSymbolCount);
				assert(Symbols.has_value());
				if (!KAResult.MeetAcceptRequireConsume && AcceptConsumeSymbol(*Symbols, Symbol))
				{
					KAResult.MeetAcceptRequireConsume = true;
				}
				else if (AcceptConsumeSymbol(*Symbols, Reg::EndOfFile()))
				{
					auto List = EdgeReader.ReadObjectArray<DFA::ActionT const>(Edge->ActionCount);
					auto Par = EdgeReader.ReadObjectArray<StandardT const>(Edge->ParmaterCount);
					auto ToIndex = EdgeReader.ReadObjectArray<StandardT const>(Edge->ToIndexCount);
					assert(List.has_value() && Par.has_value() && ToIndex.has_value());
					auto AR = ApplyAction(*List, *Par, Target, Source, TokenIndex, Edge->NeedContentChange);
					auto NodexIndex = (*ToIndex)[AR.NodeArrayIndex];
					if (NodexIndex != std::numeric_limits<StandardT>::max())
					{
						KAResult.ContentNeedChange = Edge->NeedContentChange;
						KAResult.AcceptData = AR.AcceptData;
						return KAResult;
					}
					else {
						return {};
					}
				}
				if (Edge->NextEdgeOffset == 0)
					Edge = nullptr;
				else
				{
					Reader = Reader.SubSpan(Edge->NextEdgeOffset);
					Edge = Reader.ReadObject<TableWrapper::ZipEdge const>();
					EdgeReader = Reader;
				}
			}
		}
		return KAResult;
	}

	void MatchProcessor::Clear()
	{
		NodeIndex = StartupNodeOffset;
		Contents.Clear();
		TempBuffer.Clear();
	}

	template<typename TableT>
	bool MatchProcessorConsumeSymbol(MatchProcessor& Pro, TableT Table, char32_t Symbol, std::size_t TokenIndex)
	{
		assert(Symbol != Reg::EndOfFile());
		Pro.TempBuffer.Clear();
		auto Re = CoreProcessor::ConsumeSymbol(Pro.TempBuffer, Pro.Contents, Pro.NodeIndex, Table, Symbol, TokenIndex, false);
		if (Re.has_value())
		{
			assert(!Re->AcceptData);
			Pro.NodeIndex = Re->NextNodeIndex;
			if (Re->ContentNeedChange)
				std::swap(Pro.TempBuffer, Pro.Contents);
			return true;
		}
		return false;
	}

	bool MatchProcessor::Consume(char32_t Symbol, std::size_t TokenIndex) {
		assert(Symbol != Reg::EndOfFile());
		TempBuffer.Clear();
		std::optional<CoreProcessor::Result> Re;
		if (std::holds_alternative<std::reference_wrapper<DFA const>>(Table))
		{
			Re = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<std::reference_wrapper<DFA const>>(Table).get(), Symbol, TokenIndex, false);
		}
		else {
			Re = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<TableWrapper>(Table), Symbol, TokenIndex, false);
		}
		if (Re.has_value())
		{
			assert(!Re->AcceptData);
			NodeIndex = Re->NextNodeIndex;
			if (Re->ContentNeedChange)
				std::swap(TempBuffer, Contents);
			return true;
		}
		return false;
	}

	auto MatchProcessor::EndOfFile(std::size_t TokenIndex) -> std::optional<Result>
	{
		TempBuffer.Clear();
		std::optional<CoreProcessor::Result> Re;
		if (std::holds_alternative<std::reference_wrapper<DFA const>>(Table))
		{
			Re = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<std::reference_wrapper<DFA const>>(Table).get(), Reg::EndOfFile(), TokenIndex, false);
		}
		else {
			Re = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<TableWrapper>(Table), Reg::EndOfFile(), TokenIndex, false);
		}
		if (Re.has_value())
		{
			assert(Re->AcceptData);
			MatchProcessor::Result NRe;
			NRe.AcceptData = *Re->AcceptData.AcceptData;
			auto Span = Re->AcceptData.CaptureSpan.Slice(Contents.CaptureBlocks);
			for (auto Ite : Span)
			{
				NRe.SubCaptures.push_back(Ite.CaptureData);
			}
			Clear();
			return NRe;
		}
		return {};
	}

	template<typename TableT>
	std::optional<std::optional<HeadMatchProcessor::Result>> HeadMatchProcessorConsumeSymbol(HeadMatchProcessor& Pro, char32_t Symbol, std::size_t TokenIndex, TableT& Table, std::size_t StartupNodeIndex, bool Greedy = false)
	{
		
	}

	template<typename TableT>
	std::optional<HeadMatchProcessor::Result> HeadMatchProcessorEndOfFile(HeadMatchProcessor& Pro, std::size_t TokenIndex, TableT& Table, std::size_t StartupNodeIndex)
	{
		Pro.TempBuffer.Clear();
		auto Re1 = CoreProcessor::ConsumeSymbol(Pro.TempBuffer, Pro.Contents, Pro.NodeIndex, Table, Reg::EndOfFile(), TokenIndex, true);
		if (Re1.has_value())
		{
			assert(Re1->AcceptData);
			HeadMatchProcessor::Result Re;
			if (Pro.CacheResult.has_value())
			{
				Re = std::move(*Pro.CacheResult);
				Pro.CacheResult.reset();
			}
			Re.AcceptData = *Re1->AcceptData.AcceptData;
			std::span<ProcessorContent::CaptureBlock const> Span = Re1->ContentNeedChange ? std::span(Pro.TempBuffer.CaptureBlocks) : std::span(Pro.Contents.CaptureBlocks);
			Re.SubCaptures.clear();
			for (auto Ite : Span)
				Re.SubCaptures.push_back(Ite.CaptureData);
			Re.MainCapture = { 0, TokenIndex };
			Pro.Clear(StartupNodeIndex);
			return Re;
		}
		if (Pro.CacheResult.has_value())
		{
			HeadMatchProcessor::Result Re = std::move(*Pro.CacheResult);
			Pro.Clear(StartupNodeIndex);
			return Re;
		}
		else {
			return {};
		}
	}
	

	void HeadMatchProcessor::Clear()
	{
		Contents.Clear();
		TempBuffer.Clear();
		CacheResult.reset();
		NodeIndex = StartupNode;
	}

	auto HeadMatchProcessor::Consume(char32_t Symbol, std::size_t TokenIndex) ->std::optional<std::optional<Result>>
	{
		assert(Symbol != Reg::EndOfFile());
		TempBuffer.Clear();
		CoreProcessor::KeepAcceptResult Re1;
		if (std::holds_alternative<std::reference_wrapper<DFA const>>(Table))
		{
			Re1 = CoreProcessor::KeepAcceptConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<std::reference_wrapper<DFA const>>(Table).get(), Symbol, TokenIndex);
		}
		else {
			Re1 = CoreProcessor::KeepAcceptConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<TableWrapper>(Table), Symbol, TokenIndex);
		}
		if (Re1.AcceptData)
		{
			HeadMatchProcessor::Result Re;
			if (CacheResult.has_value())
			{
				Re = std::move(*CacheResult);
				CacheResult.reset();
			}
			Re.AcceptData = *Re1.AcceptData.AcceptData;
			std::span<ProcessorContent::CaptureBlock const> Span = Re1.ContentNeedChange ? std::span(TempBuffer.CaptureBlocks) : std::span(Contents.CaptureBlocks);
			Re.SubCaptures.clear();
			for (auto Ite : Span)
				Re.SubCaptures.push_back(Ite.CaptureData);
			Re.MainCapture = { 0, TokenIndex };
			CacheResult = std::move(Re);

			if (!Greedy && !Re1.MeetAcceptRequireConsume)
			{
				auto Re = std::move(CacheResult);
				Clear();
				return Re;
			}
		}

		std::optional<CoreProcessor::Result> Re2;
		if (std::holds_alternative<std::reference_wrapper<DFA const>>(Table))
		{
			Re2 = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<std::reference_wrapper<DFA const>>(Table).get(), Symbol, TokenIndex, true);
		}
		else {
			Re2 = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<TableWrapper>(Table), Symbol, TokenIndex, true);
		}

		if (Re2.has_value())
		{
			assert(!Re2->AcceptData);
			NodeIndex = Re2->NextNodeIndex;
			if (Re2->ContentNeedChange)
				std::swap(TempBuffer, Contents);
			return std::optional<HeadMatchProcessor::Result>{};
		}
		else {
			if (CacheResult.has_value())
			{
				auto Re = std::move(CacheResult);
				Clear();
				return Re;
			}
			else {
				return {};
			}
		}
	}

	auto HeadMatchProcessor::EndOfFile(std::size_t TokenIndex) -> std::optional<Result>
	{
		TempBuffer.Clear();

		std::optional<CoreProcessor::Result> Re1;
		if (std::holds_alternative<std::reference_wrapper<DFA const>>(Table))
		{
			Re1 = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<std::reference_wrapper<DFA const>>(Table).get(), Reg::EndOfFile(), TokenIndex, true);
		}
		else {
			Re1 = CoreProcessor::ConsumeSymbol(TempBuffer, Contents, NodeIndex, std::get<TableWrapper>(Table), Reg::EndOfFile(), TokenIndex, true);
		}

		if (Re1.has_value())
		{
			assert(Re1->AcceptData);
			HeadMatchProcessor::Result Re;
			if (CacheResult.has_value())
			{
				Re = std::move(*CacheResult);
				CacheResult.reset();
			}
			Re.AcceptData = *Re1->AcceptData.AcceptData;
			std::span<ProcessorContent::CaptureBlock const> Span = Re1->ContentNeedChange ? std::span(TempBuffer.CaptureBlocks) : std::span(Contents.CaptureBlocks);
			Re.SubCaptures.clear();
			for (auto Ite : Span)
				Re.SubCaptures.push_back(Ite.CaptureData);
			Re.MainCapture = { 0, TokenIndex };
			Clear();
			return Re;
		}
		if (CacheResult.has_value())
		{
			HeadMatchProcessor::Result Re = std::move(*CacheResult);
			Clear();
			return Re;
		}
		else {
			return {};
		}
	}

	template<typename CharT>
	auto MatchTemp(MatchProcessor& Pro, std::basic_string_view<CharT> Str) ->MatchResult<typename MatchProcessor::Result>
	{
		char32_t TempBuffer;
		std::span<char32_t> OutputSpan{ &TempBuffer, 1 };
		std::size_t TotalSize = Str.size();

		while (!Str.empty())
		{
			auto Re2 = StrEncode::CharEncoder<CharT, char32_t>::EncodeOnceUnSafe(Str, OutputSpan);
			assert(Re2);
			auto Re = Pro.Consume(TempBuffer, TotalSize - Str.size());
			if (Re)
			{
				Str = Str.substr(Re2.SourceSpace);
			}
			else {
				return { {}, TotalSize - Str.size() };
			}
		}
		return { Pro.EndOfFile(TotalSize), TotalSize };
	}

	auto Match(MatchProcessor& Pro, std::u8string_view Str)->MatchResult<MatchProcessor::Result>
	{
		return MatchTemp(Pro, Str);
	}

	template<typename CharT>
	auto HeadMatchTemp(HeadMatchProcessor& Pro, std::basic_string_view<CharT> Str) ->MatchResult<typename HeadMatchProcessor::Result>
	{
		char32_t TempBuffer;
		std::span<char32_t> OutputSpan{ &TempBuffer, 1 };
		std::size_t TotalSize = Str.size();

		while (!Str.empty())
		{
			auto Re2 = StrEncode::CharEncoder<CharT, char32_t>::EncodeOnceUnSafe(Str, OutputSpan);
			assert(Re2);
			auto Re = Pro.Consume(TempBuffer, TotalSize - Str.size());
			if (Re.has_value())
			{
				if (Re->has_value())
				{
					return { **Re, TotalSize - Str.size() };
				}
				Str = Str.substr(Re2.SourceSpace);
			}
			else
				break;
		}
		return { Pro.EndOfFile(TotalSize), TotalSize };
	}

	auto HeadMatch(HeadMatchProcessor& Pro, std::u8string_view Str)->MatchResult<HeadMatchProcessor::Result>
	{
		return HeadMatchTemp(Pro, Str);
	}

	void MulityRegCreater::LowPriorityLink(std::u8string_view Str, bool IsRow, Accept Acce)
	{
		if (ETable.has_value())
		{
			ETable->Link(EpsilonNFA::Create(Str, IsRow, Acce));
		}
		else {
			ETable = EpsilonNFA::Create(Str, IsRow, Acce);
		}
	}

	std::optional<DFA> MulityRegCreater::GenerateDFA() const
	{
		if (ETable.has_value())
		{
			return DFA{ *ETable };
		}
		else {
			return {};
		}
	}

	std::optional<std::vector<StandardT>> MulityRegCreater::GenerateTableBuffer() const
	{
		auto DFA = GenerateDFA();
		if (DFA.has_value())
		{
			return TableWrapper::Create(*DFA);
		}
		return {};
	}

	namespace Exception
	{
		char const* Interface::what() const
		{
			return "PotatoRegException";
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