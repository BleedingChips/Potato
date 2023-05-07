module;

export module Potato.Reg;
export import Potato.SLRX;
export import Potato.Interval;

export namespace Potato::Reg
{
	
	

	using IntervalT = Misc::IntervalT<char32_t>;

	inline constexpr char32_t MaxChar() { return 0x110000; };
	inline constexpr char32_t EndOfFile() { return 0; }

	struct RegLexerT
	{
		
		enum class ElementEnumT : Potato::SLRX::StandardT
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

		struct ElementT
		{
			ElementEnumT Value;
			IntervalT Chars;
			Misc::IndexSpan<> Token;
		};

		std::span<ElementT const> GetSpan() const { return std::span(StoragedSymbol); }

		bool Consume(char32_t InfoutSymbol, Misc::IndexSpan<> TokenIndex);
		bool EndOfFile();

		RegLexerT(bool IsRaw = false);

	protected:

		enum class StateT
		{
			Normal,
			Transfer,
			BigNumber,
			Number,
			Done,
			Raw,
		};

		StateT CurrentState;
		std::size_t Number = 0;
		char32_t NumberChar = 0;
		bool NumberIsBig = false;
		char32_t RecordSymbol = 0;
		std::size_t RecordTokenIndex = 0;
		std::size_t TokenIndexIte = 0;
		std::vector<ElementT> StoragedSymbol;
	};


	struct NfaT
	{

		static NfaT Create(std::u32string_view Str, bool IsRaw = false, std::size_t Mask = 0);
		
		NfaT(std::u32string_view Str, bool IsRaw = false, std::size_t Mask = 0)
			: NfaT(Create(Str, IsRaw, Mask)) {}
		NfaT(NfaT const&) = default;
		NfaT(NfaT&&) = default;
		void Link(NfaT const&);

	protected:

		NfaT(std::span<RegLexerT::ElementT const> InputSpan, std::size_t Mask = 0);
		NfaT() = default;

		
		struct NodeSetT
		{
			std::size_t In;
			std::size_t Out;
		};

		std::size_t AddNode();

		struct ContentT
		{
			Misc::IndexSpan<> TokenIndex;
			std::span<RegLexerT::ElementT const> Tokens;
		};

		void AddConsume(NodeSetT Set, IntervalT Chars, ContentT Content);
		NodeSetT AddCapture(NodeSetT Inside, ContentT Content, std::size_t CaptureIndex);
		NodeSetT AddCounter(NodeSetT Inside, std::optional<std::size_t> Min, std::optional<std::size_t> Max, bool Greedy, ContentT Content, std::size_t CaptureIndex);

		enum class EdgePropertyE
		{
			CaptureBegin,
			CaptureEnd,
			OneCounter,
			AddCounter,
			LessCounter,
			BiggerCounter,
		};

		struct PropertyT
		{
			EdgePropertyE Type = EdgePropertyE::CaptureBegin;
			std::size_t Index = 0;
			std::size_t Par = 0;
			bool operator==(PropertyT const& T1) const { return Type == T1.Type && Index == T1.Index && Par == T1.Par; }
		};

		struct EdgeT
		{
			std::vector<PropertyT> Propertys;
			std::size_t ToNode;
			IntervalT CharSets;
			Misc::IndexSpan<> TokenIndex;
			std::size_t MaskIndex = 0;
			bool IsEpsilonEdge() const { return CharSets.Size() == 0; }
			bool HasCapture() const;
			bool HasCounter() const;
			bool operator==(EdgeT const& T1) const {
				return ToNode == T1.ToNode && Propertys == T1.Propertys && CharSets == T1.CharSets;
			}
		};

		struct AcceptT
		{
			std::size_t Mask;
			std::size_t MaskIndex;
			bool operator==(AcceptT const& T1) const {
				return Mask == T1.Mask && MaskIndex == T1.MaskIndex;
			}
		};

		struct NodeT
		{
			std::vector<EdgeT> Edges;
			std::size_t CurIndex;
			std::optional<AcceptT> Accept;
			bool operator==(NodeT const& I1) const {
				return Accept == I1.Accept && Edges == I1.Edges;
			}
		};

		static Misc::IndexSpan<> CollectTokenIndexFromNodePath(std::span<NodeT const> NodeView, Misc::IndexSpan<> Default, std::span<std::size_t const> NodeStateView);

		std::vector<NodeT> Nodes;
		std::size_t MaskIndex = 1;

		friend struct DfaT;
	};

	struct NfaEdgeKeyT
	{
		std::size_t From;
		std::size_t EdgeIndex;
		std::strong_ordering operator<=>(NfaEdgeKeyT const&) const = default;
	};

	struct NfaEdgePropertyT
	{
		std::size_t ToNode;
		std::size_t MaskIndex;
		bool ToAccept = false;
		bool HasCounter = false;
		bool HasCapture = false;

		struct RangeT
		{
			std::size_t Index;
			std::size_t Min;
			std::size_t Max;
		};

		std::vector<RangeT> Ranges;
	};

	struct NfaActionKeyT
	{
		std::size_t Index;
		std::size_t MaskIndex;
	};

	struct MartixStateT
	{
		enum class StateE
		{
			UnSet,
			True,
			False
		};
		
		std::vector<StateE> States;
		std::size_t RowCount = 0;
		std::size_t LineCount = 0;
		
		MartixStateT() {}
		void ResetRowCount(std::size_t RowCount);
		std::size_t GetLineCount() const { return LineCount; }
		std::size_t GetRowCount() const { return RowCount; }
		std::size_t CopyLine(std::size_t SourceLine);
		bool RemoveAllFalseLine();
		std::span<StateE const> ReadLine(std::size_t LineIndex) const;

		StateE& Get(std::size_t RowIndex, std::size_t LineIndex);

	};

	struct DfaT
	{

		enum class FormatE
		{
			March,
			HeadMarch,
			GreedyHeadMarch,
		};

		DfaT(FormatE Format, NfaT const& T1);
		DfaT(FormatE Format, std::u32string_view Str, bool IsRaw = false, std::size_t Mask = 0);
		
		std::size_t GetStartupNodeIndex() const { return 0; }
		std::size_t GetCacheCounterCount() const { return CacheRecordCount; }
	
	protected:

		
		enum class ActionE
		{
			True,
			False,
			Ignore
		};

		struct ConstraintT
		{
			std::size_t Source;
			ActionE PassAction;
			ActionE UnpassAction;
			std::strong_ordering operator<=>(ConstraintT const&) const = default;
		};

		struct TempToNodeT
		{
			std::vector<ActionE> Actions;
			std::size_t ToNode;
		};

		struct TempPropertyT
		{
			std::map<NfaEdgeKeyT, NfaEdgePropertyT>::const_iterator Key;
			std::vector<ConstraintT> Constraints;
		};

		struct TempEdgeT
		{
			IntervalT CharSets;
			std::vector<TempPropertyT> Propertys;
			std::vector<TempToNodeT> ToNode;
		};

		struct TempNodeT
		{
			std::vector<TempEdgeT> TempEdge;
			std::vector<std::size_t> OriginalToNode;
			std::optional<NfaT::AcceptT> Accept;
		};

		enum class PropertyActioE
		{
			CopyValue,
			RecordLocation,
			NewContext,
			ConstraintsTrueTrue,
			ConstraintsFalseFalse,
			ConstraintsFalseTrue,
			ConstraintsTrueFalse,
			OneCounter,
			AddCounter,
			LessCounter,
			BiggerCounter,
		};

		struct PropertyT
		{
			PropertyActioE Action;
			std::size_t Solt = 0;
			std::size_t Par = 0;
		};

		struct ConditionT
		{
			enum CommandE
			{
				Next,
				ToNode,
				Fail,
			};
			CommandE PassCommand = CommandE::Fail;
			CommandE UnpassCommand = CommandE::Fail;
			std::size_t Pass = 0;
			std::size_t Unpass = 0;
		};

		struct EdgeT
		{
			IntervalT CharSets;
			std::vector<PropertyT> Propertys;
			std::vector<ConditionT> Conditions;
		};

		struct AcceptT
		{
			std::size_t Mask;
			Misc::IndexSpan<> CaptureIndex;
		};

		struct NodeT
		{
			std::vector<EdgeT> Edges;
			std::optional<AcceptT> Accept;
		};

		FormatE Format;
		std::size_t CacheRecordCount;
		std::vector<NodeT> Nodes;
		
		friend class RegProcessor;
		friend class DfaBinaryTable;
	};

	struct ProcessorAcceptT
	{
		std::size_t Mask;
		std::vector<Misc::IndexSpan<>> Capture;
		Misc::IndexSpan<> MainCapture;
	};

	struct RegProcessor
	{
		RegProcessor(DfaT& Table);
		RegProcessor(RegProcessor const&) = default;

		bool Consume(char32_t Token, std::size_t TokenIndex);
		bool EndOfFile(std::size_t TokenIndex) { return Consume(Reg::EndOfFile(), TokenIndex); }
		void Reset();
		bool HasAccept() const { return Accept.has_value(); }
		std::optional<ProcessorAcceptT> GetAccept() const;
	
	public:
		
		DfaT& Table;
		std::size_t CurNodeIndex;

		enum class DetectResultE
		{
			True,
			False
		};

		std::vector<DetectResultE> TempResult;
		std::vector<std::size_t> CacheIndex;
		std::optional<DfaT::AcceptT> Accept;
		std::optional<Misc::IndexSpan<>> CurMainCapture;
	};

	struct DfaBinaryTable
	{
		using StandardT = std::uint32_t;
		using HalfStandardT = std::uint16_t;

		struct NodeT
		{
			HalfStandardT EdgeCount = 0;
			HalfStandardT AcceptOffset = 0;
		};

		struct CharSetPropertyT
		{
			HalfStandardT CharCount = 0;
			HalfStandardT EdgeOffset = 0;
		};

		struct ConditionT
		{
			HalfStandardT PassCommand = 0;
			HalfStandardT UnpassCommand = 0;
			StandardT Pass = 0;
			StandardT Unpass = 0;
		};

		struct EdgeT
		{
			HalfStandardT PropertyCount;
			HalfStandardT ConditionCount;
		};

		struct AcceptT
		{
			StandardT Mask = 0;
			HalfStandardT CaptureIndexBegin = 0;
			HalfStandardT CaptureIndexEnd = 0;
		};
		
		static std::size_t PredicateSize(DfaT const& RefTable)
		{
			Misc::StructedSerilizerWriter<StandardT> Writer;
			SerilizeToExe(Writer, RefTable);
			return Writer.GetWritedSize();
		}
		static std::size_t SerilizeTo(std::span<StandardT> Buffer, DfaT const& RefTable)
		{
			Misc::StructedSerilizerWriter<StandardT> Writer(Buffer);
			SerilizeToExe(Writer, RefTable);
			return Writer.GetWritedSize();
		}
		
		template<typename AllocatorT = std::allocator<StandardT>>
		static auto Create(DfaT const& RefTable, AllocatorT const& Acclcator= {}) -> std::vector<StandardT, AllocatorT>
		{
			std::vector<StandardT, AllocatorT> Buffer(Acclcator);
			Buffer.resize(PredicateSize(RefTable));
			SerilizeTo(std::span(Buffer), RefTable);
			return Buffer;
		}

	private:
		static void SerilizeToExe(Misc::StructedSerilizerWriter<StandardT>& Writer, DfaT const& RefTable);
		std::span<StandardT> Wrapper;
	};

	namespace Exception
	{
		struct Interface : public std::exception
		{ 
			virtual ~Interface() = default;
			virtual char const* what() const override;
		};

		struct UnaccaptableRegexTokenIndex : public Interface
		{
			enum class TypeT
			{
				OutOfCharRange,
				UnSupportEof,
				UnaccaptableNumber,
				RawRegexInNotNormalState,
				BadRegex,
			};

			TypeT Type;
			Misc::IndexSpan<> BadIndex;

			UnaccaptableRegexTokenIndex(UnaccaptableRegexTokenIndex const&) = default;
			UnaccaptableRegexTokenIndex(TypeT Type, Misc::IndexSpan<> TokenIndex) :
				Type(Type), BadIndex(TokenIndex) {}
			virtual char const* what() const override;
		};

		struct UnaccaptableRegex : protected UnaccaptableRegexTokenIndex
		{
			using TypeT = UnaccaptableRegexTokenIndex::TypeT;
			std::wstring TotalString;
			std::wstring_view GetErrorRegex() const { return BadIndex.Slice(std::wstring_view(TotalString)); }
			UnaccaptableRegex(TypeT Type, std::u8string_view Str, Misc::IndexSpan<> BadIndex);
			UnaccaptableRegex(TypeT Type, std::wstring_view Str, Misc::IndexSpan<> BadIndex);
			UnaccaptableRegex(TypeT Type, std::u16string_view Str, Misc::IndexSpan<> BadIndex);
			UnaccaptableRegex(TypeT Type, std::u32string_view Str, Misc::IndexSpan<> BadIndex);
			UnaccaptableRegex() = default;
			UnaccaptableRegex(UnaccaptableRegex const&) = default;
			virtual char const* what() const override;
		};

		struct RegexOutOfRange : public Interface
		{
			enum class TypeT
			{
				EdgeCount,
				NodeOffset,
				CharCount,
				PropertyCount,
				ConditionCount,
				Solt,
				Counter,
				CaptureIndex,
				Mask,
				/*
				Counter,
				NodeCount,
				NodeOffset,
				ToIndeCount,
				ActionCount,
				ActionParameterCount,
				EdgeCount,
				EdgeOffset,
				AcceptableCharCount,
				PropertyCount,
				CounterCount,
				EdgeLength,
				ContentIndex,
				*/
			};

			TypeT Type;
			std::size_t BadIndex;

			RegexOutOfRange(TypeT Type, std::size_t BadIndex) : Type(Type), BadIndex(BadIndex) {}
			RegexOutOfRange(RegexOutOfRange const&) = default;
			virtual char const* what() const override;
		};

		struct CircleShifting : public Interface
		{
			CircleShifting() = default;
			CircleShifting(CircleShifting const&) = default;
			virtual char const* what() const override;
		};
	}

}
