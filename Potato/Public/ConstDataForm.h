#pragma once
#include <optional>
#include <string_view>
#include <cassert>
#include <vector>
#include "Misc.h"
#include "Types.h"
namespace Potato::ConstData
{
	struct Mask
	{
		operator bool() const noexcept { return storage.has_value(); }
		size_t Index() const noexcept { assert(*this); return *storage; }
		Mask() = default;
		Mask(Mask const&) = default;
		Mask(size_t index) : storage(index) {}
		std::partial_ordering operator <=> (Mask const& mask) { if (*this && mask) return Index() <=> mask.Index(); return std::partial_ordering::unordered; }
	private:
		std::optional<size_t> storage;
		friend struct Table;
	};

	struct From
	{

		struct Element
		{
			Types::BuildIn type;
			Types::Layout layout;
			Potato::IndexSpan<size_t> span;
			size_t array_count;
		};

		template<typename Type>
		Mask Insert(Type&& Input) requires(Types::BuildInEnumV<std::remove_cvref_t<Type>> != Types::BuildIn::UNKNOW) {
			return Insert(Types::BuildInEnumV<std::remove_cvref_t<Type>>, 0, Types::BuildInLayoutV<std::remove_cvref_t<Type>>, std::span<std::byte const>{reinterpret_cast<std::byte const*>(&Input), sizeof(std::remove_cvref_t<Type>)});
		}

		template<typename Type>
		Mask Insert(std::span<Type> Input) requires(Types::BuildInEnumV<std::remove_cvref_t<Type>> != Types::BuildIn::UNKNOW) {
			return Insert(Types::BuildInEnumV<std::remove_cvref_t<Type>>, Input.size(), Types::BuildInLayoutV<std::remove_cvref_t<Type>>, std::span<std::byte const>{reinterpret_cast<std::byte const*>(&Input), sizeof(std::remove_cvref_t<Type>) * Input.size()});
		}

		template<typename Type, typename Tri>
		Mask Insert(std::basic_string_view<Type, Tri> str) { return Insert(std::span{str.data(), str.size()});  }

		template<typename Type, typename All, typename Tri>
		Mask Insert(std::basic_string<Type, All, Tri> str) { return Insert(std::span{ str.data(), str.size() }); }

	private:

		Mask Insert(Types::BuildIn type, size_t count, Types::Layout layout, std::span<std::byte const> datas);

		std::vector<std::byte> datas;
		std::vector<Element> symbols;
	};
}