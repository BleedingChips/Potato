#include "../Public/PotatoReg.h"
#include "../Public/PotatoDLr.h"
#include "../Public/PotatoStrEncode.h"
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

	void UnfaTable::AddCounter(NodeSet OutsideSet, NodeSet InsideSet, Counter EndCounter, bool Greedy)
	{
		Edge Begin;
		Begin.ShiftNode = InsideSet.Out;
		Begin.Type = EdgeType::CounterBegin;

		Edge Continue;
		Continue.ShiftNode = InsideSet.In;
		Continue.Type = EdgeType::CounterContinue;
		Continue.CounterDatas = Counter{ 0, EndCounter.Max - 1 };

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
		Edge.UniqueID = TemporaryUniqueID;
		Cur.Edges.push_back(std::move(Edge));
	}

	void UnfaTable::Link(UnfaTable const& OtherTable, bool ThisHasHigherPriority)
	{
		if (!OtherTable.Nodes.empty())
		{
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
			Counter Tem;
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Min, Count, Reg::Exception::RegexOutOfRange{});
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Max, Count + 1, Reg::Exception::RegexOutOfRange{});
			Output.AddCounter(Set, Last, Tem, NT.Datas.size() == 4);;
			return Set;
		}
		case 21:
		{
			auto Count = NT[3].Consume<std::size_t>();
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Counter Tem;
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Min, 0, Reg::Exception::RegexOutOfRange{});
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Max, Count + 1, Reg::Exception::RegexOutOfRange{});
			Output.AddCounter(Set, Last, Tem, NT.Datas.size() == 5);
			return Set;
		}
		case 22:
		{
			auto Count = NT[2].Consume<std::size_t>();
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Counter Tem;
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Min, Count, Reg::Exception::RegexOutOfRange{});
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Max, std::numeric_limits<SerilizeT>::max(), Reg::Exception::RegexOutOfRange{});
			Output.AddCounter(Set, Last, Tem, NT.Datas.size() == 5);
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
			Counter Tem;
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Min, Count, Reg::Exception::RegexOutOfRange{});
			Misc::SerilizerHelper::TryCrossTypeSet(Tem.Max, Count2 + 1, Reg::Exception::RegexOutOfRange{});
			Output.AddCounter(Set, Last, Tem, NT.Datas.size() == 6);
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

	void CreateUnfaTable(LexerTranslater& Translater, UnfaTable& Output, Accept AcceptData)
	{
		auto N1 = Output.NewNode();
		auto N2 = Output.NewNode();
		auto N3 = Output.NewNode();
		Output.AddComsumeEdge(N2, N3, { {MaxChar(), MaxChar() + 1} });
		auto Wrapper = RexDLrWrapper();
		auto Steps = DLr::ProcessSymbol(Wrapper, 0, [&](std::size_t Input){ return Translater.GetSymbol(Input); });
		UnfaTable::NodeSet Set = DLr::ProcessStepWithOutputType<UnfaTable::NodeSet>(std::span(Steps),
			[&](DLr::StepElement Elements)-> std::any {
				if(Elements.IsTerminal())
					return RegTerminalFunction(Elements.AsTerminal(), Translater);
				else
					return RegNoTerminalFunction(Elements.AsNoTerminal(), Output);
			}
		);
		Output.AddComsumeEdge(N1, Set.In, {});
		Output.AddAcceptableEdge(Set.Out, N2, AcceptData);
	}

	UnfaTable UnfaTable::Create(std::u32string_view Str, bool IsRaw, Accept AcceptData, SerilizeT UniqueID)
	{
		UnfaTable Result(UniqueID);
		try {
			if(IsRaw)
			{
				RawRexLexerTranslater Translater;
				for (std::size_t Index = 0; Index < Str.size(); ++Index)
					Translater.Insert(Str[Index], Index);
				Translater.EndOfFile();
				CreateUnfaTable(Translater, Result, AcceptData);
			}
			else {
				RexLexerTranslater Translater;
				for (std::size_t Index = 0; Index < Str.size(); ++Index)
					Translater.Insert(Str[Index], Index);
				Translater.EndOfFile();
				CreateUnfaTable(Translater, Result, AcceptData);
			}
			return Result;
		}
		catch (DLr::Exception::UnaccableSymbol const& Symbol)
		{
			throw Exception::UnaccaptableRegex{ Symbol.TokenIndex, AcceptData };
		}
		catch (...)
		{
			throw;
		}
	}

	struct ExpandResult
	{
		std::vector<std::size_t> ContainNodes;
		std::vector<UnserilizedTable::Edge> Edges;
	};

	struct SearchCore
	{
		std::size_t ToNode;
		std::vector<UnserilizedTable::EdgeProperty> Propertys;
		struct UniqueID
		{
			bool HasUniqueID;
			SerilizeT UniqueID;
		};
		std::optional<UniqueID> UniqueID;
		void AddUniqueID(SerilizeT ID)
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

	ExpandResult SearchEpsilonNode(UnfaTable const& Input, std::size_t SearchNode)
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

				bool CirculDependenc = std::find(Result.ContainNodes.begin(), Result.ContainNodes.end(), Ite.ShiftNode) != Result.ContainNodes.end();
				
				Cur.ToNode = Ite.ShiftNode;

				if (CirculDependenc && Ite.Type != UnfaTable::EdgeType::Consume && !Ite.ConsumeChars.empty()) [[unlikely]]
				{
					throw Exception::CircleShifting{};
				}

				switch (Ite.Type)
				{
				case UnfaTable::EdgeType::Consume:
					if (Ite.ConsumeChars.empty())
					{
						Result.ContainNodes.push_back(Ite.ShiftNode);
						if(IsLast)
							SearchStack.push_back(std::move(Cur));
						else
							SearchStack.push_back(Cur);
					}
					else {
						Cur.AddUniqueID(Ite.UniqueID);
						UnserilizedTable::Edge Edge;
						Edge.ToNode = Ite.ShiftNode;
						if (IsLast)
							Edge.ConsumeChars = std::move(Ite.ConsumeChars);
						else
							Edge.ConsumeChars = Ite.ConsumeChars;
						assert(Cur.UniqueID.has_value()); 
						if (Cur.UniqueID->HasUniqueID)
							Edge.UniqueID = Cur.UniqueID->UniqueID;
						if (IsLast)
							Edge.Propertys = std::move(Cur.Propertys);
						else
							Edge.Propertys = Cur.Propertys;
						Result.Edges.push_back(std::move(Edge));
					}
					break;
				default:
					Result.ContainNodes.push_back(Ite.ShiftNode);
					if (IsLast)
					{
						Cur.AddUniqueID(Ite.UniqueID);
						Cur.Propertys.push_back({ Ite.Type, Ite.AcceptData, Ite.CounterDatas });
						SearchStack.push_back(std::move(Cur));
					}
					else {
						auto NewCur = Cur;
						NewCur.AddUniqueID(Ite.UniqueID);
						NewCur.Propertys.push_back({ Ite.Type, Ite.AcceptData, Ite.CounterDatas });
						SearchStack.push_back(std::move(NewCur));
					}
					break;
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
			// For Acceptable Node
			std::optional<std::size_t> RecordedIndex;
			for (std::size_t Index = 0; Index < Nodes.size(); )
			{
				auto& Ref = Nodes[Index];
				if (Ref.Edges.empty())
				{
					if (RecordedIndex.has_value())
					{
						for (std::size_t Index2 = 0; Index2 < Nodes.size(); ++Index2)
						{
							auto& Temp = Nodes[Index2];
							for (auto& Ite : Temp.Edges)
							{
								if (Ite.ToNode == Index)
								{
									Ite.ToNode = *RecordedIndex;
								}
								else if (Ite.ToNode > Index)
								{
									Ite.ToNode -= 1;
								}
							}
						}
						Nodes.erase(Nodes.begin() + Index);
						continue;
					}
					else {
						RecordedIndex = Index;
					}
				}
				++Index;
			}
		}

		/*
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
		*/
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

	auto TableWrapper::Create(UnserilizedTable const& Table) ->std::vector<SerilizeT>
	{
		std::vector<SerilizeT> Result;
		Result.resize(2);

		Misc::SerilizerHelper::TryCrossTypeSet(Result[0], Table.Nodes.size(), Exception::RegexOutOfRange{});

		struct Record
		{
			std::size_t EdgeOffset;
			std::size_t MappingNode;
		};

		std::vector<Record> Records;

		std::vector<std::size_t> NodeOffset;

		std::vector<ZipChar> Chars;
		std::vector<ZipProperty> Propertys;
		std::vector<SerilizeT> CounterDatas;

		for (auto& Ite : Table.Nodes)
		{
			ZipNode NewNode;
			Misc::SerilizerHelper::TryCrossTypeSet(NewNode.EdgeCount, Ite.Edges.size(), Exception::RegexOutOfRange{});
			auto NodeResult = Misc::SerilizerHelper::WriteObject(Result, NewNode);
			NodeOffset.push_back(NodeResult.StartOffset);
			for (auto EdgeIte = Ite.Edges.rbegin(); EdgeIte != Ite.Edges.rend(); ++EdgeIte)
			{
				auto& Ite2 = *EdgeIte;
				Propertys.clear();
				CounterDatas.clear();
				std::optional<Accept> AcceptData;
				std::size_t CounterPropertyCount = 0;
				for (auto& Ite3 : Ite2.Propertys)
				{
					switch (Ite3.Type)
					{
					case UnfaTable::EdgeType::CounterBegin:
						Propertys.push_back(ZipProperty::CounterBegin);
						++CounterPropertyCount;
						break;
					case UnfaTable::EdgeType::CounterContinue:
						Propertys.push_back(ZipProperty::CounterContinue);
						CounterDatas.push_back(Ite3.CounterData.Max);
						++CounterPropertyCount;
						break;
					case UnfaTable::EdgeType::CounterEnd:
						Propertys.push_back(ZipProperty::CounterEnd);
						CounterDatas.push_back(Ite3.CounterData.Min);
						CounterDatas.push_back(Ite3.CounterData.Max);
						++CounterPropertyCount;
						break;
					}
				}

				for (auto& Ite3 : Ite2.Propertys)
				{
					switch (Ite3.Type)
					{
					case UnfaTable::EdgeType::CaptureBegin:
						Propertys.push_back(ZipProperty::CaptureBegin);
						break;
					case UnfaTable::EdgeType::CaptureEnd:
						Propertys.push_back(ZipProperty::CaptureEnd);
						break;
					case UnfaTable::EdgeType::Acceptable:
						AcceptData = Ite3.AcceptData;
						break;
					}
				}

				ZipEdge Edges;
				if (AcceptData.has_value())
					Misc::SerilizerHelper::TryCrossTypeSet(Edges.AcceptableCharCount, 0, Exception::RegexOutOfRange{});
				else {
					Chars.clear();
					TranslateIntervalT(std::span(Ite2.ConsumeChars), Chars);
					Misc::SerilizerHelper::TryCrossTypeSet(Edges.AcceptableCharCount, Chars.size(), Exception::RegexOutOfRange{});
				}
				
				Edges.HasCounter = (CounterPropertyCount != 0);

				Edges.PropertyCount = static_cast<decltype(Edges.PropertyCount)>(Propertys.size());

				if(Edges.PropertyCount != Propertys.size())
					throw Exception::RegexOutOfRange{};

				Misc::SerilizerHelper::TryCrossTypeSet(Edges.CounterDataCount, CounterDatas.size(), Exception::RegexOutOfRange{});
				if (Ite2.UniqueID.has_value())
				{
					Edges.HasUniqueID = true;
					Edges.UniqueID = *Ite2.UniqueID;
				}
				else {
					Edges.HasUniqueID = false;
					Edges.UniqueID = 0;
				}

				auto EdgeResult = Misc::SerilizerHelper::WriteObject(Result, Edges);
				Records.push_back({ EdgeResult.StartOffset, Ite2.ToNode });
				if (AcceptData.has_value())
				{
					Misc::SerilizerHelper::WriteObject(Result, *AcceptData);
				}
				else {
					Misc::SerilizerHelper::WriteObjectArray(Result, std::span(Chars));
				}
				Misc::SerilizerHelper::WriteObjectArray(Result, std::span(Propertys));
				Misc::SerilizerHelper::WriteObjectArray(Result, std::span(CounterDatas));
				auto EdgeReadResult = Misc::SerilizerHelper::ReadObject<ZipEdge>(std::span(Result).subspan(EdgeResult.StartOffset));

				EdgeReadResult->EdgeTotalLength = Result.size() - EdgeResult.StartOffset;
				if (EdgeReadResult->EdgeTotalLength != Result.size() - EdgeResult.StartOffset)
				{
					throw Exception::RegexOutOfRange{};
				}
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

			struct DebugEdge
			{
				std::size_t ToNode;
				std::span<TableWrapper::ZipProperty const> Propertys;
				std::span<SerilizeT const> CounterDatas;
				std::span<ZipChar const> ConsumeChars;
				std::optional<Accept> AcceptData;
			};

			struct DebugNode
			{
				SerilizeT NodeOffset;
				std::vector<DebugEdge> Edges;
			};

			std::vector<DebugNode> DebugsNode;

			TableWrapper Wrapper(Result);

			for (auto& Ite : NodeOffset)
			{
				SerilizeT Offset = static_cast<SerilizeT>(Ite);
				auto Node = Wrapper[Offset];
				DebugNode DNode;
				DNode.NodeOffset = Node.NodeOffset;
				DNode.Edges.reserve(Node.EdgeCount);
				auto EdgeSpan = Node.AppendData;
				for (std::size_t Index = 0; Index < Node.EdgeCount; ++Index)
				{
					DebugEdge DebugEdges;
					auto IteSpan = EdgeSpan;
					auto EdgesResult = Misc::SerilizerHelper::ReadObject<ZipEdge const>(IteSpan);
					EdgeSpan = EdgeSpan.subspan(EdgesResult->EdgeTotalLength);
					DebugEdges.ToNode = EdgesResult->ToNode;
					if (EdgesResult->AcceptableCharCount != 0)
					{
						auto ConsumeResult = Misc::SerilizerHelper::ReadObjectArray<ZipChar const>(EdgesResult.LastSpan, EdgesResult->AcceptableCharCount);
						DebugEdges.ConsumeChars = *ConsumeResult;
						IteSpan = ConsumeResult.LastSpan;
					}
					else {
						auto AcceptResult = Misc::SerilizerHelper::ReadObject<Accept const>(EdgesResult.LastSpan);
						DebugEdges.AcceptData = *AcceptResult;
						IteSpan = AcceptResult.LastSpan;
					}
					auto PropertyResult = Misc::SerilizerHelper::ReadObjectArray<ZipProperty const>(IteSpan, EdgesResult->PropertyCount);
					DebugEdges.Propertys = *PropertyResult;
					auto CounterResult = Misc::SerilizerHelper::ReadObjectArray<SerilizeT const>(PropertyResult.LastSpan, EdgesResult->CounterDataCount);
					DebugEdges.CounterDatas = *CounterResult;
					DNode.Edges.push_back(DebugEdges);
				}
				std::reverse(DNode.Edges.begin(), DNode.Edges.end());
				DebugsNode.push_back(std::move(DNode));
			}

			volatile int i = 0;
		}
#endif

		return Result;
	}

	auto TableWrapper::operator[](SerilizeT Offset) const ->NodeViewer
	{
		auto Result = Misc::SerilizerHelper::ReadObject<ZipNode const>(Wrapper.subspan(Offset));
		NodeViewer Viewer;
		Viewer.NodeOffset = Offset;
		Viewer.EdgeCount = Result->EdgeCount;
		Viewer.AppendData = Result.LastSpan;
		return Viewer;
	}

	std::vector<SerilizeT> TableWrapper::Create(std::u32string_view Str, Accept Mask, SerilizeT UniqueID, bool IsRaw)
	{
		auto Tab1 = Reg::UnfaTable::Create(Str, IsRaw, Mask, UniqueID);
		return Create(Tab1);
	}

	CoreProcessor::CoreProcessor(TableWrapper Wrapper, bool KeepAcceptableViewer, std::size_t InputStartupTokenIndex)
		: Wrapper(Wrapper), CurrentState(Wrapper.StartupNodeOffset()), KeepAcceptableViewer(KeepAcceptableViewer) , RequireTokenIndex(InputStartupTokenIndex),
		StartupTokenIndex(InputStartupTokenIndex)
	{

	}

	std::optional<CoreProcessor::ConsumeResult> CoreProcessor::ConsumeTokenInput(char32_t InputSymbols, std::size_t NextTokenIndex)
	{

		auto Node = Wrapper[CurrentState];
		auto SpanData = Node.AppendData;

		for (std::size_t Index = 0; Index < Node.EdgeCount; ++Index)
		{
			auto ZipEdge = Misc::SerilizerHelper::ReadObject<TableWrapper::ZipEdge const>(SpanData);
			SpanData = SpanData.subspan(ZipEdge->EdgeTotalLength);

			AmbiguousPoint Point;
			std::optional<Accept> AcceptResult;
			std::span<SerilizeT const> SpanIte;

			if (ZipEdge->AcceptableCharCount == 0)
			{
				if (KeepAcceptableViewer || InputSymbols == MaxChar())
				{
					auto Acce = Misc::SerilizerHelper::ReadObject<Accept const>(ZipEdge.LastSpan);
					Point.AcceptData = *Acce;
					SpanIte = Acce.LastSpan;
				}
				else
					continue;
			}
			else {
				auto Acce = Misc::SerilizerHelper::ReadObjectArray<TableWrapper::ZipChar const>(ZipEdge.LastSpan, ZipEdge->AcceptableCharCount);
				if (!Acceptable(InputSymbols, *Acce))
					continue;
				SpanIte = Acce.LastSpan;
			}

			if (ZipEdge->PropertyCount != 0)
			{
				auto RR = Misc::SerilizerHelper::ReadObjectArray<TableWrapper::ZipProperty const>(SpanIte, ZipEdge->PropertyCount);
				Point.Propertys = *RR;
				SpanIte = RR.LastSpan;
			}

			if (ZipEdge->HasCounter)
			{
				std::size_t Count = ZipEdge->CounterDataCount;
				auto RR = Misc::SerilizerHelper::ReadObjectArray<SerilizeT const>(SpanIte, Count);
				bool Pass = true;

				std::size_t LastCounterCount = CounterRecord.size();
				std::size_t CounterDataIte = 0;
				bool MeetContinue = false;
				bool MeetEnd = false;

				for (std::size_t Index = 0; Pass && Index < Point.Propertys.size(); ++Index)
				{
					switch (Point.Propertys[Index])
					{
					case TableWrapper::ZipProperty::CounterBegin:
						LastCounterCount += 1;
						break;
					case TableWrapper::ZipProperty::CounterContinue:
					{
						MeetContinue = true;
						assert(!MeetEnd);
						assert(CounterDataIte < RR->size());
						auto Range = (*RR)[CounterDataIte];
						++CounterDataIte;
						std::size_t CurCount = 0;
						assert(LastCounterCount >= 1);
						if (LastCounterCount <= CounterRecord.size())
							CurCount = CounterRecord[LastCounterCount - 1];
						if (CurCount >= Range)
							Pass = false;
						break;
					}
					case TableWrapper::ZipProperty::CounterEnd:
					{
						MeetEnd = true;
						assert(!MeetContinue);
						assert(CounterDataIte + 1 < RR->size());
						auto RangeMin = (*RR)[CounterDataIte];
						++CounterDataIte;
						auto RangeMax = (*RR)[CounterDataIte];
						++CounterDataIte;
						assert(LastCounterCount >= 1);
						std::size_t CurCount = 0;
						assert(LastCounterCount >= 1);
						if (LastCounterCount <= CounterRecord.size())
							CurCount = CounterRecord[LastCounterCount - 1];
						if (CurCount < RangeMin || CurCount >= RangeMax)
							Pass = false;
						else
							--LastCounterCount;
						break;
					}
					default:
						Index = Point.Propertys.size();
						break;
					}
				}

				if(!Pass)
					continue;
			}

			Point.ToNode = ZipEdge->ToNode;
			Point.CaptureCount = CaptureRecord.size();
			Point.CounterHistory = CounterHistory;
			Point.NextTokenIndex = NextTokenIndex;
			Point.CurrentTokenIndex = RequireTokenIndex;

			if(ZipEdge->HasUniqueID)
				Point.UniqueID = ZipEdge->UniqueID;

			if (AmbiguousPoints.empty() || AmbiguousPoints.rbegin()->CounterHistory != CounterHistory)
			{
				Point.CounterIndex = {AmbiguousCounter.size(), CounterRecord.size()};
				AmbiguousCounter.insert(AmbiguousCounter.end(), CounterRecord.begin(), CounterRecord.end());
			}
			else {
				Point.CounterIndex = AmbiguousPoints.rbegin()->CounterIndex;
			}

			AmbiguousPoints.push_back(Point);
		}

		if (AmbiguousPoints.empty())
		{
			return {};
		}

		auto Ite = *AmbiguousPoints.rbegin();
		AmbiguousPoints.pop_back();

		if (Ite.CounterHistory != CounterHistory)
		{
			CounterRecord.clear();
			auto Require = Ite.CounterIndex.Slice(AmbiguousCounter);
			CounterRecord.insert(CounterRecord.end(), Require.begin(), Require.end());
		}

		if (AmbiguousPoints.empty() || AmbiguousPoints.rbegin()->CounterHistory != Ite.CounterHistory)
			AmbiguousCounter.resize(Ite.CounterIndex.Begin());

		CurrentState = Ite.ToNode;

		bool IsEndOfFile = (RequireTokenIndex == Ite.NextTokenIndex && InputSymbols == EndOfFile());

		RequireTokenIndex = Ite.NextTokenIndex;

		CaptureRecord.resize(Ite.CaptureCount);

		bool CounterChange = false;
		for (auto& Ite2 : Ite.Propertys)
		{
			switch (Ite2)
			{
			case TableWrapper::ZipProperty::CaptureBegin:
				CaptureRecord.push_back({true, Ite.CurrentTokenIndex});
				break;
			case TableWrapper::ZipProperty::CaptureEnd:
				CaptureRecord.push_back({ false, Ite.CurrentTokenIndex });
				break;
			case TableWrapper::ZipProperty::CounterBegin:
				CounterRecord.push_back(0);
				CounterChange = true;
				break;
			case TableWrapper::ZipProperty::CounterContinue:
				assert(!CounterRecord.empty());
				++(* CounterRecord.rbegin());
				CounterChange = true;
				break;
			case TableWrapper::ZipProperty::CounterEnd:
				assert(!CounterRecord.empty());
				CounterRecord.pop_back();
				CounterChange = true;
				break;
			default:
				assert(false);
				break;
			}
		}

		if (CounterChange)
		{
			++CounterHistory;
			if (CounterHistory == std::numeric_limits<decltype(CounterHistory)>::max())
			{
				if (AmbiguousPoints.empty())
				{
					CounterHistory = 0;
				}
				else {
					std::size_t HistoryIte = 0;
					std::size_t LastHistory = AmbiguousPoints.begin()->CounterHistory;
					for (auto& Ite : AmbiguousPoints)
					{
						if (Ite.CounterHistory != LastHistory)
						{
							LastHistory = Ite.CounterHistory;
							++HistoryIte;
						}
						Ite.CounterHistory = HistoryIte;
					}
					CounterHistory = HistoryIte;
				}
			}
		}

		ConsumeResult Result;
		if(Ite.AcceptData.has_value())
		{
			Result.Accept = {*Ite.AcceptData, *Ite.UniqueID, std::span(CaptureRecord)};
		}
		Result.CurrentTokenIndex = Ite.CurrentTokenIndex;
		Result.StartupTokenIndex = StartupTokenIndex;
		Result.IsEndOfFile = IsEndOfFile;
		return Result;
	}

	void CoreProcessor::Reset(std::size_t InputStartupTokenIndex)
	{
		AmbiguousCounter.clear();
		CounterRecord.clear();
		CounterHistory = 0;
		AmbiguousPoints.clear();
		CaptureRecord.clear();
		CurrentState = Wrapper.StartupNodeOffset();
		RequireTokenIndex = InputStartupTokenIndex;
		StartupTokenIndex = InputStartupTokenIndex;
	}

	void CoreProcessor::RemoveAmbiuosPoint(SerilizeT UniqueID)
	{
		if (!AmbiguousPoints.empty())
		{
			for (std::size_t Index = 0; Index < AmbiguousPoints.size();)
			{
				auto& Cur = AmbiguousPoints[Index];
				if (Cur.UniqueID.has_value() && *Cur.UniqueID == UniqueID)
				{
					bool SameHistory = false;
					if (!SameHistory && Index > 0)
						SameHistory = (AmbiguousPoints[Index - 1].CounterHistory == Cur.CounterHistory);
					if(!SameHistory && Index + 1< AmbiguousPoints.size())
						SameHistory = (AmbiguousPoints[Index + 1].CounterHistory == Cur.CounterHistory);
					if (!SameHistory && Cur.CounterIndex.Count() != 0)
					{
						CounterRecord.erase(CounterRecord.begin() + Cur.CounterIndex.Begin(), CounterRecord.begin() + Cur.CounterIndex.End());
						for (std::size_t Index2 = Index + 1; Index2 < AmbiguousPoints.size(); ++Index2)
						{
							AmbiguousPoints[Index2].CounterIndex.Offset -= Cur.CounterIndex.End();
						}
					}
					AmbiguousPoints.erase(AmbiguousPoints.begin() + Index);
				}
				else {
					++Index;
				}
			}
		}
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

	ProcessResult::ProcessResult(CoreProcessor::ConsumeResult const& Result)
	{
		assert(Result.Accept.has_value());
		std::vector<Capture> TempCaptures;
		TempCaptures.insert(TempCaptures.end(), Result.Accept->CaptureData.begin(), Result.Accept->CaptureData.end());
		Captures = std::move(TempCaptures);
		MainCapture = { Result .StartupTokenIndex, Result .CurrentTokenIndex - Result .StartupTokenIndex};
		AcceptData = Result.Accept->AcceptData;
		UniqueID = Result.Accept->UniqueID;
	}

	auto MatchProcessor::ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex)->std::optional<Result>
	{
		auto Re1 = Processor.ConsumeTokenInput(Token, NextTokenIndex);
		if (Re1.has_value())
		{
			Result ReturnResult;
			ReturnResult.IsEndOfFile = Re1->IsEndOfFile;
			if (Re1->Accept.has_value())
			{
				ReturnResult.Accept = ProcessResult{*Re1};
			}
			return ReturnResult;
		}
		else {
			return {};
		}
	}

	std::optional<ProcessResult> ProcessMatch(TableWrapper Wrapper, std::u32string_view Span)
	{
		MatchProcessor Pro(Wrapper);
		while (true)
		{
			auto Re = Pro.GetRequireTokenIndex();
			char32_t Input = (Re >= Span.size() ? EndOfFile() : Span[Re]);
			auto Re2 = Pro.ConsumeTokenInput(Input, Re + 1);
			if (Re2.has_value())
			{
				if(Re2->Accept.has_value())
					return Re2->Accept;
				if(Re2->IsEndOfFile)
					return {};
			}
			else {
				return {};
			}
		}
		return {};
	}

	auto FrontMatchProcessor::ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex) -> std::optional<Result>
	{
		auto Re1 = Processor.ConsumeTokenInput(Token, NextTokenIndex);
		if (Re1.has_value())
		{
			Result ReturnResult;
			ReturnResult.IsEndOfFile = Re1->IsEndOfFile;
			if (Re1->Accept.has_value())
			{
				ReturnResult.Accept = ProcessResult{ *Re1 };
			}
			return ReturnResult;
		}
		else {
			return {};
		}
	}

	std::optional<ProcessResult> ProcessFrontMatch(TableWrapper Wrapper, std::u32string_view Span)
	{
		FrontMatchProcessor Pro(Wrapper);
		while (true)
		{
			auto Re = Pro.GetRequireTokenIndex();
			char32_t Input = (Re >= Span.size() ? EndOfFile() : Span[Re]);
			auto Re2 = Pro.ConsumeTokenInput(Input, Re + 1);
			if (Re2.has_value())
			{
				if (Re2->Accept.has_value())
					return Re2->Accept;
				if(Re2->IsEndOfFile)
					return {};
			}
			else {
				return {};
			}
		}
		return {};
	}

	auto SearchProcessor::ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex) -> std::optional<Result>
	{
		auto R1 = Processor.ConsumeTokenInput(Token, NextTokenIndex);
		if (R1.has_value())
		{
			Result ReturnResult;
			ReturnResult.IsEndOfFile = R1->IsEndOfFile;
			if (R1->Accept.has_value())
			{
				ReturnResult.Accept = ProcessResult{ *R1 };
				return ReturnResult;
			}

			if (Processor.GetRequireTokenIndex() == RetryStartupTokenIndex && Processor.GetCurrentState() == Processor.GetWrapper().StartupNodeOffset())
			{
				++RetryStartupTokenIndex;
			}

			return ReturnResult;
		}
		else {
			if(Token == EndOfFile())
				return {};
			Processor.Reset(RetryStartupTokenIndex);
			++RetryStartupTokenIndex;
			Result ReturnResult;
			ReturnResult.IsEndOfFile = false;
			return ReturnResult;
		}
	}

	std::optional<ProcessResult> ProcessSearch(TableWrapper Wrapper, std::u32string_view Span)
	{
		SearchProcessor Pro(Wrapper);
		while (true)
		{
			auto Re = Pro.GetRequireTokenIndex();
			char32_t Input = (Re >= Span.size() ? EndOfFile() : Span[Re]);
			auto Re2 = Pro.ConsumeTokenInput(Input, Re + 1);
			if (Re2.has_value())
			{
				if (Re2->Accept.has_value())
					return Re2->Accept;
				if(Re2->IsEndOfFile)
					return {};
			}
			else {
				return {};
			}
		}
		return {};
	}

	auto GreedyFrontMatchProcessor::ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex) -> std::optional<Result>
	{
		auto R1 = Processor.ConsumeTokenInput(Token, NextTokenIndex);
		if (R1.has_value())
		{
			Result ReturnResult;
			ReturnResult.IsEndOfFile = R1->IsEndOfFile;
			if (R1->Accept.has_value())
			{
				if (!TempResult.has_value() || (TempResult->UniqueID != R1->Accept->UniqueID && TempResult->MainCapture.Count() < R1->CurrentTokenIndex))
				{
					TempResult = ProcessResult{ *R1 };
				}
				if (R1->IsEndOfFile)
				{
					ReturnResult.Accept = std::move(TempResult);
					return ReturnResult;
				}
				Processor.RemoveAmbiuosPoint(R1->Accept->UniqueID);
			}
			return ReturnResult;
		}
		else {
			if (TempResult.has_value())
			{
				Result ReturnResult;
				ReturnResult.Accept = std::move(TempResult);
				TempResult.reset();
				ReturnResult.IsEndOfFile = false;
				return ReturnResult;
			}
			return {};
		}
	}

	std::optional<ProcessResult> ProcessGreedyFrontMatch(TableWrapper Wrapper, std::u32string_view Span)
	{
		GreedyFrontMatchProcessor Pro(Wrapper);
		while (true)
		{
			auto Re = Pro.GetRequireTokenIndex();
			char32_t Input = (Re >= Span.size() ? EndOfFile() : Span[Re]);
			auto Re2 = Pro.ConsumeTokenInput(Input, Re + 1);
			if (Re2.has_value())
			{
				if (Re2->Accept.has_value())
					return Re2->Accept;
				if (Re2->IsEndOfFile)
					return {};
			}
			else {
				return {};
			}
		}
		return {};
	}


	SerilizeT MulityRegexCreator::Push(UnfaTable const& UT)
	{
		if (Temporary.has_value())
		{
			Temporary->Link(UT, true);
		}
		else {
			Temporary = UT;
		}
		++CountedUniqueID;
		return CountedUniqueID;
	}

	SerilizeT MulityRegexCreator::AddRegex(std::u32string_view Regex, Accept Mask, SerilizeT UniqueID, bool IsRaw)
	{
		return Push(UnfaTable::Create(Regex, IsRaw, Mask, UniqueID));
	}


	std::vector<SerilizeT> MulityRegexCreator::Generate() const
	{
		assert(Temporary.has_value());
		return TableWrapper::Create(*Temporary);
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