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

	auto P = FileSystem::Current();
	auto Ite = P.FindFileFromParent(U"Potato", 6);

	Lexical::RegexInitTuple Rexs[] = {
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

	int result = std::any_cast<int>(Lr::Process(His, [&](Lr0::NTElement& E) -> std::any {
		switch (E.mask)
		{
		case 1: return E[0].Consume();
		case 2: return E[0].Consume<int>() + E[2].Consume<int>();
		case 3: return E[0].Consume<int>() * E[2].Consume<int>();
		default:
			break;
		}
		return {};
	},
	[&](Lr0::TElement& E) -> std::any
	{
		if (E.value == *Terminal::Num)
			return Datas[E.token_index];
		return {};
	}
	));

	std::cout << result << std::endl;
	
	Ebnf::Table tab2 = Ebnf::CreateTable(EbnfCode1());
	//auto His2 = Ebnf::Process(tab2, U"1 + 2 + 3 * 4 - 4 / 2 + 2 * +3 * -2");
	auto His2 = Ebnf::Process(tab2, U" 1 + 23 * !!!#123");
	int result2 = std::any_cast<int>(Ebnf::Process(His2, [](Ebnf::NTElement& e) -> std::any {
		if (!e.IsPredefine())
		{
			switch (e.mask)
			{
			case 1: return e[0].Consume();
			case 2: return e[0].Consume<int>() + e[2].Consume<int>();
			case 3: return e[0].Consume<int>() * e[2].Consume<int>();
			case 4: return e[0].Consume<int>() / e[2].Consume<int>();
			case 5: return e[0].Consume<int>() - e[2].Consume<int>();
			case 6: return e[0].Consume();
			case 7: return int(0);
			}
		}
		return {};
	},
	[](Ebnf::TElement& e)->std::any
	{
		if (e.mask == 1)
		{
			int Data;
			StrScanner::DirectProcess(e.capture, Data);
			return Data;
		}
		return {};
	}
	));

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
	:= {'!'} {'&'} '#' Num : [7]

%%%

('*' '/') ('+' '-')
)";
}