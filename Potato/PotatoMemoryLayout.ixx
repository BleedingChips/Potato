module;

#include <cassert>

export module PotatoMemLayout;

import std;
import PotatoTMP;
import PotatoMisc;

export namespace Potato::MemLayout
{
	struct Layout
	{
		std::size_t align = 1;
		std::size_t size = 0;

		template<typename Type>
		static constexpr Layout Get() { return { alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>) }; }
		bool operator==(Layout const& l) const noexcept = default;
		std::strong_ordering operator<=>(Layout const& l2) const noexcept = default;
	};

	struct MermberOffset
	{
		std::size_t buffer_offset;
		std::size_t element_count;
		std::size_t next_element_offset;

		std::byte* GetMember(std::byte* buffer, std::size_t array_index) const { assert(array_index < element_count);  return buffer + buffer_offset + array_index * next_element_offset; };
		std::byte* GetMember(std::byte* buffer) const { return buffer + buffer_offset; }
		std::byte const* GetMember(std::byte const* buffer, std::size_t array_index = 0) const { assert(array_index < element_count);  return buffer + buffer_offset + array_index * next_element_offset; };
		std::byte const * GetMember(std::byte const* buffer) const { return buffer + buffer_offset; }
		std::span<std::byte> GetMemberBuffer(std::byte* buffer) const {
			return std::span<std::byte>{ buffer + buffer_offset, buffer + buffer_offset + element_count * next_element_offset };
		};
		std::span<std::byte const> GetMemberBuffer(std::byte const* buffer) const {
			return std::span<std::byte const>{ buffer + buffer_offset, buffer + buffer_offset + element_count * next_element_offset };
		};

		bool operator==(MermberOffset const&) const = default;
	};

	constexpr std::optional<MermberOffset> CPPCombineMemberFunc(Layout&, Layout, std::optional<std::size_t>);
	constexpr std::optional<MermberOffset> HLSLConstBufferCombineMemberFunc(Layout&, Layout, std::optional<std::size_t>);
	constexpr std::optional<Layout> CPPCompleteLayoutFunc(Layout);

	struct LayoutPolicyRef
	{
		using CombinationMemberFuncType = TMP::FunctionRef<std::optional<MermberOffset>(Layout& current, Layout member, std::optional<std::size_t> array_count)>;
		using CompletionLayoutFuncType = TMP::FunctionRef<std::optional<Layout>(Layout current)>;
		constexpr LayoutPolicyRef(
			CombinationMemberFuncType combine_func = CPPCombineMemberFunc,
			CompletionLayoutFuncType complete_func = CPPCompleteLayoutFunc
		)
			:combine_func(combine_func), complete_func(complete_func)
		{

		}
		constexpr LayoutPolicyRef(LayoutPolicyRef const&) = default;
		constexpr operator bool() const { return combine_func && complete_func; }
		constexpr std::optional<MermberOffset> Combine(Layout& current_layout, Layout member_layout, std::optional<std::size_t> array_count = std::nullopt) const
		{
			return combine_func(current_layout, member_layout, array_count);
		}
		constexpr std::optional<Layout> Complete(Layout current_layout) const
		{
			return complete_func(current_layout);
		}
	protected:
		CombinationMemberFuncType combine_func;
		CompletionLayoutFuncType complete_func;
	};

	struct PolicyLayout
	{
		PolicyLayout(Layout& layout_reference, LayoutPolicyRef ref = {})
			: layout(layout_reference), policy(std::move(ref))
		{
		}
		constexpr std::optional<MermberOffset> Combine(Layout member_layout, std::optional<std::size_t> array_count = std::nullopt)
		{
			return policy.Combine(layout, member_layout, array_count);
		}
		constexpr std::optional<Layout> Complete() const
		{
			return policy.Complete(layout);
		}
	protected:
		Layout& layout;
		LayoutPolicyRef policy;
	};

	LayoutPolicyRef GetCPPLikePolicy() { return LayoutPolicyRef(CPPCombineMemberFunc, CPPCompleteLayoutFunc); }

	//LayoutPolicyRef GetHLSLConstBufferPolicy() { return LayoutPolicyRef(CPPCombinationMemberFunc, CPPCompletionLayoutFunc); }

	constexpr std::optional<MermberOffset> CPPCombineMemberFunc(Layout& target_layout, Layout member, std::optional<std::size_t> array_count)
	{
		MermberOffset offset;

		offset.element_count = 1;
		offset.next_element_offset = member.size;

		if (array_count.has_value())
		{
			offset.element_count = *array_count;
			if (*array_count == 0)
				return std::nullopt;

			if (*array_count > 1)
				member.size *= *array_count;
		}

		if (target_layout.align < member.align)
			target_layout.align = member.align;
		if (target_layout.size % member.align != 0)
			target_layout.size += member.align - (target_layout.size % member.align);
		offset.buffer_offset = target_layout.size;
		target_layout.size += member.size;
		return offset;
	}

	constexpr std::optional<MermberOffset> HLSLConstBufferCombineMemberFunc(Layout& target_layout, Layout member, std::optional<std::size_t> array_count)
	{

		if (array_count.has_value() && *array_count == 0)
			return std::nullopt;

		if (member.align >= sizeof(float) * 4)
			return std::nullopt;

		MermberOffset offset;
		offset.element_count = 1;
		offset.next_element_offset = member.size;

		if (array_count.has_value())
		{
			offset.element_count = *array_count;

			if (*array_count > 1)
			{
				if (member.align % member.size != 0)
				{
					auto fill = (member.size % member.align);
					member.size += member.align - fill;
				}
				else {
					offset.next_element_offset = std::max(member.align, member.size);
				}
				if (member.size % member.align == 0)
				{
					member.align += 1;
				}
			}
				
		}

		return std::nullopt;
	}

	constexpr std::optional<Layout> CPPCompleteLayoutFunc(Layout target_layout)
	{
		auto moded_size = (target_layout.size % target_layout.align);
		if (moded_size != 0)
		{
			return Layout{
				target_layout.align,
				target_layout.size + target_layout.align - moded_size
			};
		}
		return target_layout;
	}
}

	