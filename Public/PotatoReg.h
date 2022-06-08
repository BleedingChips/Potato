#pragma once
#include <string_view>

#include "PotatoMisc.h"
#include "PotatoInterval.h"
#include "PotatoStrEncode.h"

namespace Potato::Reg
{
	using IntervalT = Misc::Interval<char32_t>;
	using SeqIntervalT = Misc::SequenceInterval<char32_t>;
	using SeqIntervalWrapperT = Misc::SequenceIntervalWrapper<char32_t>;

	inline constexpr char32_t MaxChar() { return 0x110000; };

	inline constexpr char32_t EndOfFile() { return MaxChar(); }

	using SerilizeT = uint32_t;
	using HalfSerilizeT = uint16_t;

	struct Accept
	{
		SerilizeT Mask = 0;
		std::strong_ordering operator<=>(Accept const&) const = default;
	};

	struct Capture
	{
		bool IsBegin;
		std::size_t Index;
	};

	struct Counter
	{
		SerilizeT Min = 0;
		SerilizeT Max = 0;
		std::strong_ordering operator<=>(Counter const&) const = default;
	};

	struct UnfaTable
	{

		enum class EdgeType
		{
			Consume = 0,
			Acceptable,
			CaptureBegin,
			CaptureEnd,
			CounterBegin,
			CounterEnd,
			CounterContinue,
		};

		

		struct Edge
		{
			EdgeType Type;
			std::size_t ShiftNode;
			std::vector<IntervalT> ConsumeChars;
			Accept AcceptData;
			Counter CounterDatas;
			SerilizeT UniqueID;
		};

		struct Node
		{
			std::size_t Index;
			std::vector<Edge> Edges;
		};

		struct NodeSet
		{
			std::size_t In;
			std::size_t Out;
		};

		static UnfaTable Create(std::u32string_view Str, bool IsRaw, Accept AcceptData, SerilizeT UniqueID = 0);

		void Link(UnfaTable const& OtherTable, bool ThisHasHigherPriority = true);

		UnfaTable(UnfaTable&&) = default;
		UnfaTable(UnfaTable const&) = default;
		UnfaTable& operator=(UnfaTable const&) = default;
		UnfaTable& operator=(UnfaTable &&) = default;

		UnfaTable(SerilizeT UniqueID = 0) : TemporaryUniqueID(UniqueID) {};

		std::size_t NewNode();
		void AddComsumeEdge(std::size_t From, std::size_t To, std::vector<IntervalT> Acceptable);
		void AddAcceptableEdge(std::size_t From, std::size_t To, Accept Data);
		void AddCapture(NodeSet OutsideSet, NodeSet InsideSet);
		void AddCounter(NodeSet OutsideSet, NodeSet InsideSet, Counter Counter, bool Greedy = true);
		void AddEdge(std::size_t From, Edge Edge);

		std::vector<Node> Nodes;
		SerilizeT TemporaryUniqueID;
	};

	struct UnserilizedTable
	{

		struct EdgeProperty
		{
			UnfaTable::EdgeType Type;
			Accept AcceptData;
			Counter CounterData;
			std::strong_ordering operator<=>(EdgeProperty const& I2) const = default;
			bool operator==(EdgeProperty const&) const = default;
		};

		struct Edge
		{
			std::size_t ToNode;
			std::vector<EdgeProperty> Propertys;
			std::vector<IntervalT> ConsumeChars;
			std::optional<SerilizeT> UniqueID;
		};

		struct Node
		{
			std::vector<Edge> Edges;
		};

		std::vector<Node> Nodes;

		UnserilizedTable(UnfaTable const& Table);
	};

	
	struct TableWrapper
	{

		using StorageT = SerilizeT;

		struct ZipChar
		{
			char32_t IsSingleChar : 1;
			char32_t Char : 31;
			std::strong_ordering operator<=>(ZipChar const&) const = default;
			static_assert(sizeof(SerilizeT) == sizeof(char32_t));
		};

		struct alignas(alignof(SerilizeT)) ZipNode
		{
			SerilizeT EdgeCount;
		};

		struct alignas(alignof(SerilizeT)) ZipEdge
		{
			SerilizeT ToNode;
			SerilizeT UniqueID;
			HalfSerilizeT AcceptableCharCount;
			HalfSerilizeT HasCounter : 1;
			HalfSerilizeT PropertyCount : 15;
			HalfSerilizeT CounterDataCount;
			HalfSerilizeT HasUniqueID : 1;
			HalfSerilizeT EdgeTotalLength : 15;


			static_assert(sizeof(HalfSerilizeT) == 2);
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
			SerilizeT NodeOffset;
			SerilizeT EdgeCount;
			std::span<SerilizeT const> AppendData;
		};

		TableWrapper(std::span<SerilizeT const> Input) : Wrapper(Input) {};
		TableWrapper(TableWrapper const&) = default;
		TableWrapper& operator=(TableWrapper const&) = default;
		SerilizeT StartupNodeOffset() const { return  Wrapper[1]; }

		static std::vector<SerilizeT> Create(UnserilizedTable const& Table);

		static std::vector<SerilizeT> Create(std::u32string_view Str, Accept Mask = {}, SerilizeT UniqueID = 0, bool IsRaw = false);

		/*
		static std::vector<SerilizeT> Create(std::u16string_view Str, std::size_t Index = 0, std::size_t Mask = 0);
		static std::vector<SerilizeT> Create(std::u8string_view Str, std::size_t Index = 0, std::size_t Mask = 0);
		static std::vector<SerilizeT> Create(std::wstring_view Str, std::size_t Index = 0, std::size_t Mask = 0);
		*/

		NodeViewer operator[](SerilizeT Offset) const;

	private:

		std::span<SerilizeT const> Wrapper;
		friend struct Table;
	};

	struct CoreProcessor
	{

		struct AmbiguousPoint
		{
			SerilizeT ToNode;
			Misc::IndexSpan<> CounterIndex;
			std::size_t CurrentTokenIndex;
			std::size_t NextTokenIndex;
			std::size_t CaptureCount;
			std::size_t CounterHistory;
			std::optional<SerilizeT> UniqueID;
			std::optional<Accept> AcceptData;
			std::span<TableWrapper::ZipProperty const> Propertys;
		};

		std::vector<std::size_t> AmbiguousCounter;

		std::vector<std::size_t> CounterRecord;
		std::size_t CounterHistory = 0;

		std::vector<AmbiguousPoint> AmbiguousPoints;
		std::vector<Capture> CaptureRecord;
		SerilizeT CurrentState;
		std::size_t StartupTokenIndex = 0;
		std::size_t RequireTokenIndex = 0;
		
		TableWrapper Wrapper;
		bool KeepAcceptableViewer = false;

		CoreProcessor(TableWrapper Wrapper, bool KeepAcceptableViewer = false, std::size_t StartupTokenIndex = 0);
		std::size_t GetRequireTokenIndex() const { return RequireTokenIndex; }
		void SetRequireTokenIndex(std::size_t Index) { RequireTokenIndex = Index; }
		std::size_t GetCurrentState() const { return CurrentState; };
		void Reset(std::size_t StartupTokenIndex = 0);
		void RemoveAmbiuosPoint(SerilizeT UniqueID);
		TableWrapper GetWrapper() const { return Wrapper; }
		std::size_t GetStarupTokenIndex() const { return StartupTokenIndex; }

		struct AcceptResult
		{
			Accept AcceptData;
			SerilizeT UniqueID;
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

		Table(std::u32string_view Regex, Accept Mask = {}, SerilizeT UniqueID = 0, bool IsRaw = false) : Storage(TableWrapper::Create(Regex, Mask, UniqueID, IsRaw)) {}
		
		Table& operator=(Table&&) = default;
		Table& operator=(Table const&) = default;

		operator TableWrapper() const noexcept { return TableWrapper(Storage); }
		TableWrapper AsWrapper() const noexcept { return TableWrapper(Storage); };

	private:

		std::vector<SerilizeT> Storage;
	};

	struct ProcessResult
	{
		Misc::IndexSpan<> MainCapture;
		Accept AcceptData;
		std::vector<Capture> Captures;
		TableWrapper::StorageT UniqueID;

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
		SerilizeT AddRegex(std::u32string_view Regex, Accept Mask, SerilizeT UniqueID, bool IsRaw = false);
		std::vector<SerilizeT> Generate() const;
		operator bool() const { return Temporary.has_value(); }
		SerilizeT GetCountedUniqueID() const { return CountedUniqueID; }
		SerilizeT Push(UnfaTable const& Table);
	private:
		SerilizeT CountedUniqueID = 0;
		std::optional<UnfaTable> Temporary;
	};


	namespace Exception
	{
		struct Interface : public std::exception
		{ 
			virtual ~Interface() = default;
			virtual char const* what() const override;
		};

		struct UnaccaptableRegex : public Interface
		{
			std::size_t TokenIndex;
			Accept AcceptData;
			UnaccaptableRegex(std::size_t TokenIndex, Accept AcceptData)
				: TokenIndex(TokenIndex), AcceptData(AcceptData)
			{

			}
			UnaccaptableRegex() = default;
			UnaccaptableRegex(UnaccaptableRegex const&) = default;
			virtual char const* what() const override;
		};

		struct RegexOutOfRange : public Interface
		{
			RegexOutOfRange() = default;
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
