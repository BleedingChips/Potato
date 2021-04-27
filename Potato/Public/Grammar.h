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

	/*
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
	*/
	
	struct SymbolForm
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

		/*
		ValueMask InsertValue(std::u32string_view type_name, std::span<std::byte const> datas) {
			return InsertValue(FindActiveSymbolAtLast(type_name), type_name, datas);
		}
		ValueMask InsertValue(SymbolMask type, std::u32string_view type_name, std::span<std::byte const> datas);
		*/

		AreaMask PopSymbolAsUnactive(size_t count);

		SymbolMask FindActiveSymbolAtLast(std::u32string_view name) const noexcept;
		std::vector<SymbolMask> FindActiveAllSymbol(std::u32string_view name) const{ return active_scope; }

		template<typename ...InputType> struct AvailableInvocableDetecter {
			static constexpr bool Value = false;
		};

		template<typename InputType> struct AvailableInvocableDetecter<InputType> {
			using RequireType = std::conditional_t<std::is_same_v<std::remove_cvref_t<InputType>, Property>, void, std::remove_cvref_t<InputType>>;
			static constexpr bool AppendProperty = false;
			static constexpr bool Value = true;
		};

		template<typename InputType, typename InputType2> struct AvailableInvocableDetecter<InputType, InputType2> {
			using RequireType = std::remove_cvref_t<InputType>;
			static constexpr bool AppendProperty = true;
			static constexpr bool Value = std::conditional_t < std::is_same_v<std::remove_cvref_t<InputType>, Property>;
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

		SymbolForm(SymbolForm&&) = default;
		SymbolForm(SymbolForm const&) = default;
		SymbolForm() = default;

	private:

		template<typename FunObj>
		static bool Execute(Property& pro, FunObj&& fo) requires AvailableInvocableV<FunObj>
		{
			using Detect = AvailableInvocable<FunObj>;
			if constexpr (std::is_same_v<typename Detect::RequireType, void>)
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
						if constexpr (Detect::LeftProperty)
							std::forward<FunObj>(fo)(pro, RT);
						else
							std::forward<FunObj>(fo)(RT, pro);
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

	struct MemoryTag
	{
		size_t align = 0;
		size_t size = 0;
		MemoryTag(size_t i_align = 0, size_t i_size = 0)
			: align(i_align), size(i_size)
		{  
			assert(((align - 1) & align) == 0);
			assert(align == 0 || size % align == 0);
		};
		MemoryTag(MemoryTag const&) = default;
		MemoryTag& operator=(MemoryTag const&) = default;
	};

	struct MemoryTagStyle
	{
		struct Result 
		{
			size_t member_offset;
			MemoryTag owner;
		};
		static constexpr size_t MinAlign() noexcept{ return alignof(std::byte); };
		//Result InsertMember(MemoryTag owner, MemoryTag member) = 0;
		MemoryTag Finalize(MemoryTag owner);
	};

	struct CLikeMemoryTagStyle : MemoryTagStyle
	{
		Result InsertMember(MemoryTag owner, MemoryTag member);
	};

	struct HlslMemoryTagStyle : MemoryTagStyle
	{
		static constexpr size_t MinAlign() noexcept {return alignof(uint32_t);}
		Result InsertMember(MemoryTag owner, MemoryTag member);
	};

	struct MemoryTagFactoryCore
	{
		MemoryTag scope;
		std::optional<MemoryTag> history;
		std::optional<MemoryTag> finalize;
	};

	template<typename FactoryStyle = CLikeMemoryTagStyle>
	requires std::is_base_of_v<MemoryTagStyle, FactoryStyle>
	struct MemoryTagFatcory : protected MemoryTagFactoryCore
	{
		size_t InsertMember(MemoryTag const& input) {
			history = scope;
			finalize = std::nullopt;
			MemoryTagStyle::Result result = style.InsertMember(scope, input);
			scope = result.owner;
			return result.member_offset;
		}
		MemoryTag Finalize() {
			if(finalize)
				return *finalize;
			else {
				MemoryTag fin = style.Finalize(scope);
				finalize = fin;
				return fin;
			}
		}
		MemoryTagFatcory() {
			scope = {style.MinAlign(), 0};
		}
		MemoryTagFatcory(MemoryTagFatcory const&) = default;
		MemoryTagFatcory& operator=(MemoryTagFatcory const&) = default;
	protected:
		FactoryStyle style;
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