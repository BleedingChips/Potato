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
			bool& NeedTokenIndexRef,
			std::size_t& MaxFormatDetect,
			std::vector<SLRX::ProductionBuilder>& Builder,
			std::vector<SLRX::OpePriority>& OpePriority
		);

		std::wstring TotalString;
		std::vector<Misc::IndexSpan<>> StringMapping;

		struct RegScriptionT
		{
			std::size_t SymbolValue;
			std::optional<std::size_t> UserMask;
		};

		std::vector<RegScriptionT> RegScriptions;

		Reg::DfaT Lexical;
		SLRX::LRX Syntax;

		friend struct EbnfLexicalProcessor;
		friend struct LexicalElementT;
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
		Misc::LineRecorder Line;
		UnacceptableEbnf(TypeE Type, std::wstring Str, Misc::LineRecorder Line)
			: Type(Type), Str(Str), Line(Line) {}
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
}