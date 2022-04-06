#pragma once
#include "../Include/PotatoMisc.h"
#include <compare>
#include <optional>
#include <functional>
#include <variant> 
#include <map>
#include <array>
#include <exception>

namespace Potato::DLr
{

	struct Symbol
	{
		enum class ValueType
		{
			TERMINAL = 0,
			NOTERMIAL,
			STARTSYMBOL,
			ENDOFFILE,
		};

		constexpr std::strong_ordering operator<=>(Symbol const& Input) const = default;
		constexpr static Symbol EndOfFile() { return { ValueType::ENDOFFILE, 0 }; }
		constexpr static Symbol StartSymbol() { return { ValueType::STARTSYMBOL, 0 }; }
		constexpr static Symbol AsNoTerminal(std::size_t Value) { return { ValueType::NOTERMIAL, Value};  }
		constexpr static Symbol AsTerminal(std::size_t Value) { return { ValueType::TERMINAL, Value }; }
		constexpr bool IsTerminal() const { return Type == ValueType::TERMINAL || IsEndOfFile(); }
		constexpr bool IsNoTerminal() const { return Type == ValueType::NOTERMIAL || IsStartSymbol(); }
		constexpr bool IsEndOfFile() const { return Type == ValueType::ENDOFFILE; }
		constexpr bool IsStartSymbol() const { return Type == ValueType::STARTSYMBOL; }

		ValueType Type = ValueType::ENDOFFILE;
		std::size_t Value = 0;

	};

	struct Step
	{
		Symbol Value;
		union
		{
			struct ReduceT {
				std::size_t ProductionIndex;
				std::size_t ProductionCount;
				std::size_t Mask;
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
		TElement(Step const& value);
		TElement(TElement const&) = default;
		TElement& operator=(TElement const&) = default;
	};

	struct NTElementData
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

	struct NTElement
	{
		Symbol Value;
		std::size_t ProductionIndex;
		std::size_t Mask;
		std::size_t FirstTokenIndex;
		std::span<NTElementData> Datas;
		NTElementData& operator[](std::size_t index) { return Datas[index]; }
		NTElement(Step const& StepValue, std::size_t FiestTokenIndex, NTElementData* DataPtr);
		NTElement(NTElement const&) = default;
		NTElement& operator=(NTElement const&) = default;
	};

	struct StepElement
	{
		std::variant<TElement, NTElement> Element;
		bool IsTerminal() const { return std::holds_alternative<TElement>(Element); }
		bool IsNoTerminal() const  { return std::holds_alternative<NTElement>(Element); }
		TElement& AsTerminal() { return std::get<TElement>(Element); }
		NTElement& AsNoTerminal() { return std::get<NTElement>(Element); }
	};

	struct StepProcessor
	{

		std::size_t Consume(Step Input, std::any (*)(StepElement, void* Data), void* Data);
		template<typename FunT>
		std::size_t Consume(Step Input, FunT&& Func)
			requires(std::is_invocable_r_v<std::any, FunT, StepElement>)
		{
			return Consume(Input, [](StepElement Steps, void* Data) -> std::any {
				return (*reinterpret_cast<std::remove_reference_t<FunT>*>(Data))(std::move(Steps));
			}, &Func);
		}

		std::any EndOfFile();
	private:
		std::size_t LastTokenIndex = 0;
		std::vector<NTElementData> DataBuffer;
	};

	template<typename Fun>
	std::any ProcessStep(std::span<Step const> Input, Fun&& Func) requires(std::is_invocable_r_v<std::any, Fun, StepElement>)
	{
		StepProcessor TempProcesser;
		for (auto Ite : Input)
		{
			TempProcesser.Consume(Ite, Func);
		}
		return TempProcesser.EndOfFile();
	}

	template<typename RT, typename Fun>
	RT ProcessStepWithOutputType(std::span<Step const> Input, Fun&& Func) requires(std::is_invocable_r_v<std::any, Fun, StepElement>)
	{
		return std::any_cast<RT>(ProcessStep(Input, Func));
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

	struct ProductionBuilderElement
	{
		constexpr ProductionBuilderElement(ProductionBuilderElement const&) = default;
		constexpr ProductionBuilderElement& operator=(ProductionBuilderElement const&) = default;
		constexpr ProductionBuilderElement(Symbol Value) : ProductionValue(Value) {};
		constexpr ProductionBuilderElement(std::size_t Mask) : IsMask(true), ProductionMask(Mask) {}

		bool IsMask = false;
		Symbol ProductionValue = Symbol::EndOfFile();
		std::size_t ProductionMask = 0;
	};

	struct ProductionBuilder
	{

		ProductionBuilder(Symbol ProductionValue, std::vector<ProductionBuilderElement> ProductionElement, std::size_t ProductionMask)
			: ProductionValue(ProductionValue), Element(std::move(ProductionElement)), ProductionMask(ProductionMask) {}
		ProductionBuilder(Symbol ProductionValue, std::size_t ProductionMask) : ProductionBuilder(ProductionValue, {}, ProductionMask) {}
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
		static std::tuple<std::size_t, std::size_t> IntersectionSet(std::span<std::size_t> Source1AndOutout,  std::span<std::size_t> Source2);

		std::vector<Production> ProductionDescs;
		std::map<Symbol, std::vector<std::size_t>> TrackedAcceptableNoTerminalSymbol;
	};

	struct UnserilizeTable
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
			std::size_t ReduceMask;
		};

		struct Node
		{
			std::vector<ShiftEdge> Shifts;
			std::vector<Reduce> Reduces;
		};

		std::vector<Node> Nodes;

		UnserilizeTable(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, bool ForceLr0 = false);
		UnserilizeTable(UnserilizeTable&&) = default;
		UnserilizeTable(UnserilizeTable const&) = default;
	};

	
	struct TableWrapper
	{

		using SerilizedT = std::uint32_t;
		using HalfSeilizeT = std::uint16_t;
		using HalfHalfSeilizeT = std::uint8_t;

		enum class SymbolType : SerilizedT
		{
			Terminal = 0,
			NoTerminal,
			EndOfFile,
			StartSymbol,
		};

		struct SerilizeSymbol
		{
			SymbolType Type : 2;
			SerilizedT Value : 30;
			std::strong_ordering operator<=>(SerilizeSymbol const& Sym) const = default;
			bool operator==(Symbol Sym) const { return static_cast<std::size_t>(Type) == static_cast<std::size_t>(Sym.Type) && Value == Sym.Value; }
		};

		static std::vector<SerilizedT> Create(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority, bool ForceLR0 = false);
		static std::vector<SerilizedT> Create(UnserilizeTable const& Table);
		static SerilizeSymbol TranslateSymbol(Symbol Input);
		static Symbol TranslateSymbol(SerilizeSymbol Input);
		TableWrapper(std::span<SerilizedT const> InWrapper) : Wrapper(InWrapper) {}
		TableWrapper(TableWrapper const&) = default;
		TableWrapper() = default;
		static constexpr std::size_t MaxData() { return std::numeric_limits<SerilizedT>::max(); }
		static constexpr std::size_t MaxSymbolValue() { return (1L << 30) - 1; }
		TableWrapper& operator=(TableWrapper const&) = default;
		std::span<SerilizedT const> Wrapper;

		struct alignas(alignof(SerilizedT)) SerlizedNode
		{
			HalfSeilizeT ReduceCount;
			HalfSeilizeT ShiftCount;
		};

		struct alignas(alignof(SerilizedT)) SerlizedShift
		{
			SerilizeSymbol Symbol;
			SerilizedT ToNodeOffset;
			HalfSeilizeT ReverseStorage : 1;
			HalfSeilizeT AcceptableProductionCount : 15;
		};

		struct alignas(alignof(SerilizedT)) SerlizedReduce
		{
			SerilizeSymbol Symbol;
			SerilizedT ProductionIndex;
			HalfSeilizeT ProductionElementCount;
			HalfSeilizeT Mask;
		};

		struct ShiftViewer
		{
			SerilizeSymbol ShiftSymbol;
			SerilizedT NodeOffset = 0;
			bool ReserverMeaning = false;
			std::span<HalfSeilizeT const> AcceptableProduction;
		};

		struct NodeViewer
		{
			SerilizedT NodeOffset;
			HalfSeilizeT ShiftCount = 0;
			std::span<SerlizedReduce const> Reduces;
			std::span<SerilizedT const> ShiftDatas;
			std::optional<SerilizedT> Acceptable(Symbol ShiftValue, std::size_t ProductionIndex) const;
		};

		SerilizedT NodeCount() const {  return Wrapper.empty() ? 0 : Wrapper[0]; }
		static constexpr SerilizedT FirstNodeOffset() noexcept {return 2;}
		SerilizedT StartupNode() const { return Wrapper[1]; }
		NodeViewer const ReadNode(SerilizedT Offset) const { return ReadNodeFromSpan(Wrapper.subspan(Offset)); }
		NodeViewer const operator[](SerilizedT Offset) const { return ReadNode(Offset); }
		NodeViewer ReadNodeFromSpan(std::span<SerilizedT const> Input) const;
		static std::tuple<ShiftViewer, std::span<SerilizedT const>> ReadShiftFromSpan(std::span<SerilizedT const> Input);
	};

	struct Table
	{
		Table(Table const& Input) : Datas(Input.Datas) { Wrapper = TableWrapper{ Datas }; }
		Table(Table&& Input) : Datas(std::move(Input.Datas)) { Wrapper = TableWrapper{ Datas }; Input = {}; }
		Table& operator=(Table const& Input) {
			Datas = Input.Datas;
			Wrapper = TableWrapper{ Datas };
			return *this;
		}
		Table& operator=(Table&& Input) noexcept {
			Datas = std::move(Input.Datas);
			Wrapper = TableWrapper{ Datas };
			Input = {};
			return *this;
		}
		Table() = default;
		Table(Symbol StartSymbol, std::vector<ProductionBuilder> Production, std::vector<OpePriority> Priority)
		{
			Datas = TableWrapper::Create(StartSymbol, std::move(Production), std::move(Priority));
			Wrapper = TableWrapper(Datas);
		}
		std::size_t NodeCount() const { return Wrapper.NodeCount(); }
		operator TableWrapper() const { return Wrapper; }
		std::optional<TableWrapper::NodeViewer const> ReadNode(TableWrapper::SerilizedT Index) const { return Wrapper.ReadNode(Index); }
		std::optional<TableWrapper::NodeViewer const> operator[](TableWrapper::SerilizedT Index) const { return ReadNode(Index); }
		std::vector<TableWrapper::SerilizedT> Datas;
		TableWrapper Wrapper;
	};

	struct CoreProcessor
	{

		struct AmbiguousPoint
		{
			std::size_t StateIndex;
			std::size_t MaxState;
			std::size_t StepsIndex;
			std::size_t TokenIndex;
			Step Step;
		};

		CoreProcessor(TableWrapper Wrapper) : Wrapper(Wrapper) {};
		CoreProcessor(CoreProcessor&&) = default;
		CoreProcessor(CoreProcessor&) = default;
		CoreProcessor& operator=(CoreProcessor&&) = default;
		CoreProcessor& operator=(CoreProcessor const&) = default;

		std::vector<Step> Steps;
		std::vector<AmbiguousPoint> AmbiguousPoints;
		std::vector<TableWrapper::SerilizedT> States;
		TableWrapper Wrapper;

		operator bool() const { return States.size(); }
		void Clear() {
			States.clear();
			AmbiguousPoints.clear();
			Steps.clear();
		}

		bool NoAmbiguousPoint() const { return AmbiguousPoints.empty(); }
		bool ConsumeSymbol(Symbol Value, std::size_t Index, bool InputSaveStep = true);

		std::optional<AmbiguousPoint> ConsumAmbiguousPoint(CoreProcessor& Processor);

		struct FlushResult
		{
			bool ReachEndOfFile;
		};
		
		template<typename Fun1, typename Fun2>
		std::optional<FlushResult> Flush(std::optional<std::size_t> Startup, std::size_t End, Fun1&& F1, Fun2&& F2)
			requires(std::is_invocable_r_v<Symbol, Fun1, std::size_t>&& std::is_invocable_r_v<bool, Fun2, AmbiguousPoint, CoreProcessor&>);

		template<typename Fun2>
		std::optional<FlushResult> Flush(std::optional<std::size_t> Startup, std::size_t End, 
			Symbol(*)(std::size_t, void* Data), void* Data,
			Fun2&& F2)
			requires(std::is_invocable_r_v<bool, Fun2, AmbiguousPoint, CoreProcessor&>);

		std::optional<FlushResult> Flush(std::optional<std::size_t> Startup, std::size_t End,
			Symbol(*)(std::size_t, void* Data), void * Data, bool (*)(AmbiguousPoint, CoreProcessor& Processor, void* Data2), void* Data2);
	};

	template<typename Fun1, typename Fun2>
	auto CoreProcessor::Flush(std::optional<std::size_t> Startup, std::size_t End, Fun1&& F1, Fun2&& F2) ->std::optional<FlushResult>
		requires(std::is_invocable_r_v<Symbol, Fun1, std::size_t> && std::is_invocable_r_v<bool, Fun2, AmbiguousPoint, CoreProcessor&>)
	{
		return Flush(Startup, End, [](std::size_t Index, void* Data) -> Symbol {
			return (*reinterpret_cast<std::remove_reference_t<Fun1>*>(Data))(Index);
		}, &F1, [](AmbiguousPoint AP, CoreProcessor& Pro, void* Data)-> bool{
			return (*reinterpret_cast<std::remove_reference_t<Fun2>*>(Data))(AP, Pro);
		}, &F2);
	}

	template<typename Fun2>
	auto CoreProcessor::Flush(std::optional<std::size_t> Startup, std::size_t End,
		Symbol(*F1)(std::size_t, void* Data), void* Data,
		Fun2&& F2) ->std::optional<FlushResult>
		requires(std::is_invocable_r_v<bool, Fun2, AmbiguousPoint, CoreProcessor&>) 
	{
		return Flush(Startup, End, F1, Data, [](AmbiguousPoint AP, CoreProcessor& Pro, void* Data)-> bool {
				return (*reinterpret_cast<std::remove_reference_t<Fun2>*>(Data))(AP, Pro);
			}, &F2);
	}

	/*
	struct CoreProcessor
	{

		void OverwriteCacheSymbol(std::span<ProcrssorInput const> Caches) { CacheSymbols = Caches;}
		std::size_t ComsumeTerminalSymbol(std::size_t CachedSymbolOffset, std::vector<Step>& OutputStep);
		std::size_t EndOfSymbolStream(std::vector<Step>& OutputStep);
		void Reset();
		bool CanRefleshCachesSymbols() const noexcept { return MainProcesserIndex == 1 && MainProcessor[0].AmbiguousPoints.empty(); }
		CoreProcessor(TableWrapper Wrapper);

		static constexpr std::size_t Length = 4;

	private:

		std::array<SubCoreProcessor, Length> MainProcessor;
		std::size_t MainProcesserIndex;
		TableWrapper Wrapper;
		std::span<ProcrssorInput const> CacheSymbols;
	};
	*/

	struct Processor
	{
		struct Walkway
		{
			std::size_t StateCount;
			std::size_t CachedSymbolIndex;
			TableWrapper::SerilizedT LastState;
			std::strong_ordering operator<=>(Walkway const&) const = default;
		};


	};


	std::vector<Step> ProcessSymbol(TableWrapper Wrapper, std::span<Symbol const> Input);
	std::vector<Step> ProcessSymbol(TableWrapper Wrapper, std::size_t Startup, Symbol(*)(std::size_t, void* Data), void* Data);

	template<typename Fun1>
	std::vector<Step> ProcessSymbol(TableWrapper Wrapper, std::size_t Startup, Fun1&& Func1)
		requires(std::is_invocable_r_v<Symbol, Fun1, std::size_t>)
	{
		return ProcessSymbol(Wrapper, Startup, [](std::size_t Input, void* Data)->Symbol{
			return (*reinterpret_cast<std::remove_reference_t<Fun1>*>(Data))(Input);
		}, &Func1);
	}





	namespace Exception
	{
		struct Interface : public std::exception
		{
			virtual char const* what() const override;
			virtual ~Interface() = default;
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
				Symbol,
				NodeCount,
				NodeOffset,
				ReduceCount,
				ShiftCount,
				ProductionIndex,
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
			Symbol Value;
			std::size_t LastState;
			std::size_t TokenIndex;

			UnaccableSymbol(Symbol Value, std::size_t LastState, std::size_t TokenIndex)
				: LastState(LastState), TokenIndex(TokenIndex)
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