import PotatoIR;
import std;


struct K
{
	std::size_t k = 0;
	float I = 0.0f;
	std::size_t o[2] = {1, 2};
};

using namespace Potato;
using namespace Potato::IR;

int main()
{
	
	StructLayout::Member me[] = 
	{
		{
			StructLayout::CreateAtomicStructLayout<std::size_t>(u8"size_t"),
			u8"k",
			std::nullopt
		},
		{
			StructLayout::CreateAtomicStructLayout<float>(u8"float"),
			u8"I",
			std::nullopt
		},
		{
			StructLayout::CreateAtomicStructLayout<std::size_t>(u8"float"),
			u8"o",
			2
		},
	};

	auto P = StructLayout::CreateDynamicStructLayout(u8"K", me);

	K i;

	auto span = P->GetMemberView();

	auto ref1 = P->GetDataAs<std::size_t>(span[0], &i);
	*ref1 = 10086;
	auto ref2 = P->GetDataAs<float>(span[1], &i);
	*ref2 = 1.0f;

	auto ref3 = P->GetDataAs<std::size_t>(span[2], &i);

	ref3[0] = 3;
	ref3[1] = 4;

	auto ref4 = P->GetDataSpanAs<std::size_t>(span[2], &i);


	std::cout << "TMP Pass !" << std::endl;

	return 0;
}