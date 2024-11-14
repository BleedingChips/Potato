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
	};

	struct MemLayoutC
	{
		MemLayoutC() = default;
		MemLayoutC(MemLayoutC&&) = default;
		MemLayoutC(MemLayoutC const&) = default;
		MemLayoutC(Layout layout) : target(layout) {}
		MemLayoutC(std::size_t align, std::size_t size) : target(align, size) {}

		Layout target;

		template<typename Type>
		static constexpr MemLayoutC Get() { return { alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>) }; }

		constexpr std::size_t Insert(Layout const Inserted)
		{
			if (target.align < Inserted.align)
				target.align = Inserted.align;
			if (target.size % Inserted.align != 0)
				target.size += Inserted.align - (target.size % Inserted.align);
			std::size_t Offset = target.size;
			target.size += Inserted.size;
			return Offset;
		}

		inline constexpr std::size_t Insert(Layout const Inserted, std::size_t ArrayCount)
		{
			if (target.align < Inserted.align)
				target.align = Inserted.align;
			if (target.size % Inserted.align != 0)
				target.size += Inserted.align - (target.size % Inserted.align);
			std::size_t Offset = target.size;
			target.size += Inserted.size * ArrayCount;
			return Offset;
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

		static inline constexpr Layout Sum(std::span<Layout> Layouts)
		{
			MemLayoutC target;
			for (auto Ite : Layouts)
				target.Insert(Ite);
			return target.Get();
		}

		MemLayoutC& operator= (Layout const& layout) { target = layout; return *this; }
		MemLayoutC& operator= (MemLayoutC const& input) = default;
	};

	using MemLayoutCPP = MemLayoutC;

}