module;

export module Potato.EBNF;

export import Potato.Reg;
export import Potato.Encode;
export import Potato.Format;
export import Potato.STD;

export namespace Potato::EBNF
{
	
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
			std::size_t Line;
			std::size_t Row;
			std::size_t ElementIndex;
		};

		ElementT& operator[](std::size_t Index) { return Elements[Index]; };
		ElementT const& operator[](std::size_t Index) const { return Elements[Index]; };
		std::size_t Size() const { return Elements.size(); }

	protected:

		static Reg::DfaBinaryTableWrapper GetRegTable();

		std::vector<ElementT> Elements;
		std::size_t Line = 0;
		std::size_t Row = 0;

		friend struct EbnfT;
	};

	struct EbnfT
	{
		template<typename CharT, typename CharTraisT>
		EbnfT(std::basic_string_view<CharT, CharTraisT> EbnfStr);

		EbnfT(EbnfT&&) = default;

		EbnfT() = default;

	protected:

		using ElementT = EbnfLexerT::ElementT;

		struct RegMappingT
		{
			std::size_t RegName;
			std::size_t Reg;
			std::optional<std::size_t> Mask;
		};

		void Parsing(
			EbnfLexerT const& Input,
			std::vector<RegMappingT>& RegMapping,
			SLRX::Symbol& StartSymbol,
			std::vector<SLRX::ProductionBuilder>& Builder,
			std::vector<SLRX::OpePriority>& OpePriority
		);

		std::wstring TotalString;
		std::vector<Misc::IndexSpan<>> TerminalMapping;

		//struct 

		//void Translate(EbnfLexer const& Outout, std::vector<RegMappingT>& OutputMapping, std::vector<SLRXProductionT>& Productions, )

		/*
		std::vector<std::u8string> MappingTerminalSymbols;
		std::vector<std::u8string> MappingNoterminalSymbols;
		Reg::DfaT Lexical;
		SLRX::LRX Syntax;

		SLRX::LRX const& GetSyntaxWrapper() const { return Syntax; }
		Reg::DfaT const& GetLexicalWrapper() const { return Lexical; }
		std::optional<std::u8string_view> ReadSymbol(SLRX::Symbol Value) const;
		bool IsNoName(SLRX::Symbol Value) const;
		*/
	};

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
		std::size_t Line;
		std::size_t Row;
		UnacceptableEbnf(TypeE Type, std::wstring Str, std::size_t Line, std::size_t Row) 
			: Type(Type), Str(Str), Line(Line), Row(Row) {}
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
					Line += 1;
					Row = 0;
				}else if(Symbol != T::Empty)
					Elements.push_back({ Symbol, {IteIndex, IteIndex + Accept.MainCapture.End()}, Line, Row, Elements.size()});
				StrIte = StrIte.substr(Accept.MainCapture.End());
				IteIndex += Accept.MainCapture.End();
				Row += Accept.MainCapture.End();
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

		SLRX::Symbol StartSymbol;
		std::vector<RegMappingT> RegMapping;
		std::vector<SLRX::ProductionBuilder> Builder;
		std::vector<SLRX::OpePriority> OpePriority;

		try {
			Parsing(Lexer, RegMapping, StartSymbol, Builder, OpePriority);
		}
		catch (BuildInUnacceptableEbnf const& Error)
		{
			std::basic_string_view<CharT, CharTraistT> ErrorStr;
			std::size_t Line = Lexer.Line;
			std::size_t Row = Lexer.Line;
			if (Error.TokenIndex < Lexer.Elements.size())
			{
				auto& Ref = Lexer[Error.TokenIndex];
				ErrorStr = Ref.TokenIndex.Slice(EbnfStr);
				Line = Ref.Line;
				Row = Ref.Row;
			}
			
			throw Exception::UnacceptableEbnf{ Error.Type, *Encode::StrEncoder<CharT, wchar_t>::EncodeToString(ErrorStr), Line, Row };
		}

		std::map<std::basic_string_view<CharT, CharTraistT>, std::size_t> FinalRegMapping;

		struct RegT
		{
			std::basic_string_view<CharT, CharTraistT> Reg;
			std::size_t Mask;
			std::optional<std::size_t> UserMask;
		};

		std::vector<RegT> Regs;

		auto ReLocateSymbol = [&](SLRX::Symbol& Sym){
			if (Sym.Value < Lexer.Elements.size())
			{
				auto LocateName = Lexer[Sym.Value].TokenIndex.Slice(EbnfStr);
				auto [Ite2, B] = FinalRegMapping.insert({ LocateName, FinalRegMapping.size() });
				Sym.Value = Ite2->second;
				return true;
			}
			return false;
		};

		for (auto Ite : RegMapping)
		{
			auto RegName = Lexer[Ite.RegName].TokenIndex.Slice(EbnfStr);
			auto Reg = Lexer[Ite.Reg].TokenIndex.Slice(EbnfStr);
			auto [Ite2, B] = FinalRegMapping.insert({RegName, FinalRegMapping.size()});
			Regs.push_back({
				Reg,
				Ite2->second,
				Ite.Mask
			});
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
				Symbol.Value = Symbol.Value - Lexer.Size() + FinalRegMapping.size();
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

		volatile int i = 0;
	}
}