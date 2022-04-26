

#include "Potato/Public/PotatoDLr.h"
#include "Potato/Public/PotatoReg.h"
#include "Potato/Public/PotatoStrEncode.h"
#include "Potato/Public/PotatoStrFormat.h"
#include "Potato/Public/PotatoEbnf.h"
#include <iostream>

using namespace Potato;

void TestingStrEncode();

void TestingStrFormat();

void TestingDLr();

void TestingReg();

void TestingEbnf();

struct UnpassedUnit : public std::exception
{
	char const* StrPtr = nullptr;
	UnpassedUnit(char const* StrPtr) : StrPtr(StrPtr) {}
	UnpassedUnit(UnpassedUnit const&) = default;
	virtual const char* what() const { return StrPtr; }
};

int main()
{

	try {
		TestingStrEncode();

		TestingDLr();

		TestingReg();

		TestingStrFormat();

		TestingEbnf();
	}
	catch (std::exception const& E)
	{
		std::cout << E.what() << std::endl;
		return -1;
	}
	std::cout << "All Passed" << std::endl;
	return 0;
}

template<typename ST, typename TT>
struct StrEncodeTesting
{
	bool operator()(std::basic_string_view<ST> Source, std::basic_string_view<TT> Target)
	{
		using namespace StrEncode;
		EncodeStrInfo I1 = StrEncode::StrCodeEncoder<ST, TT>::RequireSpace(Source);
		std::basic_string<TT> R1;
		R1.resize(I1.TargetSpace);
		StrEncode::StrCodeEncoder<ST, TT>::EncodeUnSafe(Source, R1, I1.CharacterCount);
		return R1 == Target;
	}
};


void TestingStrEncode()
{

	std::u32string_view TestingStr1 = UR"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";
	std::u16string_view TestingStr2 = uR"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";
	std::u8string_view TestingStr3 = u8R"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";
	std::wstring_view TestingStr4 = LR"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";

	if(!StrEncodeTesting<char32_t, char8_t>{}(TestingStr1, TestingStr3))
		throw UnpassedUnit{"TestingStrEncode : UTF32 To UTF8"};
	if (!StrEncodeTesting<char32_t, char16_t>{}(TestingStr1, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To UTF16" };
	if (!StrEncodeTesting<char32_t, wchar_t>{}(TestingStr1, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To wchar_t" };
	if (!StrEncodeTesting<char32_t, char32_t>{}(TestingStr1, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To UTF32" };
	if (!StrEncodeTesting<char16_t, char32_t>{}(TestingStr2, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To UTF32" };
	if (!StrEncodeTesting<char16_t, char16_t>{}(TestingStr2, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To UTF16" };
	if (!StrEncodeTesting<char16_t, char8_t>{}(TestingStr2, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To UTF8" };
	if (!StrEncodeTesting<char16_t, wchar_t>{}(TestingStr2, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To wchar_t" };
	if (!StrEncodeTesting<char8_t, char32_t>{}(TestingStr3, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To UTF32" };
	if (!StrEncodeTesting<char8_t, char16_t>{}(TestingStr3, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To UTF16" };
	if (!StrEncodeTesting<char8_t, char8_t>{}(TestingStr3, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To UTF8" };
	if (!StrEncodeTesting<char8_t, wchar_t>{}(TestingStr3, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To wchar_t" };
	if (!StrEncodeTesting<wchar_t, char32_t>{}(TestingStr4, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To UTF32" };
	if (!StrEncodeTesting<wchar_t, char16_t>{}(TestingStr4, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To UTF16" };
	if (!StrEncodeTesting<wchar_t, char8_t>{}(TestingStr4, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To UTF8" };
	if (!StrEncodeTesting<wchar_t, wchar_t>{}(TestingStr4, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To wchar_t" };

	std::wcout<< LR"(TestingStrEncode Pass !)" << std::endl;
}

enum class Noterminal
{
	Exp = 0,
};

constexpr DLr::Symbol operator*(Noterminal input) { return DLr::Symbol::AsNoTerminal(static_cast<std::size_t>(input)); }

enum class Terminal
{
	Num = 0,
	Add,
	Multi,
	Sub,
	Dev,
};

constexpr DLr::Symbol operator*(Terminal input) { return DLr::Symbol::AsTerminal(static_cast<std::size_t>(input)); }

void TestingDLr()
{

	using namespace DLr;

	struct SymbolTuple
	{
		Terminal Ope;
		int32_t Value;
	};

	std::vector<SymbolTuple> Wtf = {
		{Terminal::Num, 10},
		{Terminal::Add, 20},
		{Terminal::Num, 30},
		{Terminal::Multi, 40},
		{Terminal::Num, 50},
		{Terminal::Sub, 20},
		{Terminal::Num, 2}
	};

	Table tab(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Terminal::Num}, 1},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 2},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Sub, *Noterminal::Exp}, 4},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Dev, *Noterminal::Exp}, 5},
		},
		{
			{{*Terminal::Multi, *Terminal::Dev}},
			{{*Terminal::Add, *Terminal::Sub}, DLr::Associativity::Left},
		}
		);

	auto Stes = DLr::ProcessSymbol(tab.Wrapper, 0, [&](std::size_t Index)->Symbol{
		if (Index < Wtf.size())
			return *Wtf[Index].Ope;
		else
			return Symbol::EndOfFile();
	});

	auto Func = [&](StepElement Ele) -> std::any {
		if (Ele.IsNoTerminal())
		{
			auto NE = Ele.AsNoTerminal();
			switch (NE.Mask)
			{
			case 1:
				return NE[0].Consume();
			case 2:
				return NE[0].Consume<int32_t>() + NE[2].Consume<int32_t>();
			case 3:
				return NE[0].Consume<int32_t>() * NE[2].Consume<int32_t>();
			case 4:
				return NE[0].Consume<int32_t>() - NE[2].Consume<int32_t>();
			case 5:
				return NE[0].Consume<int32_t>() / NE[2].Consume<int32_t>();
			default:
				break;
			}
			return {};
		}
		else {
			auto TE = Ele.AsTerminal();
			return Wtf[TE.TokenIndex].Value;
		}
	};

	auto Result = DLr::ProcessStepWithOutputType<int32_t>(Stes, Func);

	if(Result != 1508)
		throw UnpassedUnit{ "TestingDLr : Bad Output Result" };

	std::vector<DLr::TableWrapper::HalfSeilizeT> L = {4, 2};
	std::vector<DLr::TableWrapper::SerilizedT> S;
	Misc::SerilizerHelper::WriteObjectArray(S, std::span(L));
	auto K = Misc::SerilizerHelper::ReadObjectArray<uint16_t>(std::span(L), 2);
	volatile int  i = 0;

	Table tab2(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Terminal::Num}, 1},
			{*Noterminal::Exp, {*Noterminal::Exp, 3, *Terminal::Add, *Noterminal::Exp, 3}, 2},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
			{*Noterminal::Exp, {*Noterminal::Exp, 3, *Terminal::Sub, *Noterminal::Exp, 3}, 4}
		},
		{}
		);
	
	auto Stes32 = DLr::ProcessSymbol(tab2.Wrapper, 0, [&](std::size_t Index)->Symbol {
		if (Index < Wtf.size())
			return *Wtf[Index].Ope;
		else
			return Symbol::EndOfFile();
		});

	auto Result2 = DLr::ProcessStepWithOutputType<int32_t>(Stes32, Func);

	if (Result2 != 1920)
		throw UnpassedUnit{ "TestingDLr : Bad Output Result2" };
	

	std::wcout << LR"(TestingDLr Pass !)" << std::endl;

}

void TestingStrFormat()
{

	using namespace StrFormat;

	FormatPattern<char32_t> Pattern(UR"(12345{}6789{{}})");
	std::u32string Output;
	int32_t Data = 10086;
	uint64_t Data2 = 10081;
	Pattern.Format(Output, Data, Data2);

	if(Output != UR"(12345100866789{10081})")
		throw UnpassedUnit{ "TestingStrFormat : Bad Format Output" };

	std::u32string_view Str = UR"(sdasdasd12445sdasdasd)";

	int32_t R1 = 0;

	SearchScan(UR"(([0-9]+))", Str, R1);

	if(R1 != 12445)
		throw UnpassedUnit{ "TestingStrFormat : Bad SearchScan Output" };

	uint64_t R2 = 0;

	MarchScan(UR"(sdasdasd([0-9]+)sdasdasd)", Str, R2);

	if (R2 != 12445)
		throw UnpassedUnit{ "TestingStrFormat : Bad SearchScan Output" };

	std::wcout << LR"(TestingStrFormat Pass !)" << std::endl;
}

void TestingReg()
{

	using namespace Reg;
	
	{
		std::u32string_view Str = UR"(475746)";
		Table Tab1(UR"([^123]+)");
		auto Result = ProcessMarch(Tab1.AsWrapper(), Str);
		if(!Result.has_value())
			throw UnpassedUnit{ "TestingReg : Bad Sample 1" };

		std::u32string_view Str2 = UR"(123475746)";
		auto Result2 = ProcessSearch(Tab1.AsWrapper(), Str2);
		if(!Result2.has_value() || Str2.substr(Result2->MainCapture.Begin(), Result2->MainCapture.Count()) != Str2.substr(3))
			throw UnpassedUnit{ "TestingReg : Bad Sample 2" };

		Table Tab2(UR"([123]{1,4})");
		std::u32string_view Str3 = UR"(645123475746)";
		auto Result3 = ProcessSearch(Tab2.AsWrapper(), Str3);
		if (!Result3.has_value() || Str3.substr(Result3->MainCapture.Begin(), Result3->MainCapture.Count()) != Str3.substr(3, 3))
			throw UnpassedUnit{ "TestingReg : Bad Sample 3" };

	}

	std::wcout << LR"(TestingReg Pass !)" << std::endl;
}

void TestingEbnf()
{
	std::u32string_view EbnfCode1 = 
UR"(
$ := '\s+'
Num := '[1-9][0-9]*' : [1]

%%%%

$ := <Exp>;

<Exp> := Num : [1];
	:= <Exp> '+' <Exp> : [2];
	:= <Exp> '*' <Exp> : [3];
	:= <Exp> '/' <Exp> : [4];
	:= <Exp> '-' <Exp> : [5];
	:= '(' <Exp> ')' : [6];

%%%%

+('*' '/') +('+' '-')
)";

	auto TableBuffer = Ebnf::TableWrapper::Create(EbnfCode1);

	auto Wrapper = Ebnf::TableWrapper{ TableBuffer };

	std::u32string_view Str = UR"(1+24+12*3-4/(1+1))";

	auto Symbols = Ebnf::ProcessSymbol(Wrapper, Str);

	auto Ite = Ebnf::ProcessStepWithOutputType<std::size_t>(Wrapper, Symbols, [=](Ebnf::StepElement Ele)->std::any {
		if (Ele.IsTerminal())
		{
			auto T = Ele.AsTerminal();
			if (T.Mask == 1)
			{
				std::size_t Result = 0;
				auto Syms = T.Index;
				StrFormat::DirectScan(Str.substr(Syms.Begin(), Syms.Count()), Result);
				return Result;
			}
		}
		else {
			auto NT = Ele.AsNoTerminal();
			switch (NT.Mask)
			{
			case 1:
				return NT[0].Consume();
			case 2:
				return NT[0].Consume<std::size_t>() + NT[2].Consume<std::size_t>();
			case 3:
				return NT[0].Consume<std::size_t>() * NT[2].Consume<std::size_t>();
			case 4:
				return NT[0].Consume<std::size_t>() / NT[2].Consume<std::size_t>();
			case 5:
				return NT[0].Consume<std::size_t>() - NT[2].Consume<std::size_t>();
			case 6:
				return NT[1].Consume();
			default:
				break;
			}
		}
		return {};
		});

	if(Ite != 59)
		throw UnpassedUnit{ "TestingEbnf : Bad Output Result" };

	std::wcout << LR"(TestingEbnf Pass !)" << std::endl;
}