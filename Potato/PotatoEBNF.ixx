module;

#include <cassert>

export module PotatoEBNF;

export import PotatoReg;
export import PotatoEncode;
export import PotatoFormat;
export import PotatoSTD;

export namespace Potato::EBNF
{
	
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

	std::wstring_view OrStatementRegName();
	std::wstring_view RoundBracketStatementRegName();
	std::wstring_view SquareBracketStatementRegName();
	std::wstring_view CurlyBracketStatementRegName();

	struct SymbolInfo
	{
		SLRX::Symbol Symbol;
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

	struct EbnfOperator
	{
		virtual std::any HandleSymbol(SymbolInfo Symbol, std::size_t UserMask) { return {}; };
		virtual std::any HandleReduce(SymbolInfo Symbol, ReduceProduction Production) { return {}; };
	};

	struct Ebnf
	{
		template<typename CharT, typename CharTraisT>
		Ebnf(std::basic_string_view<CharT, CharTraisT> EbnfStr);

		Ebnf(Ebnf&&) = default;

		Ebnf() = default;

		Reg::Dfa const& GetLexical() const { return Lexical; }
		SLRX::LRX const& GetSyntax() const { return Syntax; }

		struct RegInfoT
		{
			std::size_t MapSymbolValue;
			std::optional<std::size_t> UserMask;
		};

		RegInfoT GetRgeInfo(std::size_t Index) const { return RegMap[Index]; }
		std::wstring_view GetRegName(std::size_t Index) const;

	protected:

		std::wstring TotalString;

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
		std::wstring_view GetRegName(std::size_t Index) const;
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

		void SetObserverTable(Ebnf const& Table, Pointer::ObserverPtr<EbnfOperator> Ope, std::size_t StartupTokenIndex = 0);
		void SetObserverTable(EbnfBinaryTableWrapper Table, Pointer::ObserverPtr<EbnfOperator> Ope, std::size_t StartupTokenIndex = 0);

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

		Pointer::ObserverPtr<EbnfOperator> Operator;

		Reg::DfaProcessor LexicalProcessor;
		SLRX::LRXProcessor SyntaxProcessor;

		std::vector<ReduceProduction::Element> TempElement;
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
					auto EncodeInfo = Encode::CharEncoder<CharT, char32_t>::EncodeOnceUnSafe(InputSpan, OutputBuffer);
					TokenIndex = Misc::IndexSpan<>{ RequireSize, RequireSize + EncodeInfo.SourceSpace };
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
					Gen.AppendReg(RegStr, false, RegMap.size());
					RegMap.push_back({ IIte->second, UserMask });
					break;
				}
				case EbnfBuilder::RegTypeE::Empty:
				{
					auto RegStr = Ite.Reg.SubIndex(1, Ite.Reg.Size() - 2).Slice(EbnfStr);
					Gen.AppendReg(RegStr, false, RegMap.size());
					RegMap.push_back({ 0, 0 });
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
				else if(Ite.StartSymbol.ElementType == EbnfBuilder::ElementTypeE::Temporary)
				{
					ProdutionMask = Ite.UserMask.Begin();
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
}