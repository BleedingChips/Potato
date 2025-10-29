
#include <cassert>

import PotatoIR;
import std;
import Potato;


struct K
{
	std::size_t k = 0;
	float I = 0.0f;
	std::size_t o[2] = {1, 2};
};

using namespace Potato;
using namespace Potato::IR;


struct test_class{

};

int main()
{


	auto j = StructLayout::GetStatic<test_class>();
	auto n = j->GetName();

	std::size_t u = 100;
	float icc = 100.0f;
	std::size_t kl[2] = {288, 899};
	
	StructLayout::Member me[] = 
	{
		{
			StaticAtomicStructLayout<std::size_t>::Create(),
			u8"k",
			1
		},
		{
			StaticAtomicStructLayout<float>::Create(),
			u8"I",
			1
		},
		{
			StaticAtomicStructLayout<std::size_t>::Create(),
			u8"o",
			2
		},
	};

	auto P = StructLayout::CreateDynamic(u8"K", me);

	K i;

	K pp[2];
	pp[0].I = 1;
	pp[0].o[0] = 199;
	pp[1].o[1] = 199;

	auto span = P->GetMemberView();

	auto ref1 = span[0].As<std::size_t>(&i);
	*ref1 = 10086;
	auto ref2 = span[1].As<float>(&i);
	*ref2 = 1.0f;

	assert(i.k == 10086);
	assert(i.I == 1.0f);

	auto ref3 = span[2].As<std::size_t>(&i);

	ref3[0] = 3;
	ref3[1] = 4;

	assert(i.o[0] == 3);
	assert(i.o[1] == 4);

	auto ref4 = span[2].As<std::size_t>(&i, 0);
	auto ref4_1 = span[2].As<std::size_t>(&i, 1);

	assert(*ref4 == 3);
	assert(*ref4_1 == 4);

	auto so = StructLayoutObject::CopyConstruct(P, &i);

	K* k = reinterpret_cast<K*>(so->GetObject());

	assert(k->I == i.I);
	assert(k->k == i.k);
	assert(k->o[0] == i.o[0]);
	assert(k->o[1] == i.o[1]);

	auto so2 = StructLayoutObject::CopyConstruct(*so);

	K* k2 = reinterpret_cast<K*>(so->GetObject());

	assert(k2->I == i.I);
	assert(k2->k == i.k);
	assert(k2->o[0] == i.o[0]);
	assert(k2->o[1] == i.o[1]);

	std::cout << "IR Pass !" << std::endl;

	return 0;
}