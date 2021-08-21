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

		CUSTOM,
	};

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

	template<typename Type> struct DefaultTypeIndex { static constexpr bool Value = false; };
	template<> struct DefaultTypeIndex<uint8_t> {
		static constexpr bool Value = true;
		TypeIndex operator()() const noexcept { return TypeIndex{ MemoryDescription::UINT8, 0 }; }
	};
	template<> struct DefaultTypeIndex<uint16_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::UINT16, 0 }; }
	};
	template<> struct DefaultTypeIndex<uint32_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::UINT32, 0 }; }
	};
	template<> struct DefaultTypeIndex<uint64_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::UINT64, 0 }; }
	};
	template<> struct DefaultTypeIndex<int8_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::INT8, 0 }; }
	};
	template<> struct DefaultTypeIndex<int16_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::INT16, 0 }; }
	};
	template<> struct DefaultTypeIndex<int32_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::INT32, 0 }; }
	};
	template<> struct DefaultTypeIndex<int64_t> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::INT64, 0 }; }
	};
	template<> struct DefaultTypeIndex<float> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::FLOAT32, 0 }; }
	};
	template<> struct DefaultTypeIndex<double> {
		static constexpr bool Value = true;
		constexpr TypeIndex operator()() const noexcept { return { MemoryDescription::FLOAT32, 0 }; }
	};

	using TypeMask = std::optional<TypeIndex>;

	struct FunctionIndex
	{
		size_t index;
		size_t parameter_count;
	};

	using FunctionMask = std::optional<FunctionIndex>;

	namespace Implement
	{
		template<typename Input>
		static uint64_t Transfer(Input&& input) requires(sizeof(Input) <= sizeof(uint64_t)) {
			uint64_t result = 0;
			std::memcpy(&result, &input, sizeof(input));
			return result;
		}
	}

	struct RegisterIndex
	{
		enum class Category
		{
			CONST,
			STACK,
			MEMORY,
			IA_UINT8,
			IA_UINT16,
			IA_UINT32,
			IA_UINT64,
			IA_INT8,
			IA_INT16,
			IA_INT32,
			IA_INT64,
			IA_FLOAT32,
			IA_FLOAT64,
		};

		Category type;
		uint64_t index;
		RegisterIndex(Category input_type, uint64_t input_index) : type(input_type), index(input_index){}
		explicit RegisterIndex(uint8_t ia) : RegisterIndex(Category::IA_UINT8, Implement::Transfer(ia)) {}
		explicit RegisterIndex(uint16_t ia) : RegisterIndex(Category::IA_UINT16, Implement::Transfer(ia)) {}
		explicit RegisterIndex(uint32_t ia) : RegisterIndex(Category::IA_UINT32, Implement::Transfer(ia)) {}
		explicit RegisterIndex(uint64_t ia) : RegisterIndex(Category::IA_UINT64, Implement::Transfer(ia)) {}
		explicit RegisterIndex(int8_t ia) : RegisterIndex(Category::IA_INT8, Implement::Transfer(ia)) {}
		explicit RegisterIndex(int16_t ia) : RegisterIndex(Category::IA_INT16, Implement::Transfer(ia)) {}
		explicit RegisterIndex(int32_t ia) : RegisterIndex(Category::IA_INT32, Implement::Transfer(ia)) {}
		explicit RegisterIndex(int64_t ia) : RegisterIndex(Category::IA_INT64, Implement::Transfer(ia)) {}
		explicit RegisterIndex(float ia) : RegisterIndex(Category::IA_FLOAT32, Implement::Transfer(ia)) {}
		explicit RegisterIndex(double ia) : RegisterIndex(Category::IA_FLOAT64, Implement::Transfer(ia)) {}
	};

	using RegisterMask = std::optional<RegisterIndex>;

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

	namespace Implement
	{
		struct ConstDataStorageTable
		{
			RegisterMask InserConstData(TypeProperty desc, TypeLayout layout, std::span<std::byte const> data);
		private:
			struct Element
			{
				TypeProperty type_reference;
				IndexSpan<> datas;
				TypeLayout layout;
			};
			std::vector<Element> elements;
			std::vector<std::byte> datas;
		};
	}

	struct MiniCore
	{

		struct MemoryModel
		{
			size_t min_alignas;
			TypeLayout pointer_layout;
			std::optional<size_t> member_feild;
		};

		TypeMask ForwardDefineType();
		bool MarkTypeDefineStart(TypeMask Input);
		bool MarkTypeDefineStart() { return MarkTypeDefineStart(ForwardDefineType()); }
		bool InsertMember(TypeProperty type_reference);
		TypeMask FinishTypeDefine(MemoryModel const& setting);
		Potato::ObserverPtr<std::optional<TypeProperty>> FindType(TypeMask tag) noexcept;
		RegisterMask InserConstData(TypeProperty desc, TypeLayout layout, std::span<std::byte const> data){ 
			return const_data_table.InserConstData(std::move(desc), std::move(layout), data);
		}

		enum class CommandType
		{
			Sub,
			Add,
			Mulity,
			Divide,
			Mod,
			And,
			Or,
			Move,
			Call,
		};

		struct TAC
		{
			CommandType Type;
			RegisterIndex Parameter1;
			RegisterIndex Parameter2;
			RegisterIndex Result;
		};

		struct SerilizedTAC
		{
			CommandType Type;
			RegisterIndex::Category ParameterCategory1;
			RegisterIndex::Category ParameterCategory2;
			RegisterIndex::Category ResultCategory;
			uint64_t Parameter1;
			uint64_t Parameter2;
			uint64_t Result;
		};

	private:

		Implement::ConstDataStorageTable const_data_table;
		std::optional<TypeLayout> CalculateTypeLayout(TypeMask const& ref) const;
		std::optional<TypeLayout> CalculateTypeLayout(TypeProperty const& ref, MemoryModel const& setting) const;
		std::vector<TypeDescription::Member> temporary_member_type;
		std::vector<std::optional<TypeProperty>> defined_types;
		std::vector<std::tuple<TypeMask, size_t>> define_stack_record;

	};
}