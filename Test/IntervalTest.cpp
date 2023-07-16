

import PotatoInterval;
import PotatoMisc;

using namespace Potato::Misc;

int main()
{

	IntervalT<int32_t> Inter1({1, 2, 4, {4, 6}, {7, 8} });
	IntervalT<int32_t> Inter2({{5, 7}, {8, 9}, {7, 8}});

	auto P = Inter1 + Inter2;
	auto K = Inter2 - Inter1;
	auto C = Inter2 & Inter1;

	IntervalT<int32_t> Inter21({ {1, 2} });
	IntervalT<int32_t> Inter31({ {2, 3 } });

	auto K2 = Inter21 & Inter31;

	volatile int i = 0;
}