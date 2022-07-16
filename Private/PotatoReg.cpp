#include "../Public/PotatoReg.h"
#include "../Public/PotatoSLRX.h"
#include "../Public/PotatoStrEncode.h"
#include <deque>
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

	constexpr Symbol operator*(T Input) { return Symbol{ Symbol::ValueType::TERMINAL, static_cast<SLRX::StandardT>(Input) }; };

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

	constexpr Symbol operator*(NT Input) { return Symbol{ Symbol::ValueType::NOTERMIAL, static_cast<SLRX::StandardT>(Input) }; };

	struct LexerElement
	{
		T Value;
		char32_t MappingSymbol;
		std::variant<SeqIntervalT, char32_t> Acceptable;
		std::size_t TokenIndex;
	};

	struct LexerTranslater
	{

		std::span<LexerElement const> GetSpan() const { return StoragedSymbol;};

		void Insert(bool IsRaw, char32_t InputSymbol);
		void EndOfFile();
		std::size_t GetNextTokenIndex() const { return TokenIndexIte; }

	protected:

		enum class State
		{
			Normal,
			Transfer,
			BigNumber,
			Number,
			Done,
		};

		State CurrentState = State::Normal;
		std::size_t Number = 0;
		char32_t NumberChar = 0;
		bool NumberIsBig = false;
		char32_t RecordSymbol;
		std::size_t RecordTokenIndex = 0;
		std::size_t TokenIndexIte = 0;

		void PushSymbolData(T InputSymbol, char32_t Character, std::variant<SeqIntervalT, char32_t> Value, std::size_t TokenIndex) { 
			StoragedSymbol.push_back({InputSymbol, Character, std::move(Value), TokenIndex });
		}
		std::vector<LexerElement> StoragedSymbol;
	};

	void LexerTranslater::Insert(bool IsRaw, char32_t InputSymbol)
	{
		if (InputSymbol < MaxChar())
		{
			auto TokenIndex = TokenIndexIte;
			++TokenIndexIte;
			if (IsRaw)
			{
				if (CurrentState == State::Normal)
				{
					PushSymbolData(T::SingleChar, InputSymbol, InputSymbol, TokenIndex);
					return;
				}
				else {
					throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::RawRegexInNotNormalState, InputSymbol, TokenIndex };
				}
			}

			switch (CurrentState)
			{
			case State::Normal:
			{
				switch (InputSymbol)
				{
				case U'-':
					PushSymbolData(T::Min, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'[':
					PushSymbolData(T::BracketsLeft, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U']':
					PushSymbolData(T::BracketsRight, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'{':
					PushSymbolData(T::CurlyBracketsLeft, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'}':
					PushSymbolData(T::CurlyBracketsRight, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U',':
					PushSymbolData(T::Comma, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'(':
					PushSymbolData(T::ParenthesesLeft, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U')':
					PushSymbolData(T::ParenthesesRight, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'*':
					PushSymbolData(T::Mulity, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'?':
					PushSymbolData(T::Question, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'.':
					PushSymbolData(T::CharSet, InputSymbol, MaxIntervalRange(), TokenIndex);
					break;
				case U'|':
					PushSymbolData(T::Or, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'+':
					PushSymbolData(T::Add, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'^':
					PushSymbolData(T::Not, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U':':
					PushSymbolData(T::Colon, InputSymbol, InputSymbol, TokenIndex);
					break;
				case U'\\':
					CurrentState = State::Transfer;
					RecordSymbol = InputSymbol;
					RecordTokenIndex = TokenIndex;
					break;
				default:
					if (InputSymbol >= U'0' && InputSymbol <= U'9')
						PushSymbolData(T::Num, InputSymbol, InputSymbol, TokenIndex);
					else
						PushSymbolData(T::SingleChar, InputSymbol, InputSymbol, TokenIndex);
					break;
				}
				break;
			}
			case State::Transfer:
			{
				switch (InputSymbol)
				{
				case U'd':
					PushSymbolData(T::CharSet, InputSymbol, SeqIntervalT{ {U'0', U'9' + 1} }, RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				case U'D':
				{
					SeqIntervalT Tem{ { {1, U'0'},{U'9' + 1, MaxChar()} } };
					PushSymbolData(T::CharSet, InputSymbol, std::move(Tem), RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				}
				case U'f':
					PushSymbolData(T::SingleChar, InputSymbol, U'\f', RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				case U'n':
					PushSymbolData(T::SingleChar, InputSymbol, U'\n', RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				case U'r':
					PushSymbolData(T::SingleChar, InputSymbol, U'\r', RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				case U't':
					PushSymbolData(T::SingleChar, InputSymbol, U'\t', RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				case U'v':
					PushSymbolData(T::SingleChar, InputSymbol, U'\v', RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				case U's':
				{
					SeqIntervalT tem({ IntervalT{ 1, 33 }, IntervalT{127, 128} });
					PushSymbolData(T::CharSet, InputSymbol, std::move(tem), RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				}
				case U'S':
				{
					SeqIntervalT tem({ IntervalT{33, 127}, IntervalT{128, MaxChar()} });
					PushSymbolData(T::CharSet, InputSymbol, std::move(tem), RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				}
				case U'w':
				{
					SeqIntervalT tem({ IntervalT{ U'a', U'z' + 1 }, IntervalT{ U'A', U'Z' + 1 }, IntervalT{ U'_', U'_' + 1} });
					PushSymbolData(T::CharSet, InputSymbol, std::move(tem), RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				}
				case U'W':
				{
					SeqIntervalT tem({ IntervalT{ U'a', U'z' + 1 }, IntervalT{ U'A', U'Z' + 1 }, IntervalT{ U'_', U'_' + 1} });
					SeqIntervalT total({ 1, MaxChar() });
					PushSymbolData(T::CharSet, InputSymbol, tem.Complementary(MaxIntervalRange()), RecordTokenIndex);
					CurrentState = State::Normal;
					break;
				}
				case U'z':
				{
					SeqIntervalT tem(IntervalT{ 256, MaxChar() });
					PushSymbolData(T::CharSet, InputSymbol, std::move(tem), RecordTokenIndex);
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
					PushSymbolData(T::SingleChar, InputSymbol, InputSymbol, RecordTokenIndex);
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
					throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::UnaccaptableNumber, InputSymbol, TokenIndex };
				}
				if ((Number == 4 && !NumberIsBig) || (NumberIsBig && Number == 6))
				{
					PushSymbolData(T::SingleChar, InputSymbol, NumberChar, RecordTokenIndex);
					CurrentState = State::Normal;
				}
				break;
			}
			default:
				assert(false);
			}
		}else {
			throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::OutOfCharRange, InputSymbol, TokenIndexIte};
		}
	}

	void LexerTranslater::EndOfFile()
	{
		if(CurrentState == State::Normal)
			CurrentState = State::Done;
		else
			throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::UnSupportEof, Reg::EndOfFile(), TokenIndexIte};
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
					{*NT::CharList, {*NT::CharList, *NT::CharList, 3}, 3},
					
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

	}

	std::any RegNoTerminalFunction(Potato::SLRX::NTElement& NT, UnfaTable& Output)
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
			Output.AddComsumeEdge(T1, T2, { {Tar, Tar + 1} });
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
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Min, Count, RegexOutOfRange::TypeT::Counter, Count);
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Max, Count + 1, RegexOutOfRange::TypeT::Counter, Count + 1);
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
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Min, static_cast<std::size_t>(0), RegexOutOfRange::TypeT::Counter, static_cast<std::size_t>(0));
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Max, Count + 1, RegexOutOfRange::TypeT::Counter, Count + 1);
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
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Min, Count, RegexOutOfRange::TypeT::Counter, Count);
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Max, std::numeric_limits<StandardT>::max(), RegexOutOfRange::TypeT::Counter, std::numeric_limits<StandardT>::max());
			Output.AddCounter(Set, Last, Tem, NT.Datas.size() == 5);
			return Set;
		}
		case 23:
		{
			auto Count = NT[2].Consume<std::size_t>();
			auto Count2 = NT[4].Consume<std::size_t>();
			Counter Tem;
			if (Count <= Count2)
			{
				Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Min, Count, RegexOutOfRange::TypeT::Counter, Count );
				Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Max, Count2 + 1, RegexOutOfRange::TypeT::Counter, Count2 + 1 );
			}
			else {
				Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Min, Count2, RegexOutOfRange::TypeT::Counter, Count2 );
				Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Tem.Max, Count + 1, RegexOutOfRange::TypeT::Counter, Count + 1 );
			}
			auto T1 = Output.NewNode();
			auto T2 = Output.NewNode();
			auto Last = NT[0].Consume<NodeSet>();
			NodeSet Set{ T1, T2 };
			Output.AddCounter(Set, Last, Tem, NT.Datas.size() == 6);
			return Set;
		}
		default:
			assert(false);
			return {};
			break;
		}
	}

	std::any RegTerminalFunction(Potato::SLRX::TElement& Ele, LexerTranslater& Translater)
	{
		auto& Ref = Translater.GetSpan()[Ele.TokenIndex].Acceptable;
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

	void CreateUnfaTable(LexerTranslater& Translater, UnfaTable& Output, Accept AcceptData)
	{
		auto N1 = Output.NewNode();
		auto N2 = Output.NewNode();
		auto N3 = Output.NewNode();
		Output.AddComsumeEdge(N2, N3, { {MaxChar(), MaxChar() + 1} });
		auto Wrapper = RexSLRXWrapper();

		try {
			SLRX::CoreProcessor Pro(Wrapper);
			for (auto& Ite : Translater.GetSpan())
			{
				Pro.Consume(*Ite.Value);
			}
			auto Steps = Pro.EndOfFile();

			//auto Steps = SLRX::ProcessParsingStep(Wrapper, 0, [&](std::size_t Input){ return Translater.GetSymbol(Input); });
			UnfaTable::NodeSet Set = SLRX::ProcessParsingStepWithOutputType<UnfaTable::NodeSet>(std::span(Steps),
				[&](SLRX::VariantElement Elements)-> std::any {
					if (Elements.IsTerminal())
						return RegTerminalFunction(Elements.AsTerminal(), Translater);
					else
						return RegNoTerminalFunction(Elements.AsNoTerminal(), Output);
				}
			);
			Output.AddComsumeEdge(N1, Set.In, {});
			Output.AddAcceptableEdge(Set.Out, N2, AcceptData);
		}
		catch (SLRX::Exception::UnaccableSymbol const& Symbol)
		{
			auto Span = Translater.GetSpan();
			if (Span.size() > Symbol.TokenIndex)
			{
				auto& Ref = Span[Symbol.TokenIndex];
				throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Ref.MappingSymbol, Ref.TokenIndex};
			}
			else {
				throw UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Reg::EndOfFile(), Translater.GetNextTokenIndex()};
			}	
		}
	}

	UnfaTable UnfaTable::Create(std::u32string_view Str, bool IsRaw, Accept AcceptData, StandardT UniqueID)
	{
		UnfaTable Result(UniqueID);
		LexerTranslater Tran;

		for (auto Ite : Str)
		{
			Tran.Insert(IsRaw, Ite);
		}
		Tran.EndOfFile();
		CreateUnfaTable(Tran, Result, AcceptData);

		return Result;
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
			StandardT UniqueID;
		};
		std::optional<UniqueID> UniqueID;
		void AddUniqueID(StandardT ID)
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

	bool CheckProperty(std::vector<std::size_t>& Temp, UnserilizedTable::EdgeProperty Ite)
	{
		switch (Ite.Type)
		{
		case UnfaTable::EdgeType::CounterBegin:
			Temp.push_back(0);
			break;
		case UnfaTable::EdgeType::CounterContinue:
			if (!Temp.empty())
				*Temp.rbegin() += 1;
			break;
		case UnfaTable::EdgeType::CounterEnd:
			if (!Temp.empty())
			{
				auto Last = *Temp.rbegin();
				if (Last >= Ite.CounterData.Min && Last < Ite.CounterData.Max)
				{
					Temp.pop_back();
				}
				else
					return false;
			}
			break;
		default:
			break;
		}
		return true;
	}

	bool CheckPropertys(std::span<UnserilizedTable::EdgeProperty const> Source, UnserilizedTable::EdgeProperty Last)
	{
		std::vector<std::size_t> Temp;
		for (auto& Ite : Source)
		{
			if(!CheckProperty(Temp, Ite))
				return false;
		}
		return CheckProperty(Temp, Last);
	}

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
				case UnfaTable::EdgeType::CounterBegin:
				case UnfaTable::EdgeType::CounterContinue:
				case UnfaTable::EdgeType::CounterEnd:
					if(!CheckPropertys(Cur.Propertys, {Ite.Type, {}, Ite.CounterDatas}))
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

	auto TableWrapper::Create(UnserilizedTable const& Table) ->std::vector<StandardT>
	{
		std::vector<StandardT> Result;
		Result.resize(2);

		Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Result[0], Table.Nodes.size(), RegexOutOfRange::TypeT::Node, Table.Nodes.size());

		struct Record
		{
			std::size_t EdgeOffset;
			std::size_t MappingNode;
		};

		std::vector<Record> Records;

		std::vector<std::size_t> NodeOffset;

		std::vector<ZipChar> Chars;
		std::vector<ZipProperty> Propertys;
		std::vector<StandardT> CounterDatas;

		for (auto& Ite : Table.Nodes)
		{
			ZipNode NewNode;
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(NewNode.EdgeCount, Ite.Edges.size(), RegexOutOfRange::TypeT::EdgeCount, Ite.Edges.size());
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
					Edges.AcceptableCharCount = 0;
				else {
					Chars.clear();
					TranslateIntervalT(std::span(Ite2.ConsumeChars), Chars);
					Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Edges.AcceptableCharCount, Chars.size(), RegexOutOfRange::TypeT::AcceptableCharCount, Chars.size());
				}
				
				Edges.HasCounter = (CounterPropertyCount != 0);

				Edges.PropertyCount = static_cast<decltype(Edges.PropertyCount)>(Propertys.size());

				if(Edges.PropertyCount != Propertys.size())
					throw Exception::RegexOutOfRange{ RegexOutOfRange::TypeT::PropertyCount, Propertys.size() };

				Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Edges.CounterDataCount, CounterDatas.size(), RegexOutOfRange::TypeT::CounterCount, CounterDatas.size());
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
					throw Exception::RegexOutOfRange{ RegexOutOfRange::TypeT::EdgeLength, Result.size() - EdgeResult.StartOffset };
				}
			}
		}
		Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Result[1], NodeOffset[0], RegexOutOfRange::TypeT::Node, NodeOffset[0]);

		for (auto& Ite : Records)
		{
			auto Ptr = Misc::SerilizerHelper::ReadObject<ZipEdge>(std::span(Result).subspan(Ite.EdgeOffset));
			Misc::SerilizerHelper::TryCrossTypeSet<RegexOutOfRange>(Ptr->ToNode, NodeOffset[Ite.MappingNode], RegexOutOfRange::TypeT::Node , NodeOffset[Ite.MappingNode]);
		}

#if _DEBUG
		{

			struct DebugEdge
			{
				std::size_t ToNode;
				std::span<TableWrapper::ZipProperty const> Propertys;
				std::span<StandardT const> CounterDatas;
				std::span<ZipChar const> ConsumeChars;
				std::optional<Accept> AcceptData;
			};

			struct DebugNode
			{
				StandardT NodeOffset;
				std::vector<DebugEdge> Edges;
			};

			std::vector<DebugNode> DebugsNode;

			TableWrapper Wrapper(Result);

			for (auto& Ite : NodeOffset)
			{
				StandardT Offset = static_cast<StandardT>(Ite);
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
					auto CounterResult = Misc::SerilizerHelper::ReadObjectArray<StandardT const>(PropertyResult.LastSpan, EdgesResult->CounterDataCount);
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

	auto TableWrapper::operator[](StandardT Offset) const ->NodeViewer
	{
		auto Result = Misc::SerilizerHelper::ReadObject<ZipNode const>(Wrapper.subspan(Offset));
		NodeViewer Viewer;
		Viewer.NodeOffset = Offset;
		Viewer.EdgeCount = Result->EdgeCount;
		Viewer.AppendData = Result.LastSpan;
		return Viewer;
	}

	std::vector<StandardT> TableWrapper::Create(std::u32string_view Str, Accept Mask, StandardT UniqueID, bool IsRaw)
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
			std::span<StandardT const> SpanIte;

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
				auto RR = Misc::SerilizerHelper::ReadObjectArray<StandardT const>(SpanIte, Count);
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

	void CoreProcessor::RemoveAmbiuosPoint(StandardT UniqueID)
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


	StandardT MulityRegexCreator::Push(UnfaTable const& UT)
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

	StandardT MulityRegexCreator::AddRegex(std::u32string_view Regex, Accept Mask, StandardT UniqueID, bool IsRaw)
	{
		return Push(UnfaTable::Create(Regex, IsRaw, Mask, UniqueID));
	}


	std::vector<StandardT> MulityRegexCreator::Generate() const
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