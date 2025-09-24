module;

#include <cassert>

export module PotatoIR;

import std;
import PotatoPointer;
import PotatoMisc;
export import PotatoMemLayout;
import PotatoEncode;

export namespace Potato::IR
{
	using MemLayout::Layout;
	using MemLayout::LayoutPolicyRef;
	using MemLayout::PolicyLayout;

	struct MemoryResourceRecord
	{
		std::pmr::memory_resource* resource = nullptr;
		Layout layout;
		void* adress = nullptr;
		void* Get() const { return adress; }
		std::byte* GetByte() const { return static_cast<std::byte*>(adress); }
		std::byte* GetByte(std::size_t byte_offset) const { return static_cast<std::byte*>(adress) + byte_offset; }

		std::pmr::memory_resource* GetMemoryResource() const{ return resource; }

		template<typename Type>
		Type* Cast() const {  assert(sizeof(Type) <= layout.size && alignof(Type) <= layout.align); return static_cast<Type*>(adress); }

		template<typename Type>
		std::span<Type> GetArray(std::size_t element_size, std::size_t offset) const
		{
			return std::span<Type>{ reinterpret_cast<Type*>(static_cast<std::byte*>(adress) + offset), element_size };
		}

		operator bool () const;
		static MemoryResourceRecord Allocate(std::pmr::memory_resource* resource, Layout layout);

		template<typename Type>
		static MemoryResourceRecord Allocate(std::pmr::memory_resource* resource) { return  Allocate(resource, Layout::Get<Type>()); }
		
		template<typename Type, typename ...OT>
		static Type* AllocateAndConstruct(std::pmr::memory_resource* resource, OT&&... ot)
			requires(std::is_constructible_v<Type, MemoryResourceRecord, OT&&...>)
		{
			auto re = Allocate<Type>(resource);
			if(re)
			{
				try{
					return new (re.Get()) Type{re, std::forward<OT>(ot)...};
				}catch(...)
				{
					re.Deallocate();
					throw;
				}
			}
			return nullptr;
		}
		
		bool Deallocate();
	};

	struct MemoryResourceRecordIntrusiveInterface
	{
		void AddRef() const { ref_count.AddRef(); }
		void SubRef() const;
		virtual ~MemoryResourceRecordIntrusiveInterface() = default;
	protected:
		MemoryResourceRecordIntrusiveInterface(MemoryResourceRecord record)
			: record(record) {
		}
		MemoryResourceRecord record;
		mutable Misc::AtomicRefCount ref_count;
	};

	enum class CustomConstructOperator
	{
		Move,
		Copy,
		Default
	};

	struct StructLayout
	{

		struct OperateProperty
		{
			bool construct_default = true;
			bool construct_copy = true;
			bool construct_move = true;
			bool assign_copy = true;
			bool assign_move = true;

			OperateProperty operator&&(OperateProperty const& i) const
			{
				return {
					construct_default&& i.construct_default,
					construct_copy&& i.construct_copy,
					construct_move&& i.construct_move,
					assign_copy&& i.assign_copy,
					assign_move&& i.assign_move
				};
			}

			bool IsEnableConstruct() const { return construct_default || construct_copy || construct_move; }
			bool EnableEqual() const { return assign_copy || assign_move; }
		};

		struct Wrapper
		{
			void AddRef(StructLayout const* ptr) const { ptr->AddStructLayoutRef(); }
			void SubRef(StructLayout const* ptr) const { ptr->SubStructLayoutRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<StructLayout const, Wrapper>;

		struct Member
		{
			Ptr struct_layout;
			std::u8string_view name;
			std::size_t array_count = 1;
		};

		static StructLayout::Ptr CreateDynamic(std::u8string_view name, std::span<Member const> members, LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual OperateProperty GetOperateProperty() const = 0;
		virtual bool DefaultConstruction(void* target, std::size_t array_count, std::size_t next_element_offset) const;
		virtual bool CopyConstruction(void* target, void const* source, std::size_t array_count, std::size_t next_element_offset) const;
		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count, std::size_t next_element_offset) const;
		virtual bool Destruction(void* target, std::size_t array_count, std::size_t next_element_offset) const;
		virtual bool CopyAssigned(void* target, void const* source, std::size_t array_count, std::size_t next_element_offset) const;
		virtual bool MoveAssigned(void* target, void* source, std::size_t array_count, std::size_t next_element_offset) const;
		
		bool DefaultConstruction(void* target) const { return DefaultConstruction(target, 1, 0); }
		bool CopyConstruction(void* target, void const* source) const { return CopyConstruction(target, source, 1, 0); }
		bool MoveConstruction(void* target, void* source) const { return MoveConstruction(target, source, 1, 0); }
		bool Destruction(void* target) const { return Destruction(target, 1, 0); }
		bool CopyAssigned(void* target, void const* source) const { return CopyConstruction(target, source, 1, 0); }
		bool MoveAssigned(void* target, void* source) const { return MoveAssigned(target, source, 1, 0); }
		
		struct CustomConstruct
		{
			CustomConstructOperator construct_operator = CustomConstructOperator::Default;
			union
			{
				void* construct_parameter_object = nullptr;
				void const* construct_parameter_const_object;
			}paramter_object;
		};

		virtual bool CustomConstruction(void* target, std::span<CustomConstruct const> custom_construct, std::size_t array_count, std::size_t next_element_offset) const;
		bool CustomConstruction(void* target, std::span<CustomConstruct const> custom_construct) const { return CustomConstruction(target, custom_construct, 1, 0); }

		template<typename Type>
		static Ptr GetStatic();

		struct MemberView
		{
			Ptr struct_layout;
			std::u8string_view name;
			MemLayout::MermberOffset offset;
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
		MemberView operator[](std::size_t index) const { auto span = GetMemberView(); assert(span.size() > index);  return span[index]; }
		virtual std::u8string_view GetName() const = 0;
		std::optional<MemberView> FindMemberView(std::u8string_view member_name) const;
		std::optional<MemberView> FindMemberView(std::size_t index) const;
		virtual Layout GetLayout() const = 0;

		static void* GetMemberData(MemberView const& member_view, void* target, std::size_t array_index = 0)
		{ 
			assert(target != nullptr); 
			return member_view.offset.GetMember(reinterpret_cast<std::byte*>(target), array_index);
		}

		static void const* GetMemberData(MemberView const& member_view, void const* target, std::size_t array_index = 0)
		{ 
			assert(target != nullptr);
			return member_view.offset.GetMember(reinterpret_cast<std::byte const*>(target), array_index);
		}

		static std::span<std::byte> GetMemberDataBuffer(MemberView const& member_view, void* target) {
			return member_view.offset.GetMemberBuffer(reinterpret_cast<std::byte*>(target));
		}

		static std::span<std::byte const> GetMemberDataBuffer(MemberView const& member_view, void const* target) {
			return member_view.offset.GetMemberBuffer(reinterpret_cast<std::byte const*>(target));
		}
		
		template<typename Type>
		static Type* GetMemberDataWithStaticCast(MemberView const& view, void* target, std::size_t array_index = 0)
		{
			return static_cast<Type*>(GetMemberData(view, target, array_index));
		}

		virtual std::size_t GetHashCode() const = 0;
		virtual ~StructLayout() = default;
		template<typename Type>
		bool IsStatic() const { return operator==(*GetStatic<Type>()); }
		
		bool operator== (StructLayout const& other) const;

	protected:

		virtual bool IsEqual(StructLayout const* other) const { return false; }
		virtual void AddStructLayoutRef() const = 0;
		virtual void SubStructLayoutRef() const = 0;
	};

	struct StructLayoutObject : public MemoryResourceRecordIntrusiveInterface
	{

		struct Wrapper
		{
			void AddRef(StructLayoutObject const* ptr) { ptr->AddStructLayoutObjectRef(); }
			void SubRef(StructLayoutObject const* ptr) { ptr->SubStructLayoutObjectRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<StructLayoutObject, Wrapper>;
		using ConstPtr = Potato::Pointer::IntrusivePtr<StructLayoutObject const, Wrapper>;

		static Ptr DefaultConstruct(StructLayout::Ptr struct_layout, std::size_t array_count = 1, std::pmr::memory_resource * resource = std::pmr::get_default_resource());
		static Ptr CopyConstruct(StructLayoutObject const& source, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return CopyConstruct(source.GetStructLayout(), source.GetArrayData(), source.GetArrayCount(), resource);
		}
		static Ptr MoveConstruct(StructLayoutObject& source, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return MoveConstruct(source.GetStructLayout(), source.GetArrayData(), source.GetArrayCount(), resource);
		}

		static Ptr CopyConstruct(StructLayout::Ptr struct_layout, void const* source, std::size_t array_count = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr struct_layout, void* source, std::size_t array_count = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		static Ptr CustomConstruction(StructLayout::Ptr struct_layout, std::span<StructLayout::CustomConstruct const> construct_parameter, std::size_t array_count, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		StructLayout::Ptr GetStructLayout() const { return struct_layout; };
		std::size_t GetArrayCount() const { return offset.element_count; }

		void const* GetArrayData(std::size_t element_index = 0) const { return const_cast<StructLayoutObject*>(this)->GetArrayData(element_index); };
		void const* GetArrayMemberData(StructLayout::MemberView const& member_view, std::size_t element_index = 0) const { return const_cast<StructLayoutObject*>(this)->GetArrayMemberData(member_view, element_index); }
		void const* GetArrayMemberData(std::size_t member_index, std::size_t element_index = 0) const { return const_cast<StructLayoutObject*>(this)->GetArrayMemberData(member_index, element_index); }

		void* GetArrayData(std::size_t element_index = 0) { return offset.GetMember(reinterpret_cast<std::byte*>(buffer), element_index); };
		void* GetArrayMemberData(StructLayout::MemberView const& member_view, std::size_t element_index = 0);
		void* GetArrayMemberData(std::size_t member_index, std::size_t element_index = 0);

		template<typename Type>
		Type* TryGetDataWithStaticCast(std::size_t array_index = 0);

		template<typename Type>
		Type* TryGetMemberDataWithStaticCast(std::size_t member_index, std::size_t array_index = 0);

		template<typename Type>
		Type const* TryGetDataWithStaticCast(std::size_t array_index = 0) const {
			return const_cast<StructLayoutObject*>(this)->TryGetDataWithStaticCast<Type>(array_index);
		}

		template<typename Type>
		Type const* TryGetMemberDataWithStaticCast(std::size_t member_index, std::size_t array_index = 0) const
		{
			return const_cast<StructLayoutObject*>(this)->TryGetMemberDataWithStaticCast<Type>(member_index, array_index);
		}

	protected:

		StructLayoutObject(MemoryResourceRecord record, void* buffer, MemLayout::MermberOffset offset, StructLayout::Ptr struct_layout)
			: MemoryResourceRecordIntrusiveInterface(record), buffer(buffer), offset(offset), struct_layout(std::move(struct_layout)) {}

		virtual ~StructLayoutObject();

		void* buffer = nullptr;
		StructLayout::Ptr struct_layout;
		MemLayout::MermberOffset offset;

		friend struct Pointer::DefaultIntrusiveWrapper;

		virtual void AddStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	template<typename Type>
	Type* StructLayoutObject::TryGetDataWithStaticCast(std::size_t array_index)
	{
		if (struct_layout->IsStatic<Type>())
		{
			return static_cast<Type*>(GetArrayData(array_index));
		}
		return nullptr;
	}

	template<typename Type>
	Type* StructLayoutObject::TryGetMemberDataWithStaticCast(std::size_t member_index, std::size_t array_index)
	{
		auto member = struct_layout->FindMemberView(member_index);
		if (member.has_value() && member->struct_layout->IsStatic<Type>())
		{
			return static_cast<Type*>(GetArrayMemberData(*member, array_index));
		}
		return nullptr;
	}

	template<typename AtomicType>
	struct StaticAtomicStructLayout : public StructLayout
	{

		static StructLayout::Ptr Create()
		{
			static StaticAtomicStructLayout<AtomicType> layout;
			return &layout;
		}
		virtual Layout GetLayout() const override { return Layout::Get<AtomicType>(); }
		virtual void AddStructLayoutRef() const override { }
		virtual void SubStructLayoutRef() const override {  }
		virtual std::u8string_view GetName() const override
		{
			std::u8string_view type_name = {reinterpret_cast<char8_t const*>(typeid(AtomicType).name())};
			return type_name;
		}
		std::span<MemberView const> GetMemberView() const override { return {}; }
		OperateProperty GetOperateProperty() const override
		{
			return {
				std::is_constructible_v<AtomicType>,
				 std::is_constructible_v<AtomicType, AtomicType const&>,
				 std::is_constructible_v<AtomicType, AtomicType &&>,
				std::is_copy_assignable_v<AtomicType>,
				std::is_move_assignable_v<AtomicType>
			};
		}

		virtual bool DefaultConstruction(void* target, std::size_t array_count, std::size_t elemnt_length) const override
		{
			assert(target != nullptr);
			if constexpr (std::is_constructible_v<AtomicType>)
			{
				std::byte* tar = static_cast<std::byte*>(target);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i * elemnt_length) AtomicType{};
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool CopyConstruction(void* target, void const* source, std::size_t array_count, std::size_t next_element_offset) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_constructible_v<AtomicType, AtomicType const&>)
			{
				std::byte* tar = static_cast<std::byte*>(target);
				std::byte const* sou = static_cast<std::byte const*>(source);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i * next_element_offset) AtomicType{ *reinterpret_cast<AtomicType const*>(sou + i * next_element_offset) };
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count, std::size_t next_element_offset) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_constructible_v<AtomicType, AtomicType&&>)
			{
				std::byte* tar = static_cast<std::byte*>(target);
				std::byte* sou = static_cast<std::byte*>(source);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i * next_element_offset) AtomicType{ std::move(*reinterpret_cast<AtomicType*>(sou + i * next_element_offset)) };
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool CopyAssigned(void* target, void const* source, std::size_t array_count, std::size_t next_element_offset) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_copy_assignable_v<AtomicType>)
			{
				std::byte* tar = static_cast<std::byte*>(target);
				std::byte const* sou = static_cast<std::byte const*>(source);
				for (std::size_t i = 0; i < array_count; ++i)
				{
					*reinterpret_cast<AtomicType*>(tar + i * next_element_offset) = *reinterpret_cast<AtomicType const*>(sou + i * next_element_offset);
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool MoveAssigned(void* target, void* source, std::size_t array_count, std::size_t next_element_offset) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_move_assignable_v<AtomicType>)
			{
				std::byte* tar = static_cast<std::byte*>(target);
				std::byte* sou = static_cast<std::byte*>(source);
				for (std::size_t i = 0; i < array_count; ++i)
				{
					*reinterpret_cast<AtomicType*>(tar + i * next_element_offset) = std::move(*reinterpret_cast<AtomicType*>(sou + i * next_element_offset));
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		std::size_t GetHashCode() const override { return typeid(AtomicType).hash_code(); }

		virtual bool Destruction(void* target, std::size_t array_count, std::size_t next_element_offset) const override
		{
			assert(target != nullptr);
			std::byte* tar = static_cast<std::byte*>(target);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				reinterpret_cast<AtomicType*>(tar + i * next_element_offset)->~AtomicType();
			}
			return true;
		}
		virtual bool CustonConstruction(void* target, std::span<CustomConstruct const> custom_construct, std::size_t array_count, std::size_t next_element_offset) { return false; }
	};

	template<typename Type>
	StructLayout::Ptr StructLayout::GetStatic()
	{
		return StaticAtomicStructLayout<std::remove_cvref_t<Type>>::Create();
	}

	

}