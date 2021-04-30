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
#include "IntrusivePointer.h"
#include <span>
#include <cassert>

namespace Potato::Symbol
{

	struct Mask
	{
		operator bool() const noexcept{ return storage.has_value(); }
		std::u32string_view Name() const noexcept { assert(*this); return storage->name; }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		Mask() = default;
		Mask(Mask const&) = default;
		Mask(size_t index, std::u32string_view name) : storage(Storage{index, name}){}
		std::partial_ordering operator <=> (Mask const& mask) { if(*this && mask) return Index() <=> mask.Index(); return std::partial_ordering::unordered; }
	private:
		struct Storage
		{
			size_t index;
			std::u32string_view name;
		};
		std::optional<Storage> storage;
		friend struct Table;
	};

	struct Area
	{
		operator bool() const noexcept { return storage.has_value(); }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		size_t Count() const noexcept{assert(*this); return storage->count;}
		Area() = default;
		Area(Area const&) = default;
		Area(size_t index, size_t count) : storage(Storage{ index, count }) {}
	private:
		struct Storage
		{
			size_t index;
			size_t count;
		};
		std::optional<Storage> storage;
		friend struct Form;
	};
	
	struct Form
	{

		struct Property
		{
			Mask mask;
			Area area;
			Section section;
			std::any property;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&property); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const>(&property); }
			template<typename FunObject>
			bool operator()(FunObject&& fo) { return AnyViewer(property, std::forward<FunObject>(fo)); }
		};

		Mask InsertSymbol(std::u32string_view name, std::any property, Section section = {});
		Mask InsertSearchArea(std::u32string_view name, Area area, Section section = {});
		
		Area PopSymbolAsUnactive(size_t count);
		Mask FindActiveSymbolAtLast(std::u32string_view name) const noexcept;

		Potato::ObserverPtr<Property const> FindActivePropertyAtLast(std::u32string_view name) const noexcept;
		Potato::ObserverPtr<Property> FindActivePropertyAtLast(std::u32string_view name) noexcept { return reinterpret_cast<Form const*>(this)->FindActivePropertyAtLast(name).RemoveConstCast(); }
		
		Potato::ObserverPtr<Property const> FindProperty(Mask InputMask) const noexcept;
		Potato::ObserverPtr<Property> FindProperty(Mask InputMask) noexcept { return reinterpret_cast<Form const*>(this)->FindProperty(InputMask).RemoveConstCast(); }

		std::span<Property const> FindProperty(Area Input) const noexcept;
		std::span<Property> FindProperty(Area Input) noexcept {
			auto Result = reinterpret_cast<Form const*>(this)->FindProperty(Input);
			return {const_cast<Property*>(Result.data()), Result.size()};
		}

		std::span<Property const> GetAllActiveProperty() const noexcept { return active_scope;  }
		std::span<Property> GetAllActiveProperty() noexcept { return active_scope; }
		
		template<typename ...InputType> struct AvailableInvocableDetecter {
			static constexpr bool Value = false;
		};

		template<typename InputType> struct AvailableInvocableDetecter<InputType> {
			using RequireType = std::remove_reference_t<InputType>;
			static constexpr bool AppendProperty = false;
			static constexpr bool Value = true;
		};

		template<typename InputType, typename InputType2> struct AvailableInvocableDetecter<InputType, InputType2> {
			using RequireType = std::remove_reference_t<InputType>;
			static constexpr bool AppendProperty = true;
			static constexpr bool Value = std::is_same_v<std::remove_cvref_t<InputType2>, Property>;
		};

		template<typename FunObj> using AvailableInvocable = FunctionObjectInfo<std::remove_cvref_t<FunObj>>::template PackParameters<AvailableInvocableDetecter>;
		template<typename FunObj> static constexpr bool AvailableInvocableV = AvailableInvocable<FunObj>::Value;

		template<typename FunObj>
		bool FindProperty(Mask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		bool FindProperty(Mask mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindProperty(Area mask, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindProperty(Area mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t GetAllAciveProperty(FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t GetAllActiveProperty(FunObj&& Function) const requires AvailableInvocableV<FunObj>;

		Form(Form&&) = default;
		Form(Form const&) = default;
		Form() = default;

	private:

		struct InsideSearchReference { Area area; };

		struct IteratorTuple
		{
			std::vector<Property>::const_reverse_iterator start;
			std::vector<Property>::const_reverse_iterator end;
		};

		std::variant<
			Potato::ObserverPtr<Form::Property const>,
			std::tuple<IteratorTuple, IteratorTuple>
		> SearchElement(IteratorTuple Input, std::u32string_view name) const noexcept;

		template<typename PropertyT, typename FunObj>
		static bool Execute(PropertyT&& pro, FunObj&& fo) requires AvailableInvocableV<FunObj>;

		template<typename PropertyT, typename FunObj>
		static size_t ExecuteRange(std::span<PropertyT> span, FunObj&& fo) requires AvailableInvocableV<FunObj>;

		struct Mapping
		{
			bool is_active;
			size_t index;
		};
		std::vector<Property> unactive_scope;
		std::vector<Property> active_scope;
		std::vector<Mapping> mapping;
	};

	template<typename PropertyT, typename FunObj>
	bool Form::Execute(PropertyT&& pro, FunObj&& fo) requires AvailableInvocableV<FunObj>
	{
		using Detect = AvailableInvocable<FunObj>;
		if constexpr (!Detect::RequireAppendProperty)
		{
			std::forward<FunObj>(fo)(pro);
			return true;
		}
		else {
			auto Ptr = pro->TryCast<typename Detect::RequireType>();
			if (Ptr != nullptr)
			{
				std::forward<FunObj>(fo)(*Ptr, pro);
				return true;
			}
			return false;
		}
	}

	template<typename PropertyT, typename FunObj>
	size_t Form::ExecuteRange(std::span<PropertyT> span, FunObj&& fo) requires AvailableInvocableV<FunObj>
	{
		size_t index = 0;
		for (auto ite : result)
		{
			assert(ite != nullptr);
			if (this->Execute(*ite, std::forward<FunObj>(Function)))
				index += 1;
		}
		return index;
	}

	template<typename FunObj>
	bool Form::FindProperty(Mask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		auto result = this->FindProperty(mask);
		if (result)
			return this->Execute(*result, std::forward<FunObj>(Function));
		return false;
	}

	template<typename FunObj>
	bool Form::FindProperty(Mask mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>
	{
		auto result = this->FindProperty(mask);
		if (result)
			return this->Execute(*result, std::forward<FunObj>(Function));
		return false;
	}

	template<typename FunObj>
	size_t Form::FindProperty(Area mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		return this->ExecuteRange(this->FindProperty(mask), std::forward<FunObj>(Function));
	}

	template<typename FunObj>
	size_t Form::FindProperty(Area mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>
	{
		return this->ExecuteRange(this->FindProperty(mask), std::forward<FunObj>(Function));
	}

	template<typename FunObj>
	size_t Form::GetAllAciveProperty(FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		return this->ExecuteRange(this->GetAllAciveProperty(), std::forward<FunObj>(Function));
	}

	template<typename FunObj>
	size_t Form::GetAllActiveProperty(FunObj&& Function) const requires AvailableInvocableV<FunObj>
	{
		return this->ExecuteRange(this->GetAllAciveProperty(), std::forward<FunObj>(Function));
	}

	/*

	template<typename FunObj>
	bool SymbolForm::FindProperty(SymbolMask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		if (mask && mask.Index() < scope.size())
		{
			return Table::Execute(scope[mask.Index()], std::forward<FunObj>(Function));
		}
		return false;
	}

	template<typename FunObj>
	size_t SymbolForm::FindProperty(AreaMask mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
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
	size_t SymbolForm::FindAllProperty(FunObj&& Function) requires AvailableInvocableV<FunObj>
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
	size_t SymbolForm::FindActiveProperty(size_t count, FunObj&& Function) requires AvailableInvocableV<FunObj>
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
	*/

	/*
	struct IDReference
	{
		SymbolMask symbol_ref;
		std::u32string_view raw_name;
		IDReference() = default;
		IDReference(SymbolMask input) : symbol_ref(input) {}
		IDReference(SymbolMask input, std::u32string_view input2) { if(input) symbol_ref = input; else raw_name = input2; }
		IDReference(std::u32string_view input) : raw_name(input) {}
		bool IsNotASymbol() const noexcept{ return !symbol_ref; }
		std::u32string_view Name() const noexcept { return symbol_ref ? symbol_ref.Name() : raw_name; }
	};

	enum class TypeModification
	{
		Const,
		Reference,
		Pointer,
	};

	struct TypeProperty
	{
		IDReference id;
		std::vector<TypeModification> modification;
		std::vector<size_t> arrays;
	};

	struct TypeSymbol
	{
		AreaMask member;
	};

	struct ValueSymbol
	{
		TypeProperty type_property;
		bool is_member;
	};

	

	/*
	struct MemoryModelMaker
	{
		struct HandleResult
		{
			size_t align = 0;
			size_t size_reserved = 0;
		};
		size_t operator()(MemoryModel const& info);
		MemoryModelMaker(MemoryModel info = {}) : info(std::move(info)) {}
		MemoryModelMaker(MemoryModelMaker const&) = default;
		MemoryModelMaker& operator=(MemoryModelMaker const&) = default;
		static size_t MaxAlign(MemoryModel out, MemoryModel in) noexcept;
		static size_t ReservedSize(MemoryModel out, MemoryModel in) noexcept;
	protected:
		virtual HandleResult Handle(MemoryModel cur, MemoryModel input) const;
		virtual MemoryModel Finalize(MemoryModel cur) const;
	private:
		MemoryModel info;
		std::optional<MemoryModel> history;
		std::optional<MemoryModel> finalize;
	};
	*/

	



}