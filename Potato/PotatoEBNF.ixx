module;

#include <cassert>

export module PotatoEBNF;

import std;
import PotatoReg;
import PotatoEncode;
import PotatoFormat;
import PotatoSLRX;
import PotatoMisc;
import PotatoPointer;
import PotatoTMP;

export namespace Potato::EBNF
{

	struct Ebnf;

	struct EbnfBuilder : protected SLRX::ProcessorOperator
	{

		EbnfBuilder(std::size_t StartupTokenIndex);

		std::size_t GetRequireTokenIndex() const { return RequireTokenIndex; }
		bool Consume(char32_t InputValue, std::size_t NextTokenIndex);
		bool EndOfFile();

	protected:

		bool AddSymbol(SLRX::Symbol Symbol, Misc::IndexSpan<> TokenIndex);
		bool AddEndOfFile();
		std::any HandleSymbol(SLRX::SymbolInfo Value);
		std::any HandleReduce(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro);
		std::any HandleSymbolStep1(SLRX::SymbolInfo Value);
		std::any HandleReduceStep1(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro);
		std::any HandleSymbolStep2(SLRX::SymbolInfo Value);
		std::any HandleReduceStep2(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro);
		std::any HandleSymbolStep3(SLRX::SymbolInfo Value);
		std::any HandleReduceStep3(SLRX::SymbolInfo Value, SLRX::ReduceProduction Pro);

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
		Reg::DfaProcessor Processor;
		SLRX::LRXProcessor LRXProcessor;
		std::size_t TerminalProductionIndex = 0;
		std::optional<ElementT> LastProductionStartSymbol;
		std::size_t OrMaskIte = 0;

		friend struct Ebnf;
	};

	std::u8string_view OrStatementRegName();
	std::u8string_view RoundBracketStatementRegName();
	std::u8string_view SquareBracketStatementRegName();
	std::u8string_view CurlyBracketStatementRegName();

	struct SymbolInfo
	{
		SLRX::Symbol Symbol;
		std::u8string_view SymbolName;
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
			template<typename Type>
			bool IsA() const {
				auto P = std::any_cast<Type const>(&AppendData);
				return P != nullptr;
			}
		};
		std::size_t UserMask;
		std::span<Element> Elements;
		std::size_t Size() const { return Elements.size(); }
		Element& GetProduction(std::size_t Input) { return Elements[Input]; }
		Element& operator[](std::size_t Index) { return GetProduction(Index); }
	};

	struct BuilInStatement
	{
		SymbolInfo Info;
		std::size_t UserMask;
		std::vector<ReduceProduction::Element> ProductionElements;
	};

	struct EbnfProcessor;
	struct EbnfBinaryTableWrapper;

	struct Ebnf
	{
		/*
		template<typename CharT, typename CharTraisT>
		Ebnf(std::basic_string_view<CharT, CharTraisT> EbnfStr);
		*/

		Ebnf(Ebnf&&) = default;

		Ebnf() = default;

		struct Config
		{
			std::pmr::memory_resource* temporary_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* resource = std::pmr::get_default_resource();
		};

		Ebnf(std::u8string_view ebnf_str, Config config = {});

		Reg::Dfa const& GetLexical() const { return Lexical; }
		SLRX::LRX const& GetSyntax() const { return Syntax; }

		struct RegInfoT
		{
			std::size_t MapSymbolValue;
			std::optional<std::size_t> UserMask;
		};

		RegInfoT GetRgeInfo(std::size_t Index) const { return RegMap[Index]; }
		std::u8string_view GetRegName(std::size_t Index) const;

	protected:

		std::u8string TotalString;

		struct SymbolMapT
		{
			Misc::IndexSpan<> StrIndex;
		};

		std::vector<SymbolMapT> SymbolMap;

		std::vector<RegInfoT> RegMap;

		Reg::Dfa Lexical;
		SLRX::LRX Syntax;

		friend struct EbnfProcessor;
		friend struct EbnfBinaryTableWrapper;
	};


	struct EbnfBinaryTableWrapper
	{
		using StandardT = std::uint32_t;

		struct HeadT
		{
			StandardT TotalNameOffset = 0;
			StandardT TotalNameCount = 0;
			StandardT SymbolMapOffset = 0;
			StandardT SymbolMapCount = 0;
			StandardT RegMapOffset = 0;
			StandardT RegMapCount = 0;
			StandardT LexicalOffset = 0;
			StandardT LexicalCount = 0;
			StandardT SyntaxOffset = 0;
			StandardT SyntaxCount = 0;
		};

		struct SymbolMapT
		{
			Misc::IndexSpan<StandardT> StrIndex;
		};

		struct RegInfoT
		{
			StandardT MapSymbolValue;
			StandardT UserMask;
		};

		EbnfBinaryTableWrapper() = default;
		EbnfBinaryTableWrapper(std::span<StandardT const> Buffer) : Buffer(Buffer) {}

		Ebnf::RegInfoT GetRgeInfo(std::size_t Index) const;
		std::u8string_view GetRegName(std::size_t Index) const;
		Reg::DfaBinaryTableWrapper GetLexicalTable() const;
		SLRX::LRXBinaryTableWrapper GetSyntaxTable() const;

		static void Serilize(Misc::StructedSerilizerWritter<StandardT>& Write, Ebnf const& Table);

		HeadT const* GetHead() const;

		std::span<StandardT const> Buffer;

		friend struct EbnfProcessor;
	};

	std::vector<EbnfBinaryTableWrapper::StandardT> CreateEbnfBinaryTable(Ebnf const& Table);

	struct EbnfProcessor : protected SLRX::ProcessorOperator
	{

		void Clear(std::size_t Startup = 0);
		std::size_t GetRequireTokenIndex() const { return RequireTokenIndex; }
		bool Consume(char32_t Value, std::size_t NextTokenIndex);
		bool EndOfFile();

		std::any& GetDataRaw() { return SyntaxProcessor.GetDataRaw(); }
		template<typename RequrieT>
		RequrieT GetData() { return SyntaxProcessor.GetData<RequrieT>(); }

		using HandleSymbolFuncT = TMP::FunctionRef<std::any(SymbolInfo Symbol, std::size_t UserMask)>;
		using HandleReduceFuncT = TMP::FunctionRef<std::any(SymbolInfo Symbol, ReduceProduction Production)>;

		void SetObserverTable(Ebnf const& Table, HandleSymbolFuncT HandleSymbol = {}, HandleReduceFuncT HandleReduce = {}, std::size_t StartupTokenIndex = 0);
		void SetObserverTable(EbnfBinaryTableWrapper Table, HandleSymbolFuncT HandleSymbol = {}, HandleReduceFuncT HandleReduce = {}, std::size_t StartupTokenIndex = 0);

		EbnfProcessor(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: LexicalProcessor(resource), SyntaxProcessor(resource), TempElement(resource) {}

	protected:

		std::tuple<SymbolInfo, std::size_t> Tranlate(std::size_t Mask, Misc::IndexSpan<> TokenIndex) const;
		SymbolInfo Tranlate(SLRX::Symbol Symbol, Misc::IndexSpan<> TokenIndex) const;

		bool AddTerminalSymbol(std::size_t RegIndex, Misc::IndexSpan<> TokenIndex);
		bool TerminalEndOfFile();
		std::any HandleReduce(SLRX::SymbolInfo Value, SLRX::ReduceProduction Desc);

		std::variant<
			std::monostate,
			Pointer::ObserverPtr<Ebnf const>,
			EbnfBinaryTableWrapper
		> TableWrapper;
		
		HandleSymbolFuncT HandleSymbolFunc;
		HandleReduceFuncT HandleReduceFunc;

		Reg::DfaProcessor LexicalProcessor;
		SLRX::LRXProcessor SyntaxProcessor;

		std::pmr::vector<ReduceProduction::Element> TempElement;
		std::size_t RequireTokenIndex = 0;
		std::size_t LastSymbolTokenIndex = 0;
	};

	template<typename CharT, typename CharTT>
	bool Process(EbnfProcessor& Pro, std::basic_string_view<CharT, CharTT> Str)
	{
		std::size_t Index = 0;
		char32_t InputValue = 0;
		std::span<char32_t> OutputBuffer{&InputValue, 1};
		std::size_t LastRequireSize = std::numeric_limits<std::size_t>::max();
		Misc::IndexSpan<> TokenIndex;
		while (true)
		{
			auto RequireSize = Pro.GetRequireTokenIndex();

			if (RequireSize < Str.size())
			{
				if (LastRequireSize != RequireSize)
				{
					auto InputSpan = Str.substr(RequireSize);
					Encode::EncodeCutOffSetting cutoff;
					cutoff.max_character_count = 1;
					auto EncodeInfo = Encode::UnicodeEncoder<CharT, char32_t>::EncodeTo(InputSpan, OutputBuffer, cutoff);
					TokenIndex = Misc::IndexSpan<>{ RequireSize, RequireSize + EncodeInfo.source_space };
					LastRequireSize = RequireSize;
				}

				if (!Pro.Consume(InputValue, TokenIndex.End()))
				{
					return false;
				}
			}
			else if (RequireSize == Str.size())
			{
				if (!Pro.EndOfFile())
					return false;
			}
			else {
				return true;
			}
		}
	}

	struct LexicalSymbol
	{
		SLRX::Symbol symbol;
		std::u8string_view name;
		Misc::IndexSpan<> token_index;
		std::size_t user_mask;
		bool IsIgnoredSymbol() const { return symbol.symbol == 0; }
	};


	struct LexicalProcessor
	{
		void SetObserverTable(Ebnf const& ebnf)
		{
			table_wrapper = &ebnf;
			lexical_processor.SetObserverTable(ebnf.GetLexical());
		}
		void SetObserverTable(EbnfBinaryTableWrapper ebnf_table)
		{
			table_wrapper = ebnf_table;
			lexical_processor.SetObserverTable(ebnf_table.GetLexicalTable());
		}

		struct ProcessResult
		{
			std::size_t input_consumed = 0;
			LexicalSymbol lecical_symbol;
			bool need_expand_buffer = false;
			bool accept = true;
		};

		ProcessResult FragmentProcess(
			std::span<Encode::Unicode::CodePointT const> input_code,
			bool is_single_span
		);

		void Clear() { lexical_processor.Clear(); }

	protected:

		LexicalSymbol Tranlate(std::size_t Mask, Misc::IndexSpan<> TokenIndex) const;

		std::variant<
			std::monostate,
			Pointer::ObserverPtr<Ebnf const>,
			EbnfBinaryTableWrapper
		> table_wrapper;

		Reg::DfaProcessor lexical_processor;
	};
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
		UnacceptableRegex(std::basic_string_view<CharT> InputRegex) {
			Encode::UnicodeEncoder<CharT, wchar_t>::EncodeTo(InputRegex, std::back_inserter(Regex));
		}
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
		std::pmr::wstring Str;
		UnacceptableEbnf(TypeE Type, std::pmr::wstring Str)
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
}