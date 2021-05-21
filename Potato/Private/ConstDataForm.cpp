#include "../Public/ConstDataForm.h"
namespace Potato::ConstData
{
    Mask From::Insert(Types::BuildIn type, size_t count, Types::Layout layout, std::span<std::byte const> input_datas)
    {
        IndexSpan<> span(datas.size(), input_datas.size());
        datas.reserve(datas.size() + input_datas.size());
        Element ele{ type, layout, span, count };
        Mask result{ symbols.size()};
        symbols.push_back(ele);
        return result;
    }
}