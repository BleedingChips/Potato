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
#include "Types.h"

namespace Potato::HIR
{
	enum class StorageType
	{
		UINT8,
		UINT16,
		UINT32,
		UINT64,

		INT8,
		INT16,
		INT32,
		INT64,

		FLOAT32,
		FLOAT64,

		CUSTOM,
	};
	
	template<typename Type> struct StorageTypeEnum { static inline constexpr StorageType value = StorageType::CUSTOM; };
	template<> struct StorageTypeEnum<int32_t> { static inline constexpr StorageType value = StorageType::INT32; };
	template<> struct StorageTypeEnum<uint32_t> { static inline constexpr StorageType value = StorageType::UINT32; };
	template<> struct StorageTypeEnum<float> { static inline constexpr StorageType value = StorageType::FLOAT32; };
	template<> struct StorageTypeEnum<double> { static inline constexpr StorageType value = StorageType::FLOAT64; };
	template<> struct StorageTypeEnum<int64_t> { static inline constexpr StorageType value = StorageType::INT64; };
	template<> struct StorageTypeEnum<uint64_t> { static inline constexpr StorageType value = StorageType::UINT64; };
	template<typename Type> constexpr StorageType StorageTypeEnumV = StorageTypeEnum<Type>::value;

	struct Layout
	{
		size_t align = 1;
		size_t size = 0;
		constexpr Layout(size_t i_align = 0, size_t i_size = 0)
			: align(i_align), size(i_size)
		{
			assert(((align - 1) & align) == 0);
			assert(size % align == 0);
		};
		constexpr Layout(Layout const&) = default;
		constexpr Layout& operator=(Layout const&) = default;
	};

	struct Modifier
	{
		enum class Type
		{
			CONST,
			POINTER,
			ARRAY,
		};
		Type type;
		size_t parameter;
	};

	struct TypeTag
	{
		operator bool() const noexcept {return storage_type.has_value(); }
		bool IsCustomType() const noexcept {return *this && *storage_type == StorageType::CUSTOM;}
		size_t AsIndex() const noexcept {return index;}
		std::optional<StorageType> storage_type;
		size_t index;
	};

	struct FunctionTag
	{
		std::optional<size_t> index;
	};

	struct TypeReference
	{
		TypeTag type;
		std::vector<Modifier> modifier;
		operator bool () const {return type; }
		bool IsPointer() const noexcept;
		std::optional<size_t> ArrayCount() const noexcept;
	};

	struct TypeProperty
	{
		enum class Solt
		{
			Construction,
			Max,
		};
		struct Member
		{
			TypeReference type_reference;
			std::u32string name;
			size_t offset;
			Layout layout;
		};
		Layout layout;
		std::vector<Member> members;
		std::vector<FunctionTag> functions;
		FunctionTag destruction_functions;
		std::array<IndexSpan<>, static_cast<size_t>(Solt::Max)> functions;
	};

	struct TypeForm
	{
		struct Setting
		{
			size_t min_alignas;
			Layout pointer_layout;
			std::optional<size_t> member_feild;
		};
		TypeTag ForwardDefineType();
		bool MarkTypeDefineStart(TypeTag Input);
		bool MarkTypeDefineStart() { return MarkTypeDefineStart(ForwardDefineType()); }
		bool InsertMember(TypeReference type_reference, std::u32string name);
		TypeTag FinishTypeDefine(Setting const& setting);
	private:
		std::optional<Layout> CalculateTypeLayout(TypeTag const& ref) const;
		std::optional<Layout> CalculateTypeLayout(TypeReference const& ref, Setting const& setting) const;
		std::vector<TypeProperty::Member> temporary_member_type;
		std::vector<std::optional<TypeProperty>> defined_types;
		std::vector<std::tuple<TypeTag, size_t>> define_stack_record;
	};


	struct Register
	{

		enum class Type
		{
			CONST,
			STACK,
			TYPE,
			BUILINVALUE,
			VALUE,
			None
		};

		Register(Register const&) = default;
		Register& operator=(Register const&) = default;
		Register(Type register_type, uint64_t index) : type(register_type), index(index) {};
		Register() : Register(Type::None, 0) {}
		Register(StorageType desc) : Register(Type::BUILINVALUE, static_cast<uint64_t>(desc)) {};
		operator bool() const noexcept { return type != Type::None; }
		Type Category() const noexcept { return type; }
		uint64_t Index() const noexcept { return index; }
	private:
		Type type;
		uint64_t index;
	};

	

	

	

	struct ConstForm
	{
		struct Element
		{
			TypeReference type_reference;
			IndexSpan<> span;
			Layout layout;
		};
		size_t InserConstData(TypeReference desc, Layout layout, std::span<std::byte const> data);
		template<typename Type>
		size_t InserConstData(Type&& data) requires (StorageTypeEnumV<std::remove_cvref_t<Type>> != StorageType::CUSTOM)
		{
			using Type = std::remove_cvref_t<Type>;
			return InserConstData(TypeReference{ StorageTypeEnumV<Type>, {} }, { alignof(Type), sizeof(Type) }, std::span<std::byte const>{reinterpret_cast<std::byte const*>(&data), sizeof(data)});
		}
		template<typename Type>
		size_t InserConstData(std::span<Type> span) requires(StorageTypeEnumV<std::remove_pointer_t<std::remove_const_t<Type>>> != StorageType::CUSTOM)
		{
			using Type = std::remove_const_t<Type>;
			return InserConstData(TypeReference{ StorageTypeEnumV<Type>, {Modifier::Type::ARRAY, span.size()} }, { alignof(Type), sizeof(Type) }, std::span<std::byte const>{reinterpret_cast<std::byte const*>(span.data()), sizeof(Type)* span.size()});
		}
	private:
		std::vector<std::byte> const_form;
		std::vector<Element> elements;
	};

	struct Symbol
	{
		operator bool() const noexcept { return storage.has_value(); }
		std::u32string_view Name() const noexcept { assert(*this); return storage->name; }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		Symbol() = default;
		Symbol(Symbol const&) = default;
		Symbol(size_t index, std::u32string_view name) : storage(Storage{ index, name }) {}
		std::partial_ordering operator <=> (Symbol const& mask) { if (*this && mask) return Index() <=> mask.Index(); return std::partial_ordering::unordered; }
	private:
		struct Storage
		{
			size_t index;
			std::u32string_view name;
			bool is_build_in;
		};
		std::optional<Storage> storage;
		friend struct Table;
	};

	struct SymbolArea
	{
		operator bool() const noexcept { return storage.has_value(); }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		size_t Count() const noexcept { assert(*this); return storage->count; }
		SymbolArea() = default;
		SymbolArea(SymbolArea const&) = default;
		SymbolArea(size_t index, size_t count) : storage(Storage{ index, count }) {}
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
			Symbol mask;
			SymbolArea area;
			Section section;
			std::any property;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&property); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const>(&property); }
			template<typename FunObject>
			bool operator()(FunObject&& fo) { return AnyViewer(property, std::forward<FunObject>(fo)); }
		};

		Symbol InsertSymbol(std::u32string_view name, std::any property, Section section = {});
		Symbol InsertSearchArea(std::u32string_view name, SymbolArea area, Section section = {});

		void MarkSymbolActiveScopeBegin();
		SymbolArea PopSymbolActiveScope();

		Symbol FindActiveSymbolAtLast(std::u32string_view name) const noexcept;

		Potato::ObserverPtr<Property const> FindActivePropertyAtLast(std::u32string_view name) const noexcept;
		Potato::ObserverPtr<Property> FindActivePropertyAtLast(std::u32string_view name) noexcept { return reinterpret_cast<Form const*>(this)->FindActivePropertyAtLast(name).RemoveConstCast(); }
		
		Potato::ObserverPtr<Property const> FindProperty(Symbol InputMask) const noexcept;
		Potato::ObserverPtr<Property> FindProperty(Symbol InputMask) noexcept { return reinterpret_cast<Form const*>(this)->FindProperty(InputMask).RemoveConstCast(); }

		std::span<Property const> FindProperty(SymbolArea Input) const noexcept;
		std::span<Property> FindProperty(SymbolArea Input) noexcept {
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
		bool FindProperty(Symbol mask, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		bool FindProperty(Symbol mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindProperty(SymbolArea mask, FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t FindProperty(SymbolArea mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t GetAllAciveProperty(FunObj&& Function) requires AvailableInvocableV<FunObj>;

		template<typename FunObj>
		size_t GetAllActiveProperty(FunObj&& Function) const requires AvailableInvocableV<FunObj>;

		Form(Form&&) = default;
		Form(Form const&) = default;
		Form() = default;

	private:

		struct InsideSearchReference { SymbolArea area; };

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
			enum class Category
			{
				UNACTIVE,
				ACTIVE,
				BUILDIN,
			};
			Category category;
			size_t index;
		};
		std::vector<size_t> activescope_start_index;
		std::vector<Property> unactive_scope;
		std::vector<Property> active_scope;
		std::vector<Mapping> mapping;
		std::vector<std::byte> const_data;
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
		for (auto ite : span)
		{
			assert(ite != nullptr);
			if (this->Execute(*ite, std::forward<FunObj>(fo)))
				index += 1;
		}
		return index;
	}

	template<typename FunObj>
	bool Form::FindProperty(Symbol mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		auto result = this->FindProperty(mask);
		if (result)
			return this->Execute(*result, std::forward<FunObj>(Function));
		return false;
	}

	template<typename FunObj>
	bool Form::FindProperty(Symbol mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>
	{
		auto result = this->FindProperty(mask);
		if (result)
			return this->Execute(*result, std::forward<FunObj>(Function));
		return false;
	}

	template<typename FunObj>
	size_t Form::FindProperty(SymbolArea mask, FunObj&& Function) requires AvailableInvocableV<FunObj>
	{
		return this->ExecuteRange(this->FindProperty(mask), std::forward<FunObj>(Function));
	}

	template<typename FunObj>
	size_t Form::FindProperty(SymbolArea mask, FunObj&& Function) const requires AvailableInvocableV<FunObj>
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
}