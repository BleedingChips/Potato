#pragma once
#include <stdint.h>
#include <string>
#include "Types.h"
#include "Misc.h"
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

	

	struct Mask
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

		Mask(Mask const&) = default;
		Mask& operator=(Mask const&) = default;
		Mask(Type register_type, uint64_t index) : register_type(register_type), index(index) {};
		Mask() : Mask(Type::None, 0) {}
		Mask(StorageType desc) : Mask(Type::BUILINVALUE, static_cast<uint64_t>(desc)) {};
		operator bool () const noexcept {return register_type != Type::None; }
		Type Register() const noexcept {return register_type; };
		uint64_t Index() const noexcept{return index;}
	private:
		Type register_type;
		uint64_t index;
	};

	struct Layout
	{
		uint64_t align = 1;
		uint64_t size = 0;
		constexpr Layout(uint64_t i_align = 0, uint64_t i_size = 0)
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
			REFERENCE,
			POINTER,
			ARRAY,
		};
		Type type;
		uint64_t parameter;
	};

	struct TypeReference
	{
		Mask type;
		std::vector<Modifier> modifier;
	};

	struct TypeProperty
	{
		struct Member
		{
			TypeReference type_reference;
			std::u32string name;
			uint32_t offset;
			Layout layout;
		};
		Layout layout;
		std::vector<Member> members;
	};


	struct HLForm
	{

		struct MemberOrderPolicy
		{

		};


		Mask InserConstData(TypeReference desc, Layout layout, std::span<std::byte const> data);
		
		template<typename Type>
		Mask InserConstData(Type&& data) requires (StorageTypeEnumV<std::remove_cvref_t<Type>> != StorageType::CUSTOM)
		{
			using Type = std::remove_cvref_t<Type>;
			return InserConstData(TypeReference{ StorageTypeEnumV<Type>, {}}, {alignof(Type), sizeof(Type)}, std::span<std::byte const>{reinterpret_cast<std::byte const*>(&data), sizeof(data)});
		}

		template<typename Type>
		Mask InserConstData(std::span<Type> span) requires(StorageTypeEnumV<std::remove_pointer_t<std::remove_const_t<Type>>> != StorageType::CUSTOM)
		{
			using Type = std::remove_const_t<Type>;
			return InserConstData(TypeReference{ StorageTypeEnumV<Type>, {Modifier::Type::ARRAY, span.size()} }, { alignof(Type), sizeof(Type) }, std::span<std::byte const>{reinterpret_cast<std::byte const*>(span.data()), sizeof(Type) * span.size()});
		}

		Mask ForwardDefineTypeproperty();

		void PushTypePropertyMember(TypeReference reference, std::u32string_view);
		bool DefineTypeProperty(Mask ForwardDefine, uint64_t member_count, MemberOrderPolicy policy);

	private:

		enum class Operator
		{
			
		};

		struct TAC
		{
			Operator operator_enum;
			uint64_t source1;
			uint64_t source2;
			uint64_t desction;
		};

		std::vector<TAC> command;
		std::vector<TAC> function_command;
		std::vector<std::byte> const_buffer;
		struct ConstElement
		{
			TypeReference type_reference;
			IndexSpan<> span;
			Layout layout;
		};
		std::vector<ConstElement> const_elements;
		std::vector<std::optional<TypeProperty>> type_define;
		std::vector<TypeProperty::Member> temporary_member;
		struct StackElement
		{
			TypeReference type_reference;
			Layout layout;
		};
		std::vector<StackElement> stack_elements;
	};
}