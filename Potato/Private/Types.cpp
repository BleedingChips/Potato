#include "../Public/Types.h"
namespace Potato::Types
{
	size_t LayoutStyle::MinAlign() const noexcept { return alignof(std::byte); };

	Layout LayoutStyle::Finalize(Layout owner) noexcept
	{
		auto mod = owner.size % owner.align;
		if (mod == 0)
			return owner;
		else
			return { owner.align, (owner.size + owner.align - mod) };
	}
	   
	LayoutStyle::Result LayoutFactory::InserMember(Layout member)
	{
		auto result = style_ref.InsertMember(scope, member);
		scope = result.owner;
		return result;
	}

	auto CLikeLayoutStyle::InsertMember(Layout owner, Layout member) noexcept -> Result
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

	auto HlslLikeLayOutStyle::InsertMember(Layout owner, Layout member) noexcept ->Result
	{
		assert(member.align == MinAlign());
		constexpr size_t AlignSize = MinAlign() * 4;
		Layout temp{ owner };
		size_t rever_size = AlignSize - temp.size % AlignSize;
		if (member.size >= AlignSize || rever_size < member.size)
			temp.size += rever_size;
		return { temp.size, {temp.align, temp.size + member.size} };
	}

}