import Potato.EBNF;

using namespace Potato::EBNF;

struct StringMaker : public EbnfOperator
{
	virtual std::any HandleSymbol(SymbolInfo Symbol, std::size_t UserMask) override 
	{
		std::wstring Str = std::wstring{ Symbol.TokenIndex.Slice(TotalStr) };
		return Str;
	};
	virtual std::any HandleReduce(SymbolInfo Symbol, ReduceProduction Production) { 
		std::wstring Str = L"(";
		for (auto& Ite : Production.Elements)
		{
			Str += Ite.Consume<std::wstring>();
		}
		Str += L")";
		return Str;
	};
	std::wstring_view TotalStr;
};

void Test(std::wstring_view Table, std::wstring_view InputStr, std::wstring_view TargetReg, const char* Error)
{
	try {
		Ebnf Tab { Table };

		EbnfProcessor Pro;

		StringMaker Maker;
		Maker.TotalStr = InputStr;

		Pro.SetObserverTable(Tab, &Maker);

		auto Re = Process(Pro, InputStr);

		if (Re)
		{
			auto K = Pro.GetData<std::wstring>();
			if (K != TargetReg)
			{
				throw Error;
			}
		}
		else {
			throw Error;
		}

		auto Buffer = CreateEbnfBinaryTable(Tab);

		Pro.SetObserverTable(EbnfBinaryTableWrapper{Buffer}, &Maker);

		auto Re2 = Process(Pro, InputStr);

		if (Re2)
		{
			auto K = Pro.GetData<std::wstring>();
			if (K != TargetReg)
			{
				throw Error;
			}
		}
		else {
			throw Error;
		}

		volatile int i = 0;
	}
	catch (Exception::Interface const&)
	{
		throw Error;
	}
}



void TestingEbnf()
{
	
	std::wstring_view EbnfCode1 =
		LR"(

Num := '[1-9][0-9]*' : [1]
$ := '\s+'

%%%%

$ := <Exp> ;

<Exp> := Num : [1];
	:= <Exp> '+' <Exp> : [2];
	:= <Exp> '*' <Exp> : [3];
	:= <Exp> '/' <Exp> : [4];
	:= <Exp> '-' <Exp> : [5];
	:= '<' <Exp> '>' : [6];

%%%%

+('*' '/') +('+' '-')
)";

	
	std::wstring_view Source = LR"(1*< 2 + 3 > * 4)";


	Test(EbnfCode1, Source, LR"((((1)*(<((2)+(3))>))*(4)))", "TestingEbnf : Case 1");

	/*
	std::u8string_view EbnfCode2 =
		u8R"(
$ := '\s+'
Num := '[1-9][0-9]*' : [1]

%%%%

$ := <Exp> ;

<Exp> := Num : [1];
	:= <Exp> <Exp> $ : [2];

%%%%
)";

	std::u8string_view Source2 = u8R"(123 123 123 456)";

	Test(EbnfCode2, Source2, u8R"(((((123)(123))(123))(456)))", "TestingEbnf : Case 2");

	std::u8string_view EbnfCode3 =
		u8R"(
$ := '\s+'
Num := '[1-9][0-9]*' : [1]

%%%%

$ := <Exp> ;

<Exp> := Num | '+': [1];
	:= <Exp> <Exp> $ : [2];

%%%%
)";

	std::u8string_view Source3 = u8R"(123 + 123 123 456)";

	Test(EbnfCode3, Source3, u8R"((((((<|123>)(<|+>))(<|123>))(<|123>))(<|456>)))", "TestingEbnf : Case 3");
	*/
	std::wcout << LR"(TestingEbnf Pass !)" << std::endl;

}

int main()
{
	try
	{
		TestingEbnf();
	}
	catch (char const* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	return 0;
}