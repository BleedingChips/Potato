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
		std::optional<TypeTag> FinishTypeDefine(Setting const& setting);
		Potato::ObserverPtr<std::optional<TypeProperty>> FindType(TypeTag tag) noexcept;
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
			MEMORY,

			BI_UINT8,
			BI_UINT16,
			BI_UINT32,
			BI_UINT64,

			BI_INT8,
			BI_INT16,
			BI_INT32,
			BI_INT64,

			BI_FLOAT32,
			BI_FLOAT64,

			None
		};

		Register(Register const&) = default;
		Register& operator=(Register const&) = default;
		Register(Type register_type, uint64_t index) : type(register_type), index(index) {};
		Register(uint8_t input) : Register(Type::BI_UINT8, input) {}
		Register(uint16_t input) : Register(Type::BI_UINT16, input) {}
		Register(uint32_t input) : Register(Type::BI_UINT32, input) {}
		Register(uint64_t input) : Register(Type::BI_UINT64, input) {}
		Register(int8_t input) : Register(Type::BI_INT8, input) {}
		Register(int16_t input) : Register(Type::BI_INT16, input) {}
		Register(int32_t input) : Register(Type::BI_INT32, input) {}
		Register(int64_t input) : Register(Type::BI_INT64, input) {}
		Register(float input) : type(Type::BI_FLOAT32), index(0) {  std::memcpy(&index, &input, sizeof(input)); }
		Register(double input) : type(Type::BI_FLOAT64), index(0) { std::memcpy(&index, &input, sizeof(input)); }
		operator bool() const noexcept { return type != Type::None; }
		Type Category() const noexcept { return type; }
		uint64_t Index() const noexcept { return index; }
	private:
		Type type;
		uint64_t index;
	};

	struct HIRForm
	{
		Register InserConstData(TypeReference desc, Layout layout, std::span<std::byte const> data);
		template<typename Type>
		Register InserConstData(Type&& data) requires (StorageTypeEnumV<std::remove_cvref_t<Type>> != StorageType::CUSTOM)
		{
			using Type = std::remove_cvref_t<Type>;
			return InserConstData(TypeReference{ StorageTypeEnumV<Type>, {} }, { alignof(Type), sizeof(Type) }, std::span<std::byte const>{reinterpret_cast<std::byte const*>(&data), sizeof(data)});
		}
		template<typename Type>
		Register InserConstData(std::span<Type> span) requires(StorageTypeEnumV<std::remove_pointer_t<std::remove_const_t<Type>>> != StorageType::CUSTOM)
		{
			using Type = std::remove_const_t<Type>;
			return InserConstData(TypeReference{ StorageTypeEnumV<Type>, {Modifier::Type::ARRAY, span.size()} }, { alignof(Type), sizeof(Type) }, std::span<std::byte const>{reinterpret_cast<std::byte const*>(span.data()), sizeof(Type)* span.size()});
		}
	private:
		struct Element
		{
			TypeReference type_reference;
			IndexSpan<> datas;
			Layout layout;
		};
		std::vector<std::byte> datas;
		std::vector<Element> elements;
	};
	
	/*
	struct StackValueForm
	{
		struct Element
		{
			TypeReference type_reference;
			IndexSpan<> datas;
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
		std::vector<std::byte> datas;
		size_t usaged_buffer_length = 0;
		std::vector<Element> elements;
		std::vector<size_t> stack;
	};
	*/

	struct SymbolTag
	{
		operator bool() const noexcept { return storage.has_value(); }
		std::u32string_view Name() const noexcept { assert(*this); return storage->name; }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		SymbolTag() = default;
		SymbolTag(SymbolTag const&) = default;
		SymbolTag(size_t index, std::u32string_view name) : storage(Storage{ index, name }) {}
		std::partial_ordering operator <=> (SymbolTag const& mask) { if (*this && mask) return Index() <=> mask.Index(); return std::partial_ordering::unordered; }
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

	struct SymbolAreaTag
	{
		operator bool() const noexcept { return storage.has_value(); }
		size_t Index() const noexcept { assert(*this); return storage->index; }
		size_t Count() const noexcept { assert(*this); return storage->count; }
		SymbolAreaTag() = default;
		SymbolAreaTag(SymbolAreaTag const&) = default;
		SymbolAreaTag(size_t index, size_t count) : storage(Storage{ index, count }) {}
	private:
		struct Storage
		{
			size_t index;
			size_t count;
		};
		std::optional<Storage> storage;
		friend struct Form;
	};

	struct SymbolForm
	{

		struct Property
		{
			SymbolTag mask;
			SymbolAreaTag area;
			Section section;
			std::any property;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&property); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const>(&property); }
			template<typename FunObject>
			bool operator()(FunObject&& fo) { return AnyViewer(property, std::forward<FunObject>(fo)); }
		};

		SymbolTag InsertSymbol(std::u32string_view name, std::any property, Section section = {});
		SymbolTag InsertSearchArea(std::u32string_view name, SymbolAreaTag area, Section section = {});

		void MarkSymbolActiveScopeBegin();
		SymbolAreaTag PopSymbolActiveScope();

		SymbolTag FindActiveSymbolAtLast(std::u32string_view name) const noexcept;

		Potato::ObserverPtr<Property const> FindActivePropertyAtLast(std::u32string_view name) const noexcept;
		Potato::ObserverPtr<Property> FindActivePropertyAtLast(std::u32string_view name) noexcept { return reinterpret_cast<SymbolForm const*>(this)->FindActivePropertyAtLast(name).RemoveConstCast(); }
		
		Potato::ObserverPtr<Property const> FindProperty(SymbolTag InputMask) const noexcept;
		Potato::ObserverPtr<Property> FindProperty(SymbolTag InputMask) noexcept { return reinterpret_cast<SymbolForm const*>(this)->FindProperty(InputMask).RemoveConstCast(); }

		std::span<Property const> FindProperty(SymbolAreaTag Input) const noexcept;
		std::span<Property> FindProperty(SymbolAreaTag Input) noexcept {
			auto Result = reinterpret_cast<SymbolForm const*>(this)->FindProperty(Input);
			return {const_cast<Property*>(Result.data()), Result.size()};
		}

		std::span<Property const> GetAllActiveProperty() const noexcept { return active_scope;  }
		std::span<Property> GetAllActiveProperty() noexcept { return active_scope; }

		SymbolForm(SymbolForm&&) = default;
		SymbolForm(SymbolForm const&) = default;
		SymbolForm() = default;

	private:

		struct InsideSearchReference { SymbolAreaTag area; };

		struct IteratorTuple
		{
			std::vector<Property>::const_reverse_iterator start;
			std::vector<Property>::const_reverse_iterator end;
		};

		std::variant<
			Potato::ObserverPtr<SymbolForm::Property const>,
			std::tuple<IteratorTuple, IteratorTuple>
		> SearchElement(IteratorTuple Input, std::u32string_view name) const noexcept;

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

	struct Form
	{
		Form(Form&&) = default;
		Form(Form const&) = default;
		TypeForm& Type() noexcept{ return type; }
		TypeForm const& Type() const noexcept { return type; }
		HIRForm& HIR() noexcept { return hir; }
		HIRForm const& HIR() const noexcept { return hir; }
		SymbolForm& Symbol() noexcept { return symbol; }
		SymbolForm const& Symbol() const noexcept { return symbol; }
	private:
		TypeForm type;
		HIRForm hir;
		SymbolForm symbol;
	};
}