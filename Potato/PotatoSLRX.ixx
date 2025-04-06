export module PotatoSLRX;

import std;
import PotatoMisc;

import PotatoPointer;

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

		constexpr std::strong_ordering operator<=>(Symbol const&) const = default;
		constexpr static Symbol EndOfFile() { return { T::ENDOFFILE, 0 }; }
		constexpr static Symbol StartSymbol() { return { T::STARTSYMBOL, 0 }; }
		constexpr static Symbol AsNoTerminal(std::size_t symbol) { return { T::NOTERMIAL, symbol };  }
		constexpr static Symbol AsTerminal(std::size_t symbol) { return { T::TERMINAL, symbol }; }
		constexpr bool IsTerminal() const { return type == T::TERMINAL || IsEndOfFile(); }
		constexpr bool IsNoTerminal() const { return type == T::NOTERMIAL || IsStartSymbol(); }
		constexpr bool IsEndOfFile() const { return type == T::ENDOFFILE; }
		constexpr bool IsStartSymbol() const { return type == T::STARTSYMBOL; }

		T type = T::ENDOFFILE;
		std::size_t symbol = 0;
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
		bool IsA() const {
			auto P = std::any_cast<Type const>(&AppendData);
			return P != nullptr;
		}
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
		std::size_t Size() const { return Elements.size(); }
	};

	struct ProcessorOperator
	{
		bool EnableSuggestSymbol = false;
		virtual std::any HandleReduce(SymbolInfo Value, ReduceProduction Desc) = 0;
		virtual void GetSuggestSymbol(Symbol Value) {};
	};

	struct TableConsumeResult
	{
		std::size_t State;
		std::size_t RequireNode;
		std::optional<LR0::Reduce> Reduce;
	};

	struct TableReduceResult
	{
		LR0::Reduce Reduce;
		std::size_t State;
	};

	struct TableClearInfo
	{
		std::size_t StartupTokenIndex;
	};

	export struct LRXProcessor;

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

		static constexpr std::size_t StartupNodeIndex() { return 1; }

	private:

		std::optional<TableConsumeResult> TableConsume(Symbol Value, LRXProcessor const& Info, ProcessorOperator& Ope) const;
		std::optional<TableReduceResult> TableReduce(LRXProcessor const& Info) const;

		LRX(std::vector<Node> Nodes) : Nodes(std::move(Nodes)) {}

		friend struct LRXProcessor;
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

		std::optional<TableConsumeResult> TableConsume(Symbol Value, LRXProcessor const& Info, ProcessorOperator& Ope) const;
		std::optional<TableReduceResult> TableReduce(LRXProcessor const& Info) const;

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


	export struct LRXProcessor
	{

		bool Consume(Symbol Value, Misc::IndexSpan<> TokenIndex, std::any AppendInfo);
		bool EndOfFile();

		void SetObserverTable(LRX const& Table, Pointer::ObserverPtr<ProcessorOperator> Ope);

		void SetObserverTable(LRXBinaryTableWrapper Table, Pointer::ObserverPtr<ProcessorOperator> Ope);

		std::any& GetDataRaw();

		template<typename RequrieT>
		RequrieT GetData() {
			return std::any_cast<RequrieT>(std::move(GetDataRaw()));
		}

		void Clear();

		LRXProcessor(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: CacheSymbols(resource), States(resource) {}

	protected:

		std::variant<
			std::monostate,
			std::reference_wrapper<LRX const>,
			LRXBinaryTableWrapper
		> TableWrapper;

		Pointer::ObserverPtr<ProcessorOperator> Operator;

		void TryReduce();

		std::pmr::deque<ProcessElement> CacheSymbols;
		std::pmr::vector<ProcessElement> States;
		std::size_t CurrentTopState = 0;
		std::size_t RequireNode = 0;
		friend struct LRX;
		friend struct LRXBinaryTableWrapper;
	};

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