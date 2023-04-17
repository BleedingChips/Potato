module;

export module Potato.Reg;
export import Potato.SLRX;
export import Potato.Interval;

export namespace Potato::Reg
{
	
	using StandardT = std::uint32_t;

	using IntervalT = Misc::IntervalT<char32_t>;

	inline constexpr char32_t MaxChar() { return 0x110000; };
	inline constexpr char32_t EndOfFile() { return 0; }

	struct RegLexerT
	{
		
		enum class ElementEnumT : StandardT
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

		static NfaT Create(std::u32string_view Str, bool IsRaw = false, StandardT Mask = 0);

		
		NfaT(std::u32string_view Str, bool IsRaw = false, StandardT Mask = 0)
			: NfaT(Create(Str, IsRaw, Mask)) {}
		NfaT(NfaT const&) = default;
		NfaT(NfaT&&) = default;
		void Link(NfaT const&);

	protected:

		NfaT(std::span<RegLexerT::ElementT const> InputSpan, StandardT Mask = 0);
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

		enum class EdgePropertyT
		{
			CaptureBegin,
			CaptureEnd,
			ZeroCounter,
			AddCounter,
			LessCounter,
			BiggerCounter,
		};

		struct PropertyT
		{
			EdgePropertyT Type = EdgePropertyT::CaptureBegin;
			std::size_t Index = 0;
			StandardT Par1 = 0;
			bool operator==(PropertyT const& T1) const { return Type == T1.Type && Index == T1.Index && Par1 == T1.Par1; }
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
			StandardT Mask;
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

		std::vector<NodeT> Nodes;
		std::size_t MaskIndex = 1;

		friend struct DfaT;
		friend struct NoEpsilonNfaT;
	};

	struct NoEpsilonNfaT : protected NfaT
	{
		NoEpsilonNfaT(NfaT const& Ref);
		NoEpsilonNfaT(NoEpsilonNfaT const&) = default;
		NoEpsilonNfaT(NoEpsilonNfaT&&) = default;

	protected:

		friend struct DfaT;
	};

	struct DfaT
	{

		enum class FormatE
		{
			March,
			HeadMarch,
			GreedyHeadMarch,
		};

		DfaT(NoEpsilonNfaT const& T1, FormatE Format = FormatE::March);
	
	protected:

		struct DivergenceNodeT
		{
			DivergenceNodeT(FormatE Format) : Format(Format) {}

			std::size_t GetTotalNodeCount() const { return TotalNodeCount; }
			void Clear();

			bool InsertCondition(bool HasCounter, bool ToAcceptNode, bool Pass, std::size_t MaskIndex);

			std::optional<std::size_t> GetCurrentNodeIndex() const { 
				if (!Redefine)
					return CurIndex;
				else
					return {};
			}

			const FormatE Format;
			std::size_t CurIndex = 0;
			std::size_t DivergenceCount = 0;
			std::size_t TotalNodeCount = 1;
			bool HasAccept = false;
			std::vector<std::size_t> RemoveMaskIndex;
		};

		enum class ActioE : uint8_t
		{
			CaptureRecord,
			ZeroCounter,
			AddCounter,
			LessEqualCounter,
			BiggerEqualCounter,
			EndCounter,
			CopyRecord,
		};

		struct PropertyT
		{
			ActioE Action;
			std::size_t SoltIndex = 0;
			StandardT Par1 = 0;
		};

		struct EdgeT
		{
			std::vector<PropertyT> Propertys;
			IntervalT CharSets;
			std::vector<std::size_t> ToNode;
		};

		struct NodeT
		{
			std::vector<PropertyT> Accept;
			bool AcceptIsFront = false;
			std::vector<EdgeT> Edges;
		};

		FormatE Format;
		std::vector<NodeT> Nodes;
		
	};

	

	

	
	/*
	struct Accept
	{
		StandardT Mask = 0;
		StandardT SubMask = 0;
		std::strong_ordering operator<=>(Accept const&) const = default;
	};

	struct Capture
	{
		bool IsBegin;
		std::size_t Index;
	};

	struct Counter
	{
		StandardT Target = 0;
		std::strong_ordering operator<=>(Counter const&) const = default;
	};


	struct EpsilonNFA
	{

		enum class EdgeType
		{
			Consume = 0,
			Acceptable,
			CaptureBegin,
			CaptureEnd,
			CounterPush,
			CounterPop,
			CounterAdd,
			CounterEqual,
			CounterBigEqual,
			CounterSmallEqual,
		};

		struct PropertyT
		{
			EdgeType Type;
			std::variant<std::monostate, std::vector<IntervalT>, Accept, Counter> Datas;
			bool operator==(PropertyT const&) const = default;
		};

		struct Edge
		{
			PropertyT Property;
			std::size_t ShiftNode;
			std::size_t UniqueID = 0;
		};

		struct Node
		{
			std::size_t Index;
			std::vector<Edge> Edges;
			std::size_t TokenIndex;
		};

		struct NodeSet
		{
			std::size_t In;
			std::size_t Out;
		};

		static EpsilonNFA Create(std::u8string_view Str, bool IsRaw, Accept AcceptData);
		static EpsilonNFA Create(std::wstring_view Str, bool IsRaw, Accept AcceptData);
		static EpsilonNFA Create(std::u16string_view Str, bool IsRaw, Accept AcceptData);
		static EpsilonNFA Create(std::u32string_view Str, bool IsRaw, Accept AcceptData);

		void Link(EpsilonNFA const& OtherTable, bool ThisHasHigherPriority = true);

		EpsilonNFA(EpsilonNFA&&) = default;
		EpsilonNFA(EpsilonNFA const&) = default;
		EpsilonNFA& operator=(EpsilonNFA const&) = default;
		EpsilonNFA& operator=(EpsilonNFA&&) = default;
		EpsilonNFA() {}
		bool IsAvailable() const { return !Nodes.empty(); }
		operator bool() const { return IsAvailable(); }

		std::size_t NewNode(std::size_t TokenIndex);
		void AddComsumeEdge(std::size_t From, std::size_t To, std::vector<IntervalT> Acceptable);
		void AddAcceptableEdge(std::size_t From, std::size_t To, Accept Data);
		void AddCapture(NodeSet OutsideSet, NodeSet InsideSet);
		NodeSet AddCounter(std::size_t TokenIndex, NodeSet InSideSet, std::optional<std::size_t> Equal, std::optional<std::size_t> Min, std::optional<std::size_t> Max, bool IsGreedy);
		//void AddCounter(EdgeType Type, std::size_t From, std::size_t To, Counter Counter);
		void AddEdge(std::size_t From, Edge Edge);

		std::vector<Node> Nodes;
	};

	struct NFA
	{

		struct Edge
		{
			std::vector<EpsilonNFA::PropertyT> Propertys;
			std::vector<IntervalT> ConsumeChars;
			std::size_t ToNode;
			std::size_t UniqueID;
		};

		struct Node
		{
			std::vector<Edge> Edges;
		};

		std::vector<Node> Nodes;

		static NFA Create(EpsilonNFA const& Table);
		static NFA Create(std::u8string_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}
		static NFA Create(std::wstring_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}
		static NFA Create(std::u16string_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}
		static NFA Create(std::u32string_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}

		NFA(EpsilonNFA const& Table) : NFA(Create(Table)) {}
		NFA(std::u8string_view Str, bool IsRow = false, Accept Mask = {0}) : NFA(Create(Str, IsRow, Mask)) {}
		NFA(std::wstring_view Str, bool IsRow = false, Accept Mask = { 0 }) : NFA(Create(Str, IsRow, Mask)) {}
		NFA(std::u16string_view Str, bool IsRow = false, Accept Mask = { 0 }) : NFA(Create(Str, IsRow, Mask)) {}
		NFA(std::u32string_view Str, bool IsRow = false, Accept Mask = { 0 }) : NFA(Create(Str, IsRow, Mask)) {}
		NFA(NFA const&) = default;
		NFA(NFA&&) = default;
		NFA& operator=(NFA const&) = default;
		NFA& operator=(NFA &&) = default;
		NFA() = default;
		bool IsAvailable() const { return !Nodes.empty(); }
		operator bool() const { return IsAvailable(); }
	private:
		NFA(std::vector<Node> Nodes) : Nodes(std::move(Nodes)) {}
	};

	struct DFA
	{

		enum class ActionT : StandardT
		{
			CaptureBegin,
			CaptureEnd,
			ContentBegin,
			ContentEnd,
			CounterSmaller,
			CounterBigEqual,
			CounterEqual,
			CounterAdd,
			CounterPush,
			CounterPop,
			Accept
		};

		struct Edge
		{
			std::vector<ActionT> List;
			std::vector<StandardT> Parameter;
			std::vector<std::size_t> ToIndex;
			std::vector<IntervalT> ConsumeSymbols;
			bool ContentNeedChange = true;
		};

		struct Node
		{
			std::vector<Edge> Edges;
			std::vector<Edge> KeepAcceptableEdges;
		};

		std::vector<Node> Nodes;
		static constexpr std::size_t StartupNode() { return 0; }

		static DFA Create(EpsilonNFA const& Table) {
			return Create(NFA::Create(Table));
		}
		static DFA Create(NFA const& Table);
		static DFA Create(std::u8string_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}
		static DFA Create(std::wstring_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}
		static DFA Create(std::u16string_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}
		static DFA Create(std::u32string_view Str, bool IsRow = false, Accept Mask = { 0 }) {
			return Create(EpsilonNFA::Create(Str, IsRow, Mask));
		}

		DFA(EpsilonNFA const& ETable) : DFA(Create(ETable)) {}
		DFA(NFA const& Input) : DFA(Create(Input)) {}
		DFA(std::u8string_view Str, bool IsRaw = false, Accept Mask = {0}) : DFA(Create(Str, IsRaw, Mask)) {}
		DFA(std::wstring_view Str, bool IsRaw = false, Accept Mask = { 0 }) : DFA(Create(Str, IsRaw, Mask)) {}
		DFA(std::u16string_view Str, bool IsRaw = false, Accept Mask = { 0 }) : DFA(Create(Str, IsRaw, Mask)) {}
		DFA(std::u32string_view Str, bool IsRaw = false, Accept Mask = { 0 }) : DFA(Create(Str, IsRaw, Mask)) {}
		DFA(DFA const&) = default;
		DFA(DFA&&) = default;
		DFA& operator=(DFA const&) = default;
		DFA& operator=(DFA&&) = default;
		DFA() = default;
		bool IsAvailable() const { return !Nodes.empty(); }
		operator bool() const { return IsAvailable(); }
	private:
		DFA(std::vector<Node> Nodes) : Nodes(std::move(Nodes)) {}
	};

	struct TableWrapper
	{
		using StandardT = Reg::StandardT;

		using HalfStandardT = std::uint16_t;

		static_assert(sizeof(HalfStandardT) * 2 == sizeof(StandardT));

		struct alignas(StandardT) ZipChar
		{
			char32_t IsSingleChar : 1;
			char32_t Char : 31;
			std::strong_ordering operator<=>(ZipChar const&) const = default;
			static_assert(sizeof(StandardT) == sizeof(char32_t));
		};

		struct alignas(StandardT) ZipEdge
		{
			StandardT ConsumeSymbolCount;
			StandardT ToIndexCount;
			HalfStandardT NeedContentChange;
			HalfStandardT ParmaterCount;
			HalfStandardT ActionCount;
			HalfStandardT NextEdgeOffset;
		};

		struct alignas(StandardT) ZipNode
		{
			HalfStandardT AcceptEdgeOffset;
			HalfStandardT EdgeCount;
		};

		static std::size_t CalculateRequireSpaceWithStanderT(DFA const& Tab);
		static std::size_t SerializeTo(DFA const& Tab, std::span<StandardT> Source);
		static std::vector<StandardT> Create(DFA const& Tab);
		static std::vector<StandardT> Create(std::u8string_view Reg, bool IsRow = false, Accept Acce = {0}) { return Create(DFA{Reg, IsRow, Acce}); }
		static std::vector<StandardT> Create(std::wstring_view Reg, bool IsRow = false, Accept Acce = { 0 }) { return Create(DFA{ Reg, IsRow, Acce }); }
		static std::vector<StandardT> Create(std::u16string_view Reg, bool IsRow = false, Accept Acce = { 0 }) { return Create(DFA{ Reg, IsRow, Acce }); }
		static std::vector<StandardT> Create(std::u32string_view Reg, bool IsRow = false, Accept Acce = { 0 }) { return Create(DFA{ Reg, IsRow, Acce }); }

		TableWrapper(std::span<StandardT const> Wrapper) : Wrapper(Wrapper) {}
		TableWrapper() = default;
		TableWrapper(TableWrapper const&) = default;
		TableWrapper& operator=(TableWrapper const& )= default;
		std::size_t BufferSize() const { return Wrapper.size(); }
		operator bool() const { return !Wrapper.empty(); }
		std::size_t NodeSize() const { return *this ? Wrapper[0] : 0; };
		static constexpr std::size_t StartupNode() { return 1; };
		
		std::span<StandardT const> Wrapper;
	};

	struct Table
	{
		Table(std::u8string_view Str, bool IsRow = false, Accept AcceptData = {}) : SerializeBuffer(TableWrapper::Create(Str, IsRow, AcceptData)) {}
		Table(std::wstring_view Str, bool IsRow = false, Accept AcceptData = {}) : SerializeBuffer(TableWrapper::Create(Str, IsRow, AcceptData)) {}
		Table(std::u16string_view Str, bool IsRow = false, Accept AcceptData = {}) : SerializeBuffer(TableWrapper::Create(Str, IsRow, AcceptData)) {}
		Table(std::u32string_view Str, bool IsRow = false, Accept AcceptData = {}) : SerializeBuffer(TableWrapper::Create(Str, IsRow, AcceptData)) {}
		TableWrapper AsWrapper() const { return TableWrapper{ SerializeBuffer }; };
	protected:
		std::vector<StandardT> SerializeBuffer;
	};

	struct CaptureWrapper
	{
		CaptureWrapper() = default;
		CaptureWrapper(std::span<Capture const> SubCaptures)
			: SubCaptures(SubCaptures) {}
		CaptureWrapper(CaptureWrapper const&) = default;
		CaptureWrapper(Misc::IndexSpan<> CurCapture) : CurrentCapture(CurCapture) {}
		CaptureWrapper(Misc::IndexSpan<> CurCapture, std::span<Capture const> SubCaptures) : CurrentCapture(CurCapture), SubCaptures(SubCaptures) {}
		CaptureWrapper& operator=(CaptureWrapper const&) = default;
		bool HasSubCapture() const { return !SubCaptures.empty(); }
		bool HasNextCapture() const { return !NextCaptures.empty(); }
		bool HasCapture() const { return CurrentCapture.has_value(); }

		auto Slice(std::u8string_view Source) const {return decltype(Source){CurrentCapture->Slice(Source)}; }
		auto Slice(std::wstring_view Source) const { return decltype(Source){CurrentCapture->Slice(Source)}; }
		auto Slice(std::u16string_view Source) const { return decltype(Source){CurrentCapture->Slice(Source)}; }
		auto Slice(std::u32string_view Source) const { return decltype(Source){CurrentCapture->Slice(Source)}; }

		Misc::IndexSpan<> GetCapture() const { return *CurrentCapture; }

		CaptureWrapper GetTopSubCapture() const;
		CaptureWrapper GetNextCapture() const;
		std::size_t Count() const { return CurrentCapture->Count(); }

		static std::optional<Misc::IndexSpan<>> FindFirstCapture(std::span<Capture const> Captures);

	private:

		std::span<Capture const> SubCaptures;
		std::span<Capture const> NextCaptures;
		std::optional<Misc::IndexSpan<>> CurrentCapture;
	};

	struct ProcessorContent
	{
		struct CaptureBlock
		{
			Capture CaptureData;
			std::size_t BlockIndex;
		};

		struct CounterBlock
		{
			StandardT CountNumber;
			std::size_t BlockIndex;
		};

		std::vector<CaptureBlock> CaptureBlocks;
		std::vector<CounterBlock> CounterBlocks;
		void Clear() { CounterBlocks.clear(); CaptureBlocks.clear(); }
		Misc::IndexSpan<> FindCaptureIndex(std::size_t BlockIndex) const;
		std::span<CaptureBlock const> FindCaptureSpan(std::size_t BlockIndex) const { return FindCaptureIndex(BlockIndex).Slice(CaptureBlocks); }
		Misc::IndexSpan<> FindCounterIndex(std::size_t BlockIndex) const;
		std::span<CounterBlock const> FindCounterSpan(std::size_t BlockIndex) const { return FindCounterIndex(BlockIndex).Slice(CounterBlocks); }
	};

	struct CoreProcessor
	{

		struct AcceptResult
		{
			std::optional<Accept> AcceptData;
			Misc::IndexSpan<> CaptureSpan;
			operator bool () const { return AcceptData.has_value(); }
		};

		struct Result
		{
			std::size_t NextNodeIndex;
			AcceptResult AcceptData;
			bool ContentNeedChange = true;
		};

		static std::optional<Result> ConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeIndex, DFA const& Table, char32_t Symbol, std::size_t TokenIndex, bool KeepAccept);
		static std::optional<Result> ConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeOffset, TableWrapper Wrapper, char32_t Symbol, std::size_t TokenIndex, bool KeepAccept);

		struct KeepAcceptResult
		{
			AcceptResult AcceptData;
			bool ContentNeedChange = true;
			bool MeetAcceptRequireConsume = false;
		};

		static KeepAcceptResult KeepAcceptConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeIndex, DFA const& Table, char32_t Symbol, std::size_t TokenIndex);
		static KeepAcceptResult KeepAcceptConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeOffset, TableWrapper Wrapper, char32_t Symbol, std::size_t TokenIndex);
	
	};

	struct MatchProcessor
	{
		struct Result
		{
			Misc::IndexSpan<> MainCapture;
			std::vector<Capture> SubCaptures;
			Accept AcceptData;
			CaptureWrapper GetCaptureWrapper() const { return CaptureWrapper{ MainCapture, SubCaptures }; }
		};

		MatchProcessor(DFA const& Table) : Table(Table), StartupNodeOffset(DFA::StartupNode()), NodeIndex(DFA::StartupNode()) {}
		MatchProcessor(TableWrapper Table) : Table(Table), StartupNodeOffset(Table.StartupNode()), NodeIndex(Table.StartupNode()) {}
		bool Consume(char32_t Symbol, std::size_t TokenIndex);
		std::optional<Result> EndOfFile(std::size_t TokenIndex);
		void Clear();

	private:

		ProcessorContent Contents;
		ProcessorContent TempBuffer;
		std::size_t NodeIndex = 0;
		std::size_t StartupNodeOffset;
		std::variant<TableWrapper, std::reference_wrapper<DFA const>> Table;
	};

	struct HeadMatchProcessor
	{
		using Result = MatchProcessor::Result;

		HeadMatchProcessor(DFA const& Table, bool Greedy = false) : Table(Table), StartupNode(DFA::StartupNode()), NodeIndex(DFA::StartupNode()), Greedy(Greedy) {}
		HeadMatchProcessor(TableWrapper Table, bool Greedy = false) : Table(Table), StartupNode(Table.StartupNode()), NodeIndex(Table.StartupNode()), Greedy(Greedy) {}
		HeadMatchProcessor(HeadMatchProcessor const&) = default;

		std::optional<std::optional<Result>> Consume(char32_t Symbol, std::size_t TokenIndex);
		std::optional<Result> EndOfFile(std::size_t TokenIndex);
		void Clear();

		ProcessorContent Contents;
		ProcessorContent TempBuffer;
		std::optional<Result> CacheResult;
		std::size_t NodeIndex = 0;
		std::size_t StartupNode = 0;
		bool Greedy = false;
		std::variant<TableWrapper, std::reference_wrapper<DFA const>> Table;
	};

	struct SearchMatchProcessor
	{
		struct Result
		{
			Misc::IndexSpan<> MainCapture;
			std::vector<Capture> SubCaptures;
			Accept AcceptData;
			CaptureWrapper GetCaptureWrapper() const { return CaptureWrapper{ SubCaptures }; }
			CaptureWrapper GetMainCaptureWrapper() const {}
		};

		SearchMatchProcessor(DFA const& Table, bool Greedy = false) : Table(Table), StartupNode(DFA::StartupNode()), NodeIndex(DFA::StartupNode()), Greedy(Greedy) {}
		SearchMatchProcessor(TableWrapper Table, bool Greedy = false) : Table(Table), StartupNode(Table.StartupNode()), NodeIndex(Table.StartupNode()), Greedy(Greedy) {}
		SearchMatchProcessor(SearchMatchProcessor const&) = default;

		std::optional<std::optional<Result>> Consume(char32_t Symbol, std::size_t TokenIndex);
		std::optional<Result> EndOfFile(std::size_t TokenIndex);
		void Clear();

		ProcessorContent Contents;
		ProcessorContent TempBuffer;
		std::optional<Result> CacheResult;
		std::size_t NodeIndex = 0;
		std::size_t StartupNode = 0;
		bool Greedy = false;
		std::variant<TableWrapper, std::reference_wrapper<DFA const>> Table;
	};

	template<typename ResultT>
	struct MatchResult
	{
		std::optional<ResultT> SuccessdResult;
		std::size_t LastTokenIndex;
		operator bool () const { return SuccessdResult.has_value(); }
		ResultT* operator->() { return SuccessdResult.operator->(); }
		ResultT const* operator->() const { return SuccessdResult.operator->(); }
		ResultT& operator*() { return *SuccessdResult; }
		ResultT const& operator*() const { return *SuccessdResult; }
	};

	auto Match(MatchProcessor& Processor, std::u8string_view Str) ->MatchResult<MatchProcessor::Result>;
	auto Match(MatchProcessor& Processor, std::wstring_view Str) -> MatchResult<MatchProcessor::Result>;
	auto Match(MatchProcessor& Processor, std::u16string_view Str) -> MatchResult<MatchProcessor::Result>;
	auto Match(MatchProcessor& Processor, std::u32string_view Str) -> MatchResult<MatchProcessor::Result>;

	inline auto Match(DFA const& Table, std::u8string_view Str)->MatchResult<MatchProcessor::Result>{
		MatchProcessor Pro{Table};
		return Match(Pro, Str);
	}

	inline auto Match(DFA const& Table, std::wstring_view Str) -> MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Table };
		return Match(Pro, Str);
	}

	inline auto Match(DFA const& Table, std::u16string_view Str) -> MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Table };
		return Match(Pro, Str);
	}

	inline auto Match(DFA const& Table, std::u32string_view Str) -> MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Table };
		return Match(Pro, Str);
	}


	inline auto Match(TableWrapper Wrapper, std::u8string_view Str)->MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Wrapper };
		return Match(Pro, Str);
	}
	inline auto Match(TableWrapper Wrapper, std::wstring_view Str) -> MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Wrapper };
		return Match(Pro, Str);
	}
	inline auto Match(TableWrapper Wrapper, std::u16string_view Str) -> MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Wrapper };
		return Match(Pro, Str);
	}
	inline auto Match(TableWrapper Wrapper, std::u32string_view Str) -> MatchResult<MatchProcessor::Result> {
		MatchProcessor Pro{ Wrapper };
		return Match(Pro, Str);
	}


	auto HeadMatch(HeadMatchProcessor& Table, std::u8string_view Str)->MatchResult<HeadMatchProcessor::Result>;
	auto HeadMatch(HeadMatchProcessor& Table, std::wstring_view Str) -> MatchResult<HeadMatchProcessor::Result>;
	auto HeadMatch(HeadMatchProcessor& Table, std::u16string_view Str) -> MatchResult<HeadMatchProcessor::Result>;
	auto HeadMatch(HeadMatchProcessor& Table, std::u32string_view Str) -> MatchResult<HeadMatchProcessor::Result>;


	inline auto HeadMatch(DFA const& Table, std::u8string_view Str, bool Greddy = false)->MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}
	inline auto HeadMatch(DFA const& Table, std::wstring_view Str, bool Greddy = false) -> MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}
	inline auto HeadMatch(DFA const& Table, std::u16string_view Str, bool Greddy = false) -> MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}
	inline auto HeadMatch(DFA const& Table, std::u32string_view Str, bool Greddy = false) -> MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}

	inline auto HeadMatch(TableWrapper Table, std::u8string_view Str, bool Greddy = false)->MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}
	inline auto HeadMatch(TableWrapper Table, std::wstring_view Str, bool Greddy = false) -> MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}
	inline auto HeadMatch(TableWrapper Table, std::u16string_view Str, bool Greddy = false) -> MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}
	inline auto HeadMatch(TableWrapper Table, std::u32string_view Str, bool Greddy = false) -> MatchResult<HeadMatchProcessor::Result>
	{
		HeadMatchProcessor Pro{ Table, Greddy };
		return HeadMatch(Pro, Str);
	}

	struct MulityRegCreater
	{
		MulityRegCreater() = default;
		MulityRegCreater(MulityRegCreater const&) = default;
		MulityRegCreater(MulityRegCreater &&) = default;
		MulityRegCreater& operator= (MulityRegCreater const&)  = default;
		MulityRegCreater& operator= (MulityRegCreater &&) = default;

		MulityRegCreater(std::u8string_view Str, bool IsRow, Accept Acce) : ETable(EpsilonNFA::Create(Str, IsRow, Acce)) {}
		MulityRegCreater(std::wstring_view Str, bool IsRow, Accept Acce) : ETable(EpsilonNFA::Create(Str, IsRow, Acce)) {}
		MulityRegCreater(std::u16string_view Str, bool IsRow, Accept Acce) : ETable(EpsilonNFA::Create(Str, IsRow, Acce)) {}
		MulityRegCreater(std::u32string_view Str, bool IsRow, Accept Acce) : ETable(EpsilonNFA::Create(Str, IsRow, Acce)) {}
		void LowPriorityLink(std::u8string_view Str, bool IsRow, Accept Acce);
		std::optional<DFA> GenerateDFA() const;
		std::optional<std::vector<StandardT>> GenerateTableBuffer() const;

	protected:

		std::optional<EpsilonNFA> ETable;
	};
	*/

	namespace Exception
	{
		struct Interface : public std::exception
		{ 
			virtual ~Interface() = default;
			virtual char const* what() const override;
		};

		struct UnaccaptableRegex : public Interface
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
			std::wstring TotalString;
			Misc::IndexSpan<> BadIndex;
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
			};

			TypeT Type;
			Misc::IndexSpan<> Index;

			RegexOutOfRange(TypeT Type, Misc::IndexSpan<> Index) : Type(Type), Index(Index) {}
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
