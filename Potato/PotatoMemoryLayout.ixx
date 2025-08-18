module;

#include <cassert>

export module PotatoMemLayout;

import std;

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

	enum class LayoutCategory
	{
		CLike,
		HLSLCBuffer
	};

	struct MemLayout
	{
		MemLayout(LayoutCategory category = LayoutCategory::CLike)
			: category(category) {
			if (category == LayoutCategory::HLSLCBuffer)
			{
				target.align = sizeof(float) * 4;
			}
		}
		MemLayout(Layout layout, LayoutCategory category = LayoutCategory::CLike)
			: target(layout), category(category) 
		{
			if (category == LayoutCategory::HLSLCBuffer)
			{
				target.align = sizeof(float) * 4;
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
}