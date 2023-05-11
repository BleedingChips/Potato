module;

export module Potato.Reg;
export import Potato.SLRX;
export import Potato.Interval;
export import Potato.Encode;
export import Potato.STD;

export namespace Potato::Reg
{

	using IntervalT = Misc::IntervalT<char32_t>;

	inline constexpr char32_t MaxChar() { return 0x110000; };
	inline constexpr char32_t EndOfFile() { return 0; }

	struct RegLexerT
	{
		
		enum class ElementEnumT
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
		RegLexerT(RegLexerT&&) = default;

		template<typename CharT, typename CharTraiT>
		static RegLexerT Create(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw = false);

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
		
		template<typename CharT, typename CharTraiT>
		NfaT(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw = false, std::size_t Mask = 0);

		NfaT(NfaT const&) = default;
		NfaT(NfaT&&) = default;
		void Link(NfaT const&);

	protected:

		NfaT(RegLexerT const& Lexer, std::size_t Mask = 0);
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
			Match,
			HeadMatch,
			GreedyHeadMatch,
		};

		DfaT(FormatE Format, NfaT const& T1);

		template<typename CharT, typename CharTraisT>
		DfaT(FormatE Format, std::basic_string_view<CharT, CharTraisT> Str, bool IsRaw = false, std::size_t Mask = 0);
		
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
		
		friend struct DfaProcessor;
		friend struct DfaBinaryTableWrapper;
		friend struct DfaBinaryTableWrapperProcessor;
	};

	struct ProcessorAcceptT
	{
		std::size_t Mask;
		std::vector<Misc::IndexSpan<>> Capture;
		Misc::IndexSpan<> MainCapture;
	};

	struct ProcessorUnacceptT
	{
		Misc::IndexSpan<> FailCapture;
	};

	template<typename ProcessorT, typename CharT, typename CharTraidT>
	std::variant<ProcessorAcceptT, ProcessorUnacceptT> CoreProcessorWithUnaccept(ProcessorT& Pro, std::basic_string_view<CharT, CharTraidT> Str)
	{
		char32_t TemBuffer = 0;
		std::span<char32_t> OutputSpan{ &TemBuffer, 1 };

		using EncoderT = Potato::Encode::CharEncoder<CharT, char32_t>;

		auto IteStr = std::span(Str);

		bool NeedEndOfFile = true;

		std::size_t TokeIndex = 0;

		while (!IteStr.empty())
		{
			auto EncodeRe = EncoderT::EncodeOnceUnSafe(IteStr, OutputSpan);
			auto Re = Pro.Consume(TemBuffer, TokeIndex);
			IteStr = IteStr.subspan(EncodeRe.SourceSpace);
			TokeIndex += EncodeRe.SourceSpace;
			if (!Re)
			{
				NeedEndOfFile = false;
				break;
			}
		}

		if (NeedEndOfFile)
			Pro.EndOfFile(TokeIndex);

		auto Accept = Pro.GetAccept();

		if(Accept.has_value())
			return std::move(*Accept);
		else
			return ProcessorUnacceptT{{0, TokeIndex} };
	}

	template<typename ProcessorT, typename CharT, typename CharTraidT>
	std::optional<ProcessorAcceptT> CoreProcessor(ProcessorT& Pro, std::basic_string_view<CharT, CharTraidT> Str)
	{
		auto Re = CoreProcessorWithUnaccept(Pro, Str);
		if(std::holds_alternative<ProcessorAcceptT>(Re))
			return std::move(std::get<ProcessorAcceptT>(Re));
		return {};
	}

	struct DfaProcessor
	{
		DfaProcessor(DfaT const& Table);
		DfaProcessor(DfaProcessor const&) = default;

		bool Consume(char32_t Token, std::size_t TokenIndex);
		bool EndOfFile(std::size_t TokenIndex) { return Consume(Reg::EndOfFile(), TokenIndex); }
		void Reset();
		bool HasAccept() const { return Accept.has_value(); }
		std::optional<ProcessorAcceptT> GetAccept() const;
	
	public:
		
		DfaT const& Table;
		std::size_t CurNodeIndex;
		std::vector<std::size_t> TempResult;
		std::vector<std::size_t> CacheIndex;
		std::optional<DfaT::AcceptT> Accept;
		std::optional<Misc::IndexSpan<>> CurMainCapture;
		
		friend struct DfaBinaryTableWrapperProcessor;
	};

	template<typename CharT, typename CharTrais>
	std::optional<ProcessorAcceptT> Process(DfaT const& Table, std::basic_string_view<CharT, CharTrais> Str)
	{
		DfaProcessor Processor{ Table };
		return CoreProcessor(Processor, Str);
	}

	struct DfaBinaryTableWrapper
	{
		using StandardT = std::uint32_t;
		using HalfStandardT = std::uint16_t;

		std::size_t GetStartupNodeIndex() const { return reinterpret_cast<HeadT const*>(Wrapper.data())->StartupNodeIndex; }
		std::size_t GetCacheCounterCount() const { return reinterpret_cast<HeadT const*>(Wrapper.data())->CacheSolt; }


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
			DfaT::FormatE Format;
			StandardT StartupNodeIndex = 0;
			StandardT NodeCount = 0;
			StandardT CacheSolt = 0;
		};

		struct AcceptT
		{
			StandardT Mask = 0;
			HalfStandardT CaptureIndexBegin = 0;
			HalfStandardT CaptureIndexEnd = 0;
		};
		
		static void Serilize(Misc::StructedSerilizerWritter<StandardT>& Writer, DfaT const& RefTable);
		
		template<typename AllocatorT = std::allocator<StandardT>>
		static auto Create(DfaT const& RefTable, AllocatorT const& Acclcator= {}) -> std::vector<StandardT, AllocatorT>
		{
			Misc::StructedSerilizerWritter<StandardT> Predicted;
			Serilize(Predicted, RefTable);
			std::vector<StandardT, AllocatorT> Buffer(Acclcator);
			Buffer.resize(Predicted.GetWritedSize());
			auto Span = std::span(Buffer);
			Misc::StructedSerilizerWritter<StandardT> Writter(Span);
			Serilize(Writter, RefTable);
			return Buffer;
		}

		DfaBinaryTableWrapper(std::span<StandardT> Buffer) : Wrapper(Buffer) {};

	private:

		
		std::span<StandardT> Wrapper;

		friend struct DfaBinaryTableWrapperProcessor;
	};

	


	struct DfaBinaryTableWrapperProcessor
	{
		DfaBinaryTableWrapperProcessor(DfaBinaryTableWrapper Table);
		bool Consume(char32_t Token, std::size_t TokenIndex);
		bool EndOfFile(std::size_t TokenIndex) { return Consume(Reg::EndOfFile(), TokenIndex); }
		void Reset();
		bool HasAccept() const { return Accept.has_value(); }
		std::optional<ProcessorAcceptT> GetAccept() const;
	protected:
		DfaBinaryTableWrapper Table;
		std::size_t CurrentNode;
		std::vector<std::size_t> TempResult;
		std::vector<std::size_t> CacheIndex;
		std::optional<DfaBinaryTableWrapper::AcceptT> Accept;
		std::optional<Misc::IndexSpan<>> CurMainCapture;
	};

	template<typename CharT, typename CharTrais>
	std::optional<ProcessorAcceptT> Process(DfaBinaryTableWrapper const& Table, std::basic_string_view<CharT, CharTrais> Str)
	{
		DfaBinaryTableWrapperProcessor Processor{ Table };
		return RegCoreProcessor(Processor, Str);
	}

	export namespace Exception
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
	RegLexerT RegLexerT::Create(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw)
	{
		using namespace Exception;
		RegLexerT Lex(IsRaw);
		using EncodeT = Encode::CharEncoder<CharT, char32_t>;
		using EncodeT = Encode::CharEncoder<CharT, char32_t>;

		auto IteSpan = std::span(Str);

		char32_t TemBuffer = 0;

		std::span<char32_t> OutputSpan = { &TemBuffer, 1 };

		while (!IteSpan.empty())
		{
			auto StartIndex = Str.size() - IteSpan.size();
			auto EncodeRe = EncodeT::EncodeOnceUnSafe(IteSpan, OutputSpan);
			if (!Lex.Consume(TemBuffer, { StartIndex, StartIndex + EncodeRe.SourceSpace }))
				throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {StartIndex, Str.size()} };
			IteSpan = IteSpan.subspan(EncodeRe.SourceSpace);
		}
		if (!Lex.EndOfFile())
			throw Exception::UnaccaptableRegex{ UnaccaptableRegex::TypeT::BadRegex, Str, {Str.size(), Str.size()} };
		return Lex;
	}

	template<typename CharT, typename CharTraiT>
	NfaT::NfaT(std::basic_string_view<CharT, CharTraiT> Str, bool IsRaw, std::size_t Mask)
		try : NfaT(RegLexerT::Create(Str, IsRaw), Mask) {}
	catch (Exception::UnaccaptableRegexTokenIndex const& EIndex)
	{
		throw Exception::UnaccaptableRegex{ EIndex.Type, Str, EIndex.BadIndex };
	}

	template<typename CharT, typename CharTraisT>
	DfaT::DfaT(FormatE Format, std::basic_string_view<CharT, CharTraisT> Str, bool IsRaw, std::size_t Mask)
		try : DfaT(Format, NfaT{ Str, IsRaw, Mask }) {}
	catch (Exception::UnaccaptableRegexTokenIndex const& EIndex)
	{
		throw Exception::UnaccaptableRegex{ EIndex.Type, Str, EIndex.BadIndex };
	}
}
