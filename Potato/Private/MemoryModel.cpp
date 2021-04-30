#include "../Public/MemoryModel.h"
namespace Potato::MemoryModel
{
	size_t Style::MinAlign() const noexcept { return alignof(std::byte); };

	Property Style::Finalize(Property owner)
	{
		auto mod = owner.size % owner.align;
		if (mod == 0)
			return owner;
		else
			return { owner.align, (owner.size + owner.align - mod) };
	}
	   
	Style::Result Factory::InserMember(Property member)
	{
		auto result = style_ref.InsertMember(scope, member);
		scope = result.owner;
		return result;
	}

	Property Factory::Finalize() const noexcept
	{
		return style_ref.Finalize(scope);
	}

	auto CLikeStyle::InsertMember(Property owner, Property member) -> Result
	{
		auto align = std::max(owner.align, member.align);
		auto mod = owner.size % member.align;
		if (mod == 0)
		{
			return { owner.size, {align, owner.size + member.size} };
		}
		else {
			auto offset = owner.size + (member.align - mod);
			return { offset, {align, offset + member.size} };
		}
	}

	auto HlslStyle::InsertMember(Property owner, Property member)->Result
	{
		assert(member.align == MinAlign());
		constexpr size_t AlignSize = MinAlign() * 4;
		Property temp{ owner };
		size_t rever_size = AlignSize - temp.size % AlignSize;
		if (member.size >= AlignSize || rever_size < member.size)
			temp.size += rever_size;
		return { temp.size, {temp.align, temp.size + member.size} };
	}

}