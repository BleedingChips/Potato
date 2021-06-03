#include "../Public/HIR.h"
#include <cassert>
namespace Potato::HIR
{
    Mask HLForm::InserConstData(TypeReference desc, Layout layout, std::span<std::byte const> data)
    {
        ConstElement element{std::move(desc), {const_buffer.size(), data.size()}, layout};
        size_t old_size = const_buffer.size();
        const_buffer.resize(const_buffer.size() + data.size());
        std::memcpy(const_buffer.data() + old_size, data.data(), data.size());
        Mask mask{ Mask::Type::CONST, const_elements.size()};
        const_elements.push_back(std::move(element));
        return mask;
    }

    Mask HLForm::ForwardDefineTypeproperty()
    {
        Mask mask(Mask::Type::TYPE, type_define.size());
        return mask;
    }
}