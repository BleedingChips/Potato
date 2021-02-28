#include "../Public/lr.h"

namespace Potato::Lr
{
	std::any History::operator()(std::function<std::any(NTElement&)> NTFunc, std::function<std::any(TElement&)> TFun) const
	{
		if (NTFunc && TFun)
		{
			std::vector<std::tuple<Symbol, std::any>> DataBuffer;
			for (auto& ite : steps)
			{
				if (ite.IsTerminal())
				{
					TElement ele{ ite };
					if (TFun)
					{
						auto Result = TFun(ele);
						DataBuffer.push_back({ ite.value, std::move(Result) });
					}
				}
				else {
					NTElement ele{ ite };
					assert(DataBuffer.size() >= ite.reduce.production_count);
					size_t CurrentAdress = DataBuffer.size() - ite.reduce.production_count;
					ele.datas = DataBuffer.data() + CurrentAdress;
					auto Result = NTFunc(ele);
					DataBuffer.resize(CurrentAdress);
					DataBuffer.push_back({ ite.value, std::move(Result) });
				}
			}
			assert(DataBuffer.size() == 1);
			return std::move(std::get<1>(DataBuffer[0]));
		}
		return {};
	}
}