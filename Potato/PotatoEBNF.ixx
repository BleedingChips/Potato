module;

#include <cassert>

export module Potato.EBNF;

export import Potato.Reg;
export import Potato.Encode;
export import Potato.Format;
export import Potato.STD;

export namespace Potato::EBNF
{
	
	struct EbnfBuilder
	{

		EbnfBuilder(std::size_t StartupTokenIndex);

		std::size_t GetRequireTokenIndex() const { return RequireTokenIndex; }
		bool Consume(char32_t InputValue, std::size_t NextTokenIndex);
		bool EndOfFile();

	protected:

		bool AddSymbol(SLRX::Symbol Symbol, Misc::IndexSpan<> TokenIndex);
		bool AddEndOfFile();

		enum RegTypeE
		{
			Terminal,
			Empty,
			Reg,
		};

		struct RegMapT
		{
			Misc::IndexSpan<> RegName;
			RegTypeE RegNameType = RegTypeE::Terminal;
			Misc::IndexSpan<> Reg;
			std::optional<Misc::IndexSpan<>> UserMask;
		};

		enum class ElementTypeE
		{
			Mask,
			Terminal,
			NoTerminal,
			Self,
			Temporary
		};

		struct ElementT
		{
			ElementTypeE ElementType = ElementTypeE::Terminal;
			Misc::IndexSpan<> Value;
		};

		std::vector<RegMapT> RegMappings;
		std::optional<ElementT> StartSymbol;
		std::optional<ElementT> MaxForwardDetect;

		struct BuilderT
		{
			ElementT StartSymbol;
			std::vector<ElementT> Productions;
			Misc::IndexSpan<> UserMask;
		};

		std::vector<BuilderT> Builder;

		struct OpePriority
		{
			std::vector<ElementT> Ope;
			SLRX::OpePriority::Associativity Associativity;
		};

		std::vector<OpePriority> OpePriority;

		enum StateE
		{
			Step1,
			Step2,
			Step3
		};

		StateE State = StateE::Step1;

		std::size_t RequireTokenIndex = 0;
		std::size_t LastSymbolToken = 0;
		Reg::DfaBinaryTableProcessor Processor;
		SLRX::CoreProcessor::Context Context;
		std::size_t TerminalProductionIndex = 0;
		std::optional<ElementT> LastProductionStartSymbol;
		std::size_t OrMaskIte = 0;
	
		struct BuilderStep1
		{
			EbnfBuilder& Ref;

			std::any operator()(SLRX::SymbolInfo Value, std::size_t Index);
			std::any operator()(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro);
		};

		struct BuilderStep2
		{
			EbnfBuilder& Ref;

			std::any operator()(SLRX::SymbolInfo Value, std::size_t Index);
			std::any operator()(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro) { return {}; }
		};

		struct BuilderStep3
		{
			EbnfBuilder& Ref;

			std::any operator()(SLRX::SymbolInfo Value, std::size_t Index) { return {}; }
			std::any operator()(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro) { return {}; }
		};

		friend struct Ebnf;
	};



	struct Ebnf
	{
		template<typename CharT, typename CharTraisT>
		Ebnf(std::basic_string_view<CharT, CharTraisT> EbnfStr);

		Ebnf(Ebnf&&) = default;

		Ebnf() = default;

	protected:

		std::wstring TotalString;

		struct SymbolMapT
		{
			Misc::IndexSpan<> StrIndex;
		};

		std::vector<SymbolMapT> SymbolMap;

		struct RegMapT
		{
			std::size_t MapSymbolValue;
			std::optional<std::size_t> UserMask;
		};

		std::vector<RegMapT> RegMap;

		Reg::Dfa Lexical;
		SLRX::LRX Syntax;
	};


	struct SymbolInfo
	{
		std::wstring_view SymbolName;
		Misc::IndexSpan<> TokenIndex;
	};

	struct ReduceProduction
	{
		struct Element : public SymbolInfo 
		{
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
		std::span<Element> Elements;
		std::size_t GetProduction
	};


	struct CoreEbnfProcessor
	{
		CoreEbnfProcessor(std::size_t StartupTokenIndex)
			: StartupTokenIndex(StartupTokenIndex) {}

	protected:
		std::size_t StartupTokenIndex;
		std::size_t RequireTokenIndex;
	};


	template<typename AppendInfo, typename Function>
	requires(
		std::is_invocable_r_v<std::any, Function, SymbolInfo, AppendInfo>
		&& std::is_invocable_r_v<std::any, Function, SymbolInfo, ReduceProduction>
	)
	struct EbnfProcessor : CoreEbnfProcessor
	{
		Ebnf const& Table;
		Reg::DfaProcessor Lexical;
		SLRX::LRXProcessor<> Syntax;
	};




	/*
	struct EbnfLexerT
	{
		
		template<typename CharT, typename CharTrais>
		EbnfLexerT(std::basic_string_view<CharT, CharTrais> Str);

		enum class T
		{
			Empty = 0,
			Terminal,
			Equal,
			Rex,
			NoTerminal,
			StartSymbol,
			LB_Brace,
			RB_Brace,
			LM_Brace,
			RM_Brace,
			LS_Brace,
			RS_Brace,
			LeftPriority,
			RightPriority,
			Or,
			Number,
			Start,
			Colon,
			Semicolon,
			Command,
			Barrier,
			Line
		};

		struct ElementT
		{
			T Symbol;
			Misc::IndexSpan<> TokenIndex;
			Misc::LineRecorder Line;
			std::size_t ElementIndex;
		};

		ElementT& operator[](std::size_t Index) { return Elements[Index]; };
		ElementT const& operator[](std::size_t Index) const { return Elements[Index]; };
		std::size_t Size() const { return Elements.size(); }

	protected:

		static Reg::DfaBinaryTableWrapper GetRegTable();

		std::vector<ElementT> Elements;
		Misc::LineRecorder Line;

		friend struct EbnfT;
	};

	

	struct LexicalElementT
	{
		EbnfT::RegScriptionT SceiptionT;
		Misc::IndexSpan<> TokenIndex;
	};

	struct EbnfLexicalProcessor
	{
		EbnfLexicalProcessor(EbnfT const& TableRef, std::size_t StartupTokenIndex = 0)
			: StartupTokenIndex(StartupTokenIndex), TableRef(TableRef), DfaProcessor(TableRef.Lexical)
		{}

		void Reset();

		bool Consume(char32_t Input, std::size_t NextTokenIndex);
		bool EndOfFile() { return Consume(Reg::EndOfFile(), RequireStrTokenIndex + 1); }
		std::size_t GetRequireStrTokenIndex() const { return RequireStrTokenIndex; }

		std::vector<LexicalElementT>& GetElements() { return LexicalElementT; }
		std::vector<LexicalElementT> const& GetElements() const { return LexicalElementT; }

	protected:

		std::vector<LexicalElementT> LexicalElementT;
		std::size_t StartupTokenIndex = 0;
		std::size_t RequireStrTokenIndex = 0;
		EbnfT const& TableRef;
		Reg::DfaProcessor DfaProcessor;
	};

	template<typename CharT, typename CharTraitT>
	bool LexicalProcessor(EbnfLexicalProcessor& Pro, std::basic_string_view<CharT, CharTraitT> InputStr)
	{
		char32_t InputSymbol = 0;
		std::span<char32_t> OutputSpan{ &InputSymbol, 1 };

		while (true)
		{
			auto RequireSize = Pro.GetRequireStrTokenIndex();
			if (RequireSize == InputStr.size())
			{
				if(!Pro.EndOfFile())
					return false;
			}
			else if (RequireSize > InputStr.size())
			{
				return true;
			}
			else {
				auto P = Potato::Encode::CharEncoder<char8_t, char32_t>::EncodeOnceUnSafe(
					InputStr.substr(RequireSize),
					OutputSpan
				);
				if (!Pro.Consume(InputSymbol, RequireSize + P.SourceSpace))
				{
					return false;
				}
			}
		}
	}

	struct EbnfSyntaxProcessor
	{
		EbnfSyntaxProcessor(EbnfT const& TableRef)
			: TableRef(TableRef), DfaProcessor(TableRef.Lexical)
		{}

		void Reset();

		bool Consume(SLRX::Symbol );
		bool EndOfFile() { return Consume(Reg::EndOfFile(), RequireStrTokenIndex + 1); }

	protected:

		SLRX::SymbolProcessor SymbolProcessor;
	};
	*/



	/*
	struct EBnfBinaryTableWrapper
	{

		using StandardT = std::uint32_t;

		static std::size_t CalculateRequireSpace(EBnfT const& Ref);
		static std::size_t SerilizeTo(std::span<StandardT> OutputBuffer, EBnfT const& Ref);
		static std::vector<StandardT> Create(EBnfT const& Le);
		static std::vector<StandardT> Create(std::u8string_view Str) {
			return Create(EBnfT::Create(Str));
		}

		SLRX::LRXBinaryTableWrapper GetSyntaxWrapper() const;
		Reg::DfaBinaryTableWrapper GetLexicalWrapper() const;
		std::optional<std::u8string_view> ReadSymbol(SLRX::Symbol Value) const;
		operator bool() const { return !Wrapper.empty(); }
		bool IsNoName(SLRX::Symbol Value) const;

		std::span<StandardT const> Wrapper; 
	};

	struct LexicalElement
	{
		std::u8string_view CaptureValue;
		std::u8string_view SymbolStr;
		SLRX::Symbol Value;
		std::size_t Mask;
		Misc::IndexSpan<> StrIndex;
	};

	template<typename RT>
	struct Result
	{
		std::optional<RT> Element;
		std::size_t LastOffset;
		operator bool() const { return Element.has_value(); }
		RT& operator*() { return *Element; }
		RT const& operator*() const { return *Element; }
	};

	struct LexicalProcessor
	{

		std::optional<std::u8string_view> Consume(std::u8string_view Str, std::size_t MarkedOffset);

		LexicalProcessor(EBnfT const& Table) : Pro(Table.GetLexicalWrapper(), true), Table(Table) {}
		LexicalProcessor(EBnfBinaryTableWrapper Table) : Pro(Table.GetLexicalWrapper(), true), Table(Table) {}
		std::span<LexicalElement> GetSpan() { return std::span(Elements); }
		void Clear() { Elements.clear(); }

	protected:

		std::vector<LexicalElement> Elements;
		Reg::HeadMatchProcessor Pro;
		std::variant<TableWrapper, std::reference_wrapper<EBNFX const>> Table;
	};

	struct ParsingStep
	{
		std::u8string_view Symbol;
		bool IsPredict = false;

		struct ReduceT {
			std::size_t Mask;
			std::size_t ElementCount;
			std::size_t UniqueReduceID;
			bool IsNoNameReduce;
		};

		struct ShiftT {
			std::u8string_view CaptureValue;
			std::size_t Mask;
			Misc::IndexSpan<> StrIndex;
		};
		std::variant<ReduceT, ShiftT> Data;
		bool IsTerminal() const { return std::holds_alternative<ShiftT>(Data); }
	};

	struct SyntaxProcessor
	{
		bool Consume(LexicalElement Element);
		void FlushSteps();
		bool EndOfFile();
		std::span<ParsingStep> GetSteps() { return std::span(ParsingSteps); }
		static Result<std::vector<ParsingStep>> Process(EBNFX const& Table, std::span<LexicalElement const> Eles) {
			SyntaxProcessor Pro{Table};
			return Process(Pro, Eles);
		}
		static Result<std::vector<ParsingStep>> Process(TableWrapper Table, std::span<LexicalElement const> Eles) {
			SyntaxProcessor Pro{ Table };
			return Process(Pro, Eles);
		}
		SyntaxProcessor(EBNFX const& Table) : Pro(Table.GetSyntaxWrapper()), Table(Table) {}
		SyntaxProcessor(TableWrapper Table) : Pro(Table.GetSyntaxWrapper()), Table(Table) {}
	protected:
		static Result<std::vector<ParsingStep>> Process(SyntaxProcessor& Pro, std::span<LexicalElement const> Eles);
		SLRX::SymbolProcessor Pro;
		std::vector<LexicalElement> TemporaryElementBuffer;
		std::vector<ParsingStep> ParsingSteps;
		std::variant<TableWrapper, std::reference_wrapper<EBNFX const>> Table;
		std::size_t HandleStep(std::span<SLRX::ParsingStep const> Steps);
		std::tuple<ParsingStep, std::size_t> Translate(SLRX::ParsingStep In, std::span<LexicalElement const> TemporaryElementBuffer) const;
	};

	struct TElement
	{
		std::u8string_view Symbol;
		ParsingStep::ShiftT Shift;
	};

	struct NTElement
	{
		struct MetaData
		{
			std::u8string_view Symbol;
			Misc::IndexSpan<> StrIndex;
		};

		struct DataT
		{
			MetaData Meta;
			SLRX::ElementData Data;

			template<typename Type>
			decltype(auto) Consume() { return Data.Consume<Type>(); }

			decltype(auto) Consume() { return Data.Consume(); }
			template<typename Type>
			decltype(auto) TryConsume() { return Data.TryConsume<Type>(); }
		};

		std::u8string_view Symbol;
		ParsingStep::ReduceT Reduce;
		std::span<DataT> Datas;
		DataT& operator[](std::size_t index) { return Datas[index]; }
	};

	struct PredictElement
	{
		std::u8string_view Symbol;
		ParsingStep::ReduceT Reduce;
	};

	struct VariantElement
	{
		std::variant<TElement, NTElement, PredictElement> Element;
		bool IsTerminal() const { return std::holds_alternative<TElement>(Element); }
		bool IsNoTerminal() const { return std::holds_alternative<NTElement>(Element); }
		bool IsPredict() const { return std::holds_alternative<PredictElement>(Element); }
		TElement& AsTerminal() { return std::get<TElement>(Element); }
		NTElement& AsNoTerminal() { return std::get<NTElement>(Element); }
		PredictElement& AsPredict() { return std::get<PredictElement>(Element); }
	};

	struct Condition
	{
		enum class TypeT
		{
			Normal, // Normal
			Or,
			Parentheses, // ()
			SquareBrackets, // []
			CurlyBrackets, // {}
		};

		TypeT Type;
		std::size_t AppendData;
		std::vector<NTElement::DataT> Datas;
	};

	struct ParsingStepProcessor
	{
		struct Result
		{
			std::optional<VariantElement> Element;
			std::optional<NTElement::MetaData> MetaData;
		};

		std::optional<Result> Consume(ParsingStep Step);
		void Push(NTElement::MetaData MetaData, std::any Data) { TemporaryDatas.clear(); Datas.push_back({ MetaData, std::move(Data)}); }
		std::optional<std::any> EndOfFile();

	private:

		std::vector<NTElement::DataT> Datas;
		std::vector<NTElement::DataT> TemporaryDatas;
	};

	template<typename Func>
	std::optional<std::any> ProcessParsingStep(std::span<ParsingStep const> Steps, Func&& F)
	{
		ParsingStepProcessor Pro;
		for (auto Ite : Steps)
		{
			auto Re = Pro.Consume(Ite);
			if (Re.has_value())
			{
				if (Re->Element.has_value())
				{
					auto Ie = std::forward<Func>(F)(*Re->Element);
					if (Re->MetaData.has_value())
					{
						Pro.Push(*Re->MetaData, std::move(Ie));
					}
				}
			}
			else {
				return {};
			}
		}
		return Pro.EndOfFile();
	}

	template<typename RT, typename Fun>
	RT ProcessParsingStepWithOutputType(std::span<ParsingStep const> Input, Fun&& Func) requires(std::is_invocable_r_v<std::any, Fun, VariantElement>)
	{
		return std::any_cast<RT>(*ProcessParsingStep(Input, Func));
	}
	*/
}

export namespace Potato::EBNF::Exception
{

	struct Interface : public std::exception
	{
		virtual ~Interface() = default;
		virtual char const* what() const override;
	};

	struct IllegalEbnfProduction : public std::exception
	{
		SLRX::Exception::IllegalSLRXProduction::Category Type;
		std::size_t MaxForwardDetectNum;
		std::size_t DetectNum;

		struct TMap
		{
			std::u32string Mapping;
		};

		struct NTMap
		{
			std::u32string Mapping;
			std::size_t ProductionCount;
			std::size_t ProductionMask;
		};

		std::vector<std::variant<TMap, NTMap>> Steps1;
		std::vector<std::variant<TMap, NTMap>> Steps2;

		IllegalEbnfProduction(SLRX::Exception::IllegalSLRXProduction const&, std::vector<std::u32string> const& TMapping, std::vector<std::u32string> const& NTMapping);
		IllegalEbnfProduction(IllegalEbnfProduction const&) = default;
		IllegalEbnfProduction(IllegalEbnfProduction&&) = default;
		static std::vector<std::variant<TMap, NTMap>> Translate(std::span<SLRX::ParsingStep const> Steps, std::vector<std::u32string> const& TMapping, std::vector<std::u32string> const& NTMapping);
		virtual char const* what() const override;
	};

	struct UnacceptableRegex : public Interface
	{
		std::wstring Regex;

		template<typename CharT>
		UnacceptableRegex(std::basic_string_view<CharT> InputRegex) : Regex(*Encode::StrEncoder<CharT, wchar_t>::EncodeToString(InputRegex)) {}
		UnacceptableRegex(UnacceptableRegex const&) = default;
		virtual char const* what() const override;
	};

	struct UnacceptableEbnf : public Interface
	{
		enum class TypeE
		{
			WrongEbnfLexical,
			WrongEbnfSyntax,
			WrongRegex,
			WrongLr,
			WrongPriority,
			RegexOutofRange,
			LrOutOfRange,
			UnrecognizableSymbol,
			UnrecognizableRex,
			UnrecognizableTerminal,
			UnsetProductionHead,
			StartSymbolAreadySet,
			UnfindedStartSymbol,
		};
		TypeE Type;
		std::wstring Str;
		UnacceptableEbnf(TypeE Type, std::wstring Str)
			: Type(Type), Str(Str) {}
		UnacceptableEbnf(UnacceptableEbnf const&) = default;
		virtual char const* what() const override;
	};

	struct OutofRange : public Interface
	{
		enum class TypeE
		{
			TempMaskedEmptyProduction,
			FromRegex,
			FromSLRX,
			Header,
			EbnfMask,
			SymbolCount,
			TerminalCount,
		};
		TypeE Type;
		std::size_t Value;
		OutofRange(TypeE Type, std::size_t TokenIndex) : Type(Type), Value(TokenIndex) {}
		OutofRange(OutofRange const&) = default;
		virtual char const* what() const override;
	};
}

export namespace Potato::EBNF
{

	struct BuildInUnacceptableEbnf
	{
		using TypeE = Exception::UnacceptableEbnf::TypeE;
		TypeE Type;
		Misc::IndexSpan<> TokenIndex;
	};

	template<typename CharT, typename CharTraisT>
	Ebnf::Ebnf(std::basic_string_view<CharT, CharTraisT> EbnfStr)
	{
		try {
			EbnfBuilder Builder{ 0 };
			std::size_t Index = 0;
			char32_t InputValue = 0;
			std::span<char32_t> OutputBuffer{&InputValue, 1};
			while (true)
			{
				auto RequireSize = Builder.GetRequireTokenIndex();

				if (RequireSize < EbnfStr.size())
				{
					auto InputSpan = EbnfStr.substr(RequireSize);
					auto EncodeInfo = Encode::CharEncoder<CharT, char32_t>::EncodeOnceUnSafe(InputSpan, OutputBuffer);
					auto TokenIndex = Misc::IndexSpan<>{ RequireSize, RequireSize + EncodeInfo.SourceSpace };

					if (!Builder.Consume(InputValue, TokenIndex.End()))
					{
						throw Exception::UnacceptableRegex(TokenIndex.Slice(EbnfStr));
					}
				}else if (RequireSize == EbnfStr.size())
				{
					if(!Builder.EndOfFile())
						throw Exception::UnacceptableRegex(std::wstring_view{L"EndOfFile"});
				}
				else {
					break;
				}
			}
			
			Reg::MulityRegCreater Gen;
			std::map<std::basic_string_view<CharT, CharTraisT>, std::size_t> Mapping;
			Mapping.insert({{}, Mapping.size()});

			for (std::size_t I = Builder.RegMappings.size(); I > 0; --I)
			{
				auto& Ite = Builder.RegMappings[I - 1];
				switch (Ite.RegNameType)
				{
				case EbnfBuilder::RegTypeE::Reg:
				{
					auto [IIte, B] = Mapping.insert({ Ite.RegName.Slice(EbnfStr), Mapping.size()});
					if (B)
					{
						auto RegStr = Ite.Reg.SubIndex(1, Ite.Reg.Size() - 2).Slice(EbnfStr);
						Gen.AppendReg(RegStr, true, RegMap.size());
						RegMap.push_back({ IIte->second, {} });
					}
					break;
				}
				case EbnfBuilder::RegTypeE::Terminal:
				{
					auto [IIte, B] = Mapping.insert({ Ite.RegName.Slice(EbnfStr), Mapping.size() });
					auto RegStr = Ite.Reg.SubIndex(1, Ite.Reg.Size() - 2).Slice(EbnfStr);
					std::size_t UserMask = 0;
					if (Ite.UserMask.has_value())
					{
						Format::DirectScan(Ite.UserMask->Slice(EbnfStr), UserMask);
					}
					Gen.AppendReg(RegStr, true, RegMap.size());
					RegMap.push_back({ IIte->second, UserMask });
					break;
				}
				}
			}

			auto InsertElement = [&](EbnfBuilder::ElementT Ele){
				if (Ele.ElementType == EbnfBuilder::ElementTypeE::Terminal || Ele.ElementType == EbnfBuilder::ElementTypeE::NoTerminal)
				{
					Mapping.insert(
						{Ele.Value.Slice(EbnfStr), Mapping.size()}
					);
				}
			};

			assert(Builder.StartSymbol.has_value());

			InsertElement(*Builder.StartSymbol);

			for (auto& Ite : Builder.Builder)
			{
				InsertElement(Ite.StartSymbol);
				for (auto& Ite2 : Ite.Productions)
				{
					InsertElement(Ite2);
				}
			}

			for (auto& Ite : Builder.OpePriority)
			{
				for (auto& Ite2 : Ite.Ope)
				{
					InsertElement(Ite2);
				}
			}

			std::size_t RquireSize = 0;

			for (auto& Ite : Mapping)
			{
				RquireSize += Encode::StrEncoder<CharT, wchar_t>::RequireSpaceUnSafe(Ite.first).TargetSpace;
			}

			TotalString.resize(RquireSize);
			SymbolMap.resize(Mapping.size());
			std::size_t Writed = 0;
			for (auto& Ite : Mapping)
			{
				auto W = Encode::StrEncoder<CharT, wchar_t>::EncodeUnSafe(Ite.first, std::span(TotalString).subspan(Writed)).TargetSpace;
				SymbolMap[Ite.second].StrIndex = { Writed, Writed + W};
				Writed = Writed + W;
			}

			Lexical = *Gen.CreateDfa(Reg::Dfa::FormatE::GreedyHeadMatch);

			SLRX::Symbol StartSymbol;
			
			auto Find = Mapping.find(
				Builder.StartSymbol->Value.Slice(EbnfStr)
			);

			StartSymbol = SLRX::Symbol::AsNoTerminal(Find->second);

			std::size_t MaxForwardDetect = 3;

			if (Builder.MaxForwardDetect.has_value())
			{
				Format::DirectScan(Builder.MaxForwardDetect->Value.Slice(EbnfStr), MaxForwardDetect);
			}

			std::vector<SLRX::ProductionBuilder> PBuilder;

			for (auto& Ite : Builder.Builder)
			{
				SLRX::Symbol StartSymbol;
				std::vector<SLRX::ProductionBuilderElement> Element;
				std::size_t ProdutionMask = 0;
				if (Ite.StartSymbol.ElementType == EbnfBuilder::ElementTypeE::Temporary)
				{
					StartSymbol = SLRX::Symbol::AsNoTerminal(Ite.StartSymbol.Value.Begin() + Mapping.size());
				}
				else {
					StartSymbol = SLRX::Symbol::AsNoTerminal(Mapping.find(Ite.StartSymbol.Value.Slice(EbnfStr))->second);
				}
				Element.reserve(Ite.Productions.size());
				for (auto& Ite2 : Ite.Productions)
				{
					switch (Ite2.ElementType)
					{
					case EbnfBuilder::ElementTypeE::Mask:
					{
						std::size_t Mask = 0;
						auto Str = Ite2.Value.Slice(EbnfStr);
						Format::DirectScan(Ite2.Value.Slice(EbnfStr), Mask);
						Element.push_back(SLRX::ProductionBuilderElement{Mask});
						break;
					}
					case EbnfBuilder::ElementTypeE::Terminal:
					{
						auto Str = Ite2.Value.Slice(EbnfStr);
						auto RSymbol = SLRX::Symbol::AsTerminal(
							Mapping.find(Str)->second
						);
						Element.push_back(SLRX::ProductionBuilderElement{ RSymbol });
						break;
					}
					case EbnfBuilder::ElementTypeE::NoTerminal:
					{
						auto Str = Ite2.Value.Slice(EbnfStr);
						auto RSymbol = SLRX::Symbol::AsNoTerminal(
							Mapping.find(Str)->second
						);
						Element.push_back(SLRX::ProductionBuilderElement{ RSymbol });
						break;
					}
					case EbnfBuilder::ElementTypeE::Self:
					{
						Element.push_back(SLRX::ProductionBuilderElement{ SLRX::ItSelf{} });
						break;
					}
					case EbnfBuilder::ElementTypeE::Temporary:
					{
						auto RSymbol = SLRX::Symbol::AsNoTerminal(
							Ite2.Value.Begin() + Mapping.size()
						);
						Element.push_back(SLRX::ProductionBuilderElement{ RSymbol });
						break;
					}
					default:
						assert(false);
						break;
					}
				}
				if (Ite.UserMask.Size() != 0)
				{
					auto Str = Ite.UserMask.Slice(EbnfStr);
					Format::DirectScan(Str, ProdutionMask);
				}
				PBuilder.push_back({StartSymbol, std::move(Element), ProdutionMask });
			}

			std::vector<SLRX::OpePriority> OpePriority;

			for (auto& Ite : Builder.OpePriority)
			{
				std::vector<SLRX::Symbol> Symbols;
				Symbols.reserve(Ite.Ope.size());
				for (auto& Ite2 : Ite.Ope)
				{
					auto Str = Ite2.Value.Slice(EbnfStr);
					Symbols.push_back(
						SLRX::Symbol::AsTerminal(
							Mapping.find(Str)->second
						)
					);
				}
				OpePriority.push_back({std::move(Symbols), Ite.Associativity });
			}

			Syntax = SLRX::LRX::Create(StartSymbol, std::move(PBuilder), std::move(OpePriority), MaxForwardDetect);
		}
		catch (BuildInUnacceptableEbnf Bnf)
		{
			auto Str = *Encode::StrEncoder<CharT, wchar_t>::EncodeToString(Bnf.TokenIndex.Slice(EbnfStr));
			throw Exception::UnacceptableEbnf{Bnf.Type, std::move(Str)};
		}
		
	}


	/*
	struct BuildInUnacceptableEbnf
	{
		using TypeE = Exception::UnacceptableEbnf::TypeE;
		TypeE Type;
		std::size_t TokenIndex;
	};

	template<typename CharT, typename CharTrais>
	EbnfLexerT::EbnfLexerT(std::basic_string_view<CharT, CharTrais> Str)
	{
		Reg::DfaBinaryTableWrapperProcessor Pro(GetRegTable());

		auto StrIte = Str;
		std::size_t IteIndex = 0;
		while (!StrIte.empty())
		{
			Pro.Reset();
			auto Re = Reg::CoreProcessorWithUnaccept(Pro, StrIte);
			if (std::holds_alternative<Reg::ProcessorAcceptT>(Re))
			{
				auto& Accept = std::get<Reg::ProcessorAcceptT>(Re);
				auto Symbol = static_cast<T>(Accept.Mask);

				if (Symbol == T::Line)
				{
					Line.NewLine();
				}else if(Symbol != T::Empty)
					Elements.push_back({ Symbol, {IteIndex, IteIndex + Accept.MainCapture.End()}, Line, Elements.size()});
				StrIte = StrIte.substr(Accept.MainCapture.End());
				IteIndex += Accept.MainCapture.End();
				Line.AddCharacter();
			}
			else {
				auto OutputToken = std::get<Reg::ProcessorUnacceptT>(Re).FailCapture;
				throw Exception::UnacceptableRegex(OutputToken.Slice(Str));
			}
		}
	}

	template<typename CharT, typename CharTraistT>
	EbnfT::EbnfT(std::basic_string_view<CharT, CharTraistT> EbnfStr)
	{
		EbnfLexerT Lexer(EbnfStr);

#ifdef _DEBUG
		{
			struct Temp
			{
				EbnfLexerT::T Type;
				std::basic_string_view<CharT, CharTraistT> Str;
			};
			std::vector<Temp> Tes;

			for (auto& Ite : Lexer.Elements)
			{
				Tes.push_back({
					Ite.Symbol,
					Ite.TokenIndex.Slice(EbnfStr)
				});
			}
			volatile int i = 0;
		}
#endif

		SLRX::Symbol StartSymbol;
		std::size_t MaxForwardDetect = 0;
		std::vector<RegMappingT> RegMapping;
		bool NeedTokenReference = false;
		std::vector<SLRX::ProductionBuilder> Builder;
		std::vector<SLRX::OpePriority> OpePriority;

		try {
			Parsing(Lexer, RegMapping, StartSymbol, NeedTokenReference, MaxForwardDetect, Builder, OpePriority);
		}
		catch (BuildInUnacceptableEbnf const& Error)
		{
			std::basic_string_view<CharT, CharTraistT> ErrorStr;
			auto Line = Lexer.Line;
			if (Error.TokenIndex < Lexer.Elements.size())
			{
				auto& Ref = Lexer[Error.TokenIndex];
				ErrorStr = Ref.TokenIndex.Slice(EbnfStr);
				Line = Ref.Line;
			}
			
			throw Exception::UnacceptableEbnf{ Error.Type, *Encode::StrEncoder<CharT, wchar_t>::EncodeToString(ErrorStr), Line };
		}

		if (NeedTokenReference)
		{
			Format::DirectScan(Lexer[MaxForwardDetect].TokenIndex.Slice(EbnfStr), MaxForwardDetect);
		}

		std::map<std::basic_string_view<CharT, CharTraistT>, std::size_t> SymbolMapping;

		struct RegT
		{
			std::basic_string_view<CharT, CharTraistT> Reg;
			bool IsRaw;
			std::size_t Mask;
			std::optional<std::size_t> UserMask;
			Misc::IndexSpan<> TokenIndex;
		};

		std::vector<RegT> Regs;
		Regs.reserve(RegMapping.size());

		auto ReLocateSymbol = [&](SLRX::Symbol& Sym){
			if (Sym.Value < Lexer.Elements.size())
			{
				auto LocateName = Lexer[Sym.Value].TokenIndex.Slice(EbnfStr);
				auto [Ite2, B] = SymbolMapping.insert({ LocateName, SymbolMapping.size() });
				Sym.Value = Ite2->second + 1;
				return true;
			}
			return false;
		};

		std::set<std::basic_string_view<CharT, CharTraistT>> ExistEge;

		for (auto Ite : RegMapping)
		{
			auto RegNameElement = Lexer[Ite.RegName];
			auto Reg = Lexer[Ite.Reg].TokenIndex.Slice(EbnfStr);
			Reg = Reg.substr(1, Reg.size() - 2);

			if (RegNameElement.Symbol == EbnfLexerT::T::Start)
			{
				Regs.push_back({
					Reg,
					false,
					0,
					{},
					Lexer[Ite.Reg].TokenIndex
				});
			}
			else {
				auto RegName = RegNameElement.TokenIndex.Slice(EbnfStr);
				auto [Ite2, B] = SymbolMapping.insert({ RegName, SymbolMapping.size() });
				Regs.push_back({
					Reg,
					RegNameElement.Symbol == EbnfLexerT::T::Rex,
					Ite2->second + 1,
					Ite.Mask,
					Lexer[Ite.Reg].TokenIndex
				});
			}
		}

		ReLocateSymbol(StartSymbol);

		for (auto& Ite : Builder)
		{

			ReLocateSymbol(Ite.ProductionValue);

			for (auto& Ite2 : Ite.Element)
			{
				switch (Ite2.Type)
				{
				case SLRX::ProductionBuilderElement::TypeT::IsValue:
					ReLocateSymbol(Ite2.ProductionValue);
					break;
				case SLRX::ProductionBuilderElement::TypeT::IsMask:
				{
					std::size_t RequireMask = 0;
					Format::DirectScan(
						Lexer[Ite2.ProductionMask].TokenIndex.Slice(EbnfStr),
						Ite2.ProductionMask
					);
					break;
				}
				default:
					break;
				}
			}

			Format::DirectScan(
				Lexer[Ite.ProductionMask].TokenIndex.Slice(EbnfStr),
				Ite.ProductionMask
			);
		}

		for (auto& Ite : OpePriority)
		{
			for (auto& Ite2 : Ite.Symbols)
			{
				ReLocateSymbol(Ite2);
			}
		}

		auto SecondRelocateSymbol = [&](SLRX::Symbol& Symbol)
		{
			if (Symbol.Value >= Lexer.Size())
			{
				Symbol.Value = Symbol.Value - Lexer.Size() + SymbolMapping.size() + 1;
			}
		};

		for (auto& Ite : Builder)
		{
			SecondRelocateSymbol(Ite.ProductionValue);
			for (auto& Ite2 : Ite.Element)
			{
				if(Ite2.Type == SLRX::ProductionBuilderElement::TypeT::IsValue)
					SecondRelocateSymbol(Ite2.ProductionValue);
			}
		}

		{
			std::size_t TotalStringRequireSize = 0;

			for (auto& Ite : SymbolMapping)
			{
				auto Re = Encode::StrEncoder<CharT, wchar_t>::RequireSpaceUnSafe(Ite.first);
				TotalStringRequireSize += Re.TargetSpace;
			}

			TotalString.resize(TotalStringRequireSize);
			StringMapping.resize(SymbolMapping.size());

			auto OutputSpan = std::span(TotalString);

			std::size_t WritedSize = 0;
			for (auto& Ite : SymbolMapping)
			{
				auto Re = Encode::StrEncoder<CharT, wchar_t>::EncodeUnSafe(Ite.first, OutputSpan);
				StringMapping[Ite.second] = Misc::IndexSpan<>{ WritedSize, WritedSize + Re.TargetSpace};
				OutputSpan = OutputSpan.subspan(Re.TargetSpace);
				WritedSize += Re.TargetSpace;
			}
		}

		{
			RegScriptions.resize(Regs.size());
			Reg::MulityRegCreater Creater;
			for (std::size_t I = Regs.size(); I > 0; --I)
			{
				auto IteIndex = I - 1;
				auto& Ite = Regs[IteIndex];
				Creater.AppendReg(Ite.Reg, Ite.IsRaw, IteIndex);
				RegScriptions[IteIndex] = { Ite.Mask, Ite.UserMask};
			}
			Lexical = std::move(*Creater.CreateDfa(Reg::DfaT::FormatE::GreedyHeadMatch));
		}

		{
			Syntax = SLRX::LRX{
				StartSymbol,
				std::move(Builder),
				std::move(OpePriority),
				MaxForwardDetect
			};
		}
	}
	*/
}