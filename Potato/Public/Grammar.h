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
			template<typename FunObject>
			bool operator()(FunObject&& fo) { return AnyViewer(property, std::forward<FunObject>(fo)); }
		};

		SymbolMask InsertSymbol(std::u32string_view name, std::any property, Section section = {});

		ValueMask InsertValue(std::u32string_view type_name, std::span<std::byte const> datas) {
			return InsertValue(FindActiveSymbolAtLast(type_name), type_name, datas);
		}
		ValueMask InsertValue(SymbolMask type, std::u32string_view type_name, std::span<std::byte const> datas);
		
		AreaMask PopSymbolAsUnactive(size_t count);

		SymbolMask FindActiveSymbolAtLast(std::u32string_view name) const noexcept;
		std::vector<SymbolMask> FindActiveAllSymbol(std::u32string_view name) const{ return active_scope; }

		template<typename ...InputType> struct AvailableInvocableDetecter {
			static constexpr bool Value = false;
		};

		template<typename InputType> struct AvailableInvocableDetecter<InputType> {
			static constexpr bool RequireProperty = std::is_same_v<std::remove_cvref_t<InputType>, Property>;
			static constexpr bool RequireAppendProperty = false;
			static constexpr bool Value = true;
		};

		template<typename InputType, typename InputType2> struct AvailableInvocableDetecter<InputType, InputType2> {
			static constexpr bool RequireProperty = false;
			static constexpr bool RequireAppendProperty = true;
			static constexpr bool Value = !std::is_same_v<std::remove_cvref_t<InputType>, Property> && std::is_same_v<std::remove_cvref_t<InputType2>, Property>
				|| !std::is_same_v<std::remove_cvref_t<InputType2>, Property> && std::is_same_v<std::remove_cvref_t<InputType>, Property>;
			using RequireType = std::conditional_t<std::is_same_v<std::remove_cvref_t<InputType>, Property>, InputType2, InputType>;
		};

		template<typename FunObj> using AvailableInvocable = FunctionObjectInfo<std::remove_cvref_t<FunObj>>::template PackParameters<AvailableInvocableDetecter>;
		template<typename FunObj> static constexpr bool AvailableInvocableV = AvailableInvocable<FunObj>::Value;

		template<typename FunObj>
		bool FindProperty(SymbolMask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindProperty(AreaMask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindAllProperty(FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindActiveProperty(size_t count, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindAllActiveProperty(FunObj&& Function) requires AvailableInvocableV<FunObj> {
			return this->FindActiveProperty(active_scope.size(), std::forward<FunObj>(Function));
		}

		Table(Table&&) = default;
		Table(Table const&) = default;
		Table() = default;

	private:

		template<typename FunObj>
		static bool Execute(Property& pro, FunObj&& fo) requires AvailableInvocableV<FunObj>
		{
			using Detect = AvailableInvocable<FunObj>;
			if constexpr (Detect::RequireProperty)
			{
				std::forward<FunObj>(fo)(pro);
				return true;
			}
			else {
				if constexpr(!Detect::RequireAppendProperty)
					return pro(std::forward<FunObj>(fo));
				else {
					bool Re = false;
					AnyViewer(pro.property, [&](typename Detect::RequireType RT) {
						AutoInvote(std::forward<FunObj>(fo), RT, pro);
						Re = true;
					});
					return Re;
				}
			}
		}
		
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
	bool Table::FindProperty(SymbolMask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		if (mask && mask.Index() < scope.size())
		{
			return Table::Execute(scope[mask.Index()], std::forward<FunObj>(Function));
		}
		return false;
	}

	template<typename FunObj>
	size_t Table::FindProperty(AreaMask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
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
					if(Table::Execute(ite2, std::forward<FunObj>(Function)))
						++called;
				}
			}
		}
		return called;
	}

	template<typename FunObj>
	size_t Table::FindAllProperty(FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		size_t called = 0;
		for (auto& ite : scope)
		{
			if (Table::Execute(ite, std::forward<FunObj>(Function)))
				++called;
		}
		return called;
	}

	template<typename FunObj>
	size_t Table::FindActiveProperty(size_t count, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		count = std::min(count, active_scope.size());
		size_t called = 0;
		for (auto ite = active_scope.end() - count; ite != active_scope.end(); ++ite)
		{
			if (Table::Execute(scope[ite->Index()], std::forward<FunObj>(Function)))
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