#pragma once
#include <string_view>
#include <map>
#include <vector>
#include <optional>
#include <cassert>
#include <variant>
#include <typeindex>
#include <any>
#include "Lexical.h"
#include <optional>
#include "Misc.h"
#include "tmp.h"
#include <span>
#include <cassert>
namespace Potato::Grammar
{

	using Section = Lexical::Section;

	struct SymbolMask
	{
		operator bool() const noexcept{ return storage.has_value(); }
		std::u32string_view Name() const noexcept { assert(*this); return storage->name; }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		SymbolMask() = default;
		SymbolMask(SymbolMask const&) = default;
		SymbolMask(size_t index, std::u32string_view name) : storage(Storage{index, name}){}
		std::partial_ordering operator <=> (SymbolMask const& mask) { if(*this && mask) return Index() <=> mask.Index(); return std::partial_ordering::unordered; }
	private:
		struct Storage
		{
			size_t index;
			std::u32string_view name;
		};
		std::optional<Storage> storage;
		friend struct Table;
	};

	struct AreaMask
	{
		operator bool() const noexcept { return storage.has_value(); }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		size_t Count() const noexcept{assert(*this); return storage->count;}
		AreaMask() = default;
		AreaMask(AreaMask const&) = default;
		AreaMask(size_t index, size_t count) : storage(Storage{ index, count }) {}
	private:
		struct Storage
		{
			size_t index;
			size_t count;
		};
		std::optional<Storage> storage;
		friend struct Table;
	};

	struct ValueMask
	{
		operator bool() const noexcept { return storage.has_value(); }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		std::tuple<SymbolMask, std::u32string_view> Type() const noexcept { assert(*this);  return {storage->type, storage->type_name}; };
		size_t Size() const noexcept { assert(*this); return storage->size; }
		ValueMask() = default;
		ValueMask(ValueMask const&) = default;
		ValueMask(size_t index, SymbolMask type, std::u32string_view type_name, size_t size)
			: storage(Storage{ index, type, type_name, size }) {}
	private:
		struct Storage
		{
			size_t index;
			SymbolMask type;
			std::u32string_view type_name;
			size_t size;
		};
		std::optional<Storage> storage;
		friend struct Table;
	};
	
	struct Table
	{

		struct Property
		{
			SymbolMask mask;
			AreaMask area;
			Section section;
			std::any property;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&property); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const*>(&property); }
		};

		SymbolMask InsertSymbol(std::u32string_view name, std::any property, Section section = {});

		ValueMask InsertValue(std::u32string_view type_name, std::span<std::byte const> datas) {
			return InsertValue(FindActiveSymbolAtLast(type_name), type_name, datas);
		}
		ValueMask InsertValue(SymbolMask type, std::u32string_view type_name, std::span<std::byte const> datas);
		
		AreaMask PopSymbolAsUnactive(size_t count);

		SymbolMask FindActiveSymbolAtLast(std::u32string_view name) const noexcept;
		std::vector<SymbolMask> FindActiveAllSymbol(std::u32string_view name) const{ return active_scope; }

		template<typename FunObj>
		bool FindProperty(SymbolMask mask, FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>;

		template<typename FunObj>
		size_t FindProperty(AreaMask mask, FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>;

		template<typename ...OFunObj>
		bool FindPropertyData(SymbolMask mask, OFunObj&&... OF);

		template<typename ...OFunObj>
		size_t FindPropertyData(AreaMask mask, OFunObj&&... OF);

		template<typename FunObj>
		size_t FindAllProperty(FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>;

		template<typename FunObj>
		size_t FindActiveProperty(size_t count, FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>;

		template<typename FunObj>
		size_t FindAllActiveProperty(FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>;

		Table(Table&&) = default;
		Table(Table const&) = default;
		Table() = default;

	private:
		
		struct Mapping
		{
			bool is_active;
			size_t index;
		};
		std::vector<Property> scope;
		std::vector<IndexSpan<>> areas_sub;
		std::vector<IndexSpan<>> areas;
		std::vector<SymbolMask> active_scope;
		std::vector<std::byte> data_buffer;
		struct DataMapping
		{
			SymbolMask type;
			std::u32string_view type_name;
			IndexSpan<> datas;
		};
		std::vector<DataMapping> data_mapping;
	};

	template<typename FunObj>
	bool Table::FindProperty(SymbolMask mask, FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>
	{
		if (mask && mask.Index() < scope.size())
		{
			std::forward<FunObj>(Function)(scope[mask.Index()]);
			return true;
		}
		return false;
	}

	template<typename FunObj>
	size_t Table::FindProperty(AreaMask mask, FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>
	{
		size_t called = 0;
		if (mask && mask.Index() < areas.size())
		{
			auto span = areas[mask.Index()](areas_sub);
			for (auto& ite : span)
			{
				auto span2 = ite(scope);
				for (auto& ite2 : span2)
				{
					std::forward<FunObj>(Function)(ite2);
					++called;
				}
			}
		}
		return called;
	}

	template<typename ...OFunObj>
	size_t Table::FindPropertyData(AreaMask mask, OFunObj&& ...OFO)
	{
		size_t called = 0;
		this->FindProperty(mask, [&](Table::Property& P){
			if(Potato::AnyViewer(P.property, std::forward<OFunObj>(OFO)...))
				++called;
		});
		return called;
	}

	template<typename ...OFunObj>
	bool Table::FindPropertyData(SymbolMask mask, OFunObj&& ...OFO)
	{
		bool Fined = false;
		this->FindProperty(mask, [&](Table::Property& P) {
			Fined = Potato::AnyViewer(P.property, std::forward<OFunObj>(OFO)...);
		});
		return Fined;
	}

	template<typename FunObj>
	size_t Table::FindAllProperty(FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>
	{
		return this->FindActiveProperty(active_scope.size(), std::forward<FunObj>(Function));
	}

	template<typename FunObj>
	size_t Table::FindActiveProperty(size_t count, FunObj&& Function) requires std::is_invocable_v<FunObj, Property&>
	{
		count = std::min(count, active_scope.size());
		size_t called = 0;
		for (auto ite = active_scope.end() - count; ite != active_scope.end(); ++ite)
		{
			std::forward<FunObj>(Function)(scope[ite->Index()]);
			++called;
		}
		return called;
	}

	enum class TypeModification
	{
		Const,
		Reference,
		Pointer,
	};

	struct TypeProperty
	{
		SymbolMask type;
		std::u32string_view name;
		std::vector<TypeModification> modification;
	};

	struct TypeSymbol
	{
		AreaMask member;
	};

	struct ValueSymbol
	{
		TypeProperty type_property;
		std::vector<std::optional<size_t>> arrays;
		bool is_member;
	};

	struct MemoryModel
	{
		size_t align = 0;
		size_t size = 0;
	};

	struct MemoryModelMaker
	{
		
		struct HandleResult
		{
			size_t align = 0;
			size_t size_reserved = 0;
		};

		size_t operator()(MemoryModel const& info);
		operator MemoryModel() const { return Finalize(info); }

		MemoryModelMaker(MemoryModel info = {}) : info(std::move(info)) {}
		MemoryModelMaker(MemoryModelMaker const&) = default;
		MemoryModelMaker& operator=(MemoryModelMaker const&) = default;

		static size_t MaxAlign(MemoryModel out, MemoryModel in) noexcept;
		static size_t ReservedSize(MemoryModel out, MemoryModel in) noexcept;

	private:
		
		virtual HandleResult Handle(MemoryModel cur, MemoryModel input) const;
		virtual MemoryModel Finalize(MemoryModel cur) const;
		MemoryModel info;
	};

}