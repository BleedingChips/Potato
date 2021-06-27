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

namespace Potato
{
	enum class MemoryDescription
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

		BOOL,

		CUSTOM,
	};
	
	/*
	template<typename Type> struct StorageTypeEnum { static inline constexpr StorageType value = StorageType::CUSTOM; };
	template<> struct StorageTypeEnum<int32_t> { static inline constexpr StorageType value = StorageType::INT32; };
	template<> struct StorageTypeEnum<uint32_t> { static inline constexpr StorageType value = StorageType::UINT32; };
	template<> struct StorageTypeEnum<float> { static inline constexpr StorageType value = StorageType::FLOAT32; };
	template<> struct StorageTypeEnum<double> { static inline constexpr StorageType value = StorageType::FLOAT64; };
	template<> struct StorageTypeEnum<int64_t> { static inline constexpr StorageType value = StorageType::INT64; };
	template<> struct StorageTypeEnum<uint64_t> { static inline constexpr StorageType value = StorageType::UINT64; };
	template<typename Type> constexpr StorageType StorageTypeEnumV = StorageTypeEnum<Type>::value;
	*/

	struct TypeLayout
	{
		size_t align = 1;
		size_t size = 0;
		constexpr TypeLayout(size_t i_align = 0, size_t i_size = 0)
			: align(i_align), size(i_size)
		{
			assert(((align - 1) & align) == 0);
			assert(size % align == 0);
		};
		constexpr TypeLayout(TypeLayout const&) = default;
		constexpr TypeLayout& operator=(TypeLayout const&) = default;
	};

	struct TypeModifier
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

	struct TypeIndex
	{
		MemoryDescription storage_type;
		size_t index = 0;
		bool IsCustomType() const noexcept { return storage_type == MemoryDescription::CUSTOM; }
		size_t Index() const noexcept { return index; }
	};

	using TypeMask = std::optional<TypeIndex>;

	struct FunctionIndex
	{
		size_t index;
		size_t parameter_count;
	};

	using FunctionMask = std::optional<FunctionIndex>;

	struct RegisterIndex
	{
		enum class Category
		{
			CONST,
			STACK,
			MEMORY,
			IMMEDIATE_ADRESSING,
		};
		Category type;
		MemoryDescription storage_type = MemoryDescription::CUSTOM;
		uint64_t index;
	};

	using RegisterMask = std::optional<RegisterIndex>;

	namespace Implement
	{
		template<typename Input>
		static uint64_t Transfer(Input&& input) requires(sizeof(Input) <= sizeof(uint64_t)) {
			uint64_t result = 0;
			std::memcpy(&result, &input, sizeof(input));
			return result;
		}
	}

	static RegisterMask AsImmediateAdressingRegister(uint8_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::UINT8, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(uint16_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::UINT16, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(uint32_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::UINT32, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(uint64_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::UINT64, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(int8_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::INT8, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(int16_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::INT16, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(int32_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::INT32, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(int64_t input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::INT64, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(float input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::FLOAT32, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(double input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::FLOAT64, Implement::Transfer(input) }; };
	static RegisterMask AsImmediateAdressingRegister(bool input) { return RegisterIndex{ RegisterIndex::Category::IMMEDIATE_ADRESSING, MemoryDescription::UINT8, Implement::Transfer(input) }; };
	

	struct TypeProperty
	{
		TypeMask type;
		std::vector<TypeModifier> modifier;
		operator bool () const noexcept {return type.has_value(); }
		bool IsPointer() const noexcept;
	};

	struct TypeDescription
	{
		struct Member
		{
			TypeProperty type_reference;
			size_t offset;
			TypeLayout layout;
		};
		TypeLayout layout;
		std::vector<Member> members;
	};

	struct HIRForm
	{
		struct Setting
		{
			size_t min_alignas;
			TypeLayout pointer_layout;
			std::optional<size_t> member_feild;
		};
		TypeMask ForwardDefineType();
		bool MarkTypeDefineStart(TypeMask Input);
		bool MarkTypeDefineStart() { return MarkTypeDefineStart(ForwardDefineType()); }
		bool InsertMember(TypeProperty type_reference);
		TypeMask FinishTypeDefine(Setting const& setting);
		Potato::ObserverPtr<std::optional<TypeProperty>> FindType(TypeMask tag) noexcept;
		RegisterMask InserConstData(TypeProperty desc, TypeLayout layout, std::span<std::byte const> data);
	private:
		std::optional<TypeLayout> CalculateTypeLayout(TypeMask const& ref) const;
		std::optional<TypeLayout> CalculateTypeLayout(TypeProperty const& ref, Setting const& setting) const;
		std::vector<TypeDescription::Member> temporary_member_type;
		std::vector<std::optional<TypeProperty>> defined_types;
		std::vector<std::tuple<TypeMask, size_t>> define_stack_record;
		std::vector<std::byte> datas;

		struct Element
		{
			TypeProperty type_reference;
			IndexSpan<> datas;
			TypeLayout layout;
		};

		std::vector<Element> elements;
	};
}