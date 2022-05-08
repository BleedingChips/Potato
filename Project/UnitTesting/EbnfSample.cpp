#include "TestingTypes.h"
#include "Potato/Public/PotatoEbnf.h"
#include "Potato/Public/PotatoStrFormat.h"

using namespace Potato::Ebnf;

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

	auto TableBuffer = TableWrapper::Create(EbnfCode1);

	auto Wrapper = TableWrapper{ TableBuffer };

	std::u32string_view Str = UR"(1+24+12*3-4/(1+1))";

	auto Symbols = ProcessSymbol(Wrapper, Str);

	auto Ite = ProcessStepWithOutputType<std::size_t>(Wrapper, Symbols, [=](StepElement Ele)->std::any {
		if (Ele.IsTerminal())
		{
			auto T = Ele.AsTerminal();
			if (T.Mask == 1)
			{
				std::size_t Result = 0;
				auto Syms = T.Index;
				Potato::StrFormat::DirectScan(Str.substr(Syms.Begin(), Syms.Count()), Result);
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

	if (Ite != 59)
		throw UnpassedUnit{ "TestingEbnf : Bad Output Result" };

	std::wcout << LR"(TestingEbnf Pass !)" << std::endl;
}