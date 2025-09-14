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
		template<typename Type>
		static constexpr Layout GetArray(std::size_t array_count) { return { alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>) * array_count }; }
		bool operator==(Layout const& l) const noexcept = default;
		std::strong_ordering operator<=>(Layout const& l2) const noexcept = default;
		Layout WithArray(std::size_t array_count = 1) const { return { align, size * array_count }; }
	};

	constexpr std::optional<Misc::IndexSpan<>> CPPCombineMemberFunc(Layout&, Layout, std::optional<std::size_t>);
	constexpr std::optional<Misc::IndexSpan<>> HLSLCombineMemberFunc(Layout&, Layout, std::optional<std::size_t>);
	constexpr std::optional<Layout> CPPCompleteLayoutFunc(Layout);

	struct LayoutPolicyRef
	{
		using CombinationMemberFuncType = TMP::FunctionRef<std::optional<Misc::IndexSpan<>>(Layout& current, Layout member, std::optional<std::size_t> array_count)>;
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
		constexpr std::optional<Misc::IndexSpan<>> Combine(Layout& current_layout, Layout member_layout, std::optional<std::size_t> array_count = std::nullopt) const
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
		constexpr std::optional<Misc::IndexSpan<>> Combine(Layout member_layout, std::optional<std::size_t> array_count = std::nullopt)
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

	constexpr std::optional<Misc::IndexSpan<>> CPPCombineMemberFunc(Layout& target_layout, Layout member, std::optional<std::size_t> array_count)
	{
		if (array_count.has_value())
		{
			if (*array_count > 1)
				member.size *= *array_count;
			else if (*array_count == 0)
				return Misc::IndexSpan<>{ target_layout.size, 0 };
		}
		if (target_layout.align < member.align)
			target_layout.align = member.align;
		if (target_layout.size % member.align != 0)
			target_layout.size += member.align - (target_layout.size % member.align);
		std::size_t Offset = target_layout.size;
		target_layout.size += member.size;
		return Misc::IndexSpan<>{ Offset, Offset + member.size };
	}

	constexpr std::optional<Misc::IndexSpan<>> HLSLCombineMemberFunc(Layout&, Layout, std::optional<std::size_t>)
	{
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

	