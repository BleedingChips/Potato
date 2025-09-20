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
			std::string_view name;
			std::size_t array_count = 1;
		};

		static StructLayout::Ptr CreateDynamic(std::string_view name, std::span<Member const> members, LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual OperateProperty GetOperateProperty() const = 0;
		virtual bool DefaultConstruction(void* target, std::size_t array_count = 1) const;
		virtual bool CopyConstruction(void* target, void const* source, std::size_t array_count = 1) const;
		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count = 1) const;
		virtual bool Destruction(void* target, std::size_t array_count = 1) const;
		virtual bool CopyAssigned(void* target, void const* source, std::size_t array_count = 1) const;
		virtual bool MoveAssigned(void* target, void* source, std::size_t array_count = 1) const;

		template<typename Type>
		static Ptr GetStatic();

		struct MemberView
		{
			Ptr struct_layout;
			std::string_view name;
			std::size_t array_count;
			Misc::IndexSpan<> combined_offset;
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
		MemberView operator[](std::size_t index) const { auto span = GetMemberView(); assert(span.size() > index);  return span[index]; }
		virtual std::string_view GetName() const = 0;
		std::optional<MemberView> FindMemberView(std::string_view member_name) const;
		std::optional<MemberView> FindMemberView(std::size_t index) const;
		virtual Layout GetLayout() const = 0;

		static void* GetMemberData(MemberView const& member_view, void* target) { assert(target != nullptr); return static_cast<std::byte*>(target) + member_view.combined_offset.Begin(); }
		static void const* GetMemberData(MemberView const& member_view, void const* target) { return GetMemberData(member_view, const_cast<void*>(target)); }
		
		template<typename Type>
		static Type* GetMemberDataWithStaticCast(MemberView const& view, void* target)
		{
			return static_cast<Type*>(GetMemberData(view, target));
		}

		template<typename Type>
		static Type* GetMemberDataWithStaticCast(MemberView const& view, void const* target)
		{
			return static_cast<Type*>(GetMemberData(view, target));
		}

		static std::span<std::byte> GetMemberDataBuffer(MemberView const& member_view, void* target) {
			return {
				static_cast<std::byte*>(target) + member_view.combined_offset.Begin(),
				member_view.combined_offset.Size()
			};
		}
		static std::span<std::byte const> GetMemberDataBuffer(MemberView const& member_view, void const* target) {
			return GetMemberDataBuffer(member_view, const_cast<void*>(target));
		}

		template<typename Type>
		static std::span<Type> GetMemberDataArrayWithStaticCast(MemberView const& view, void* target)
		{
			return {
				GetMemberDataWithStaticCast<Type>(view, target),
				view.array_count
			};
		}

		template<typename Type>
		static std::span<Type const> GetMemberDataArrayWithStaticCast(MemberView const& view, void const* target)
		{
			return GetMemberDataArrayWithStaticCast(view, const_cast<void*>(target));
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

		enum class ConstructOperator
		{
			Move,
			Copy,
			Default
		};

		struct MemberConstruct
		{
			ConstructOperator construct_operator  = ConstructOperator::Default;
			void* construct_parameter_object = nullptr;
		};

		static Ptr DefaultConstruct(StructLayout::Ptr struct_layout, std::size_t array_count = 1,  std::pmr::memory_resource* resource = std::pmr::get_default_resource());
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

		static Ptr Construct(StructLayout::Ptr struct_layout, std::span<MemberConstruct const> construct_parameter, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		StructLayout::Ptr GetStructLayout() const { return struct_layout; };
		std::size_t GetArrayCount() const { return array_count; }

		void const* GetArrayData(std::size_t element_index = 0) const { return const_cast<StructLayoutObject*>(this)->GetArrayData(element_index); };
		void const* GetArrayMemberData(StructLayout::MemberView const& member_view, std::size_t element_index = 0) const { return const_cast<StructLayoutObject*>(this)->GetArrayMemberData(member_view, element_index); }
		void const* GetArrayMemberData(std::size_t member_index, std::size_t element_index = 0) const { return const_cast<StructLayoutObject*>(this)->GetArrayMemberData(member_index, element_index); }

		void* GetArrayData(std::size_t element_index = 0) { return static_cast<std::byte*>(start) + struct_layout->GetLayout().size * element_index; };
		void* GetArrayMemberData(StructLayout::MemberView const& member_view, std::size_t element_index = 0);
		void* GetArrayMemberData(std::size_t member_index, std::size_t element_index = 0);

		template<typename Type>
		Type* TryGetArrayDataWithStaticCast(std::size_t element_index = 0);

		template<typename Type>
		Type* TryGetArrayMemberDataWithStaticCast(std::size_t member_index, std::size_t element_index = 0);

		template<typename Type>
		Type const* TryGetArrayDataWithStaticCast(std::size_t element_index = 0) const { 
			return const_cast<StructLayoutObject*>(this)->TryGetArrayDataWithStaticCast<Type>(element_index);
		}

		template<typename Type>
		Type const* TryGetArrayMemberDataWithStaticCast(std::size_t member_index, std::size_t element_index = 0) const
		{
			return const_cast<StructLayoutObject*>(this)->TryGetArrayMemberDataWithStaticCast<Type>(member_index, element_index);
		}

	protected:

		StructLayoutObject(MemoryResourceRecord record, void* start, std::size_t array_count, StructLayout::Ptr struct_layout)
			: MemoryResourceRecordIntrusiveInterface(record), start(start), array_count(array_count), struct_layout(std::move(struct_layout)) {}

		virtual ~StructLayoutObject();

		void* start = nullptr;
		std::size_t array_count = 0;
		StructLayout::Ptr struct_layout;

		friend struct Pointer::DefaultIntrusiveWrapper;

		virtual void AddStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	template<typename Type>
	Type* StructLayoutObject::TryGetArrayDataWithStaticCast(std::size_t element_index)
	{
		if (struct_layout->IsStatic<Type>())
		{
			return static_cast<Type*>(GetArrayData(element_index));
		}
		return nullptr;
	}

	template<typename Type>
	Type* StructLayoutObject::TryGetArrayMemberDataWithStaticCast(std::size_t member_index, std::size_t element_index)
	{
		auto member = struct_layout->FindMemberView(member_index);
		if (member.has_value() && member->struct_layout->IsStatic<Type>())
		{
			return static_cast<Type*>(GetArrayMemberData(*member, element_index));
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
		virtual std::string_view GetName() const override
		{
			return typeid(AtomicType).name();
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

		virtual bool DefaultConstruction(void* target, std::size_t array_count = 1) const override
		{
			assert(target != nullptr);
			if constexpr (std::is_constructible_v<AtomicType>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i) AtomicType{};
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool CopyConstruction(void* target, void const* source, std::size_t array_count = 1) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_constructible_v<AtomicType, AtomicType const&>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				AtomicType const* sou = static_cast<AtomicType const*>(source);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i) AtomicType{ *(sou + i) };
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count = 1) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_constructible_v<AtomicType, AtomicType&&>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				AtomicType* sou = static_cast<AtomicType*>(source);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i) AtomicType{ std::move(*(sou + i)) };
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool CopyAssigned(void* target, void const* source, std::size_t array_count = 1) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_copy_assignable_v<AtomicType>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				AtomicType const* sou = static_cast<AtomicType const*>(source);
				for (std::size_t i = 0; i < array_count; ++i)
				{
					tar[i] = (sou[i]);
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool MoveAssigned(void* target, void* source, std::size_t array_count = 1) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_move_assignable_v<AtomicType>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				AtomicType* sou = static_cast<AtomicType*>(source);
				for (std::size_t i = 0; i < array_count; ++i)
				{
					tar[i] = (std::move(sou[i]));
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		std::size_t GetHashCode() const override { return typeid(AtomicType).hash_code(); }

		virtual bool Destruction(void* target, std::size_t array_count = 1) const override
		{
			assert(target != nullptr);
			AtomicType* tar = static_cast<AtomicType*>(target);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				(tar + i)->~AtomicType();
			}
			return true;
		}
	};

	template<typename Type>
	StructLayout::Ptr StructLayout::GetStatic()
	{
		return StaticAtomicStructLayout<std::remove_cvref_t<Type>>::Create();
	}

	

}