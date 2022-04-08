#include "../Include/PotatoReg.h"
#include "../Include/PotatoDLr.h"
#include "../Include/PotatoStrEncode.h"
#include <deque>
namespace Potato::Reg
{

	
	SeqIntervalT const& MaxIntervalRange() { 
		static SeqIntervalT Temp{ IntervalT{ 1, MaxChar() } };
		return Temp;
	};
	

	using DLr::Symbol;

	enum class T
	{
		SingleChar = 0, // µ¥×Ö·û
		CharSet, // ¶à×Ö·û
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

	constexpr Symbol operator*(T Input) { return Symbol{ Symbol::ValueType::TERMINAL, static_cast<std::size_t>(Input) }; };

	enum class NT
	{
		AcceptableSingleChar,
		Statement,
		CharList,
		Expression,
		ExpressionStatement,
		Number,
	};

	constexpr Symbol operator*(NT Input) { return Symbol{ Symbol::ValueType::NOTERMIAL, static_cast<std::size_t>(Input) }; };

	struct LexerTranslater
	{
		struct Storage
		{
			Symbol Value;
			std::variant<SeqIntervalT, char32_t> Acceptable;
			std::size_t Index;
		};

		Symbol GetSymbol(std::size_t Index) const {
			if (Index < StoragedSymbol.size())
				return StoragedSymbol[Index].Value;
			else
				return Symbol::EndOfFile();
		}
		std::variant<SeqIntervalT, char32_t>& GetAccept(std::size_t Index) { return StoragedSymbol[Index].Acceptable; }
		std::size_t GetSize() { return StoragedSymbol.size(); }
	protected:
		void PushSymbolData(T InputSymbol, std::variant<SeqIntervalT, char32_t> Symbol, std::size_t Index) { StoragedSymbol.emplace_back(*InputSymbol, std::move(Symbol), Index); }
		std::vector<Storage> StoragedSymbol;
	};

	struct RexLexerTranslater : public LexerTranslater
	{
		bool Insert(char32_t InputSymbol, std::size_t SymbolIndex = 0);
		bool EndOfFile();
		RexLexerTranslater() = default;
		RexLexerTranslater(RexLexerTranslater const&) = default;
		RexLexerTranslater(RexLexerTranslater&&) = default;
	protected:
		enum class State
		{
			Normal,
			Transfer,
			BigNumber,
			Number,
		};
		State CurrentState = State::Normal;
		std::size_t Number = 0;
		char32_t NumberChar = 0;
		bool NumberIsBig = false;
		std::size_t LastTokenIndex = 0;
	};

	bool RexLexerTranslater::Insert(char32_t InputSymbol, std::size_t SymbolIndex)
	{
		if (InputSymbol < MaxChar())
		{
			switch (CurrentState)
			{
			case State::Normal:
				switch (InputSymbol)
				{
				case U'-': PushSymbolData(T::Min, InputSymbol, SymbolIndex); return true;
				case U'[': PushSymbolData(T::BracketsLeft, InputSymbol, SymbolIndex); return true;
				case U']': PushSymbolData(T::BracketsRight, InputSymbol, SymbolIndex); return true;
				case U'{': PushSymbolData(T::CurlyBracketsLeft, InputSymbol, SymbolIndex); return true;
				case U'}': PushSymbolData(T::CurlyBracketsRight, InputSymbol, SymbolIndex); return true;
				case U',': PushSymbolData(T::Comma, InputSymbol, SymbolIndex); return true;
				case U'(': PushSymbolData(T::ParenthesesLeft, InputSymbol, SymbolIndex); return true;
				case U')': PushSymbolData(T::ParenthesesRight, InputSymbol, SymbolIndex); return true;
				case U'*': PushSymbolData(T::Mulity, InputSymbol, SymbolIndex); return true;
				case U'?': PushSymbolData(T::Question, InputSymbol, SymbolIndex); return true;
				case U'.': PushSymbolData(T::CharSet, SeqIntervalT{MaxIntervalRange()}, SymbolIndex); return true;
				case U'|': PushSymbolData(T::Or, InputSymbol, SymbolIndex); return true;
				case U'+': PushSymbolData(T::Add, InputSymbol, SymbolIndex); return true;
				case U'^': PushSymbolData(T::Not, InputSymbol, SymbolIndex); return true;
				case U':': PushSymbolData(T::Colon, InputSymbol, SymbolIndex); return true;
				case U'\\': CurrentState = State::Transfer; LastTokenIndex = SymbolIndex; return false;
				default:
					if (InputSymbol >= U'0' && InputSymbol <= U'9')
					{
						PushSymbolData(T::Num, InputSymbol, SymbolIndex);
						return true;
					}
					else {
						PushSymbolData(T::SingleChar, InputSymbol, SymbolIndex);
						return true;
					}
				}
				break;
			case State::Transfer:
				switch (InputSymbol)
				{
				case U'd':
					PushSymbolData(T::CharSet, SeqIntervalT{{U'0', U'9' + 1}} , LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				case U'D': {
					SeqIntervalT Tem{ { {1, U'0'},{U'9' + 1, MaxChar()} } };
					PushSymbolData(T::CharSet, std::move(Tem), LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				case U'f':
					PushSymbolData(T::SingleChar, U'\f', LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				case U'n':
					PushSymbolData(T::SingleChar, U'\n', LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				case U'r':
					PushSymbolData(T::SingleChar, U'\r', LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				case U't':
					PushSymbolData(T::SingleChar, U'\t', LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				case U'v':
					PushSymbolData(T::SingleChar, U'\v', LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				case U's':
				{
					SeqIntervalT tem({ IntervalT{ 1, 33 }, IntervalT{127, 128} });
					PushSymbolData(T::CharSet, std::move(tem), LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				case U'S':
				{
					SeqIntervalT tem({ IntervalT{33, 127}, IntervalT{128, MaxChar()} });
					PushSymbolData(T::CharSet, std::move(tem), LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				case U'w':
				{
					SeqIntervalT tem({ IntervalT{ U'a', U'z' + 1 }, IntervalT{ U'A', U'Z' + 1 }, IntervalT{ U'_', U'_' + 1}});
					PushSymbolData(T::CharSet, std::move(tem), LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				case U'W':
				{
					SeqIntervalT tem({ IntervalT{ U'a', U'z' + 1 }, IntervalT{ U'A', U'Z' + 1 }, IntervalT{ U'_', U'_' + 1}});
					SeqIntervalT total({ 1, MaxChar() });
					PushSymbolData(T::CharSet, tem.Complementary(MaxIntervalRange()), LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				case U'z':
				{
					SeqIntervalT tem(IntervalT{ 256, MaxChar()});
					PushSymbolData(T::CharSet, tem, LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				case U'u':
				{
					CurrentState = State::Number;
					NumberChar = 0;
					Number = 0;
					NumberIsBig = false;
					return false;
				}
				case U'U':
				{
					CurrentState = State::BigNumber;
					NumberChar = 0;
					Number = 0; 
					NumberIsBig = true;
					return false;
				}
				default:
					PushSymbolData(T::SingleChar, InputSymbol, LastTokenIndex);
					CurrentState = State::Normal;
					return true;
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
					throw Exception::UnaccaptableRegex{ SymbolIndex, {} };
				}
				if ((Number == 4 && !NumberIsBig) || (NumberIsBig && Number == 6))
				{
					if(Number >= MaxChar())
						throw Exception::UnaccaptableRegex{ SymbolIndex, {} };
					PushSymbolData(T::SingleChar, NumberChar, LastTokenIndex);
					CurrentState = State::Normal;
					return true;
				}
				else
					return false;
			}
			default:
				throw Exception::UnaccaptableRegex{ SymbolIndex, {} };
				return false;
			}
		}
		else {
			throw Exception::UnaccaptableRegex{ SymbolIndex, {} };
			return false;
		}
	}

	bool RexLexerTranslater::EndOfFile()
	{
		switch (CurrentState)
		{
		case State::Normal:
			return false;
		case State::Transfer:
			throw Exception::UnaccaptableRegex{ LastTokenIndex, {} };
		case State::Number:
			if (Number != 4 && !NumberIsBig || Number != 6 && NumberIsBig)
			{
				throw Exception::UnaccaptableRegex{ LastTokenIndex, {} };
			}
			else {
				PushSymbolData(T::SingleChar, NumberChar, std::numeric_limits<std::size_t>::max());
				return true;
			}
		default:
			return false;
			break;
		}
	}

	struct RawRexLexerTranslater : public LexerTranslater
	{
		bool Insert(char32_t InputSymbol, std::size_t SymbolIndex = 0) {
			PushSymbolData(T::SingleChar, InputSymbol, SymbolIndex);
			return true;
		}
		bool EndOfFile()
		{
			return false;
		}
		RawRexLexerTranslater() = default;
		RawRexLexerTranslater(RawRexLexerTranslater const&) = default;
		RawRexLexerTranslater(RawRexLexerTranslater&&) = default;
	};

	std::size_t UnfaTable::NewNode()
	{
		std::size_t Index = Nodes.size();
		Nodes.push_back({ Index, {} });
		return Index;
	}

	void UnfaTable::AddComsumeEdge(std::size_t From, std::size_t To, std::vector<IntervalT> Acceptable) {
		Edge NewEdge;
		NewEdge.Type = EdgeType::Consume;
		NewEdge.ShiftNode = To;
		NewEdge.ConsumeChars = std::move(Acceptable);
		AddEdge(From, std::move(NewEdge));
	}
	void UnfaTable::AddAcceptableEdge(std::size_t From, std::size_t To, Accept Data)
	{
		Edge NewEdge;
		NewEdge.Type = EdgeType::Acceptable;
		NewEdge.ShiftNode = To;
		NewEdge.AcceptData = std::move(Data);
		AddEdge(From, std::move(NewEdge));
	}

	void UnfaTable::AddCapture(NodeSet OutsideSet, NodeSet InsideSet)
	{
		Edge NewEdge;
		NewEdge.ShiftNode = InsideSet.In;
		NewEdge.Type = EdgeType::CaptureBegin;
		AddEdge(OutsideSet.In, std::move(NewEdge));

		NewEdge.ShiftNode = OutsideSet.Out;
		NewEdge.Type = EdgeType::CaptureEnd;
		AddEdge(InsideSet.Out, std::move(NewEdge));
	}

	void UnfaTable::AddCounter(NodeSet OutsideSet, NodeSet InsideSet, CounterEdgeData EndCounter, bool Greedy)
	{
		Edge Begin;
		Begin.ShiftNode = InsideSet.Out;
		Begin.Type = EdgeType::CounterBegin;

		Edge Continue;
		Continue.ShiftNode = InsideSet.In;
		Continue.Type = EdgeType::CounterContinue;
		Continue.CounterDatas = CounterEdgeData{ 0, EndCounter.Max - 1 };

		Edge End;
		End.ShiftNode = OutsideSet.Out;
		End.Type = EdgeType::CounterEnd;
		End.CounterDatas = EndCounter;

		AddEdge(OutsideSet.In, std::move(Begin));
		if (Greedy)
		{
			AddEdge(InsideSet.Out, std::move(Continue));
			AddEdge(InsideSet.Out, std::move(End));
		}
		else {
			AddEdge(InsideSet.Out, std::move(End));
			AddEdge(InsideSet.Out, std::move(Continue));
		}
	}

	void UnfaTable::AddEdge(std::size_t FormNodeIndex, Edge Edge)
	{
		assert(FormNodeIndex < Nodes.size());
		auto& Cur = Nodes[FormNodeIndex];
		Cur.Edges.push_back(std::move(Edge));
	}

	void UnfaTable::Link(UnfaTable const& OtherTable, bool ThisHasHigherPriority)
	{
		if (!OtherTable.Nodes.empty())
		{
			for (auto& Ite : Nodes)
			{
				for (auto& Ite2 : Ite.Edges)
				{
					Ite2.Block += 1;
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
			NewEdges.Type = EdgeType::Consume;
			NewEdges.ShiftNode = NodeOffset;
			if(ThisHasHigherPriority)
				Nodes[0].Edges.emplace_back(NewEdges);
			else {
				auto& Ref = Nodes[0].Edges;
				Ref.emplace(Ref.begin(), NewEdges);
			}	
		}
	}

	const DLr::TableWrapper RexDLrWrapper()
	{
		static DLr::Table Table(
			*NT::ExpressionStatement,
			{
				{*NT::AcceptableSingleChar, {*T::SingleChar}, 40},
				{*NT::AcceptableSingleChar, {*T::Num}, 40},
				{*NT::AcceptableSingleChar, {*T::Min}, 40},
				{*NT::AcceptableSingleChar, {*T::Comma}, 40},
				{*NT::AcceptableSingleChar, {*T::Colon}, 40},
				{*NT::CharList, {*NT::AcceptableSingleChar}, 41},
				{*NT::CharList, {*NT::AcceptableSingleChar, *T::Min, *NT::AcceptableSingleChar}, 42},
				{*NT::CharList, {*T::CharSet}, 1},
				{*NT::CharList, {*NT::CharList, *NT::CharList, 3}, 3},
				{*NT::Expression, {*T::BracketsLeft, *NT::CharList, *T::BracketsRight}, 4},
				{*NT::Expression, {*T::BracketsLeft, *T::Not, *NT::CharList, *T::BracketsRight}, 5},
				{*NT::Expression, {*T::ParenthesesLeft, *T::Question, *T::Colon, *NT::ExpressionStatement, *T::ParenthesesRight}, 6},
				{*NT::Expression, {*T::ParenthesesLeft, *NT::ExpressionStatement, *T::ParenthesesRight}, 7 },
				{*NT::ExpressionStatement, {*NT::ExpressionStatement,  *T::Or, *NT::ExpressionStatement, 8}, 8},
				{*NT::Expression, {*NT::AcceptableSingleChar}, 9},
				{*NT::Expression, {*T::CharSet}, 50},
				{*NT::ExpressionStatement, {*NT::Expression}, 10},
				{*NT::ExpressionStatement, {*NT::ExpressionStatement, 8, *NT::ExpressionStatement, 8, 11}, 11},
				{*NT::ExpressionStatement, {*NT::Expression, *T::Mulity}, 12},
				{*NT::ExpressionStatement, {*NT::Expression, *T::Add}, 13},
				{*NT::ExpressionStatement, {*NT::Expression, *T::Mulity, *T::Question}, 14},
				{*NT::ExpressionStatement, {*NT::Expression, *T::Add, *T::Question}, 15},
				{*NT::ExpressionStatement, {*NT::Expression, *T::Question}, 16},
				{*NT::ExpressionStatement, {*NT::Expression, *T::Question, *T::Question}, 17},
				{*NT::Number, {*T::Num}, 18},
				{*NT::Number, {*NT::Number, *T::Num}, 19},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::CurlyBracketsRight}, 20},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *T::Comma, *NT::Number, *T::CurlyBracketsRight}, 21},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *T::CurlyBracketsRight}, 22},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *NT::Number, *T::CurlyBracketsRight}, 23},

				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::CurlyBracketsRight, *T::Question}, 20},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *T::Comma, *NT::Number, *T::CurlyBracketsRight, *T::Question}, 21},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *T::CurlyBracketsRight, *T::Question}, 22},
				{*NT::ExpressionStatement, {*NT::Expression, *T::CurlyBracketsLeft, *NT::Number, *T::Comma, *NT::Number, *T::CurlyBracketsRight, *T::Question}, 23},
			}, {}
		);
		return Table.Wrapper;
	}

	std::any RegNoTerminalFunction(Potato::DLr::NTElement& NT, UnfaTable& Output)
	{

		using NodeSet = UnfaTable::NodeSet;

		switch (NT.Mask)
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
			if (Cur < Cur2)
			{
				SeqIntervalT Ptr({ Cur, Cur2 + 1 });
				return Ptr;
			}
			else [[unlikely]] {
				throw Exception::UnaccaptableRegex{ NT.FirstTokenIndex, {} };
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
		case 4:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			NodeSet Set{ T1, T2 };
			Output.AddComsumeEdge(T1, T2, NT[1].Consume<SeqIntervalT>());
			return Set;
		}
		case 5:
		{
			auto Tar = NT[2].Consume<SeqIntervalT>();
			auto P = MaxIntervalRange().AsWrapper().Remove(Tar);
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
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
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			NodeSet Last = NT.Datas[1].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Output.AddCapture(Set, Last);
			return Set;
		}
		case 8:
		{
			NodeSet Last1 = NT[0].Consume<NodeSet>();
			NodeSet Last2 = NT[2].Consume<NodeSet>();
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(T1, Last2.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(Last2.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 9:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Tar = NT[0].Consume<char32_t>();
			NodeSet Set{ T1, T2 };
			Output.AddComsumeEdge(T1, T2, {{Tar, Tar+1}});
			return Set;
		}
		case 50:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
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
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(T1, T2, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 13:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 14:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, T2, {});
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			return NodeSet{ T1, T2 };
		}
		case 15:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(Last1.Out, Last1.In, {});
			return NodeSet{ T1, T2 };
		}
		case 16:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last1 = NT[0].Consume<NodeSet>();
			Output.AddComsumeEdge(T1, Last1.In, {});
			Output.AddComsumeEdge(Last1.Out, T2, {});
			Output.AddComsumeEdge(T1, T2, {});
			return NodeSet{ T1, T2 };
		}
		case 17:
		{
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
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
		case 20:
		{
			auto Count = NT[2].Consume<std::size_t>();
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Output.AddCounter(Set, Last, { Count, Count + 1}, NT.Datas.size() == 4);;
			return Set;
		}
		case 21:
		{
			auto Count = NT[3].Consume<std::size_t>();
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Output.AddCounter(Set, Last, { 0, Count + 1 } , NT.Datas.size() == 5);
			return Set;
		}
		case 22:
		{
			auto Count = NT[2].Consume<std::size_t>();
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Output.AddCounter(Set, Last, { Count, std::numeric_limits<std::size_t>::max() }, NT.Datas.size() == 5);
			return Set;
		}
		case 23:
		{
			auto Count = NT[2].Consume<std::size_t>();
			auto Count2 = NT[4].Consume<std::size_t>();
			if (Count > Count2) [[unlikely]]
			{
				throw Exception::UnaccaptableRegex{NT.FirstTokenIndex, {} };
			}
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			
			Output.AddCounter(Set, Last, { Count, Count2 + 1 }, NT.Datas.size() == 6);
			return Set;
		}
		default :
			assert(false);
			return {};
			break;
		}
	}

	std::any RegTerminalFunction(Potato::DLr::TElement& Ele, LexerTranslater& Translater)
	{
		auto& Ref = Translater.GetAccept(Ele.TokenIndex);
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

	UnfaTable::NodeSet CreateImp(LexerTranslater& Translater, UnfaTable& Output)
	{
		auto Wrapper = RexDLrWrapper();
		auto Steps = DLr::ProcessSymbol(Wrapper, 0, [&](std::size_t Input){ return Translater.GetSymbol(Input); });
		return DLr::ProcessStepWithOutputType<UnfaTable::NodeSet>(std::span(Steps), 
			[&](DLr::StepElement Elements)-> std::any {
				if(Elements.IsTerminal())
					return RegTerminalFunction(Elements.AsTerminal(), Translater);
				else
					return RegNoTerminalFunction(Elements.AsNoTerminal(), Output);
			}
		);
	}

	UnfaTable::UnfaTable(std::size_t Start, std::size_t End, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData)
	{
		auto N1 = NewNode();
		auto N2 = NewNode();
		auto N3 = NewNode();
		AddComsumeEdge(N2, N3, { {MaxChar(), MaxChar() + 1} });
		RexLexerTranslater Translater;
		try {
			while (true)
			{
				auto Input = Start < End ? F1(Start, Data) : CodePoint::EndOfFile();
				if (!Input.IsEndOfFile())
				{
					Translater.Insert(Input.UnicodeCodePoint, Start);
					Start += Input.NextUnicodeCodePointOffset;
				}
				else {
					Translater.EndOfFile();
					break;
				}
			}
			auto Last = CreateImp(Translater, *this);
			AddComsumeEdge(N1, Last.In, {});
			AddAcceptableEdge(Last.Out, N2, AcceptData);
		}
		catch (DLr::Exception::UnaccableSymbol const& Symbol)
		{
			throw Exception::UnaccaptableRegex{Symbol.TokenIndex, AcceptData};
		}
		catch (...)
		{
			throw;
		}
	}

	UnfaTable::UnfaTable(RawString, std::size_t Start, std::size_t End, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData)
	{
		auto N1 = NewNode();
		auto N2 = NewNode();
		auto N3 = NewNode();
		AddComsumeEdge(N2, N3, { {MaxChar(), MaxChar() + 1} });
		RawRexLexerTranslater Translater;
		try {
			while (true)
			{
				auto Input = Start < End ? F1(Start, Data) : CodePoint::EndOfFile();
				if (!Input.IsEndOfFile())
				{
					Translater.Insert(Input.UnicodeCodePoint, Start);
					Start += Input.NextUnicodeCodePointOffset;
				}
				else {
					Translater.EndOfFile();
					break;
				}
			}
			auto Last = CreateImp(Translater, *this);
			AddComsumeEdge(N1, Last.In, {});
			AddAcceptableEdge(Last.Out, N2, AcceptData);
		}
		catch (Exception::UnaccaptableRegex const& Reg)
		{
			throw Exception::UnaccaptableRegex{ Reg.TokenIndex, AcceptData };
		}
		catch (DLr::Exception::UnaccableSymbol const& Symbol)
		{
			Exception::UnaccaptableRegex NewReg(Symbol.TokenIndex, AcceptData);
		}
		catch (...)
		{
			throw;
		}
	}

	UnserilizedTable::SpecialProperty TranslateSpecialEdge(UnfaTable::Edge const& Edge)
	{
		UnserilizedTable::SpecialProperty New;
		New.Type = Edge.Type;
		New.AcceptData = Edge.AcceptData;
		New.CounterData = Edge.CounterDatas;
		return New;
	}

	struct ExpandResult
	{
		std::vector<std::size_t> ContainNodes;
		std::vector<UnserilizedTable::Edge> Edges;
	};

	ExpandResult SearchEpsilonNode(UnfaTable const& Input, std::size_t SearchNode)
	{
		ExpandResult Result;

		struct SearchCore
		{
			std::size_t ToNode;
			std::vector<UnserilizedTable::SpecialProperty> Propertys;
			std::optional<std::size_t> Block;
		};

		std::vector<SearchCore> SearchStack;
		SearchStack.push_back({ SearchNode, {}, {} });
		Result.ContainNodes.push_back(SearchNode);
		while (!SearchStack.empty())
		{
			auto Cur = std::move(*SearchStack.rbegin());
			SearchStack.pop_back();
			assert(Input.Nodes.size() > Cur.ToNode);
			auto& Node = Input.Nodes[Cur.ToNode];
			for (auto EdgeIte = Node.Edges.rbegin(); EdgeIte != Node.Edges.rend();)
			{
				auto& Ite = *EdgeIte;
				++EdgeIte;
				/*
				if(std::find(Result.ContainNodes.begin(), Result.ContainNodes.end(), Ite.ShiftNode) != Result.ContainNodes.end()) [[unlikely]]
					continue;
				*/
				//assert(std::find(Result.ContainNodes.begin(), Result.ContainNodes.end(), Ite.ShiftNode) == Result.ContainNodes.end());
				
				Cur.ToNode = Ite.ShiftNode;
				if(!Cur.Block.has_value())
					Cur.Block = Ite.Block;
				else if (*Cur.Block != Ite.Block)
					Cur.Block = 0;
				
				if (EdgeIte == Node.Edges.rend())
				{
					switch (Ite.Type)
					{
					case UnfaTable::EdgeType::Consume:
						if (Ite.ConsumeChars.empty())
						{
							Result.ContainNodes.push_back(Ite.ShiftNode);
							SearchStack.push_back(std::move(Cur));
						}
						else {
							UnserilizedTable::Edge Edge;
							Edge.Propertys.Propertys = std::move(Cur.Propertys);
							Edge.ToNode = Ite.ShiftNode;
							Edge.Propertys.ConsumeChars = std::move(Ite.ConsumeChars);
							Edge.Block = *Cur.Block;
							Result.Edges.push_back(std::move(Edge));
						}
						break;
					default:
						Result.ContainNodes.push_back(Ite.ShiftNode);
						Cur.Propertys.push_back(TranslateSpecialEdge(Ite));
						SearchStack.push_back(std::move(Cur));
						break;
					}
					break;
				}
				else {
					switch (Ite.Type)
					{
					case UnfaTable::EdgeType::Consume:
						if (Ite.ConsumeChars.empty())
						{
							Result.ContainNodes.push_back(Ite.ShiftNode);
							SearchStack.push_back(Cur);
						}
						else {
							UnserilizedTable::Edge Edge;
							Edge.Propertys.Propertys = Cur.Propertys;
							Edge.ToNode = Ite.ShiftNode;
							Edge.Propertys.ConsumeChars = Ite.ConsumeChars;
							Edge.Block = *Cur.Block;
							Result.Edges.push_back(std::move(Edge));
						}
						break;
					default:
					{
						Result.ContainNodes.push_back(Ite.ShiftNode);
						auto NewCore = Cur;
						NewCore.Propertys.push_back(TranslateSpecialEdge(Ite));
						SearchStack.push_back(std::move(NewCore));
						break;
					}
					}
				}
			}
		}
		std::sort(Result.ContainNodes.begin(), Result.ContainNodes.end());
		return Result;
	}

	UnserilizedTable::UnserilizedTable(UnfaTable const& Table)
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

		{
			bool ContinueCheck = true;
			while (ContinueCheck)
			{
				ContinueCheck = false;
				for (std::size_t Index = 0; Index < Nodes.size(); ++Index)
				{
					for (std::size_t Index2 = Index + 1; Index2 < Nodes.size();)
					{
						auto& Ref1 = Nodes[Index];
						auto& Ref2 = Nodes[Index2];
						if (Ref1.Edges == Ref2.Edges)
						{
							ContinueCheck = true;
							Nodes.erase(Nodes.begin() + Index2);
							for (auto& Ite : Nodes)
							{
								for (auto& Ite2 : Ite.Edges)
								{
									if (Ite2.ToNode == Index2)
										Ite2.ToNode = Index;
									else if (Ite2.ToNode > Index2)
										--Ite2.ToNode;
								}
							}
						}
						else {
							++Index2;
						}
					}
				}
			}
		}
		

		{
			for (auto& Ite : Nodes)
			{
				for (std::size_t Index = 0; Index < Ite.Edges.size(); ++Index)
				{
					for (std::size_t Index2 = Index + 1; Index2 < Ite.Edges.size();)
					{
						auto& Ref = Ite.Edges[Index];
						auto& Ref2 = Ite.Edges[Index2];
						if (Ref.ToNode == Ref2.ToNode && !Nodes[Ref.ToNode].Edges.empty())
						{
							if (Ref.Propertys.ConsumeChars == Ref2.Propertys.ConsumeChars)
							{
								if(Ref.Block != Ref2.Block)
									Ref.Block = 0;
								Ite.Edges.erase(Ite.Edges.begin() + Index2);
							}
							else if (Index + 1 == Index2) {
								if (Ref.Block != Ref2.Block)
									Ref.Block = 0;
								Ref.Propertys.ConsumeChars = SeqIntervalWrapperT(Ref.Propertys.ConsumeChars).Union(Ref2.Propertys.ConsumeChars);
								Ite.Edges.erase(Ite.Edges.begin() + Index2);
							}
						}
						else
							++Index2;
					}
				}
			}
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

	bool Acceptable(char32_t InputSym, std::span<TableWrapper::ZipChar const> RequreSpan)
	{
		for (std::size_t Index = 0; Index < RequreSpan.size(); ++Index)
		{
			auto Cur = RequreSpan[Index];
			if (Cur.IsSingleChar)
			{
				if(InputSym == Cur.Char)
					return true;
			}
			else {
				++Index;
				assert(Index < RequreSpan.size());
				auto Cur2 = RequreSpan[Index];
				if(InputSym >= Cur.Char && InputSym < Cur2.Char)
					return true;
				else if(InputSym < Cur2.Char)
					return false;
			}
		}
		return false;
	}

	TableWrapper::ZipAcceptableData Translate(Accept Data)
	{
		TableWrapper::ZipAcceptableData Result;
		Misc::SerilizerHelper::TryCrossTypeSet(Result.Index, Data.Index, Exception::RegexOutOfRange{});
		Misc::SerilizerHelper::TryCrossTypeSet(Result.Mask, Data.Mask, Exception::RegexOutOfRange{});
		return Result;
	}

	TableWrapper::ZipCounterData Translate(UnfaTable::CounterEdgeData Data)
	{
		TableWrapper::ZipCounterData Result;
		if (Data.Min > std::numeric_limits<decltype(Result.Min)>::max() - 1)
		{
			Result.Min = std::numeric_limits<decltype(Result.Min)>::max() - 1;
		}
		else {
			Result.Min = static_cast<decltype(Result.Min)>(Data.Min);
		}
		if (Data.Max > std::numeric_limits<decltype(Result.Max)>::max())
		{
			Result.Max = std::numeric_limits<decltype(Result.Max)>::max();
		}
		else {
			Result.Max = static_cast<decltype(Result.Max)>(Data.Max);
		}
		return Result;
	}

	auto TableWrapper::Create(UnserilizedTable const& Table) ->std::vector<StorageT>
	{
		std::vector<StorageT> Result;
		Result.resize(2);

		Result[0] = static_cast<decltype(Result)::value_type>(Table.Nodes.size());
		if(Result[0] != Table.Nodes.size())[[unlikely]]
			throw Exception::RegexOutOfRange{};

		struct Record
		{
			std::size_t EdgeOffset;
			std::size_t MappingNode;
		};
		std::vector<Record> Records;

		std::vector<std::size_t> NodeOffset;

		std::vector<ZipChar> Chars;
		std::vector<ZipProperty> Propertys;
		std::vector<ZipAcceptableData> AccDatas;
		std::vector<ZipCounterData> CounterDatas;

		for (auto& Ite : Table.Nodes)
		{
			ZipNode NewNode;
			Misc::SerilizerHelper::TryCrossTypeSet(NewNode.EdgeCount, Ite.Edges.size(), Exception::RegexOutOfRange{});
			auto NodeResult = Misc::SerilizerHelper::WriteObject(Result, NewNode);
			NodeOffset.push_back(NodeResult.StartOffset);
			for (auto Ite22 = Ite.Edges.rbegin(); Ite22 != Ite.Edges.rend(); ++Ite22)
			{
				auto& Ite2 = *Ite22;
				Chars.clear();
				Propertys.clear();
				AccDatas.clear();
				CounterDatas.clear();
				TranslateIntervalT(std::span(Ite2.Propertys.ConsumeChars), Chars);
				for (auto& Ite3 : Ite2.Propertys.Propertys)
				{
					switch (Ite3.Type)
					{
					case UnfaTable::EdgeType::Acceptable:
						Propertys.push_back(ZipProperty::Acceptable);
						AccDatas.push_back(Translate(Ite3.AcceptData));
						break;
					case UnfaTable::EdgeType::CaptureBegin:
						Propertys.push_back(ZipProperty::CaptureBegin);
						break;
					case UnfaTable::EdgeType::CaptureEnd:
						Propertys.push_back(ZipProperty::CaptureEnd);
						break;
					case UnfaTable::EdgeType::CounterBegin:
						Propertys.push_back(ZipProperty::CounterBegin);
						break;
					case UnfaTable::EdgeType::CounterContinue:
						Propertys.push_back(ZipProperty::CounterContinue);
						CounterDatas.push_back(Translate(Ite3.CounterData));
						break;
					case UnfaTable::EdgeType::CounterEnd:
						Propertys.push_back(ZipProperty::CounterEnd);
						CounterDatas.push_back(Translate(Ite3.CounterData));
						break;
					default:
						assert(false);
						break;
					}
				}
				ZipEdge Edge;
				Misc::SerilizerHelper::TryCrossTypeSet(Edge.AcceptableCharCount, Chars.size(), Exception::RegexOutOfRange{});
				Misc::SerilizerHelper::TryCrossTypeSet(Edge.SpecialPropertyCount, Propertys.size(), Exception::RegexOutOfRange{});
				Misc::SerilizerHelper::TryCrossTypeSet(Edge.CounterDataCount, CounterDatas.size(), Exception::RegexOutOfRange{});
				assert(AccDatas.size() <= 1);
				if(!AccDatas.empty())
					Edge.AcceptableDataCount = 1;
				else
					Edge.AcceptableDataCount = 0;
				Edge.Block = Ite2.Block;
				if (Edge.Block != Ite2.Block) [[unlikely]]
					throw Exception::RegexOutOfRange{};
				auto EdgeResult = Misc::SerilizerHelper::WriteObject(Result, Edge);
				Records.push_back({ EdgeResult .StartOffset, Ite2 .ToNode});
				Misc::SerilizerHelper::WriteObjectArray(Result, std::span(Chars));
				Misc::SerilizerHelper::WriteObjectArray(Result, std::span(Propertys));
				Misc::SerilizerHelper::WriteObjectArray(Result, std::span(CounterDatas));
				Misc::SerilizerHelper::WriteObjectArray(Result, std::span(AccDatas));
			}
		}
		Misc::SerilizerHelper::TryCrossTypeSet(Result[1], NodeOffset[0], Exception::RegexOutOfRange{});

		for (auto& Ite : Records)
		{
			auto Ptr = Misc::SerilizerHelper::ReadObject<ZipEdge>(std::span(Result).subspan(Ite.EdgeOffset));
			Misc::SerilizerHelper::TryCrossTypeSet(Ptr->ToNode, NodeOffset[Ite.MappingNode], Exception::RegexOutOfRange{});
		}

#if _DEBUG
		{
			struct DebugNode
			{
				StorageT NodeOffset;
				std::vector<EdgeViewer> Edges;
			};

			std::vector<DebugNode> DebugsNode;

			TableWrapper Wrapper(Result);

			for (auto& Ite : NodeOffset)
			{
				StorageT Offset = static_cast<StorageT>(Ite);
				auto Node = Wrapper[Offset];
				DebugNode DNode;
				DNode.NodeOffset = Node.NodeOffset;
				DNode.Edges.reserve(Node.EdgeCount);
				auto IteSpan = Node.AppendData;
				for (std::size_t Index = 0; Index < Node.EdgeCount; ++Index)
				{
					auto Viewer = ReadEdgeViewer(IteSpan);
					IteSpan = Viewer.AppendDatas;
					DNode.Edges.push_back(Viewer);
				}
				std::reverse(DNode.Edges.begin(), DNode.Edges.end());
				DebugsNode.push_back(std::move(DNode));
			}

			volatile int i = 0;
		}
#endif

		return Result;
	}

	auto TableWrapper::Create(std::u32string_view Str, std::size_t Index, std::size_t Mask) ->std::vector<StorageT>
	{
		CodePointGenerator<char32_t> Gen(Str);
		Reg::UnfaTable Tab1(&decltype(Gen)::Function, &Gen, Accept{Index, Mask});
		UnserilizedTable Tab2(Tab1);
		return Create(Tab2);
	}

	auto TableWrapper::operator[](StorageT Offset) const ->NodeViewer
	{
		auto Result = Misc::SerilizerHelper::ReadObject<ZipNode const>(Wrapper.subspan(Offset));
		NodeViewer Viewer;
		Viewer.NodeOffset = Offset;
		Viewer.EdgeCount = Result->EdgeCount;
		Viewer.AppendData = Result.LastSpan;
		return Viewer;
	}

	
	auto TableWrapper::ReadEdgeViewer(std::span<StorageT const> Span) ->EdgeViewer
	{
		auto Result = Misc::SerilizerHelper::ReadObject<ZipEdge const>(Span);
		auto AcceChar = Misc::SerilizerHelper::ReadObjectArray<ZipChar const>(Result.LastSpan, Result->AcceptableCharCount);
		auto Propertys = Misc::SerilizerHelper::ReadObjectArray<ZipProperty const>(AcceChar.LastSpan, Result->SpecialPropertyCount);
		auto CounterData = Misc::SerilizerHelper::ReadObjectArray<ZipCounterData const>(Propertys.LastSpan, Result->CounterDataCount);
		auto AcceptableData = Misc::SerilizerHelper::ReadObjectArray<ZipAcceptableData const>(CounterData.LastSpan, Result->AcceptableDataCount);
		EdgeViewer Viewer;
		Viewer.ToNode = Result->ToNode;
		Viewer.ConsumeChars = *AcceChar;
		Viewer.Property = *Propertys;
		Viewer.CounterData = *CounterData;
		Viewer.AcceptData = *AcceptableData;
		Viewer.AppendDatas = AcceptableData.LastSpan;
		Viewer.Block = Result->Block;
		return Viewer;
	}

	std::size_t MulityRegexCreator::Push(UnfaTable const& UT)
	{
		if (Temporary.has_value())
		{
			Temporary->Link(UT, true);
		}
		else {
			Temporary = std::move(UT);
		}
		++Index;
		return Index;
	}

	std::size_t MulityRegexCreator::PushRegex(std::u32string_view Reg, Accept AccData)
	{
		CodePointGenerator<char32_t> Generator{Reg};
		return PushRegex(0, std::numeric_limits<std::size_t>::max(), & decltype(Generator)::Function, &Generator, AccData);
	}

	std::size_t MulityRegexCreator::PushRawString(std::u32string_view Reg, Accept AccData)
	{
		CodePointGenerator<char32_t> Generator(Reg);
		return PushRawString(0, std::numeric_limits<std::size_t>::max(), &decltype(Generator)::Function, &Generator, AccData);
	}

	std::vector<TableWrapper::StorageT> MulityRegexCreator::Generate() const
	{
		assert(Temporary.has_value());
		return TableWrapper::Create(*Temporary);
	}

	auto CoreProcessor::ConsumeCharInput(char32_t InputSymbols, std::size_t TokenIndex, std::size_t NextTokenIndex, bool KeepAcceptableViewer)
		->std::optional<ConsumeResult>
	{
		auto Node = Wrapper[CurrentState];
		Misc::IndexSpan<> CounterIndex{ AmbiguousCounter.size(), ConuterRecord.size() };
		std::optional<AmbiguousPoint> LastAP;

		Node.ReadEdge([&](TableWrapper::EdgeViewer const& Viewer)->bool{

			bool ShouldKeepViewer = std::find(Walkway.begin(), Walkway.end(), Viewer.Block) == Walkway.end() 
				&& (Acceptable(InputSymbols, Viewer.ConsumeChars) || (KeepAcceptableViewer && !Viewer.AcceptData.empty()));

			if (ShouldKeepViewer && !Viewer.CounterData.empty())
			{
				std::vector<std::size_t> TemporaryRecord = ConuterRecord;
				std::size_t CounterIndex = 0;
				for (auto Ite : Viewer.Property)
				{
					switch (Ite)
					{
					case decltype(Ite)::CounterBegin:
						TemporaryRecord.push_back(0);
						break;
					case decltype(Ite)::CounterContinue:
					{
						assert(CounterIndex < Viewer.CounterData.size());
						auto Range = Viewer.CounterData[CounterIndex];
						++CounterIndex;
						assert(!TemporaryRecord.empty());
						auto& CurCount = *TemporaryRecord.rbegin();
						if (CurCount < Range.Min || CurCount >= Range.Max)
							ShouldKeepViewer = false;
						else
							++CurCount;
						break;
					}
					case decltype(Ite)::CounterEnd:
					{
						assert(CounterIndex < Viewer.CounterData.size());
						auto Range = Viewer.CounterData[CounterIndex];
						++CounterIndex;
						assert(!TemporaryRecord.empty());
						auto& CurCount = *TemporaryRecord.rbegin();
						if (CurCount < Range.Min || CurCount >= Range.Max)
							ShouldKeepViewer = false;
						else
							TemporaryRecord.pop_back();
						break;
					}
					}
					if(!ShouldKeepViewer)
						break;
				}
			}

			if (ShouldKeepViewer)
			{
				AmbiguousPoint AP{
					CounterIndex,
					false,
					TokenIndex,
					NextTokenIndex,
					CaptureRecord.size(),
					Viewer
				};

				if (!LastAP.has_value())
					LastAP = AP;
				else {
					AP.IsSceondory = true;
					if (!LastAP->IsSceondory)
					{
						AmbiguousCounter.insert(AmbiguousCounter.end(), ConuterRecord.begin(), ConuterRecord.end());
					}

					AmbiguousPoints.push_back(*LastAP);
					LastAP = AP;
				}
			}
			return true;
		});

		if (LastAP.has_value())
		{
			return ConsumeResult{ApplyAmbiguousPoint(*LastAP)};
		}
		return {};
	}

	std::optional<Accept> CoreProcessor::ApplyAmbiguousPoint(AmbiguousPoint AP)
	{
		CurrentState = AP.Viewer.ToNode;
		CaptureRecord.resize(AP.CaptureCount);
		std::optional<Accept> AcceptData;
		for (auto Ite : AP.Viewer.Property)
		{
			switch (Ite)
			{
			case decltype(Ite)::Acceptable:
			{
				assert(!AP.Viewer.AcceptData.empty());
				assert(AP.Viewer.Block != 0);
				Walkway.push_back(AP.Viewer.Block);
				Accept Data;
				Data.Index = AP.Viewer.AcceptData[0].Index;
				Data.Mask = AP.Viewer.AcceptData[0].Mask;
				AcceptData = Data;
				break;
			}
			case decltype(Ite)::CaptureBegin:
			{
				CaptureRecord.push_back({ true, AP.TokenIndex });
				break;
			}
			case decltype(Ite)::CaptureEnd:
			{
				CaptureRecord.push_back({ false, AP.TokenIndex });
				break;
			}
			case decltype(Ite)::CounterBegin:
			{
				ConuterRecord.push_back(0);
				break;
			}
			case decltype(Ite)::CounterContinue:
			{
				assert(!ConuterRecord.empty());
				++(*ConuterRecord.rbegin());
				break;
			}
			case decltype(Ite)::CounterEnd:
			{
				assert(!ConuterRecord.empty());
				ConuterRecord.pop_back();
				break;
			}
			}
		}
		return AcceptData;
	}

	auto CoreProcessor::PopAmbiguousPoint() -> std::optional<AmbiguousPoint>
	{
		while (!AmbiguousPoints.empty())
		{
			auto Last = (*AmbiguousPoints.rbegin());
			AmbiguousPoints.pop_back();
			auto Span = Last.CounterIndex.Slice(AmbiguousCounter);
			ConuterRecord.clear();
			ConuterRecord.insert(ConuterRecord.end(), Span.begin(), Span.end());
			if (!Last.IsSceondory)
			{
				AmbiguousCounter.resize(Last.CounterIndex.Begin());
			}
			if (std::find(Walkway.begin(), Walkway.end(), Last.Viewer.Block) == Walkway.end())
			{
				ApplyAmbiguousPoint(Last);
				return Last;
			}
		}
		return {};
	}

	auto CoreProcessor::Flush(std::optional<std::size_t> Start, std::size_t End, bool StopIfMeetAcceptableViewer, CodePoint(*FuncO)(std::size_t, void*), void* Data)
		-> std::optional<FlushResult>
	{
		bool NeedConsumeAmbiguousPoint = !Start.has_value();
		std::size_t Index = Start.has_value() ? *Start : 0;
		std::size_t ForwardIndex = Index;
		std::size_t MaxFlushIndex = 0;
		while (Index < End)
		{
			if (NeedConsumeAmbiguousPoint)
			{
				NeedConsumeAmbiguousPoint = false;
				auto Ra = PopAmbiguousPoint();
				if (Ra.has_value())
				{
					Index = Ra->NextTokenIndex;
					ForwardIndex = Ra->TokenIndex;
					if (StopIfMeetAcceptableViewer && !Ra->Viewer.AcceptData.empty())
					{
						auto& Ref = Ra->Viewer.AcceptData[0];
						Accept Acce{Ref.Index, Ref.Mask};
						return FlushResult{ {ForwardIndex, Index - ForwardIndex}, Acce};
					}
				}
				else {
					return {};
				}
			}

			for (; Index < End;)
			{
				CodePoint Re = (*FuncO)(Index, Data);
				if (!Re.IsEndOfFile())
				{
					auto CRe = ConsumeCharInput(Re.UnicodeCodePoint, Index, Index + Re.NextUnicodeCodePointOffset, StopIfMeetAcceptableViewer);
					if (CRe.has_value())
					{
						if (CRe->AcceptData.has_value())
						{
							return FlushResult{ {Index, Re.NextUnicodeCodePointOffset}, *CRe->AcceptData};
						}
						else {
							ForwardIndex = Index;
							Index += Re.NextUnicodeCodePointOffset;
						}
					}
					else {
						NeedConsumeAmbiguousPoint = true;
						break;
					}
				}
				else {
					auto CRe2 = ConsumeCharInput(MaxChar(), Index, Index, StopIfMeetAcceptableViewer);
					if (CRe2.has_value() && CRe2->AcceptData.has_value())
					{
						return FlushResult{ {Index, 0}, *CRe2->AcceptData };
					}
					else {
						NeedConsumeAmbiguousPoint = true;
						break;
					}
				}
			}
		}
		return FlushResult{ {ForwardIndex, Index - ForwardIndex}, {} };
	}

	template<typename Char>
	struct StringViewFlushInput;

	template<>
	struct StringViewFlushInput<std::u32string_view>
	{
		static CodePoint Process(std::size_t Index, void* Data) {
			auto View = reinterpret_cast<std::u32string_view const*>(Data);
			if(Index < View->size())
				return CodePoint{ (*View)[Index], 1};
			else
				return CodePoint::EndOfFile();
		}
	};

	template<>
	struct StringViewFlushInput<std::wstring_view>
	{
		static CodePoint Process(std::size_t Index, void* Data) {
			auto View = reinterpret_cast<std::wstring_view const*>(Data);
			if (Index < View->size())
			{
				char32_t Source;
				auto Re = StrEncode::CoreEncoder<wchar_t, char32_t>::EncodeOnceUnSafe(View->substr(Index), {&Source, 1});
				return CodePoint{ Source, Re.SourceSpace };
			}
			else
				return CodePoint::EndOfFile();
		}
	};

	std::optional<MarchProcessor::Result> ProcessMarch(TableWrapper Wrapper, std::u32string_view Span)
	{
		return ProcessMarch(Wrapper, 0, StringViewFlushInput<std::u32string_view>::Process, &Span);
	}

	std::optional<MarchProcessor::Result> ProcessMarch(TableWrapper Wrapper, std::size_t Startup, CodePoint(*F1)(std::size_t Index, void*), void* Data)
	{
		CoreProcessor Core{ Wrapper };
		auto Cur = Core.Flush(0, std::numeric_limits<std::size_t>::max(), false, F1, Data);
		if (Cur.has_value() && Cur->AcceptData.has_value())
		{
			MarchProcessor::Result Re;
			Re.MainCapture = { Startup, Cur->LastToken.End()};
			Re.AcceptData = *Cur->AcceptData;
			Re.Captures = std::move(Core.CaptureRecord);
			return std::move(Re);
		}
		return {};
	}

	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::u32string_view SpanView)
	{
		return ProcessGreedMarch(Wrapper, 0, StringViewFlushInput<std::u32string_view>::Process, &SpanView);
	}

	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::wstring_view SpanView)
	{
		return ProcessGreedMarch(Wrapper, 0, StringViewFlushInput<std::wstring_view>::Process, &SpanView);
	}

	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::size_t Startup, CodePoint(*F1)(std::size_t Index, void*), void* Data)
	{
		Reg::CoreProcessor Cp(Wrapper);
		std::optional<GreedyMarchProcessor::Result> Re;
		std::optional<std::size_t> Ite = Startup;
		while (true)
		{
			auto Fe = Cp.Flush(Ite, std::numeric_limits<std::size_t>::max(), true, F1, Data);
			if (Fe.has_value())
			{
				if (Fe->AcceptData.has_value())
				{
					if (Re.has_value())
					{
						Ite.reset();
						if (Fe->LastToken.Begin() > Re->MainCapture.End())
						{
							Re->AcceptData = *Fe->AcceptData;
							Re->MainCapture.Length = Fe->LastToken.Begin() - Startup;
							Re->Captures = Cp.CaptureRecord;
							Re->IsEndOfFile = Fe->IsEndOfFile();
						}
						continue;
					}
					else {
						GreedyMarchProcessor::Result NewResult;
						NewResult.MainCapture = { Startup, Fe->LastToken.Begin() - Startup};
						NewResult.AcceptData = *Fe->AcceptData;
						NewResult.Captures = Cp.CaptureRecord;
						NewResult.IsEndOfFile = Fe->IsEndOfFile();
						Re = std::move(NewResult);
						Ite.reset();
					}
				}
			}
			else {
				break;
			}
		}
		return Re;
	}

	std::optional<SearchProcessor::Result> ProcessSearch(TableWrapper Wrapper, std::u32string_view Span)
	{
		return ProcessSearch(Wrapper, 0, StringViewFlushInput<std::u32string_view>::Process, &Span);
	}

	std::optional<SearchProcessor::Result> ProcessSearch(TableWrapper Wrapper, std::size_t Start, CodePoint(*F1)(std::size_t Index, void* Data), void* Data)
	{
		std::deque<CoreProcessor::AmbiguousPoint> CachedStartUpPoint;
		CoreProcessor Core{ Wrapper };
		Misc::IndexSpan<> SearchRange{ Start, 0 };
		while(true)
		{
			bool ForbiddenPreCheck = false;
			std::optional<std::size_t> CheckStart = Start;
			
			std::size_t TokenIndex = Start;
			std::size_t NextTokenIndex = Start + 1;
			auto Sym = F1(Start, Data);
			bool MeedEndOfFile = false;

			if(!Sym.IsEndOfFile())
				NextTokenIndex = Start + Sym.NextUnicodeCodePointOffset;
			else
				MeedEndOfFile = true;

			while (true)
			{
				bool NewBegin = (Core.CurrentState == Wrapper.StartupNodeOffset());

				if (NewBegin) {
					SearchRange.Offset = Start;
					ForbiddenPreCheck = true;
				}

				auto Re = Core.Flush(CheckStart, NextTokenIndex, true, F1, Data);
				if (Re.has_value())
				{
					if (Re->AcceptData.has_value())
					{
						SearchRange.Length = Re->LastToken.Begin() - SearchRange.Offset;
						SearchProcessor::Result SearchResult;
						SearchResult.MainCapture = SearchRange;
						SearchResult.Captures = std::move(Core.CaptureRecord);
						SearchResult.IsEndOfFile = Re->IsEndOfFile();
						return std::move(SearchResult);
					}
					break;
				}
				else {
					Core.Clear();
					if (!CachedStartUpPoint.empty())
					{
						auto Last = *CachedStartUpPoint.begin();
						CachedStartUpPoint.pop_front();
						Core.AmbiguousPoints.push_back(Last);
						CheckStart.reset();
						SearchRange.Offset = Last.TokenIndex;
					}
					else {
						ForbiddenPreCheck = true;
						if (!NewBegin)
						{
							CheckStart = TokenIndex;
						}
						else {
							break;
						}
					}
				}
			}

			if (!ForbiddenPreCheck && !Sym.IsEndOfFile())
			{
				std::vector<CoreProcessor::AmbiguousPoint> Points;
				auto Node = Wrapper[Wrapper.StartupNodeOffset()];
				Node.ReadEdge([&](TableWrapper::EdgeViewer Viewer) -> bool {
					if ((Viewer.ToNode != Core.CurrentState || Core.ConuterRecord.empty()) && Acceptable(Sym.UnicodeCodePoint, Viewer.ConsumeChars))
					{
						bool Find = Core.ConuterRecord.empty();
						if (!Find)
						{
							for (auto Ite = Core.AmbiguousPoints.rbegin(); Ite != Core.AmbiguousPoints.rend(); ++Ite)
							{
								if (Ite->NextTokenIndex == NextTokenIndex && Ite->Viewer.ToNode == Viewer.ToNode)
								{
									Find = true;
									break;
								}
								if (Ite->NextTokenIndex < NextTokenIndex)
									break;
							}
						}
						if (!Find)
						{
							CoreProcessor::AmbiguousPoint AP{
								{0, 0},
								true,
								TokenIndex,
								NextTokenIndex,
								0,
								Viewer
							};
							Points.push_back(AP);
						}
					}
					return true;
					});
				CachedStartUpPoint.insert(CachedStartUpPoint.end(), Points.rbegin(), Points.rend());
			}
			if(MeedEndOfFile)
				break;
			Start = NextTokenIndex;
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
	}
}