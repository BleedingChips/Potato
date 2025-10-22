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

	struct ArrayLayout
	{
		std::size_t count = 0;
		std::size_t each_element_offset = 0;

		bool IsAnArray() const { return count > 0; }
		std::optional<std::size_t> GetArrayCount() const { if (count > 0) return count; else return std::nullopt; }

		std::byte* GetElement(std::byte* buffer, std::size_t index) const { assert(index == 0 || index < count); return buffer + index * each_element_offset; };
		std::byte* GetElement(void* buffer, std::size_t index) const { return GetElement(static_cast<std::byte*>(buffer), index); };
		std::byte const* GetElement(std::byte const* buffer, std::size_t index) const { return GetElement(const_cast<std::byte*>(buffer), index); };
		std::byte const* GetElement(void const* buffer, std::size_t index) const { return GetElement(static_cast<std::byte const*>(buffer), index); };
		std::span<std::byte> GetSpanByte(std::byte* buffer, std::size_t max = std::numeric_limits<std::size_t>::max()) const {
			return std::span<std::byte>{buffer, buffer + std::clamp(count, std::size_t{ 1 }, max)};
		}

		std::span<std::byte const> GetSpanByte(std::byte const* buffer, std::size_t max = std::numeric_limits<std::size_t>::max()) const {
			return GetSpanByte(const_cast<std::byte*>(buffer), max);
		}
		bool operator==(ArrayLayout const&) const = default;
	};

	struct MermberLayout
	{
		std::size_t offset = 0;
		ArrayLayout array_layout;

		std::byte* GetMember(std::byte* buffer) const { return buffer + offset; }
		std::byte* GetMember(void* buffer) const { return GetMember(static_cast<std::byte*>(buffer)); }
		std::byte const* GetMember(std::byte const* buffer) const { return GetMember(const_cast<std::byte*>(buffer)); }
		std::byte const* GetMember(void const* buffer) const { return GetMember(static_cast<std::byte const*>(buffer)); }

		std::byte* GetMember(std::byte* buffer, std::size_t array_index) const { return array_layout.GetElement(GetMember(buffer), array_index); };
		std::byte* GetMember(void* buffer, std::size_t array_index) const { return GetMember(static_cast<std::byte*>(buffer), array_index); };

		std::byte const* GetMember(std::byte const* buffer, std::size_t array_index) const { return GetMember(const_cast<std::byte*>(buffer), array_index); };
		std::byte const* GetMember(void const* buffer, std::size_t array_index) const { return GetMember(static_cast<std::byte const*>(buffer), array_index); };

		std::span<std::byte> GetSpanByte(std::byte* buffer) const {
			return array_layout.GetSpanByte(GetMember(buffer));
		};

		std::span<std::byte const> GetSpanByte(std::byte const* buffer) const {
			return GetSpanByte(const_cast<std::byte*>(buffer));
		};

		bool operator==(MermberLayout const&) const = default;
	};

	constexpr std::optional<MermberLayout> CPPCombineMemberFunc(Layout&, Layout, std::size_t array_count);
	constexpr std::optional<MermberLayout> HLSLConstBufferCombineMemberFunc(Layout&, Layout, std::size_t array_count);
	constexpr std::optional<Layout> CPPCompleteLayoutFunc(Layout);
	constexpr std::optional<Layout> HLSLConstBufferCompleteLayoutFunc(Layout layout) { return CPPCompleteLayoutFunc(layout); }

	struct LayoutPolicyRef
	{
		using CombinationMemberFuncType = TMP::FunctionRef<std::optional<MermberLayout>(Layout& current, Layout member, std::size_t array_count)>;
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
		constexpr std::optional<MermberLayout> Combine(Layout& current_layout, Layout member_layout, std::size_t array_count = 0) const
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
		constexpr std::optional<MermberLayout> Combine(Layout member_layout, std::size_t array_count = 0)
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

	LayoutPolicyRef GetHLSLConstBufferPolicy() { return LayoutPolicyRef(HLSLConstBufferCombineMemberFunc, HLSLConstBufferCompleteLayoutFunc); }

	std::size_t AlignTo(std::size_t aglin, std::size_t target_size)
	{
		if (target_size % aglin == 0)
		{
			return target_size;
		}
		return target_size + aglin - (target_size % aglin);
	}

	constexpr std::optional<MermberLayout> CPPCombineMemberFunc(Layout& target_layout, Layout member, std::size_t array_count)
	{
		MermberLayout offset;

		offset.array_layout.count = array_count;
		offset.array_layout.each_element_offset = member.size;

		if (array_count > 1)
		{
			member.size *= array_count;
		}

		if (target_layout.align < member.align)
			target_layout.align = member.align;
		target_layout.size = AlignTo(member.align, target_layout.size);
		offset.offset = target_layout.size;
		target_layout.size += member.size;
		return offset;
	}

	constexpr std::optional<MermberLayout> HLSLConstBufferCombineMemberFunc(Layout& target_layout, Layout member, std::size_t array_count)
	{
		assert(member.align <= sizeof(float) * 4);
		if (member.align > sizeof(float) * 4)
			return std::nullopt;

		target_layout.align = sizeof(float) * 4;
		MermberLayout offset;

		if (array_count == 0)
		{
			offset.array_layout = { 0, member.size};
		}
		else {
			assert(false);
		}



		
		//offset.element_count = 1;
		//offset.next_element_offset = member.size;

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

	