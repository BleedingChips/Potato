module;

#include <stdint.h>

export module PotatoReg;

import std;
import PotatoInterval;
import PotatoEncode;
import PotatoMisc;
import PotatoSLRX;

export namespace Potato::Reg
{
	using Interval = Misc::IntervalT<char32_t>;

	inline constexpr char32_t MaxChar() { return 0x110000; };
	inline constexpr char32_t EndOfFile() { return 0; }

	struct DfaBinaryTableWrapper;
	struct Dfa;
	struct DfaProcessor;

	struct Nfa
	{
		
		template<typename CharT, typename CharTraiT>
		Nfa(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw = false, std::size_t Mask = 0);
		
		template<typename CharT>
		Nfa(CharT const* Str, bool IsRaw = false, std::size_t Mask = 0) : Nfa(std::basic_string_view<CharT>{Str}, IsRaw, Mask) {}

		Nfa(Nfa const&) = default;
		Nfa(Nfa&&) = default;

		void Link(Nfa const&);

	protected:

		Nfa() = default;

		
		struct NodeSetT
		{
			std::size_t In;
			std::size_t Out;
		};

		enum class EdgePropertyE
		{
			CaptureBegin,
			CaptureEnd,
			OneCounter,
			AddCounter,
			LessCounter,
			BiggerCounter,
			RecordAcceptLocation,
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
			Interval CharSets;
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

		struct BuilderT : public SLRX::ProcessorOperator
		{

			BuilderT(std::size_t Mask = 0, bool IsRaw = false);

			template<typename CharT, typename CharTraiT>
			static auto Create(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw = false, std::size_t Mask = 0)
				->std::vector<NodeT>;

			bool Consume(char32_t InputSymbol, Misc::IndexSpan<> TokenIndex);
			bool EndOfFile();

			std::any HandleSymbol(SLRX::SymbolInfo Value, Interval Chars);
			std::any HandleReduce(SLRX::SymbolInfo Value, SLRX::ReduceProduction Productions);

		protected:

			std::size_t AddNode();
			void AddConsume(NodeSetT Set, Interval Chars, Misc::IndexSpan<> TokenIndex);
			NodeSetT AddCapture(NodeSetT Inside, Misc::IndexSpan<> TokenIndex, std::size_t CaptureIndex);
			NodeSetT AddCounter(NodeSetT Inside, std::optional<std::size_t> Min, std::optional<std::size_t> Max, bool Greedy, Misc::IndexSpan<> TokenIndex, std::size_t CaptureIndex);
			bool InsertSymbol(SLRX::Symbol SymbolValue, char32_t CharValue, Misc::IndexSpan<> TokenIndex) {
				return InsertSymbol(SymbolValue, Interval{ CharValue }, TokenIndex);
			}
			bool InsertSymbol(SLRX::Symbol SymbolValue, Interval CharsValue, Misc::IndexSpan<> TokenIndex);
			
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
			std::size_t Mask = 0;
			char32_t NumberChar = 0;
			bool NumberIsBig = false;
			char32_t RecordSymbol = 0;
			std::size_t RecordTokenIndex = 0;
			std::size_t TokenIndexIte = 0;
			std::vector<NodeT> Nodes;
			SLRX::LRXProcessor Processor;
			std::size_t CaptureCount = 0;
			std::size_t CounterCount = 0;
		};


		std::vector<NodeT> Nodes;
		std::size_t MaskIndex = 1;

		friend struct Dfa;
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

	struct ProcessorAcceptRef
	{
		std::optional<std::size_t> Mask;
		std::span<std::size_t const> Capture;
		Misc::IndexSpan<> MainCapture;

		std::size_t GetMask() const { return *Mask; }
		std::size_t GetCaptureSize() const { return Capture.size() / 2; }
		Misc::IndexSpan<> GetMainCapture() const { return MainCapture; }
		Misc::IndexSpan<> GetCapture(std::size_t Index) const { return { Capture[Index * 2], Capture[Index * 2 + 1] }; }
		Misc::IndexSpan<> operator[](std::size_t Index) const { return GetCapture(Index); }
		operator bool() const { return Mask.has_value(); }
	};

	struct TokenIndexRecorder
	{
		std::optional<std::size_t> StartupTokenIndex;
		std::size_t AcceptTokenIndex;
		std::size_t TotalTokenIndex;

		void Clear() { StartupTokenIndex.reset(); }
		void RecordConsume(std::size_t Index) {
			if (!StartupTokenIndex.has_value())
				StartupTokenIndex = Index;
			TotalTokenIndex = Index;
		}
		void Accept(std::size_t Index)
		{
			AcceptTokenIndex = Index;
		}
		Misc::IndexSpan<> GetAcceptCapture(bool IsAccept) const
		{
			if (StartupTokenIndex.has_value())
			{
				if (IsAccept)
				{
					return { *StartupTokenIndex, AcceptTokenIndex };
				}
				else
					return { *StartupTokenIndex, TotalTokenIndex };
			}
			return {};
		}
	};

	struct DfaProcessor;

	struct Dfa
	{

		enum class FormatE
		{
			Match,
			HeadMatch,
			GreedyHeadMatch,
		};

		Dfa(FormatE Format, Nfa const& T1);
		Dfa(Dfa&&) = default;
		Dfa(Dfa const&) = default;
		Dfa() = default;
		Dfa& operator=(Dfa&&) = default;

		template<typename CharT, typename CharTraisT>
		Dfa(FormatE Format, std::basic_string_view<CharT, CharTraisT> Str, bool IsRaw = false, std::size_t Mask = 0);
		
		template<typename CharT>
		Dfa(FormatE Format, CharT const* Str, bool IsRaw = false, std::size_t Mask = 0)
			: Dfa(Format, std::basic_string_view{ Str }, IsRaw, Mask) {
		}


		std::size_t GetStartupNodeIndex() const { return 0; }
		std::size_t GetCacheCounterCount() const { return CacheRecordCount; }
		std::size_t GetTempResultCount() const { return ResultCount; }
	
	protected:

		virtual bool Consume(DfaProcessor& Context, char32_t InputValue, std::size_t TokenIndex) const;
		virtual bool HasAccept(DfaProcessor const& Context) const;
		virtual ProcessorAcceptRef GetAccept(DfaProcessor const& Context) const;
		
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
			Interval CharSets;
			std::vector<TempPropertyT> Propertys;
			std::vector<TempToNodeT> ToNode;
		};

		struct TempNodeT
		{
			std::vector<TempEdgeT> TempEdge;
			std::vector<std::size_t> OriginalToNode;
			std::optional<Nfa::AcceptT> Accept;
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
			RecordAcceptLocation,
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
				ToAcceptNode
			};
			CommandE PassCommand = CommandE::Fail;
			CommandE UnpassCommand = CommandE::Fail;
			std::size_t Pass = 0;
			std::size_t Unpass = 0;
		};

		struct EdgeT
		{
			Interval CharSets;
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
		std::size_t CacheRecordCount = 0;
		std::size_t ResultCount = 0;
		std::vector<NodeT> Nodes;
		
		friend struct DfaProcessor;
		friend struct DfaBinaryTableWrapper;
	};


	struct DfaBinaryTableWrapper
	{
		using StandardT = uint32_t;
		using HalfStandardT = uint16_t;

		std::size_t GetStartupNodeIndex() const { return reinterpret_cast<HeadT const*>(Wrapper.data())->StartupNodeIndex; }
		std::size_t GetCacheCounterCount() const { return reinterpret_cast<HeadT const*>(Wrapper.data())->CacheSolt; }
		std::size_t GetTempResultCount() const { return reinterpret_cast<HeadT const*>(Wrapper.data())->TempResult; }

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
			HalfStandardT PropertyCount = 0;
			HalfStandardT ConditionCount = 0;
		};

		struct HeadT
		{
			Dfa::FormatE Format;
			StandardT StartupNodeIndex = 0;
			StandardT NodeCount = 0;
			StandardT CacheSolt = 0;
			StandardT TempResult = 0;
		};

		struct AcceptT
		{
			StandardT Mask = 0;
			HalfStandardT CaptureIndexBegin = 0;
			HalfStandardT CaptureIndexEnd = 0;
		};

		static void Serilize(Misc::StructedSerilizerWritter<StandardT>& Writer, Dfa const& RefTable);

		DfaBinaryTableWrapper(std::span<StandardT const> Buffer) : Wrapper(Buffer) {};

	private:

		virtual bool Consume(DfaProcessor& Context, char32_t InputValue, std::size_t TokenIndex) const;
		virtual bool HasAccept(DfaProcessor const& Context) const;
		virtual ProcessorAcceptRef GetAccept(DfaProcessor const& Context) const;

		std::span<StandardT const> Wrapper;

		friend struct DfaProcessor;
	};

	template<typename AllocatorT>
	auto CreateDfaBinaryTable(Dfa const& RefTable, AllocatorT Allocator)
	{
		using StandardT = DfaBinaryTableWrapper::StandardT;
		Misc::StructedSerilizerWritter<StandardT> Predicted;
		DfaBinaryTableWrapper::Serilize(Predicted, RefTable);
		std::vector<StandardT, AllocatorT> Buffer(std::move(Allocator));
		Buffer.resize(Predicted.GetWritedSize());
		auto Span = std::span(Buffer);
		Misc::StructedSerilizerWritter<StandardT> Writter(Span);
		DfaBinaryTableWrapper::Serilize(Writter, RefTable);
		return Buffer;
	}

	inline auto CreateDfaBinaryTable(Dfa const& RefTable) { return CreateDfaBinaryTable(RefTable, std::allocator<DfaBinaryTableWrapper::StandardT>{}); }

	template<typename CharT, typename CharTraits, typename AllocatorT>
	auto CreateDfaBinaryTable(Dfa::FormatE Format, std::basic_string_view<CharT, CharTraits> Str, bool IsRaw = false, std::size_t Mask = 0, AllocatorT Allocator = {})
	{
		return CreateDfaBinaryTable(Dfa{ Format, Str, IsRaw, Mask }, std::move(Allocator));
	}

	template<typename CharT, typename CharTraits>
	auto CreateDfaBinaryTable(Dfa::FormatE Format, std::basic_string_view<CharT, CharTraits> Str, bool IsRaw = false, std::size_t Mask = 0)
	{
		return CreateDfaBinaryTable(Dfa{ Format, Str, IsRaw, Mask }, std::allocator<DfaBinaryTableWrapper::StandardT>{});
	}

	template<typename CharT>
	auto CreateDfaBinaryTable(Dfa::FormatE Format, CharT const* Str, bool IsRaw = false, std::size_t Mask = 0)
	{
		return CreateDfaBinaryTable(Dfa{ Format, std::basic_string_view<CharT>{Str}, IsRaw, Mask }, std::allocator<DfaBinaryTableWrapper::StandardT>{});
	}
	
	struct DfaProcessor
	{

		DfaProcessor(DfaProcessor const&) = default;
		DfaProcessor(DfaProcessor&&) = default;
		DfaProcessor(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: TempResult(resource), CacheIndex(resource) {}

		bool Consume(char32_t Token, std::size_t TokenIndex);
		bool EndOfFile(std::size_t TokenIndex) { return Consume(Reg::EndOfFile(), TokenIndex); }
		void Clear();
		bool HasAccept() const;
		ProcessorAcceptRef GetAccept() const;
		void SetObserverTable(Dfa const& Table) { TableWrapper = std::reference_wrapper<Dfa const>{Table}; Clear(); }
		void SetObserverTable(DfaBinaryTableWrapper Table) { TableWrapper = Table; Clear(); }

	protected:
		
		std::variant<
			std::monostate,
			std::reference_wrapper<Dfa const>,
			DfaBinaryTableWrapper
		> TableWrapper;

		std::size_t CurNodeIndex = 0;
		std::pmr::vector<std::size_t> TempResult;
		std::pmr::vector<std::size_t> CacheIndex;
		TokenIndexRecorder Record;

		friend struct Dfa;
		friend struct DfaBinaryTableWrapper;
	};
	

	template<typename CharT, typename CharTraidT>
	ProcessorAcceptRef Process(DfaProcessor& processor, std::basic_string_view<CharT, CharTraidT> str)
	{
		char32_t tem_input = 0;
		std::span<char32_t> output_span{ &tem_input, 1 };

		using EncoderT = Potato::Encode::StrEncoder<CharT, char32_t>;
		EncoderT encoder;

		auto ite_str = std::span(str);

		bool need_end_of_file = true;

		std::size_t token_index = 0;

		while (!ite_str.empty())
		{
			auto info = encoder.Encode(ite_str, output_span);
			auto re = processor.Consume(tem_input, token_index);
			ite_str = ite_str.subspan(info.source_space);
			token_index += info.source_space;
			if (!re)
			{
				need_end_of_file = false;
				break;
			}
		}

		if (need_end_of_file)
			processor.EndOfFile(token_index);

		return processor.GetAccept();
	}

	template<typename ProcessorT, typename CharT>
	ProcessorAcceptRef Process(ProcessorT& Pro, CharT const* Str)
	{
		return Process(Pro, std::basic_string_view<CharT>(Str));
	}

	template<typename CharT, typename CharTrais, typename Func>
	void Process(Dfa const& Table, std::basic_string_view<CharT, CharTrais> Str, Func&& Fun)
	{
		DfaProcessor Processor;
		Processor.SetObserverTable(Table);
		Fun(Process(Processor, Str));
	}

	template<typename CharT, typename Func>
	void Process(Dfa const& Table, CharT const* Str, Func&& Fun)
	{
		Process(Table, std::basic_string_view<CharT>(Str), std::forward<Func>(Fun));
	}

	struct MulityRegCreater
	{
		template<typename CharT, typename CharTraits>
		void AppendReg(std::basic_string_view<CharT, CharTraits> Str, bool IsRaw, std::size_t Mask)
		{
			if (Table.has_value())
			{
				Table->Link(Nfa{Str, IsRaw, Mask});
			}
			else {
				Table.emplace(Str, IsRaw, Mask);
			}
		}

		template<typename CharT>
		void AppendReg(CharT const* Str, bool IsRaw, std::size_t Mask)
		{
			return AppendReg(std::basic_string_view<CharT>(Str), IsRaw, Mask);
		}

		std::optional<Dfa> CreateDfa(Dfa::FormatE Format) const { if (Table.has_value())  return Dfa{ Format, *Table }; return {}; }

		template<typename AllocatorT = std::allocator<DfaBinaryTableWrapper::StandardT>>
		std::optional<std::vector<DfaBinaryTableWrapper::StandardT>> CreateDfaBinary(Dfa::FormatE Format, AllocatorT Allocator = {}) const { if (Table.has_value())  return CreateDfaBinaryTable(Dfa{Format, *Table}, std::move(Allocator)); return {}; }
	
	protected:
		std::optional<Nfa> Table;
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

	template<typename CharT, typename CharTraiT>
	auto Nfa::BuilderT::Create(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw, std::size_t Mask)
		-> std::vector<NodeT>
	{
		using namespace Exception;

		BuilderT Lex(Mask, IsRaw);

		using EncodeT = Encode::StrEncoder<CharT, char32_t>;
		//using EncodeT = Encode::CharEncoder<CharT, char32_t>;

		auto IteSpan = std::span(Str);

		char32_t TemBuffer = 0;

		std::span<char32_t> OutputSpan = { &TemBuffer, 1 };

		EncodeT encoder;

		while (!IteSpan.empty())
		{
			auto StartIndex = Str.size() - IteSpan.size();
			auto EncodeRe = encoder.Encode(IteSpan, OutputSpan);
			if (!Lex.Consume(TemBuffer, { StartIndex, StartIndex + EncodeRe.source_space }))
				throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {StartIndex, Str.size()} };
			IteSpan = IteSpan.subspan(EncodeRe.source_space);
		}
		if (!Lex.EndOfFile())
			throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {Str.size(), Str.size()} };
		return Lex.Nodes;
	}

	template<typename CharT, typename CharTraiT>
	Nfa::Nfa(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw, std::size_t Mask)
		try : Nodes(BuilderT::Create(Str, IsRaw, Mask)) {}
	catch (Exception::UnaccaptableRegexTokenIndex const& EIndex)
	{
		throw Exception::UnaccaptableRegex{ EIndex.Type, Str, EIndex.BadIndex };
	}

	template<typename CharT, typename CharTraisT>
	Dfa::Dfa(FormatE Format, std::basic_string_view<CharT, CharTraisT> Str, bool IsRaw, std::size_t Mask)
		try : Dfa(Format, Nfa{ Str, IsRaw, Mask }) {}
	catch (Exception::UnaccaptableRegexTokenIndex const& EIndex)
	{
		throw Exception::UnaccaptableRegex{ EIndex.Type, Str, EIndex.BadIndex };
	}
}
