import Potato.EBNF;

using namespace Potato::EBNF;


std::u8string HandleCondiction(Condition Tions)
{
	std::u8string Result;
	switch (Tions.Type)
	{
	case Condition::TypeT::Or:
		Result += u8"<|";
		break;
	case Condition::TypeT::Parentheses:
		Result += u8"<{";
		break;
	case Condition::TypeT::SquareBrackets:
		Result += u8"<[";
		break;
	}
	for (auto& Ite : Tions.Datas)
	{
		auto Re = Ite.TryConsume<Condition>();
		if (Re.has_value())
		{
			Result += HandleCondiction(std::move(*Re));
		}
		else {
			Result += Ite.Consume<std::u8string>();
		}
	}
	Result += u8">";
	return Result;
}

struct StringMaker
{
	struct PredictEle
	{
		ParsingStep::ReduceT Reudce;
		std::size_t LastTokenIndex;
	};

	StringMaker(const char* Error) : Error(Error) {}

	std::vector<PredictEle> PredictReduceStack;

	std::any operator()(VariantElement Element) {
		if (Element.IsTerminal())
		{
			++NextTokenIndex;
			auto Ele = Element.AsTerminal();
			return std::u8string{ Ele.Shift.CaptureValue };
		}
		else if (Element.IsNoTerminal())
		{
			auto Ele = Element.AsNoTerminal();
			bool HasPredict = false;

			std::u8string Tem;

			if (!PredictReduceStack.empty() && PredictReduceStack.rbegin()->Reudce.UniqueReduceID == Ele.Reduce.UniqueReduceID)
			{
				auto Last = *PredictReduceStack.rbegin();
				PredictReduceStack.pop_back();
				Tem += u8"&(";
			}
			else {
				Tem += u8"(";
			}
			for (auto& Ite : Ele.Datas)
			{
				auto Re = Ite.TryConsume<Condition>();
				if (Re.has_value())
				{
					Tem += HandleCondiction(std::move(std::move(*Re)));
				}else
					Tem += Ite.Consume<std::u8string>();
			}
			Tem += u8")";
			return Tem;
		}
		else if (Element.IsPredict())
		{
			auto Ele = Element.AsPredict();
			PredictReduceStack.push_back({ Ele.Reduce, NextTokenIndex });
			return {};
		}
		else {
			return {};
		}
	}

	std::size_t NextTokenIndex = 0;

	const char* Error;
};

void Test(std::u8string_view Table, std::u8string_view InputStr, std::u8string_view TargetReg, const char* Error)
{
	try {
		EBNFX Tab = EBNFX::Create(Table);

		{
			auto IteStr = InputStr;

			LexicalProcessor Pro{ Tab };

			while (!IteStr.empty())
			{
				auto Re = Pro.Consume(IteStr, InputStr.size() - IteStr.size());
				if (Re.has_value())
				{
					IteStr = *Re;
				}
				else {
					throw Error;
				}
			}

			auto Steps = SyntaxProcessor::Process(Tab, Pro.GetSpan());

			if (!Steps)
			{
				throw Error;
			}

			StringMaker Maker(Error);

			auto Strs = ProcessParsingStepWithOutputType<std::u8string>(*Steps, Maker);

			if(Strs != TargetReg)
				throw Error;
		}

		{
			auto TableBuffer = TableWrapper::Create(Tab);

			LexicalProcessor Pro{ TableWrapper{TableBuffer} };

			auto IteStr = InputStr;

			while (!IteStr.empty())
			{
				auto Re = Pro.Consume(IteStr, InputStr.size() - IteStr.size());
				if (Re.has_value())
				{
					IteStr = *Re;
				}
				else {
					throw Error;
				}
			}

			auto Steps = SyntaxProcessor::Process(Tab, Pro.GetSpan());

			if (!Steps)
			{
				throw Error;
			}

			StringMaker Maker(Error);

			auto Strs = ProcessParsingStepWithOutputType<std::u8string>(*Steps, Maker);

			if (Strs != TargetReg)
				throw Error;
		}
	}
	catch (Exception::Interface const&)
	{
		throw Error;
	}
}



void TestingEbnf()
{
	
	std::u8string_view EbnfCode1 =
		u8R"(
$ := '\s+'
Num := '[1-9][0-9]*' : [1]

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

	std::u8string_view Source = u8R"(1*< 2 + 3 > * 4)";


	Test(EbnfCode1, Source, u8R"((((1)*(<((2)+(3))>))*(4)))", "TestingEbnf : Case 1");

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