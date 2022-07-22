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

	inline constexpr char32_t EndOfFile() { return MaxChar(); }

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


	struct EpsilonNFATable
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
		};

		struct Edge
		{
			PropertyT Property;
			std::size_t ShiftNode;
			StandardT UniqueID = 0;
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

		static EpsilonNFATable Create(std::u32string_view Str, bool IsRaw, Accept AcceptData);

		void Link(EpsilonNFATable const& OtherTable, bool ThisHasHigherPriority = true);

		EpsilonNFATable(EpsilonNFATable&&) = default;
		EpsilonNFATable(EpsilonNFATable const&) = default;
		EpsilonNFATable& operator=(EpsilonNFATable const&) = default;
		EpsilonNFATable& operator=(EpsilonNFATable&&) = default;

		EpsilonNFATable() {}

		std::size_t NewNode();
		void AddComsumeEdge(std::size_t From, std::size_t To, std::vector<IntervalT> Acceptable);
		void AddAcceptableEdge(std::size_t From, std::size_t To, Accept Data);
		void AddCapture(NodeSet OutsideSet, NodeSet InsideSet);
		NodeSet AddCounter(NodeSet InSideSet, std::optional<std::size_t> Equal, std::optional<std::size_t> Min, std::optional<std::size_t> Max, bool IsGreedy);
		//void AddCounter(EdgeType Type, std::size_t From, std::size_t To, Counter Counter);
		void AddEdge(std::size_t From, Edge Edge);

		std::vector<Node> Nodes;
	};

	struct NFATable
	{

		struct EdgeProperty
		{
			EpsilonNFATable::EdgeType Type;
			std::variant<Accept, Counter> Data;
			std::strong_ordering operator<=>(EdgeProperty const& I2) const = default;
			bool operator==(EdgeProperty const&) const = default;
		};

		struct Edge
		{
			std::size_t ToNode;
			std::vector<EdgeProperty> Propertys;
			std::vector<IntervalT> ConsumeChars;
			std::optional<StandardT> UniqueID;
		};

		struct Node
		{
			std::vector<Edge> Edges;
		};

		std::vector<Node> Nodes;

		NFATable(EpsilonNFATable const& Table);
	};

	struct DFATable
	{

		struct EdgeProperty
		{
			std::vector<NFATable::EdgeProperty> Propertys;
		};
		

		struct Edge
		{
			std::size_t ToNode;
			std::vector<IntervalT> ConsumeChars;
			std::vector<EdgeProperty> Propertys;
		};

		struct Node
		{
			std::vector<Edge> Edges;
		};

		DFATable(NFATable const& Input);
	};

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
				Node,
				EdgeCount,
				AcceptableCharCount,
				PropertyCount,
				CounterCount,
				EdgeLength,
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
