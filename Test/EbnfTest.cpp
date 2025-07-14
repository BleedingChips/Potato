import std;
//import PotatoEBNF;

/*
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

Num := '[1-9][0-9]*' : [1];
$ := '\s+';

%%%%

$ := <Exp> ;

<Exp> := Num : [1];
	:= <Exp> '+' <Exp> : [2];
	:= <Exp> '*' <Exp> : [3];
	:= <Exp> '/' <Exp> : [4];
	:= <Exp> '-' <Exp> : [5];
	:= '<' <Exp> '>' : [6];

%%%%

+('*' '/');
+('+' '-');
)";

	
	std::wstring_view Source = LR"(1*< 2 + 3 > * 4)";

	std::wstring_view EbnfCode2 =
		LR"(
$ := '\s+';

%%%%

$ := <Exp> ;

<Exp> := [ 'a' ] : [1];
		//:= <Exp> ['b'] : [2];
		//:= <Exp> ('c') : [3];
		//:=  'd'|'e'|'f' : [4];
		//:= : [5];

)";

	Ebnf Tab2{ EbnfCode2 };

	std::wstring_view Input = LR"()";

	struct Maker : public EbnfOperator
	{
		virtual std::any HandleSymbol(SymbolInfo Symbol, std::size_t UserMask) override
		{
			volatile int i = 0;
			return {};
		};
		virtual std::any HandleReduce(SymbolInfo Symbol, ReduceProduction Production) {
			auto K = Production[0].Consume<BuilInStatement>();
			return {};
		};
	}mk;

	EbnfProcessor Pro2;
	Pro2.SetObserverTable(Tab2, &mk);

	auto Ki = Process(Pro2, Input);

	volatile int o = 0;

	std::wcout << LR"(TestingEbnf Pass !)" << std::endl;

}
*/

int main()
{
	/*
	try
	{
		TestingEbnf();
	}
	catch (char const* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	*/
	return 0;
}