module;

#include <cassert>

export module PotatoMemLayout;

import std;
import PotatoTMP;

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
		Layout WithArray(std::size_t array_count = 1) const { return {align, size * array_count}; }
	};

	constexpr std::optional<std::size_t> CPPCombinationMemberFunc(Layout&, Layout, std::optional<std::size_t>);
	constexpr std::optional<std::size_t> HLSLCombinationMemberFunc(Layout&, Layout, std::optional<std::size_t>);
	constexpr std::optional<Layout> CPPCompletionLayoutFunc(Layout);

	struct LayoutPolicyRef
	{
		using CombinationMemberFuncType = TMP::FunctionRef<std::optional<std::size_t>(Layout& current, Layout member, std::optional<std::size_t> array_count)>;
		using CompletionLayoutFuncType = TMP::FunctionRef<std::optional<Layout>(Layout current)>;
		constexpr LayoutPolicyRef(
			CombinationMemberFuncType combination_func = CPPCombinationMemberFunc,
			CompletionLayoutFuncType completion_func = CPPCompletionLayoutFunc
		)
			:combination_func(combination_func), completion_func(completion_func)
		{

		}
		constexpr LayoutPolicyRef(LayoutPolicyRef const&) = default;
		constexpr operator bool() const { return combination_func && completion_func; }
		constexpr std::optional<std::size_t> Combination(Layout& current_layout, Layout member_layout, std::optional<std::size_t> array_count = std::nullopt) const
		{
			return combination_func(current_layout, member_layout, array_count);
		}
		constexpr std::optional<Layout> Completion(Layout current_layout) const
		{
			return completion_func(current_layout);
		}
	protected:
		TMP::FunctionRef<std::optional<std::size_t>(Layout& current, Layout member, std::optional<std::size_t> array_count)> combination_func;
		TMP::FunctionRef<std::optional<Layout>(Layout current)> completion_func;
	};

	LayoutPolicyRef GetCPPLikePolicy() { return LayoutPolicyRef(CPPCombinationMemberFunc, CPPCompletionLayoutFunc); }
	
	//LayoutPolicyRef GetHLSLConstBufferPolicy() { return LayoutPolicyRef(CPPCombinationMemberFunc, CPPCompletionLayoutFunc); }

	enum class LayoutCategory
	{
		CLike,
		HLSLCBuffer
	};

	struct MemLayout
	{
		constexpr MemLayout(LayoutCategory category = LayoutCategory::CLike)
			: category(category) {
			if (category == LayoutCategory::HLSLCBuffer)
			{
				target.align = 64;
			}
		}
		MemLayout(Layout layout, LayoutCategory category = LayoutCategory::CLike)
			: target(layout), category(category) 
		{
			if (category == LayoutCategory::HLSLCBuffer)
			{
				target.align = sizeof(float);
			}
		}
		MemLayout(MemLayout const&) = default;
		MemLayout& operator=(MemLayout const&) = default;
		template<typename Type>
		static constexpr MemLayout Get(LayoutCategory category = LayoutCategory::CLike) { return { {alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>)}, category }; }
	
		constexpr std::size_t Insert(Layout const inserted_layout, std::size_t array_count = 1)
		{
			switch (category)
			{
			case LayoutCategory::CLike:
			{
				if (target.align < inserted_layout.align)
					target.align = inserted_layout.align;
				if (target.size % inserted_layout.align != 0)
					target.size += inserted_layout.align - (target.size % inserted_layout.align);
				std::size_t Offset = target.size;
				target.size += inserted_layout.size * array_count;
				return Offset;
			}
			case LayoutCategory::HLSLCBuffer:
			{
				
			}
			default:
				return std::numeric_limits<std::size_t>::max();
			}
		}

		inline constexpr Layout Get() const
		{
			auto ModedSize = (target.size % target.align);
			if (ModedSize != 0)
			{
				return {
					target.align,
					target.size + target.align - ModedSize
				};
			}
			return target;
		}

		inline constexpr Layout GetRawLayout() const
		{
			return target;
		}

		static inline constexpr Layout Sum(std::span<Layout> Layouts, LayoutCategory category = LayoutCategory::CLike)
		{
			MemLayout target{category};
			for (auto Ite : Layouts)
				target.Insert(Ite);
			return target.Get();
		}
	
	protected:
		Layout target;
		LayoutCategory category;
	};

	constexpr std::optional<std::size_t> CPPCombinationMemberFunc(Layout& target_layout, Layout member, std::optional<std::size_t> array_count)
	{
		if (array_count.has_value() && *array_count > 1)
		{
			member.size *= *array_count;
		}
		if (target_layout.align < member.align)
			target_layout.align = member.align;
		if (target_layout.size % member.align != 0)
			target_layout.size += member.align - (target_layout.size % member.align);
		std::size_t Offset = target_layout.size;
		target_layout.size += member.size;
		return Offset;
	}

	constexpr std::optional<std::size_t> HLSLCombinationMemberFunc(Layout&, Layout, std::optional<std::size_t>)
	{
		return std::nullopt;
	}

	constexpr std::optional<Layout> CPPCompletionLayoutFunc(Layout target_layout)
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