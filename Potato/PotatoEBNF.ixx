module;

export module Potato.EBNF;

export import Potato.Reg;
export import Potato.Encode;
export import Potato.Format;
export import Potato.STD;

export namespace Potato::EBNF
{

	struct EBNFX
	{
		static EBNFX Create(std::u8string_view Str);
		std::vector<std::u8string> MappingTerminalSymbols;
		std::vector<std::u8string> MappingNoterminalSymbols;
		Reg::DFA Lexical;
		SLRX::LRX Syntax;

		SLRX::LRX const& GetSyntaxWrapper() const { return Syntax; }
		Reg::DFA const& GetLexicalWrapper() const { return Lexical; }
		std::optional<std::u8string_view> ReadSymbol(SLRX::Symbol Value) const;
		bool IsNoName(SLRX::Symbol Value) const;
	};

	using StandardT = Reg::StandardT;

	struct TableWrapper
	{
		static std::size_t CalculateRequireSpace(EBNFX const& Ref);
		static std::size_t SerilizeTo(std::span<StandardT> OutputBuffer, EBNFX const& Ref);
		static std::vector<StandardT> Create(EBNFX const& Le);
		static std::vector<StandardT> Create(std::u8string_view Str) {
			return Create(EBNFX::Create(Str));
		}

		SLRX::TableWrapper GetSyntaxWrapper() const;
		Reg::TableWrapper GetLexicalWrapper() const;
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

		LexicalProcessor(EBNFX const& Table) : Pro(Table.GetLexicalWrapper(), true), Table(Table) {}
		LexicalProcessor(TableWrapper Table) : Pro(Table.GetLexicalWrapper(), true), Table(Table) {}
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
		UnacceptableRegex(std::wstring Regex) : Regex(std::move(Regex)) {}
		UnacceptableRegex(UnacceptableRegex const&) = default;
		virtual char const* what() const override;
	};

	struct UnacceptableEbnf : public Interface
	{
		enum class TypeT
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
		TypeT Type;
		std::u8string LastStr;
		std::size_t TokenIndex;
		UnacceptableEbnf(TypeT Type, std::u8string_view TotalStr, std::size_t TokenIndex) : Type(Type), TokenIndex(TokenIndex) {
			LastStr = std::u8string{ TokenIndex < 10 ? TotalStr.substr(0, TokenIndex) : TotalStr.substr(TokenIndex - 10, 10)};
		}
		UnacceptableEbnf(UnacceptableEbnf const&) = default;
		virtual char const* what() const override;
	};

	struct OutofRange : public Interface
	{
		enum class TypeT
		{
			TempMaskedEmptyProduction,
			FromRegex,
			FromSLRX,
			Header,
			EbnfMask,
			SymbolCount,
			TerminalCount,
		};
		TypeT Type;
		std::size_t Value;
		OutofRange(TypeT Type, std::size_t TokenIndex) : Type(Type), Value(TokenIndex) {}
		OutofRange(OutofRange const&) = default;
		virtual char const* what() const override;
	};
}

export namespace Potato::StrFormat
{
	/*
	template<> struct Formatter<Ebnf::Table>
	{
		std::u32string operator()(std::u32string_view, Ebnf::Table const& ref);
	};
	*/
}