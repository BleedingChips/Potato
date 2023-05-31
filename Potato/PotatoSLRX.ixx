export module Potato.SLRX;
export import Potato.Misc;
export import Potato.STD;

export namespace Potato::SLRX
{

	struct Symbol
	{
		enum class T
		{
			TERMINAL = 0,
			NOTERMIAL,
			STARTSYMBOL,
			ENDOFFILE,
		};

		constexpr std::strong_ordering operator<=>(Symbol const& Input) const = default;
		constexpr static Symbol EndOfFile() { return { T::ENDOFFILE, 0 }; }
		constexpr static Symbol StartSymbol() { return { T::STARTSYMBOL, 0 }; }
		constexpr static Symbol AsNoTerminal(std::size_t Value) { return { T::NOTERMIAL, Value};  }
		constexpr static Symbol AsTerminal(std::size_t Value) { return { T::TERMINAL, Value }; }
		constexpr bool IsTerminal() const { return Type == T::TERMINAL || IsEndOfFile(); }
		constexpr bool IsNoTerminal() const { return Type == T::NOTERMIAL || IsStartSymbol(); }
		constexpr bool IsEndOfFile() const { return Type == T::ENDOFFILE; }
		constexpr bool IsStartSymbol() const { return Type == T::STARTSYMBOL; }

		T Type = T::ENDOFFILE;
		std::size_t Value = 0;
	};

	struct ParsingStep
	{
		Symbol Value;

		struct ReduceT
		{
			std::size_t ProductionIndex = 0;
			std::size_t ElementCount = 0;
			std::size_t Mask = 0;
		};

		struct ShiftT
		{
			std::size_t TokenIndex = 0;
		};

		ReduceT Reduce;
		ShiftT Shift;

		constexpr bool IsTerminal() const { return Value.IsTerminal(); }
		constexpr bool IsNoTerminal() const { return Value.IsNoTerminal(); }
		constexpr bool IsShift() const { return IsTerminal(); }
		constexpr bool IsReduce() const { return IsNoTerminal(); }
	};

	struct OpePriority
	{
		enum class Associativity
		{
			Left,
			Right
		};

		OpePriority(std::vector<Symbol> InitList) : Symbols(std::move(InitList)) {}
		OpePriority(std::vector<Symbol> InitList, Associativity Asso) : Symbols(std::move(InitList)), Priority(Asso) {}
		std::vector<Symbol> Symbols;
		Associativity Priority = Associativity::Left;
	};

	struct ItSelf {};

	struct ProductionBuilderElement
	{
		constexpr ProductionBuilderElement(ProductionBuilderElement const&) = default;
		constexpr ProductionBuilderElement& operator=(ProductionBuilderElement const&) = default;
		constexpr ProductionBuilderElement(Symbol Value) : Type(TypeT::IsValue), ProductionValue(Value) {};
		constexpr ProductionBuilderElement(std::size_t Mask) : Type(TypeT::IsMask), ProductionMask(Mask) {};
		constexpr ProductionBuilderElement(ItSelf) : Type(TypeT::ItSelf) {};

		enum class TypeT
		{
			IsValue,
			IsMask,
			ItSelf
		};

		TypeT Type = TypeT::IsValue;
		Symbol ProductionValue = Symbol::EndOfFile();
		std::size_t ProductionMask = 0;
	};

	struct ProductionBuilder
	{
		ProductionBuilder(Symbol ProductionValue, std::vector<ProductionBuilderElement> ProductionElement, std::size_t ProductionMask = 0)
			: ProductionValue(ProductionValue), Element(std::move(ProductionElement)), ProductionMask(ProductionMask) {}

		ProductionBuilder(const ProductionBuilder&) = default;
		ProductionBuilder(ProductionBuilder&&) = default;
		ProductionBuilder& operator=(const ProductionBuilder&) = default;
		ProductionBuilder& operator=(ProductionBuilder&&) = default;

		Symbol ProductionValue;
		std::vector<ProductionBuilderElement> Element;
		std::size_t ProductionMask = 0;
	};

	struct ProductionInfo
	{

		ProductionInfo(Symbol StartSymbol, std::vector<ProductionBuilder> ProductionBuilders, std::vector<OpePriority> Priority);

		struct Element
		{
			Symbol Symbol;
			std::vector<std::size_t> AcceptableProductionIndex;
		};

		struct Production
		{
			Symbol Symbol;
			std::size_t ProductionMask;
			std::vector<Element> Elements;
		};

		struct SearchElement
		{
			std::size_t ProductionIndex;
			std::size_t ElementIndex;
			std::strong_ordering operator<=>(SearchElement const& Input) const = default;
		};

		std::optional<Symbol> GetElementSymbol(std::size_t ProductionIndex, std::size_t ElementIndex) const;
		std::span<std::size_t const> GetAcceptableProductionIndexs(std::size_t ProductionIndex, std::size_t ElementIndex) const;
		std::span<std::size_t const> GetAllProductionIndexs(Symbol Input) const;
		bool IsAcceptableProductionIndex(std::size_t ProductionIndex, std::size_t ElementIndex, std::size_t RequireIndex) const;

		SearchElement GetStartupSearchElements() const;
		SearchElement GetEndSearchElements() const;
		void ExpandSearchElements(std::vector<SearchElement>& InoutElements) const;
		static std::tuple<std::size_t, std::size_t> IntersectionSet(std::span<std::size_t> Source1AndOutout, std::span<std::size_t> Source2);

		std::vector<Production> ProductionDescs;
		std::map<Symbol, std::vector<std::size_t>> TrackedAcceptableNoTerminalSymbol;
	};

	struct LR0
	{
		struct ShiftEdge
		{
			Symbol RequireSymbol;
			std::size_t ToNode;
			bool ReverseStorage;
			std::vector<std::size_t> ProductionIndex;
		};

		struct Reduce
		{
			Symbol ReduceSymbol;
			ParsingStep::ReduceT Reduce;
			operator ParsingStep () const {
				ParsingStep New;
				New.Value = ReduceSymbol;
				New.Reduce = Reduce;
				return New;
			}
		};

		struct MappedProduction
		{
			ProductionInfo::Production Production;
			std::size_t ProductionIndex;
			std::vector<std::size_t> DetectedPoint;
		};

		struct Node
		{
			std::size_t ForwardTokenRequire = 0;
			std::vector<ShiftEdge> Shifts;
			std::vector<Reduce> Reduces;
			std::vector<MappedProduction> MappedProduction;
		};

		std::vector<Node> Nodes;

		bool IsAvailable() const { return !Nodes.empty(); }
		operator bool() const { return IsAvailable(); }

		static LR0 Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority) {
			return Create(ProductionInfo{ StartSymbol, std::move(Production), std::move(Priority) });
		}
		static LR0 Create(ProductionInfo Info);

		LR0(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
			: LR0(Create(StartSymbol, std::move(Production), std::move(Priority))) {}
		LR0(ProductionInfo Info) : LR0(Create(std::move(Info))) {}
		LR0(LR0&&) = default;
		LR0(LR0 const&) = default;
		LR0() = default;
		LR0& operator=(LR0 const&) = default;
		LR0& operator=(LR0&&) = default;
	private:
		LR0(std::vector<Node> Nodes) : Nodes(std::move(Nodes)) {}
	};

	struct LRX
	{

		struct ReduceTuple
		{
			std::size_t LastState;
			std::size_t TargetState;
			bool NeedPredict = false;
		};

		struct ReduceProperty
		{
			std::vector<ReduceTuple> Tuples;
			LR0::Reduce Property;
		};

		enum class RequireNodeType
		{
			SymbolValue,
			ShiftProperty,
			NeedPredictShiftProperty,
			ReduceProperty,
		};

		struct RequireNode
		{
			RequireNodeType Type;
			Symbol RequireSymbol;
			std::size_t ReferenceIndex;
		};

		struct Node
		{
			std::vector<std::vector<RequireNode>> RequireNodes;
			std::vector<ReduceProperty> Reduces;
			std::vector<LR0::MappedProduction> MappedProduction;
		};

		std::vector<Node> Nodes;

		static constexpr std::size_t DefaultMaxForwardDetect = 3;

		static LRX Create(LR0 const& Table, std::size_t MaxForwardDetect = DefaultMaxForwardDetect);
		static LRX Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect = DefaultMaxForwardDetect)
		{
			return Create(LR0{ StartSymbol, std::move(Production), std::move(Priority) }, MaxForwardDetect);
		}
		bool IsAvailable() const { return !Nodes.empty(); }
		operator bool() const { return IsAvailable(); }
		LRX(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect = DefaultMaxForwardDetect)
			: LRX(Create(StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetect)) {}
		LRX(LR0 const& Table, std::size_t MaxForwardDetect = 3) : LRX(Create(Table, MaxForwardDetect)) {}
		LRX(LRX const&) = default;
		LRX(LRX&&) = default;
		LRX& operator=(LRX const&) = default;
		LRX& operator=(LRX&&) = default;
		LRX() = default;

		static constexpr std::size_t StartupOffset() { return 1; }

	private:
		LRX(std::vector<Node> Nodes) : Nodes(std::move(Nodes)) {}
	};

	struct LRXBinaryTableWrapper
	{
		using StandardT = std::uint32_t;

		using HalfStandardT = std::uint16_t;
		using HalfHalfStandardT = std::uint8_t;

		static_assert(sizeof(HalfStandardT) * 2 == sizeof(StandardT));
		static_assert(sizeof(HalfHalfStandardT) * 2 == sizeof(HalfStandardT));

		struct alignas(alignof(StandardT)) ZipHeadT
		{
			StandardT NodeCount = 0;
			StandardT StartupOffset = 0;
		};

		struct alignas(alignof(StandardT)) ZipNodeT
		{
			HalfStandardT RequireNodeDescCount;
			HalfStandardT ReduceCount;
		};

		struct alignas(alignof(StandardT)) ZipRequireNodeT
		{
			LRX::RequireNodeType Type : 3;
			StandardT IsEndOfFile : 1;
			StandardT ToIndexOffset;
			StandardT Value;
		};

		struct alignas(alignof(StandardT)) ZipRequireNodeDescT
		{
			StandardT RequireNodeCount;
		};

		struct alignas(alignof(StandardT)) ZipReducePropertyT
		{
			HalfStandardT ProductionIndex;
			HalfStandardT ProductionCount;
			HalfStandardT ReduceTupleCount;
			StandardT Mask;
			StandardT NoTerminalValue;
		};

		struct alignas(alignof(StandardT)) ZipReduceTupleT
		{
			StandardT LastState;
			StandardT ToState;
			StandardT NeedPredict;
		};

		static void Serilize(Misc::StructedSerilizerWritter<StandardT>& Writer, LRX const& Ref);

		static std::vector<StandardT> Create(LRX const& Le);
		static std::vector<StandardT> Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect = 3) {
			return Create(LRX{ StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetect });
		}

		LRXBinaryTableWrapper(std::span<StandardT const> InputBuffer) : Buffer(InputBuffer) {}
		LRXBinaryTableWrapper(LRXBinaryTableWrapper const&) = default;
		LRXBinaryTableWrapper() = default;
		LRXBinaryTableWrapper& operator=(LRXBinaryTableWrapper const&) = default;

		Misc::StructedSerilizerReader<StandardT const> GetReader() const { return Misc::StructedSerilizerReader<StandardT const>(Buffer); }

		operator bool() const { return !Buffer.empty(); }
		std::size_t NodeCount() const { return reinterpret_cast<ZipHeadT const*>(Buffer.data())->NodeCount; }
		std::size_t StartupNodeIndex() const { return reinterpret_cast<ZipHeadT const*>(Buffer.data())->StartupOffset; }
		//bool StartupNeedPredict() const { return Buffer[2]; }
		std::size_t TotalBufferSize() const { return Buffer.size(); }

		std::span<StandardT const> Buffer;

	};

	struct LRXBinaryTable
	{
		LRXBinaryTable(LRXBinaryTable const& Input) : Datas(Input.Datas) { Wrapper = LRXBinaryTableWrapper{ Datas }; }
		LRXBinaryTable(LRXBinaryTable&& Input) : Datas(std::move(Input.Datas)) { Wrapper = LRXBinaryTableWrapper{ Datas }; Input.Wrapper = {}; }
		LRXBinaryTable& operator=(LRXBinaryTable const& Input) {
			Datas = Input.Datas;
			Wrapper = LRXBinaryTableWrapper{ Datas };
			return *this;
		}
		LRXBinaryTable& operator=(LRXBinaryTable&& Input) noexcept {
			Datas = std::move(Input.Datas);
			Wrapper = LRXBinaryTableWrapper{ Datas };
			return *this;
		}
		LRXBinaryTable() = default;
		LRXBinaryTable(SLRX::LRX const& Table)
		{
			Datas = LRXBinaryTableWrapper::Create(Table);
			Wrapper = LRXBinaryTableWrapper(Datas);
		}
		LRXBinaryTable(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetected)
			: LRXBinaryTable(SLRX::LRX{ StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetected })
		{

		}
		LRXBinaryTable(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
			: LRXBinaryTable(SLRX::LRX{ StartSymbol, std::move(Production), std::move(Priority) })
		{
		}
		std::size_t NodeCount() const { return Wrapper.NodeCount(); }
		operator LRXBinaryTableWrapper() const { return Wrapper; }
		std::vector<LRXBinaryTableWrapper::StandardT> Datas;
		LRXBinaryTableWrapper Wrapper;
	};


	struct SymbolInfo
	{
		Symbol Value;
		Misc::IndexSpan<> TokenIndex;
	};

	struct ReduceInfo
	{
		std::size_t ProductionCount;
		std::size_t ProductionIndex;
		std::size_t UserMask;
	};

	struct ProcessElement
	{
		std::size_t TableState;
		SymbolInfo Value;
		std::optional<ReduceInfo> Reduce;
		std::any AppendData;

		template<typename Type>
		std::remove_reference_t<Type> Consume() { return std::move(std::any_cast<std::add_lvalue_reference_t<Type>>(AppendData)); }
		std::any Consume() { return std::move(AppendData); }
		template<typename Type>
		std::optional<Type> TryConsume() {
			auto P = std::any_cast<Type>(&AppendData);
			if (P != nullptr)
				return std::move(*P);
			else
				return std::nullopt;
		}
	};

	struct ReduceProduction : public ReduceInfo
	{
		std::span<ProcessElement> Elements;
		ProcessElement& operator[](std::size_t Index) { return Elements[Index]; }
		std::size_t GetElementCount() const { return Elements.size(); }
	};

	struct CoreLRXProcessor
	{

		struct ConsumeResult
		{
			std::size_t State;
			std::size_t RequireNode;
			std::optional<LR0::Reduce> Reduce;
		};

		struct ReduceResult
		{
			LR0::Reduce Reduce;
			std::size_t State;
		};

		struct OperatorT
		{
			virtual std::any HandleReduce(SymbolInfo Value, ReduceProduction Desc) = 0;
			virtual std::optional<ConsumeResult> TableConsume(Symbol Value) = 0;
			virtual std::optional<ReduceResult> TableReduce() = 0;
			virtual void GetSuggestion() = 0;
			virtual void Clear(LRXProcessor& Processor) = 0;
			bool EnableSuggestion = false;
		};

		bool Consume(Symbol Value, Misc::IndexSpan<> TokenIndex, std::any AppendData, OperatorT& Ope);
		bool EndOfFile(OperatorT& Ope);

		std::any& GetDataRaw();

		template<typename RequrieT>
		RequrieT GetData() {
			return std::any_cast<RequrieT>(std::move(GetDataRaw()));
		}

		//void Clear(std::size_t StartupNode);

		void Clear(OperatorT& Ope);

	protected:

		void TryReduce();

		std::deque<ProcessElement> CacheSymbols;
		std::vector<ProcessElement> States;
		std::size_t CurrentTopState = 0;
		std::size_t RequireNode = 0;
	};

	struct LRXOperator : public LRXProcessor::OperatorT
	{

		LRXOperator(LRX const& Table)
			: CoreProcessor(ProcessorContext), Table(Table), EnableSuggest(EnableSuggest)
		{
		}

		void Clear() { CoreProcessor::Clear(Table.StartupOffset()); }

	protected:

		virtual std::optional<CoreProcessor::ConsumeResult> TableConsume(Symbol Value) const override final;
		virtual std::optional<CoreProcessor::ReduceResult> TableReduce() const override final;
		
		virtual void GetSuggest(Symbol Value) const = 0;

		LRX const& Table;
		bool const EnableSuggest;
	};

	struct LRXBinaryTableCoreProcessor : protected CoreProcessor
	{

		using Context = CoreProcessor::Context;

		LRXBinaryTableCoreProcessor(LRXBinaryTableWrapper Table, Context& ProcessorContext, bool EnableSuggest = false)
			: CoreProcessor(ProcessorContext), Table(Table), EnableSuggest(EnableSuggest)
		{}

		void Clear() { CoreProcessor::Clear(Table.StartupNodeIndex()); }

	protected:

		virtual std::optional<CoreProcessor::ConsumeResult> TableConsume(Symbol Value) const override final;
		virtual std::optional<CoreProcessor::ReduceResult> TableReduce() const override final;

		virtual void GetSuggest(Symbol Value) const = 0;

		LRXBinaryTableWrapper Table;
		bool const EnableSuggest;
	};

	struct IgnoreClear {};

	template<typename ProcessorT, typename AppendInfo, typename HandlFunction>
	requires(
		(std::is_same_v<ProcessorT, LRX> || std::is_same_v<ProcessorT, LRXBinaryTableWrapper>)
		&& std::is_invocable_r_v<std::any, HandlFunction, SymbolInfo, AppendInfo>
		&& std::is_invocable_r_v<std::any, HandlFunction, SymbolInfo, ReduceProduction>
	)
	struct FunctionalProcessor : protected std::conditional_t<
		std::is_same_v<ProcessorT, LRX>, LRXCoreProcessor, LRXBinaryTableCoreProcessor
	>
	{
		using BaseT = std::conditional_t<
			std::is_same_v<ProcessorT, LRX>, LRXCoreProcessor, LRXBinaryTableCoreProcessor
		>;

		using TableT = std::conditional_t<
			std::is_same_v<ProcessorT, LRX>, LRX const&, LRXBinaryTableWrapper
		>;

		using Context = CoreProcessor::Context;


		FunctionalProcessor(TableT Table, Context& ProcessorContext,  HandlFunction& Function)
			: BaseT(Table, ProcessorContext, std::is_invocable_v<HandlFunction, Symbol>), Function(Function) {
			BaseT::Clear();
		}

		FunctionalProcessor(IgnoreClear, TableT Table, Context& ProcessorContext, HandlFunction& Function)
			: BaseT(Table, ProcessorContext, std::is_invocable_v<HandlFunction, Symbol>), Function(Function) {
		}

		bool Consume(Symbol Value, Misc::IndexSpan<> TokenIndex, AppendInfo const& Info)
		{
			auto AppendData = Function(SymbolInfo{Value, TokenIndex}, Info);
			return BaseT::Consume(Value, TokenIndex, std::move(AppendData));
		}

		virtual void GetSuggest(Symbol Value) const override { 
			if constexpr (std::is_invocable_v<HandlFunction, Symbol>)
			{
				Function(Value);
			}
		}

		bool EndOfFile() { return BaseT::EndOfFile(); }

		std::any& GetDataRaw() { return BaseT::GetDataRaw(); }

		template<typename RequrieT>
		RequrieT GetData() { return this->BaseT::GetData<RequrieT>(); }

		void Clear() { return BaseT::Clear(); }
	
	protected:

		virtual std::any HandleReduce(SymbolInfo Value, ReduceProduction Productions) override {
			return Function(Value, Productions);
		}

		HandlFunction& Function;

	};

	template<typename AppendInfoT, typename HandleFunction>
	using LRXProcessor = FunctionalProcessor<LRX, AppendInfoT, HandleFunction>;

	template<typename AppendInfoT, typename HandleFunction>
	using LRXBinaryTableProcessor = FunctionalProcessor<LRXBinaryTableWrapper, AppendInfoT, HandleFunction>;


	/*
	template<typename AppendInfo>
	bool LRXProcessor::Comsume(Symbol SymbolValue, Misc::IndexSpan<> TokenIndex, AppendInfo Info, Function&& Func)
	{
		assert(Value.IsTerminal());
		assert(*States.rbegin() == CurrentTopState);

		//auto 
	}
	*/

	/*
	

	struct ElementData
	{
		std::any Data;

		template<typename Type>
		std::remove_reference_t<Type> Consume() { return std::move(std::any_cast<std::add_lvalue_reference_t<Type>>(Data)); }
		std::any Consume() { return std::move(Data); }
		template<typename Type>
		std::optional<Type> TryConsume() {
			auto P = std::any_cast<Type>(&Data);
			if (P != nullptr)
				return std::move(*P);
			else
				return std::nullopt;
		}
	};

	struct TElement
	{
		Symbol Value;
		ParsingStep::ShiftT Shift;
		TElement(ParsingStep const& value);
		TElement(TElement const&) = default;
		TElement& operator=(TElement const&) = default;
	};

	struct PredictElement
	{
		Symbol Value;
		ParsingStep::ReduceT Reduce;
	};

	struct NTElement
	{

		struct MetaData
		{
			Symbol Value;
			Misc::IndexSpan<> TokenIndex;
		};

		struct DataT
		{
			MetaData Mate;
			ElementData Data;

			template<typename Type>
				decltype(auto) Consume() { return Data.Consume<Type>(); }
			
			decltype(auto) Consume() { return Data.Consume();}
			template<typename Type>
				decltype(auto) TryConsume() { return Data.TryConsume<Type>(); }
		};

		Symbol Value;
		ParsingStep::ReduceT Reduce;
		Misc::IndexSpan<> TokenIndex;
		std::span<DataT> Datas;
		DataT& operator[](std::size_t index) { return Datas[index]; }
		NTElement(ParsingStep const& StepValue, Misc::IndexSpan<> TokenIndex, std::span<DataT> Datas);
		NTElement(NTElement const&) = default;
		NTElement& operator=(NTElement const&) = default;
	};

	struct VariantElement
	{
		std::variant<TElement, NTElement, PredictElement> Element;
		bool IsTerminal() const { return std::holds_alternative<TElement>(Element); }
		bool IsNoTerminal() const  { return std::holds_alternative<NTElement>(Element); }
		bool IsPredict() const { return std::holds_alternative<PredictElement>(Element); }
		TElement& AsTerminal() { return std::get<TElement>(Element); }
		NTElement& AsNoTerminal() { return std::get<NTElement>(Element); }
		PredictElement& AsPredict() { return std::get<PredictElement>(Element); }
	};

	struct ParsingStepProcessor
	{

		struct Result
		{
			VariantElement Element;
			std::optional<NTElement::MetaData> MetaData;
		};

		std::optional<Result> Translate(ParsingStep Input);
		void Push(NTElement::MetaData Element, std::any Result);
		std::optional<std::any> EndOfFile();

		ParsingStepProcessor() = default;
		ParsingStepProcessor(ParsingStepProcessor&&) = default;
		ParsingStepProcessor(ParsingStepProcessor const&) = default;

	private:

		std::vector<NTElement::DataT> TemporaryDataBuffer;
		std::vector<NTElement::DataT> DataBuffer;
	};

	template<typename Fun>
	std::optional<std::any> ProcessParsingStep(std::span<ParsingStep const> Input, Fun&& Func) requires(std::is_invocable_r_v<std::any, Fun, VariantElement>)
	{
		ParsingStepProcessor TempProcesser;
		for (auto Ite : Input)
		{
			auto Re = TempProcesser.Translate(Ite);
			if (Re.has_value())
			{
				auto [Ele, Data] = *Re;
				auto Re2 = Func(Ele);
				if(Data.has_value())
					TempProcesser.Push(*Data, std::move(Re2));
			}else
				return {};
		}
		return TempProcesser.EndOfFile();
	}

	template<typename RT, typename Fun>
	RT ProcessParsingStepWithOutputType(std::span<ParsingStep const> Input, Fun&& Func) requires(std::is_invocable_r_v<std::any, Fun, VariantElement>)
	{
		return std::any_cast<RT>(*ProcessParsingStep(Input, Func));
	}


	

	struct SymbolProcessor
	{

		struct CacheSymbol
		{
			Symbol Value;
			std::size_t TokenIndex;
		};

		SymbolProcessor(LRXBinaryTableWrapper Wrapper);
		SymbolProcessor(LRX const& Wrapper);
		bool Consume(Symbol Value, std::size_t TokenIndex, std::vector<Symbol>* SuggestSymbols = nullptr);
		std::span<ParsingStep const> GetSteps() const;
		
		void Clear();
		void ClearSteps();

		bool EndOfFile(std::vector<Symbol>* SuggestSymbols = nullptr) { bool Re =  Consume(Symbol::EndOfFile(), 0, SuggestSymbols); if(Re) PredictRecord.clear(); return Re; }

	private:

		void PushSteps(ParsingStep Steps, bool NeedPredict);

		struct ConsumeResult
		{
			std::size_t State;
			std::size_t RequireNode;
			std::optional<LR0::Reduce> Reduce;
			bool NeedPredict = false;
		};

		std::optional<ConsumeResult> ConsumeImp(LRX const&, Symbol Value, std::vector<Symbol>* SuggestSymbols = nullptr) const;
		std::optional<ConsumeResult> ConsumeImp(LRXBinaryTableWrapper, Symbol Value, std::vector<Symbol>* SuggestSymbols = nullptr) const;

		struct ReduceResult
		{
			LR0::Reduce Reduce;
			std::size_t State;
			bool NeedPredict = false;
		};

		std::optional<ReduceResult> TryReduceImp(LRX const& Table) const;
		std::optional<ReduceResult> TryReduceImp(LRXBinaryTableWrapper Table) const;

		void TryReduce();

		std::deque<CacheSymbol> CacheSymbols;
		std::vector<std::size_t> States;
		std::size_t CurrentTopState;
		std::size_t RequireNode;
		std::vector<ParsingStep> Steps;

		struct PredictRecord
		{
			std::size_t Index;
			bool NeedPredict;
		};

		std::vector<std::size_t> PredictRecord;
		std::variant<LRXBinaryTableWrapper, std::reference_wrapper<LRX const>> Table;
		std::size_t StartupOffset;
	};

	

	struct StepProcessorContextT
	{
		struct TerminalSymbolT
		{
			Symbol SymbolValue;
		};
	};
	*/



	namespace Exception
	{
		struct Interface : public std::exception
		{
			virtual char const* what() const override;
			virtual ~Interface() = default;
		};

		struct WrongProduction : public Interface
		{
			enum class Category
			{
				TerminalSymbolAsProductionBegin,
				MaskNotFolloedNoterminalSymbol
			};

			Category Type;
			ProductionBuilder Builders;

			WrongProduction(Category Type, ProductionBuilder Builders)
				: Type(Type), Builders(std::move(Builders)) {}

			WrongProduction(WrongProduction const&) = default;
			virtual char const* what() const override;
		};

		struct IllegalSLRXProduction : public Interface
		{
			enum class Category
			{
				EndlessReduce,
				MaxForwardDetectNotPassed,
				ConfligReduce,
			};

			Category Type;
			std::size_t MaxForwardDetectNum;
			std::size_t DetectNum;
			std::vector<ParsingStep> Steps1;
			std::vector<ParsingStep> Steps2;
			std::vector<LR0::MappedProduction> EffectProductions;
			
			IllegalSLRXProduction(Category Type, std::size_t MaxForwardDetectNum, std::size_t DetectNum, std::vector<ParsingStep> Steps1, std::vector<ParsingStep> Steps2, std::vector<LR0::MappedProduction> EffectProductions)
				: Type(Type), MaxForwardDetectNum(MaxForwardDetectNum), DetectNum(DetectNum), Steps1(std::move(Steps1)), Steps2(std::move(Steps2)), EffectProductions(std::move(EffectProductions)) {}

			IllegalSLRXProduction(IllegalSLRXProduction const&) = default;
			IllegalSLRXProduction(IllegalSLRXProduction&&) = default;
			virtual char const* what() const override;
		};

		struct BadProduction : public Interface
		{
			enum class Category
			{
				EmptyProduction,
				RedefineProduction,
				UnaccaptableSymbol,
				AmbiguityProduction,
				WrongProductionMask,
				EmptyAcceptableNoTerminal,
				OperatorPriorityConflict,
				NotLR0Production
			};
			Category Type;
			std::size_t ProductionIndex;
			std::size_t ElementIndex;
			BadProduction(Category Type, std::size_t ProductionIndex, std::size_t ElementIndex)
				: Type(Type), ProductionIndex(ProductionIndex), ElementIndex(ElementIndex)
			{

			}
			BadProduction(BadProduction const&) = default;
			virtual char const* what() const override;
		};

		struct OpePriorityConflict : public Interface
		{
			Symbol Value;
			OpePriorityConflict(Symbol Value) : Value(Value) {}
			OpePriorityConflict(OpePriorityConflict const&) = default;
			virtual char const* what() const override;
		};

		struct OutOfRange : public Interface
		{

			enum class TypeT
			{
				NodeCount,
				RequireNodeCount,
				ReduceProperty,
				RequireNodeOffset,

				NodeForwardDetectCount,
				NodeEdgeCount,
				RequireSymbolCount,
				PropertyOffset,
				ToNodeMappingCount,
				ProductionIndex,


				
				NodeOffset,
				ReduceCount,
				ShiftCount,
				
				ProductionElementIndex,
				Mask,
				AcceptableProductionCount,
				AcceptableProductionIndex,
				SymbolValue,
			};
			TypeT Type;
			std::size_t Value;
			OutOfRange(TypeT Type, std::size_t Value) : Type(Type), Value(Value) {};
			OutOfRange(OutOfRange const&) = default;
			virtual char const* what() const override;
		};
	}
}