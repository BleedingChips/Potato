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
			"k",
			1
		},
		{
			StaticAtomicStructLayout<float>::Create(),
			"I",
			1
		},
		{
			StaticAtomicStructLayout<std::size_t>::Create(),
			"o",
			2
		},
	};

	auto P = StructLayout::CreateDynamic("K", me);

	K i;

	K pp[2];
	pp[0].I = 1;
	pp[0].o[0] = 199;
	pp[1].o[1] = 199;

	auto span = P->GetMemberView();

	auto ref1 = P->GetMemberDataWithStaticCast<std::size_t>(span[0], &i);
	*ref1 = 10086;
	auto ref2 = P->GetMemberDataWithStaticCast<float>(span[1], &i);
	*ref2 = 1.0f;

	auto ref3 = P->GetMemberDataWithStaticCast<std::size_t>(span[2], &i);

	ref3[0] = 3;
	ref3[1] = 4;

	auto ref4 = P->GetMemberDataArrayWithStaticCast<std::size_t>(span[2], &i);

	auto iop = StructLayoutObject::DefaultConstruct(P, 2);

	K* iop2 = static_cast<K*>(iop->GetArrayData());
	K* iop3 = static_cast<K*>(iop->GetArrayData(1));

	auto socc = StructLayoutObject::CopyConstruct(P, &pp, 2);

	K* iop22 = static_cast<K*>(socc->GetArrayData());
	K* iop32 = static_cast<K*>(socc->GetArrayData(1));

	auto so = StructLayoutObject::CopyConstruct(P, &i);

	K* io = static_cast<K*>(so->GetArrayData());

	auto so2 = StructLayoutObject::CopyConstruct(*so);


	K* io2 = static_cast<K*>(so2->GetArrayData());

	std::cout << "IR Pass !" << std::endl;

	return 0;
}