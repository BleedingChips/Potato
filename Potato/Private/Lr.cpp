#include "../Public/lr.h"

namespace Potato
{
	std::any LrHistory::operator()(std::function<std::any(LrNTElement&)> NTFunc, std::function<std::any(LrTElement&)> TFun) const
	{
		if (NTFunc && TFun)
		{
			std::vector<LrNTElementData> DataBuffer;
			for (auto& ite : steps)
			{
				if (ite.IsTerminal())
				{
					LrTElement ele{ ite };
					if (TFun)
					{
						auto Result = TFun(ele);
						DataBuffer.push_back({ ite.value, std::move(Result) });
					}
				}
				else {
					assert(DataBuffer.size() >= ite.reduce.production_count);
					size_t CurrentAdress = DataBuffer.size() - ite.reduce.production_count;
					LrNTElement ele{ ite, DataBuffer.data() + CurrentAdress};
					auto Result = NTFunc(ele);
					DataBuffer.resize(CurrentAdress);
					DataBuffer.push_back({ ite.value, std::move(Result) });
				}
			}
			assert(DataBuffer.size() == 1);
			return DataBuffer[0].Consume();
		}
		return {};
	}
}