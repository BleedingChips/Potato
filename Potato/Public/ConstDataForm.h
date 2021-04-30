#pragma once
#include <optional>
#include <string_view>
#include <cassert>
namespace Potato::ConstData
{
	struct Mask
	{
		operator bool() const noexcept { return storage.has_value(); }
		std::u32string_view Name() const noexcept { assert(*this); return storage->name; }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		Mask() = default;
		Mask(Mask const&) = default;
		Mask(size_t index, std::u32string_view name) : storage(Storage{ index, name }) {}
		std::partial_ordering operator <=> (Mask const& mask) { if (*this && mask) return Index() <=> mask.Index(); return std::partial_ordering::unordered; }
	private:
		struct Storage
		{
			size_t index;
			std::u32string_view name;
		};
		std::optional<Storage> storage;
		friend struct Table;
	};
}