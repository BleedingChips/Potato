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

	struct Accept
	{
		std::size_t Index = 0;
		std::size_t Mask = 0;
		std::strong_ordering operator<=>(Accept const&) const = default;
	};

	struct Capture
	{
		bool IsBegin;
		std::size_t Index;
	};

	struct CodePoint
	{
		char32_t UnicodeCodePoint;
		std::size_t NextUnicodeCodePointOffset;
		static CodePoint EndOfFile() { return { MaxChar(), 0}; }
		bool IsEndOfFile() const { return UnicodeCodePoint == MaxChar(); }
	};

	struct RawString {};

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

		struct CounterEdgeData
		{
			std::size_t Min = 0;
			std::size_t Max = 0;
			std::strong_ordering operator<=>(CounterEdgeData const&) const = default;
		};

		struct Edge
		{
			EdgeType Type;
			std::size_t ShiftNode;
			std::vector<IntervalT> ConsumeChars;
			Accept AcceptData;
			CounterEdgeData CounterDatas;
			std::size_t Block = 1;
			//std::strong_ordering operator<=>(Edge const&) const = default;
			//bool operator==(Edge const&) const = default;
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

		UnfaTable(std::size_t Start, std::size_t End, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData);
		UnfaTable(CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData) :
			UnfaTable(0, std::numeric_limits<std::size_t>::max(), F1, Data, AcceptData){}

		UnfaTable(RawString, std::size_t Start, std::size_t End, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData);
		UnfaTable(RawString, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData) :
			UnfaTable(RawString{}, 0, std::numeric_limits<std::size_t>::max(), F1, Data, AcceptData) {}

		std::size_t NewNode();
		void AddComsumeEdge(std::size_t From, std::size_t To, std::vector<IntervalT> Acceptable);
		void AddAcceptableEdge(std::size_t From, std::size_t To, Accept Data);
		void AddCapture(NodeSet OutsideSet, NodeSet InsideSet);
		void AddCounter(NodeSet OutsideSet, NodeSet InsideSet, CounterEdgeData Counter, bool Greedy = true);
		void AddEdge(std::size_t From, Edge Edge);
		//void Expand(std::vector<SubStorageT>& Output) const;
		void Link(UnfaTable const& OtherTable, bool ThisHasHigherPriority = true);
		

		std::vector<Node> Nodes;
	};

	struct UnserilizedTable
	{

		struct SpecialProperty
		{
			UnfaTable::EdgeType Type;
			Accept AcceptData;
			UnfaTable::CounterEdgeData CounterData;
			std::strong_ordering operator<=>(SpecialProperty const&) const = default;
		};

		struct EdgeProperty
		{
			std::vector<IntervalT> ConsumeChars;
			std::vector<SpecialProperty> Propertys;
			std::strong_ordering operator<=>(EdgeProperty const& I2) const = default;
			bool operator==(EdgeProperty const&) const = default;
		};

		struct Edge
		{
			std::size_t ToNode;
			EdgeProperty Propertys;
			std::size_t Block = 1;
			bool operator ==(Edge const& I2) const { return ToNode == I2.ToNode && Propertys == I2.Propertys; }
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
		
		using StorageT = uint32_t;
		using SubStorageT = uint16_t;

		struct ZipChar
		{
			char32_t IsSingleChar : 1;
			char32_t Char : 31;
			std::strong_ordering operator<=>(ZipChar const&) const = default;
			static_assert(sizeof(StorageT) == sizeof(char32_t));
		};

		struct alignas(alignof(StorageT)) ZipNode
		{
			StorageT EdgeCount;
		};

		struct alignas(alignof(StorageT)) ZipEdge
		{
			StorageT ToNode;
			SubStorageT AcceptableCharCount;
			SubStorageT SpecialPropertyCount;
			SubStorageT CounterDataCount;
			SubStorageT AcceptableDataCount : 1;
			SubStorageT Block : 15;
		};

		enum class ZipProperty : SubStorageT
		{
			Acceptable,
			CaptureBegin,
			CaptureEnd,
			CounterBegin,
			CounterEnd,
			CounterContinue,
		};

		struct alignas(alignof(StorageT)) ZipAcceptableData
		{
			StorageT Index;
			StorageT Mask;
		};

		struct alignas(alignof(StorageT)) ZipCounterData
		{
			StorageT Min;
			StorageT Max;
		};

		struct EdgeViewer
		{
			StorageT ToNode;
			SubStorageT Block;
			std::span<ZipChar const> ConsumeChars;
			std::span<ZipProperty const> Property;
			std::span<ZipCounterData const> CounterData;
			std::span<ZipAcceptableData const> AcceptData;
			std::span<StorageT const> AppendDatas;
		};

		struct NodeViewer
		{
			StorageT NodeOffset;
			StorageT EdgeCount;
			std::span<StorageT const> AppendData;
			template<typename Func> std::size_t ReadEdge(Func&& FunObject) const requires(std::is_invocable_r_v<bool, Func, EdgeViewer>)
			{
				auto LastSpan = AppendData;
				for (std::size_t Index = 0; Index < EdgeCount; ++Index)
				{
					auto Edge = TableWrapper::ReadEdgeViewer(LastSpan);
					if (!std::forward<Func>(FunObject)(Edge))
						return Index;
					LastSpan = Edge.AppendDatas;
				}
				return EdgeCount;
			}
		};

		

		TableWrapper(std::span<StorageT const> Input) : Wrapper(Input) {};
		TableWrapper(TableWrapper const&) = default;
		TableWrapper& operator=(TableWrapper const&) = default;
		StorageT StartupNodeOffset() const { return  Wrapper[1]; }

		static std::vector<StorageT> Create(UnserilizedTable const& Table);
		static std::vector<StorageT> Create(std::u32string_view Str, std::size_t Index = 0, std::size_t Mask = 0);

		NodeViewer operator[](StorageT Offset) const;
		
		static EdgeViewer ReadEdgeViewer(std::span<StorageT const> Span);
		static constexpr std::size_t MaxMaskAndIndexValue() { return std::numeric_limits<decltype(ZipAcceptableData::Index)>::max(); }
		/*
		StorageT NodeCount() const noexcept { return Wrapper.empty() ? 0 : Wrapper[0]; }
		//std::size_t ComsumeCharactor(TableStorageT NodeOffset, char32_t Symbol, TableStorageT SymbolIndex, std::vector<TableStorageT>& Output, std::vector<TableStorageT>& AllreadyNode) const;
		NodeViewer ReadNodeViewer(StorageT Offset) const;
		static NodeViewer ReadNodeViewer(std::span<SubStorageT const> Ptr);
		static PropertyViewer ReadPropertyViewer(std::span<SubStorageT const> Ptr);
		TableStorageT StartupOffset() const noexcept {return 1;};
		static std::vector<SubStorageT> Create(UnfaTable const& Input);
		*/

	private:

		std::span<StorageT const> Wrapper;
		friend struct Table;
	};

	struct Table
	{
		Table(Table&&) = default;
		Table(Table const&) = default;
		Table(std::u32string_view Regex, std::size_t State = 0, std::size_t Mask = 0) : Storage(TableWrapper::Create(Regex, State, Mask)) {}
		Table& operator=(Table&&) = default;
		Table& operator=(Table const&) = default;

		operator TableWrapper() const noexcept { return TableWrapper(Storage); }
		TableWrapper AsWrapper() const noexcept { return TableWrapper(Storage); };
	private:
		std::vector<TableWrapper::StorageT> Storage;
	};

	struct MulityRegexCreator
	{
		std::size_t Push(UnfaTable const& Table);
		std::size_t PushRegex(std::u32string_view Reg, Accept AcceptData);
		std::size_t PushRegex(std::u32string_view Reg, std::size_t Mask) { return  PushRegex(Reg, {Count(), Mask}); }
		std::size_t PushRegex(std::size_t Start, std::size_t End, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData) {
			return Push(UnfaTable{ Start, End, F1, Data, AcceptData });
		}
		std::size_t PushRawString(std::u32string_view Reg, Accept AcceptData);
		std::size_t PushRawString(std::size_t Start, std::size_t End, CodePoint(*F1)(std::size_t Index, void* Data), void* Data, Accept AcceptData) {
			return Push(UnfaTable{RawString{}, Start, End, F1, Data, AcceptData });
		}
		std::vector<TableWrapper::StorageT> Generate() const;
		operator bool () const{return Temporary.has_value(); }
		std::size_t Count() const { return Index; }
	private:
		std::size_t Index = 0;
		std::optional<UnfaTable> Temporary;
	};

	struct CoreProcessor
	{

		struct AmbiguousPoint
		{
			Misc::IndexSpan<> CounterIndex;
			bool IsSceondory;
			std::size_t TokenIndex;
			std::size_t NextTokenIndex;
			std::size_t CaptureCount;
			TableWrapper::EdgeViewer Viewer;
		};
		std::vector<std::size_t> AmbiguousCounter;
		std::vector<std::size_t> ConuterRecord;
		std::vector<AmbiguousPoint> AmbiguousPoints;
		std::vector<Capture> CaptureRecord;
		std::vector<std::size_t> Walkway;
		TableWrapper::StorageT CurrentState;
		TableWrapper Wrapper;

		struct ConsumeResult
		{
			std::optional<Accept> AcceptData;
		};

		std::optional<ConsumeResult> ConsumeCharInput(char32_t InputSymbols, std::size_t TokenIndex, std::size_t NextTokenIndex, bool KeepAcceptableViewer = false);
		std::optional<AmbiguousPoint> PopAmbiguousPoint();
		CoreProcessor(TableWrapper Wrapper) : Wrapper(Wrapper), CurrentState(Wrapper.StartupNodeOffset()) {}
		void Clear() { AmbiguousCounter.clear(); ConuterRecord.clear(); AmbiguousPoints.clear(); CaptureRecord.clear(); CurrentState = Wrapper.StartupNodeOffset(); ClearWalkWay(); }
		void ClearWalkWay() { Walkway.clear(); }

		struct FlushResult
		{
			Misc::IndexSpan<> LastToken;
			std::optional<Accept> AcceptData;
			bool IsEndOfFile() const { return LastToken.Count() == 0; }
		};

		template<typename FuncO>
		std::optional<FlushResult> Flush(std::optional<std::size_t> Start, std::size_t End, bool StopIfMeetAcceptableViewer, FuncO&& Obj)
			requires(std::is_invocable_r_v<CodePoint, FuncO, std::size_t>);
		std::optional<FlushResult> Flush(std::optional<std::size_t> Start, std::size_t End, bool StopIfMeetAcceptableViewer, CodePoint(*FuncO)(std::size_t, void*), void* Data);
	private:	
		std::optional<Accept> ApplyAmbiguousPoint(AmbiguousPoint AP);
	};

	template<typename FuncO>
	auto CoreProcessor::Flush(std::optional<std::size_t> Start, std::size_t End, bool StopIfMeetAcceptableViewer, FuncO&& Obj)-> std::optional<FlushResult>
		requires(std::is_invocable_r_v<CodePoint, FuncO, std::size_t>)
	{
		return Flush(Start, End, StopIfMeetAcceptableViewer, [](std::size_t Index, void* Data)->std::optional<CodePoint> {
			return (*reinterpret_cast<std::remove_reference_t<FuncO>*>(Data))(Index);
		}, &Obj);
	}

	struct MarchProcessor
	{

		struct Result
		{
			Misc::IndexSpan<> MainCapture;
			std::optional<Accept> AcceptData;
			std::vector<Capture> Captures;
		};
	};

	std::optional<MarchProcessor::Result> ProcessMarch(TableWrapper Wrapper, std::u32string_view Span);
	std::optional<MarchProcessor::Result> ProcessMarch(TableWrapper Wrapper, std::size_t Startup, CodePoint(*F1)(std::size_t Index, void*), void* Data);
	template<typename Func>
	static std::optional<MarchProcessor::Result> ProcessMarch(TableWrapper Wrapper, std::size_t Startup, Func&& FO)
		requires(std::is_invocable_r_v<CodePoint, Func, std::size_t>)
	{
		return ProcessMarch(Wrapper, Startup, [](std::size_t Index, void* Data) -> std::optional<CoreProcessor::FlushInput> {
			return (*reinterpret_cast<std::remove_reference_t<Func>*>(Data))(Index);
			}, &FO);
	}

	struct GreedyMarchProcessor
	{
		struct Result
		{
			Misc::IndexSpan<> MainCapture;
			Reg::Accept AcceptData;
			std::vector<Reg::Capture> Captures;
			bool IsEndOfFile;
		};
	};

	template<typename Func>
	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::size_t Startup, Func&& FO)
		requires(std::is_invocable_r_v<CodePoint, Func, std::size_t>)
	{
		return ProcessGreedMarch(Wrapper, Startup, [](std::size_t Index, void* Data) -> CodePoint {
			return (*reinterpret_cast<std::remove_reference_t<Func>*>(Data))(Index);
			}, &FO);
	}

	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::size_t Startup, CodePoint(*F1)(std::size_t Index, void*), void* Data);
	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::u32string_view SpanView);
	std::optional<GreedyMarchProcessor::Result> ProcessGreedMarch(TableWrapper Wrapper, std::wstring_view SpanView);

	struct SearchProcessor
	{
		struct Result
		{
			Misc::IndexSpan<> MainCapture;
			std::vector<Capture> Captures;
			bool IsEndOfFile;
		};

		struct SearchCore
		{
			std::size_t TokenIndex;
			std::size_t NextTokenIndex;
			TableWrapper::EdgeViewer Viewer;
		};
	};

	std::optional<SearchProcessor::Result> ProcessSearch(TableWrapper Wrapper, std::u32string_view Span);

	template<typename Func>
	std::optional<SearchProcessor::Result> ProcessSearch(TableWrapper Wrapper, std::size_t Start, Func&& FO)
		requires(std::is_invocable_r_v<CodePoint, Func, std::size_t>)
	{
		return ProcessSearch(Wrapper, Start, [](std::size_t Index, void* Data) ->CodePoint {
			return (*reinterpret_cast<std::remove_reference_t<Func>*>(Data))(Index);
			}, &FO);
	}

	std::optional<SearchProcessor::Result> ProcessSearch(TableWrapper Wrapper, std::size_t Start, CodePoint(*F1)(std::size_t Index, void* Data), void* Data);

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

	template<typename CharT>
	struct CodePointGenerator
	{
		static CodePoint Function(std::size_t Index, void* Self) {
			auto This = reinterpret_cast<CodePointGenerator*>(Self);
			if (Index < This->Str.size())
			{
				CharT Result;
				auto EncodeResult = StrEncode::CoreEncoder<CharT, char32_t>::EncodeOnceUnSafe(This->Str.subspan(Index), {&Result, 1});
				return {Result, EncodeResult.SourceSpace};
			}else
				return CodePoint::EndOfFile();
		}
		std::span<CharT const> Str;
	};

}
