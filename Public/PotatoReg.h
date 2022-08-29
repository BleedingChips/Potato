#pragma once


#include "PotatoMisc.h"
#include "PotatoInterval.h"
#include "PotatoStrEncode.h"

#include <string_view>
#include <variant>

namespace Potato::Reg
{
	using IntervalT = Misc::Interval<char32_t>;
	using SeqIntervalT = Misc::SequenceInterval<char32_t>;
	using SeqIntervalWrapperT = Misc::SequenceIntervalWrapper<char32_t>;

	using StandardT = std::uint32_t;

	inline constexpr char32_t MaxChar() { return 0x110000; };

	inline constexpr char32_t EndOfFile() { return 0; }

	struct Accept
	{
		StandardT Mask = 0;
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

		static EpsilonNFA Create(std::u32string_view Str, bool IsRaw, Accept AcceptData);

		void Link(EpsilonNFA const& OtherTable, bool ThisHasHigherPriority = true);

		EpsilonNFA(EpsilonNFA&&) = default;
		EpsilonNFA(EpsilonNFA const&) = default;
		EpsilonNFA& operator=(EpsilonNFA const&) = default;
		EpsilonNFA& operator=(EpsilonNFA&&) = default;

		EpsilonNFA() {}

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

		NFA(EpsilonNFA const& Table);
		NFA(std::u32string_view Str, bool IsRow = false, Accept Mask = {0}) : NFA(EpsilonNFA::Create(Str, IsRow, Mask)) {}
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

		DFA(EpsilonNFA const& ETable) : DFA(NFA{ETable}) {}
		DFA(NFA const& Input);
		DFA(std::u32string_view Str, bool IsRaw = false, Accept Mask = {0}) : DFA(EpsilonNFA::Create(Str, IsRaw, Mask)) {}
		
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
		static std::size_t SerilizeTo(DFA const& Tab, std::span<StandardT> Source);
		static std::vector<StandardT> Create(DFA const& Tab);

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

	struct CaptureWrapper
	{
		CaptureWrapper() = default;
		CaptureWrapper(std::span<Capture const> SubCaptures)
			: SubCaptures(SubCaptures) {}
		CaptureWrapper(CaptureWrapper const&) = default;
		CaptureWrapper& operator=(CaptureWrapper const&) = default;
		bool HasSubCapture() const { return !SubCaptures.empty(); }
		bool HasNextCapture() const { return !NextCaptures.empty(); }
		bool HasCapture() const { return CurrentCapture.has_value(); }
		Misc::IndexSpan<> GetCapture() const { return *CurrentCapture; }

		CaptureWrapper GetTopSubCapture() const;
		CaptureWrapper GetNextCapture() const;

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
		static KeepAcceptResult KeepAcceptConsumeSymbol(ProcessorContent& Target, ProcessorContent const& Source, std::size_t CurNodeOffset, DFA const& Table, char32_t Symbol, std::size_t TokenIndex);
	};

	struct DFAMatchProcessor
	{
		struct Result
		{
			std::vector<Capture> SubCaptures;
			Accept AcceptData;
			CaptureWrapper GetCaptureWrapper() const { return CaptureWrapper{ SubCaptures }; }
		};

		DFAMatchProcessor(DFA const& Tables) : Tables(Tables), NodeIndex(DFA::StartupNode()) {}
		DFAMatchProcessor(DFAMatchProcessor const&) = default;
		bool ConsymeSymbol(char32_t Symbol, std::size_t TokenIndex);
		std::optional<Result> EndOfFile(std::size_t TokenIndex);
		void Clear();
	protected:
		ProcessorContent Contents;
		ProcessorContent TempBuffer;
		std::size_t NodeIndex = 0;
		DFA const& Tables;
	};

	struct DFAHeadMatchProcessor
	{

		struct Result
		{
			Misc::IndexSpan<> MainCapture;
			std::vector<Capture> SubCaptures;
			Accept AcceptData;
			CaptureWrapper GetCaptureWrapper() const { return CaptureWrapper{ SubCaptures }; }
		};

		DFAHeadMatchProcessor(DFA const& Tables) : Tables(Tables), NodeIndex(DFA::StartupNode()) {}
		DFAHeadMatchProcessor(DFAHeadMatchProcessor const&) = default;
		std::optional<std::optional<Result>> ConsymeSymbol(char32_t Symbol, std::size_t TokenIndex, bool Greedy = false);
		std::optional<Result> EndOfFile(std::size_t TokenIndex);
		void Clear();
	protected:
		ProcessorContent Contents;
		ProcessorContent TempBuffer;
		std::optional<Result> CacheResult;
		std::size_t NodeIndex = 0;
		DFA const& Tables;
	};

	auto Match(DFAMatchProcessor& Processor, std::u32string_view Str) ->std::optional<DFAMatchProcessor::Result>;
	inline auto Match(DFA const& Table, std::u32string_view Str)->std::optional<DFAMatchProcessor::Result>{
		DFAMatchProcessor Pro{Table};
		return Match(Pro, Str);
	}
	auto HeadMatch(DFAHeadMatchProcessor& Table, std::u32string_view Str, bool Greddy = false)->std::optional<DFAHeadMatchProcessor::Result>;
	inline auto HeadMatch(DFA const& Table, std::u32string_view Str, bool Greddy = false)->std::optional<DFAHeadMatchProcessor::Result>
	{
		DFAHeadMatchProcessor Pro{ Table };
		return HeadMatch(Pro, Str);
	}

	

	/*
	struct TableWrapper
	{

		using StandardT = Reg::StandardT;

		using HalfStandardT = std::uint16_t;

		static_assert(sizeof(HalfStandardT) * 2 == sizeof(StandardT));

		struct ZipChar
		{
			char32_t IsSingleChar : 1;
			char32_t Char : 31;
			std::strong_ordering operator<=>(ZipChar const&) const = default;
			static_assert(sizeof(StandardT) == sizeof(char32_t));
		};

		struct alignas(alignof(StandardT)) ZipNode
		{
			StandardT EdgeCount;
		};

		struct alignas(alignof(StandardT)) ZipEdge
		{
			StandardT ToNode;
			StandardT UniqueID;
			HalfStandardT AcceptableCharCount;
			HalfStandardT HasCounter : 1;
			HalfStandardT PropertyCount : 15;
			HalfStandardT CounterDataCount;
			HalfStandardT HasUniqueID : 1;
			HalfStandardT EdgeTotalLength : 15;
		};

		enum class ZipProperty : uint8_t
		{
			CaptureBegin,
			CaptureEnd,
			CounterBegin,
			CounterEnd,
			CounterContinue,
		};

		struct NodeViewer
		{
			StandardT NodeOffset;
			StandardT EdgeCount;
			std::span<StandardT const> AppendData;
		};

		TableWrapper(std::span<StandardT const> Input) : Wrapper(Input) {};
		TableWrapper(TableWrapper const&) = default;
		TableWrapper& operator=(TableWrapper const&) = default;
		StandardT StartupNodeOffset() const { return  Wrapper[1]; }

		static std::vector<StandardT> Create(UnserilizedTable const& Table);

		static std::vector<StandardT> Create(std::u32string_view Str, Accept Mask = {}, StandardT UniqueID = 0, bool IsRaw = false);

		NodeViewer operator[](StandardT Offset) const;

	private:

		std::span<StandardT const> Wrapper;
		friend struct Table;
	};

	struct CoreProcessor
	{

		struct AmbiguousPoint
		{
			StandardT ToNode;
			Misc::IndexSpan<> CounterIndex;
			std::size_t CurrentTokenIndex;
			std::size_t NextTokenIndex;
			std::size_t CaptureCount;
			std::size_t CounterHistory;
			std::optional<StandardT> UniqueID;
			std::optional<Accept> AcceptData;
			std::span<TableWrapper::ZipProperty const> Propertys;
		};

		std::vector<std::size_t> AmbiguousCounter;
		std::vector<std::size_t> CounterRecord;
		std::size_t CounterHistory = 0;
		std::vector<AmbiguousPoint> AmbiguousPoints;
		std::vector<Capture> CaptureRecord;
		StandardT CurrentState;
		std::size_t StartupTokenIndex = 0;
		std::size_t RequireTokenIndex = 0;
		
		TableWrapper Wrapper;
		bool KeepAcceptableViewer = false;

		CoreProcessor(TableWrapper Wrapper, bool KeepAcceptableViewer = false, std::size_t StartupTokenIndex = 0);
		std::size_t GetRequireTokenIndex() const { return RequireTokenIndex; }
		void SetRequireTokenIndex(std::size_t Index) { RequireTokenIndex = Index; }
		std::size_t GetCurrentState() const { return CurrentState; };
		void Reset(std::size_t StartupTokenIndex = 0);
		void RemoveAmbiuosPoint(StandardT UniqueID);
		TableWrapper GetWrapper() const { return Wrapper; }
		std::size_t GetStarupTokenIndex() const { return StartupTokenIndex; }

		struct AcceptResult
		{
			Accept AcceptData;
			StandardT UniqueID;
			std::span<Capture const> CaptureData;
		};

		struct ConsumeResult
		{
			std::optional<AcceptResult> Accept;
			std::size_t StartupTokenIndex;
			std::size_t CurrentTokenIndex;
			bool IsEndOfFile;
		};

		std::optional<ConsumeResult> ConsumeTokenInput(char32_t InputSymbols, std::size_t NextTokenIndex);
	};

	struct CaptureWrapper
	{
		CaptureWrapper() = default;
		CaptureWrapper(std::span<Capture const> SubCaptures)
			: SubCaptures(SubCaptures) {}
		CaptureWrapper(CaptureWrapper const&) = default;
		CaptureWrapper& operator=(CaptureWrapper const&) = default;
		bool HasSubCapture() const { return !SubCaptures.empty(); }
		bool HasNextCapture() const { return !NextCaptures.empty(); }
		bool HasCapture() const { return CurrentCapture.has_value(); }
		Misc::IndexSpan<> GetCapture() const { return *CurrentCapture; }

		CaptureWrapper GetTopSubCapture() const;
		CaptureWrapper GetNextCapture() const;

		static std::optional<Misc::IndexSpan<>> FindFirstCapture(std::span<Capture const> Captures);

	private:

		std::span<Capture const> SubCaptures;
		std::span<Capture const> NextCaptures;
		std::optional<Misc::IndexSpan<>> CurrentCapture;
	};

	struct Table
	{
		Table(Table&&) = default;
		Table(Table const&) = default;

		Table(std::u32string_view Regex, Accept Mask = {}, StandardT UniqueID = 0, bool IsRaw = false) : Storage(TableWrapper::Create(Regex, Mask, UniqueID, IsRaw)) {}
		
		Table& operator=(Table&&) = default;
		Table& operator=(Table const&) = default;

		operator TableWrapper() const noexcept { return TableWrapper(Storage); }
		TableWrapper AsWrapper() const noexcept { return TableWrapper(Storage); };

	private:

		std::vector<StandardT> Storage;
	};

	struct ProcessResult
	{
		Misc::IndexSpan<> MainCapture;
		Accept AcceptData;
		std::vector<Capture> Captures;
		TableWrapper::StandardT UniqueID;

		CaptureWrapper GetCaptureWrapper() const { return CaptureWrapper{ Captures }; };
		ProcessResult(ProcessResult const&) = default;
		ProcessResult(ProcessResult&&) = default;
		ProcessResult& operator=(ProcessResult const&) = default;
		ProcessResult& operator=(ProcessResult &&) = default;
		ProcessResult() = default;
		ProcessResult(CoreProcessor::ConsumeResult const& Result);
	};

	struct MatchProcessor
	{

		MatchProcessor(TableWrapper Wrapper, std::size_t StartupTokenIndex = 0) : Processor(Wrapper, false, StartupTokenIndex) {}
		MatchProcessor(MatchProcessor const&) = default;
		MatchProcessor(MatchProcessor&&) = default;
		std::size_t GetRequireTokenIndex() const { return Processor.GetRequireTokenIndex(); }

		struct Result
		{
			std::optional<ProcessResult> Accept;
			bool IsEndOfFile;
		};

		std::optional<Result> ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex);

	private:

		CoreProcessor Processor;
	};

	std::optional<ProcessResult> ProcessMatch(TableWrapper Wrapper, std::u32string_view Span);

	struct FrontMatchProcessor
	{

		FrontMatchProcessor(TableWrapper Wrapper, std::size_t StartupTokenIndex = 0) : Processor(Wrapper, true, StartupTokenIndex) {}
		FrontMatchProcessor(FrontMatchProcessor const&) = default;
		FrontMatchProcessor(FrontMatchProcessor&&) = default;
		std::size_t GetRequireTokenIndex() const { return Processor.GetRequireTokenIndex(); }

		struct Result
		{
			std::optional<ProcessResult> Accept;
			bool IsEndOfFile;
		};

		std::optional<Result> ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex);
	private:
		CoreProcessor Processor;
	};

	std::optional<ProcessResult> ProcessFrontMatch(TableWrapper Wrapper, std::u32string_view SpanView);

	struct SearchProcessor
	{

		SearchProcessor(TableWrapper Wrapper, std::size_t StartupTokenIndex = 0) 
			: Processor(Wrapper, true, StartupTokenIndex), RetryStartupTokenIndex(StartupTokenIndex + 1) {}
		SearchProcessor(SearchProcessor const&) = default;
		SearchProcessor(SearchProcessor&&) = default;
		std::size_t GetRequireTokenIndex() const { return Processor.GetRequireTokenIndex(); }

		struct Result
		{
			std::optional<ProcessResult> Accept;
			bool IsEndOfFile;
		};

		std::optional<Result> ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex);
	private:
		CoreProcessor Processor;
		std::size_t RetryStartupTokenIndex = 0;
	};

	std::optional<ProcessResult> ProcessSearch(TableWrapper Wrapper, std::u32string_view Span);

	struct GreedyFrontMatchProcessor
	{

		GreedyFrontMatchProcessor(TableWrapper Wrapper, std::size_t StartupTokenIndex = 0)
			: Processor(Wrapper, true, StartupTokenIndex) {}
		GreedyFrontMatchProcessor(GreedyFrontMatchProcessor const&) = default;
		GreedyFrontMatchProcessor(GreedyFrontMatchProcessor&&) = default;
		std::size_t GetRequireTokenIndex() const { return Processor.GetRequireTokenIndex(); }
		void Reset(std::size_t StartUpTokenIndex = 0) { Processor.Reset(StartUpTokenIndex); }

		struct Result
		{
			std::optional<ProcessResult> Accept;
			bool IsEndOfFile;
		};

		std::optional<Result> ConsumeTokenInput(char32_t Token, std::size_t NextTokenIndex);
	private:
		CoreProcessor Processor;
		std::optional<ProcessResult> TempResult;
	};

	std::optional<ProcessResult> ProcessGreedyFrontMatch(TableWrapper Wrapper, std::u32string_view Span);

	struct MulityRegexCreator
	{
		StandardT AddRegex(std::u32string_view Regex, Accept Mask, StandardT UniqueID, bool IsRaw = false);
		std::vector<StandardT> Generate() const;
		operator bool() const { return Temporary.has_value(); }
		StandardT GetCountedUniqueID() const { return CountedUniqueID; }
		StandardT Push(UnfaTable const& Table);
	private:
		StandardT CountedUniqueID = 0;
		std::optional<UnfaTable> Temporary;
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
			char32_t Character;
			std::size_t TokenIndex;
			UnaccaptableRegex(TypeT Type, char32_t Character, std::size_t TokenIndex)
				: Type(Type), Character(Character), TokenIndex(TokenIndex)
			{
			}
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
				EdgeCount,
				EdgeOffset,
				AcceptableCharCount,
				PropertyCount,
				CounterCount,
				EdgeLength,
				ContentIndex,
			};

			TypeT Type;
			std::size_t Value;

			RegexOutOfRange(TypeT Type, std::size_t Value) : Type(Type), Value(Value) {}
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
