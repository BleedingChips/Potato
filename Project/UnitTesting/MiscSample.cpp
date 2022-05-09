#include "Potato/Public/PotatoMisc.h"
#include "TestingTypes.h"
#include <iostream>
using namespace Potato::Misc;


void TestingMisc()
{
	

	struct Type1
	{
		int32_t Index;
		char32_t I;
		char o;
	};

	std::vector<int32_t> Datas = {1, 2, 3, 4, 5};

	std::vector<uint64_t> Buffer;

	Type1 T1;
	T1.Index = 100;
	T1.I = 1;
	T1.o = 2;

	// 将数据写入Buffer，若目标类型的对齐大于Buffer的对齐，则不保证目标数据的对齐，若小于
	SerilizerHelper::WriteObject(Buffer, T1);
	SerilizerHelper::WriteObjectArray(Buffer, std::span(Datas));

	auto Span = std::span(Buffer);

	auto Re = SerilizerHelper::ReadObject<Type1>(Span);
	if (Re->I != 1 || Re->o != 2 || Re->Index != 100)
	{
		throw UnpassedUnit{"TestingMisc Not Pass 1"};
	}

	auto Re2 = SerilizerHelper::ReadObjectArray<int32_t>(Re.LastSpan, Datas.size());

	if (!std::equal(Re2->begin(), Re2->end(), Datas.begin(), Datas.end()))
	{
		throw UnpassedUnit{ "TestingMisc Not Pass 2" };
	}

	std::cout << "Misc Pass!" << std::endl;
}