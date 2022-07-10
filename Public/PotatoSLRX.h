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
		union
		{
			struct ReduceT {
				std::size_t ProductionIndex;
				std::size_t ProductionCount;
				StandardT Mask;
			}Reduce;

			struct ShiftT {
				std::size_t TokenIndex;
			}Shift;
		};
		constexpr bool IsTerminal() const { return Value.IsTerminal(); }
		constexpr bool IsNoTerminal() const { return Value.IsNoTerminal(); }
		constexpr bool IsShift() const { return IsTerminal(); }
		constexpr bool IsReduce() const { return IsNoTerminal(); }
	};

	struct TElement
	{
		Symbol Value;
		std::size_t TokenIndex;
		TElement(ParsingStep const& value);
		TElement(TElement const&) = default;
		TElement& operator=(TElement const&) = default;
	};

	struct NTElement
	{

		struct DataT
		{
			Symbol Value;
			std::size_t FirstTokenIndex;
			std::any Data;

			operator Symbol() const { return Value; }
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


		Symbol Value;
		std::size_t ProductionIndex;
		StandardT Mask;
		std::size_t FirstTokenIndex;
		std::span<DataT> Datas;
		DataT& operator[](std::size_t index) { return Datas[index]; }
		NTElement(ParsingStep const& StepValue, std::size_t FiestTokenIndex, DataT* DataPtr);
		NTElement(NTElement const&) = default;
		NTElement& operator=(NTElement const&) = default;
	};

	struct VariantElement
	{
		std::variant<TElement, NTElement> Element;
		bool IsTerminal() const { return std::holds_alternative<TElement>(Element); }
		bool IsNoTerminal() const  { return std::holds_alternative<NTElement>(Element); }
		TElement& AsTerminal() { return std::get<TElement>(Element); }
		NTElement& AsNoTerminal() { return std::get<NTElement>(Element); }
	};

	struct ParsingStepProcessor
	{
		bool Consume(ParsingStep Input);
		std::optional<std::any> EndOfFile();

		ParsingStepProcessor(std::function<std::any(VariantElement)> Func)
			: ExecuteFunction(std::move(Func)) {};
	private:
		std::function<std::any(VariantElement)> ExecuteFunction;
		std::vector<NTElement::DataT> DataBuffer;
	};

	template<typename Fun>
	std::optional<std::any> ProcessParsingStep(std::span<ParsingStep const> Input, Fun&& Func) requires(std::is_invocable_r_v<std::any, Fun, VariantElement>)
	{
		ParsingStepProcessor TempProcesser(std::forward<Fun>(Func));
		for (auto Ite : Input)
		{
			if(!TempProcesser.Consume(Ite))
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
		ProductionBuilder(Symbol ProductionValue, std::vector<ProductionBuilderElement> ProductionElement, StandardT ProductionMask)
			: ProductionValue(ProductionValue), Element(std::move(ProductionElement)), ProductionMask(ProductionMask) {}
		ProductionBuilder(Symbol ProductionValue, StandardT ProductionMask) : ProductionBuilder(ProductionValue, {}, ProductionMask) {}
		ProductionBuilder(Symbol ProductionValue, std::vector<ProductionBuilderElement> ProductionElement)
			: ProductionBuilder(ProductionValue, std::move(ProductionElement), 0) {}
		ProductionBuilder(Symbol ProductionValue)
			: ProductionBuilder(ProductionValue, {}, 0) {}

		ProductionBuilder(const ProductionBuilder&) = default;
		ProductionBuilder(ProductionBuilder&&) = default;
		ProductionBuilder& operator=(const ProductionBuilder&) = default;
		ProductionBuilder& operator=(ProductionBuilder&&) = default;

		Symbol ProductionValue;
		std::vector<ProductionBuilderElement> Element;
		StandardT ProductionMask = 0;
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
	};

	struct LR0Table
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
			std::size_t ProductionIndex;
			std::size_t ProductionElementCount;
			StandardT ReduceMask;
			operator ParsingStep () const {
				ParsingStep New;
				New.Value = ReduceSymbol;
				New.Reduce.ProductionCount = ProductionElementCount;
				New.Reduce.Mask = ReduceMask;
				New.Reduce.ProductionIndex = ProductionIndex;
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

		LR0Table(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
			: LR0Table(ProductionInfo{StartSymbol, std::move(Production), std::move(Priority)}) {}
		LR0Table(ProductionInfo Info);
		LR0Table(LR0Table&&) = default;
		LR0Table(LR0Table const&) = default;
	};

	struct SLRXTable
	{
		struct Shift
		{
			std::vector<std::vector<Symbol>> RequireSymbolss;
			std::size_t ToNode;
		};

		struct ReduceTuple
		{
			std::size_t LastState;
			std::size_t TargetState;
		};

		struct Reduce
		{
			std::vector<std::vector<Symbol>> RequireSymbolss;
			std::vector<ReduceTuple> ReduceShifts;
			LR0Table::Reduce Property;
		};

		struct Node
		{
			std::vector<Shift> Shifts;
			std::vector<Reduce> Reduces;
			std::vector<LR0Table::MappedProduction> MappedProduction;
			std::size_t MaxForwardDetectCount = 0;
		};

		std::vector<Node> Nodes;

		SLRXTable(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect = 3)
			: SLRXTable(LR0Table{StartSymbol, std::move(Production), std::move(Priority)}, MaxForwardDetect) {}
		SLRXTable(LR0Table const& Table, std::size_t MaxForwardDetect = 3);
		SLRXTable(SLRXTable const&) = default;
		SLRXTable(SLRXTable&&) = default;
		SLRXTable& operator=(SLRXTable const&) = default;
		SLRXTable& operator=(SLRXTable&&) = default;
	};

	struct TableWrapper
	{

		using StandardT = SLRX::StandardT;

		using HalfStandardT = std::uint16_t;
		using HalfHalfStandardT = std::uint8_t;

		static_assert(sizeof(HalfStandardT) * 2 == sizeof(StandardT));
		static_assert(sizeof(HalfHalfStandardT) * 2 == sizeof(HalfStandardT));

		struct alignas(alignof(StandardT)) NodeStorageT
		{
			HalfStandardT ForwardDetectedCount;
			HalfStandardT EdgeCount;
		};

		struct alignas(alignof(StandardT)) RequireSymbolStorageT
		{
			HalfStandardT IncludeEndOfFile : 1;
			HalfStandardT IsShift : 1;
			HalfStandardT TerminalSymbolRequireCount : 14;
			HalfStandardT PropertyOffset;

			static_assert(sizeof(HalfStandardT) == sizeof(std::uint16_t));
		};

		struct alignas(alignof(StandardT)) ShiftPropertyT
		{
			StandardT ToNode;
			HalfStandardT ForwardDetectCount;
		};

		struct alignas(alignof(StandardT)) ReducePropertyT
		{
			HalfStandardT IsStartSymbol : 1;
			HalfStandardT ToNodeMappingCount : 15;
			HalfStandardT ProductionIndex;
			HalfStandardT ProductionCount;
			StandardT Mask;
			StandardT NoTerminalValue;

			static_assert(sizeof(HalfStandardT) == sizeof(std::uint16_t));
		};

		struct alignas(alignof(StandardT)) ToNodeMappingT
		{
			StandardT LastState;
			StandardT ToState;
			HalfStandardT ForwardDetectCount;
		};

		static std::vector<StandardT> Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority){ 
			return Create(SLRXTable{ StartSymbol, std::move(Production), std::move(Priority) });
		}
		static std::vector<StandardT> Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetect) {
			return Create(SLRXTable{ StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetect });
		}
		static std::vector<StandardT> Create(SLRXTable const& Table);

		TableWrapper(std::span<StandardT const> Wrapper) : Wrapper(Wrapper) {}
		TableWrapper(TableWrapper const&) = default;
		TableWrapper() = default;

		struct ReducePropertyWrapperT
		{
			ReducePropertyT Storage;
			std::span<ToNodeMappingT const> Mapping;
		};

		struct RequireSymbolWrapperT
		{
			RequireSymbolStorageT Storage;
			std::size_t Last = 0;
			std::span<StandardT const> Symbols;
			std::span<StandardT const> LastSpan;

			std::optional<RequireSymbolWrapperT> Next() const;
			ShiftPropertyT ReadShiftProperty() const;
			ReducePropertyWrapperT ReadReduceProperty() const;
		};

		struct NodeWrapperT
		{
			NodeStorageT Storage;
			std::span<StandardT const> LastSpan;
			std::optional<RequireSymbolWrapperT> Iterate() const;
		};

		NodeWrapperT ReadNode(std::size_t Offset) const;

		StandardT NodeCount() const { return Wrapper.empty() ? 0 : Wrapper[0]; }
		static constexpr StandardT FirstNodeOffset() noexcept { return 2; }
		StandardT StartupNode() const { return Wrapper[1]; }
		StandardT StartupNodeForwardDetect() const { return Wrapper[2]; }
		operator bool () const { return !Wrapper.empty() && Wrapper.size() >= 3; }

	private:

		std::span<StandardT const> Wrapper;
		friend struct CoreProcessor;
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
		Table(SLRX::SLRXTable const& Table)
		{
			Datas = TableWrapper::Create(Table);
			Wrapper = TableWrapper(Datas);
		}
		Table(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, std::size_t MaxForwardDetected)
			: Table(SLRX::SLRXTable{ StartSymbol, std::move(Production), std::move(Priority), MaxForwardDetected })
		{
			
		}
		Table(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
			: Table(SLRX::SLRXTable{ StartSymbol, std::move(Production), std::move(Priority) })
		{
		}
		std::size_t NodeCount() const { return Wrapper.NodeCount(); }
		operator TableWrapper() const { return Wrapper; }
		std::vector<TableWrapper::StandardT> Datas;
		TableWrapper Wrapper;
	};

	struct CoreProcessor
	{
		
		CoreProcessor(TableWrapper Wrapper) : Wrapper(Wrapper)
		{
			assert(Wrapper);
			States.push_back(Wrapper.StartupNode());
			MaxForwardDetect = Wrapper.StartupNodeForwardDetect();
		}

		std::size_t RequireTokenIndex() const { return TokenIteratorIndex; }

		void Consume(Symbol Symbol, std::size_t TokenIndex);
		void Consume(Symbol Symbol) { Consume(Symbol, TokenIteratorIndex); }
		std::vector<ParsingStep> EndOfFile();

	private:

		bool TryReduce();

		std::vector<std::size_t> States;
		std::size_t MaxForwardDetect;
		std::size_t TokenIteratorIndex = 0;

		struct CacheSymbol
		{
			std::size_t TokenIteratorIndex; 
			std::size_t TokenIndex;
			StandardT Value;
		};

		std::deque<CacheSymbol> CacheSymbols;
		std::vector<ParsingStep> Steps;

		static bool DetectSymbolEqual(std::deque<CacheSymbol> const& T1, std::span<StandardT const> T2);

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
			std::vector<LR0Table::MappedProduction> EffectProductions;
			
			IllegalSLRXProduction(Category Type, std::size_t MaxForwardDetectNum, std::size_t DetectNum, std::vector<ParsingStep> Steps1, std::vector<ParsingStep> Steps2, std::vector<LR0Table::MappedProduction> EffectProductions)
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
				NodeForwardDetectCount,
				NodeEdgeCount,
				RequireSymbolCount,
				PropertyOffset,
				ToNodeMappingCount,
				ProductionIndex,


				NodeCount,
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