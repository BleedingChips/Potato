#include "Potato/Public/Ebnf.h"
#include "Potato/Public/StrScanner.h"
#include "Potato/Public/FileSystem.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace Potato;

enum class Noterminal
{
	Exp = 0,
};

constexpr Lr0::Symbol operator*(Noterminal input) { return Lr0::Symbol(static_cast<size_t>(input), Lr0::noterminal); }

enum class Terminal
{
	Num = 0,
	Add,
	Multi
};

constexpr Lr0::Symbol operator*(Terminal input) { return Lr0::Symbol(static_cast<size_t>(input), Lr0::terminal); }

std::u32string_view EbnfCode1();

int main()
{

	Lexical::LexicalRegexInitTuple Rexs[] = {
		{UR"((\+|\-)?[1-9][0-9]*)", },
		{UR"(\+)", },
		{UR"(\*)", },
		{UR"(\s)", Lexical::DefaultIgnoreMask()},
	};

	Lexical::Table LexicalTable1 = Lexical::CreateLexicalFromRegexs(Rexs, std::size(Rexs));

	auto StrElement = LexicalTable1.Process(UR"(1 + +10 * -2)");

	std::vector<Lr0::Symbol> Syms;
	std::vector<int> Datas;

	for (auto& Ite : StrElement)
	{
		if (Ite.acception != 3)
		{
			Syms.push_back(Lr0::Symbol(Ite.acception, Lr0::TerminalT{}));
			if (Ite.acception == 0)
			{
				int data = 0;
				StrScanner::DirectProcess(Ite.capture, data);
				Datas.push_back(data);
			}
			else
				Datas.push_back(0);
		}
	}

	Lr0::Table tab = Lr0::CreateTable(
		*Noterminal::Exp,
	{
		{{*Noterminal::Exp, *Terminal::Num}, 1},
		{{*Noterminal::Exp, *Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 2},
		{{*Noterminal::Exp, *Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
	},
		{ {{*Terminal::Multi}}, {{*Terminal::Add}} }
	);

	auto His = Lr0::Process(tab, Syms.data(), Syms.size());

	int result = std::any_cast<int>(Lr0::Process(His, [&](Lr0::Element& E) -> std::any {
		if (E.IsTerminal())
		{
			if (E.value == *Terminal::Num)
				return Datas[E.shift.token_index];
		}
		else {
			switch (E.reduce.mask)
			{
			case 1: return std::move(E.GetRawData(0));
			case 2: return E.GetData<int>(0) + E.GetData<int>(2);
			case 3: return E.GetData<int>(0) * E.GetData<int>(2);
			default:
				break;
			}
		}
		return {};
	}));

	std::cout << result << std::endl;
	
	Ebnf::Table tab2 = Ebnf::CreateTable(EbnfCode1());
	auto His2 = Ebnf::Process(tab2, U"1 + 2 + 3 * 4 - 4 / 2 + 2 * +3 * -2");
	int result2 = std::any_cast<int>(Ebnf::Process(His2, [](Ebnf::Element& e) -> std::any {
		if (e.IsTerminal())
		{
			if (e.shift.mask == 1)
			{
				int Data;
				StrScanner::DirectProcess(e.shift.capture, Data);
				return Data;
			}
		}
		else {
			switch (e.reduce.mask)
			{
			case 1: return e[0].MoveRawData();
			case 2: return e[0].GetData<int>() + e[2].GetData<int>();
			case 3: return e[0].GetData<int>() * e[2].GetData<int>();
			case 4: return e[0].GetData<int>() / e[2].GetData<int>();
			case 5: return e[0].GetData<int>() - e[2].GetData<int>();
			case 6: return e[0].MoveRawData();
			}
		}
		return {};
	}));

	std::cout << result2 << std::endl;
}

std::u32string_view EbnfCode1()
{
	return UR"(
^ := '\s'
Num := '(\+|\-)?[1-9][0-9]*' : [1]

%%%

$ := <Exp>

<Exp> := Num : [1]
    := <Exp> '+' <Exp> : [2]
    := <Exp> '*' <Exp> : [3]
    := <Exp> '/' <Exp> : [4]
    := <Exp> '-' <Exp> : [5]
    := '(' <Exp> ')' : [6]

%%%

('*' '/') ('+' '-')
)";
}