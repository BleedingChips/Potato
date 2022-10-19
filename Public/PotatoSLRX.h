#pragma once
#include "PotatoMisc.h"
#include <compare>
#include <optional>
#include <functional>
#include <variant> 
#include <map>
#include <array>
#include <exception>
#include <deque>

namespace Potato::SLRX
{

	using StandardT = uint32_t;

	struct Symbol
	{
		enum class ValueType : StandardT
		{
			TERMINAL = 0,
			NOTERMIAL,
			STARTSYMBOL,
			ENDOFFILE,
		};

		constexpr std::strong_ordering operator<=>(Symbol const& Input) const = default;
		constexpr static Symbol EndOfFile() { return { ValueType::ENDOFFILE, 0 }; }
		constexpr static Symbol StartSymbol() { return { ValueType::STARTSYMBOL, 0 }; }
		constexpr static Symbol AsNoTerminal(StandardT Value) { return { ValueType::NOTERMIAL, Value};  }
		constexpr static Symbol AsTerminal(StandardT Value) { return { ValueType::TERMINAL, Value }; }
		constexpr bool IsTerminal() const { return Type == ValueType::TERMINAL || IsEndOfFile(); }
		constexpr bool IsNoTerminal() const { return Type == ValueType::NOTERMIAL || IsStartSymbol(); }
		constexpr bool IsEndOfFile() const { return Type == ValueType::ENDOFFILE; }
		constexpr bool IsStartSymbol() const { return Type == ValueType::STARTSYMBOL; }

		ValueType Type = ValueType::ENDOFFILE;
		StandardT Value = 0;
	};

	struct ParsingStep
	{
		Symbol Value;
		bool IsPredict = false;

		struct ReduceT
		{
			std::size_t ProductionIndex =0 ;
			std::size_t ElementCount = 0;
			SLRX::StandardT Mask = 0;	
		};

		struct ShiftT
		{
			std::size_t TokenIndex = 0;
		};

		ReduceT Reduce;
		ShiftT Shift;

		constexpr bool IsTerminal() const { return Value.IsTerminal(); }
		constexpr bool IsNoTerminal() const { return !IsPredict && Value.IsNoTerminal(); }
		constexpr bool IsPredictNoTerminal() const { return IsPredict && Value.IsNoTerminal(); }
		constexpr bool IsShift() const { return IsTerminal(); }
		constexpr bool IsReduce() const { return IsNoTerminal(); }
	};

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


	enum class Associativity
	{
		Left,
		Right,
	};

	struct OpePriority
	{
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
		constexpr ProductionBuilderElement(StandardT Mask) : Type(TypeT::IsMask), ProductionMask(Mask) {};
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
		ProductionBuilder(Symbol ProductionValue, std::vector<ProductionBuilderElement> ProductionElement, StandardT ProductionMask = 0, bool NeedPredict = false)
			: ProductionValue(ProductionValue), Element(std::move(ProductionElement)), ProductionMask(ProductionMask), NeedPredict(NeedPredict) {}
		
		/*
		ProductionBuilder(Symbol ProductionValue, StandardT ProductionMask) : ProductionBuilder(ProductionValue, {}, ProductionMask) {}
		ProductionBuilder(Symbol ProductionValue, std::vector<ProductionBuilderElement> ProductionElement)
			: ProductionBuilder(ProductionValue, std::move(ProductionElement), 0) {}
		ProductionBuilder(Symbol ProductionValue)
			: ProductionBuilder(ProductionValue, {}, 0) {}
		*/

		ProductionBuilder(const ProductionBuilder&) = default;
		ProductionBuilder(ProductionBuilder&&) = default;
		ProductionBuilder& operator=(const ProductionBuilder&) = default;
		ProductionBuilder& operator=(ProductionBuilder&&) = default;

		Symbol ProductionValue;
		std::vector<ProductionBuilderElement> Element;
		StandardT ProductionMask = 0;
		bool NeedPredict = false;
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
			StandardT ProductionMask;
			std::vector<Element> Elements;
			bool NeedPredict = false;
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
		static std::tuple<std::size_t, std::size_t> IntersectionSet(std::span<std::size_t> Source1AndOutout,  std::span<std::size_t> Source2);

		std::vector<Production> ProductionDescs;
		std::map<Symbol, std::vector<std::size_t>> TrackedAcceptableNoTerminalSymbol;
		bool NeedPredict = false;
	};

	struct LR0
	{
		struct ShiftEdge
		{
			Symbol RequireSymbol;
			std::size_t ToNode;
			bool ReverseStorage;
			std::vector<std::size_t> ProductionIndex;
			bool NeedPredict = false;
		};

		struct Reduce
		{
			Symbol ReduceSymbol;
			ParsingStep::ReduceT Reduce;
			bool NeedPredict = false;
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
			bool NeedPredict = false;
		};

		std::vector<Node> Nodes;

		LR0(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
			: LR0(ProductionInfo{StartSymbol, std::move(Production), std::move(Priority)}) {}
		LR0(ProductionInfo Info);
		LR0(LR0&&) = default;
		LR0(LR0 const&) = default;
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

		enum class RequireNodeType : StandardT
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
		bool IsStarupNeedPredict = false;

		LRX(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect = 3)
			: LRX(LR0{StartSymbol, std::move(Production), std::move(Priority)}, MaxForwardDetect) {}
		LRX(LR0 const& Table, std::size_t MaxForwardDetect = 3);
		LRX(LRX const&) = default;
		LRX(LRX&&) = default;
		LRX& operator=(LRX const&) = default;
		LRX& operator=(LRX&&) = default;

		static constexpr std::size_t StartupOffset() { return 1; }
		bool StartupNeedPredict() const { return IsStarupNeedPredict; }
	};

	struct TableWrapper
	{
		using StandardT = SLRX::StandardT;

		using HalfStandardT = std::uint16_t;
		using HalfHalfStandardT = std::uint8_t;

		static_assert(sizeof(HalfStandardT) * 2 == sizeof(StandardT));
		static_assert(sizeof(HalfHalfStandardT) * 2 == sizeof(HalfStandardT));

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
			HalfStandardT NeedPredict;
			StandardT Mask;
			StandardT NoTerminalValue;

			static_assert(sizeof(HalfStandardT) == sizeof(std::uint16_t));
		};

		struct alignas(alignof(StandardT)) ZipReduceTupleT
		{
			StandardT LastState;
			StandardT ToState;
			StandardT NeedPredict;
		};

		static std::size_t CalculateRequireSpace(LRX const& Ref);
		static std::size_t SerilizeTo(std::span<StandardT> OutputBuffer, LRX const& Ref);
		static std::vector<StandardT> Create(LRX const& Le);
		static std::vector<StandardT> Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect = 3) {
			return Create(LRX{ StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetect });
		}

		TableWrapper(std::span<StandardT const> InputBuffer) : Buffer(InputBuffer) {}
		TableWrapper(TableWrapper const&) = default;
		TableWrapper() = default;
		TableWrapper& operator=(TableWrapper const&) = default;

		Misc::SerilizerHelper::SpanReader<StandardT const> GetSpanReader(std::size_t Offset = 0){ return Misc::SerilizerHelper::SpanReader<StandardT const>{Buffer}.SubSpan(Offset); }

		operator bool() const { return !Buffer.empty(); }
		std::size_t NodeCount() const { return Buffer[0]; }
		std::size_t StartupNodeIndex() const { return Buffer[1]; }
		bool StartupNeedPredict() const { return Buffer[2]; }
		std::size_t TotalBufferSize() const { return Buffer.size(); }

		std::span<StandardT const> Buffer;

	};

	struct SymbolProcessor
	{

		struct CacheSymbol
		{
			Symbol Value;
			std::size_t TokenIndex;
		};

		SymbolProcessor(TableWrapper Wrapper);
		SymbolProcessor(LRX const& Wrapper);
		bool Consume(Symbol Value, std::size_t TokenIndex, std::vector<Symbol>* SuggestSymbols = nullptr);
		std::span<ParsingStep const> GetSteps() const;
		
		void Clear();
		void ClearSteps();

		static std::optional<std::vector<ParsingStep>> InsertPredictStep(std::span<ParsingStep const> Steps);

		std::optional<std::vector<ParsingStep>> InsertPredictStep(){ return InsertPredictStep(std::span(Steps)); }
		bool EndOfFile(std::vector<Symbol>* SuggestSymbols = nullptr) { return Consume(Symbol::EndOfFile(), 0, SuggestSymbols); }

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
		std::optional<ConsumeResult> ConsumeImp(TableWrapper , Symbol Value, std::vector<Symbol>* SuggestSymbols = nullptr) const;

		struct ReduceResult
		{
			LR0::Reduce Reduce;
			std::size_t State;
			bool NeedPredict = false;
		};

		std::optional<ReduceResult> TryReduceImp(LRX const& Table) const;
		std::optional<ReduceResult> TryReduceImp(TableWrapper Table) const;

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
		std::variant<TableWrapper, std::reference_wrapper<LRX const>> Table;
		std::size_t StartupOffset;
		bool StartupNeedPredict;
	};

	struct Table
	{
		Table(Table const& Input) : Datas(Input.Datas) { Wrapper = TableWrapper{ Datas }; }
		Table(Table&& Input) : Datas(std::move(Input.Datas)) { Wrapper = TableWrapper{ Datas }; Input.Wrapper = {}; }
		Table& operator=(Table const& Input) {
			Datas = Input.Datas;
			Wrapper = TableWrapper{ Datas };
			return *this;
		}
		Table& operator=(Table&& Input) noexcept {
			Datas = std::move(Input.Datas);
			Wrapper = TableWrapper{ Datas };
			return *this;
		}
		Table() = default;
		Table(SLRX::LRX const& Table)
		{
			Datas = TableWrapper::Create(Table);
			Wrapper = TableWrapper(Datas);
		}
		Table(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetected)
			: Table(SLRX::LRX{ StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetected })
		{
			
		}
		Table(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
			: Table(SLRX::LRX{ StartSymbol, std::move(Production), std::move(Priority) })
		{
		}
		std::size_t NodeCount() const { return Wrapper.NodeCount(); }
		operator TableWrapper() const { return Wrapper; }
		std::vector<TableWrapper::StandardT> Datas;
		TableWrapper Wrapper;
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
			};
			TypeT Type;
			std::size_t Value;
			OutOfRange(TypeT Type, std::size_t Value) : Type(Type), Value(Value) {};
			OutOfRange(OutOfRange const&) = default;
			virtual char const* what() const override;
		};

		struct UnaccableSymbol : public Interface
		{
			Symbol LastSymbol;
			std::size_t TokenIteratorIndex;
			std::size_t TokenIndex;

			struct Tuple
			{
				Symbol Value;
				std::size_t TokenIndex;
				std::strong_ordering operator<=>(Tuple const&) const = default;
			};

			std::vector<Tuple> RequireSymbols;

			UnaccableSymbol(Symbol LastSymbol, std::size_t TokenIteratorIndex, std::size_t TokenIndex, std::vector<Tuple> RequireSymbols)
				: LastSymbol(LastSymbol), TokenIteratorIndex(TokenIteratorIndex), TokenIndex(TokenIndex), RequireSymbols(std::move(RequireSymbols))
			{

			}
			UnaccableSymbol(UnaccableSymbol const&) = default;
			virtual char const* what() const override;
		};

		struct MaxAmbiguityProcesser : public Interface
		{
			Symbol InputSymbol;
			std::size_t InputIndex;
			std::size_t MaxProcesserNumber;

			MaxAmbiguityProcesser(Symbol InputSymbol, std::size_t InputIndex, std::size_t MaxProcesserNumber)
				: InputSymbol(InputSymbol), InputIndex(InputIndex), MaxProcesserNumber(MaxProcesserNumber)
			{

			}
			MaxAmbiguityProcesser(MaxAmbiguityProcesser const&) = default;
			virtual char const* what() const override;
		};

		struct UnsupportedStep : public Interface
		{
			Symbol Value;
			std::size_t TokenIndex;
			UnsupportedStep(Symbol Value, std::size_t TokenIndex) : Value(Value), TokenIndex (TokenIndex) {}
			UnsupportedStep(UnsupportedStep const&) = default;
			virtual char const* what() const override;
		};
	}
}