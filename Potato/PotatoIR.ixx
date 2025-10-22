module;

#include <cassert>

export module PotatoIR;

import std;
import PotatoPointer;
import PotatoMisc;
import PotatoTMP;
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
		struct MemberView;

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
			std::size_t array_count = 0;
			std::optional<Layout> overrided_memory_layout;
		};

		static StructLayout::Ptr CreateDynamic(std::u8string_view name, std::span<Member const> members, LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual OperateProperty GetOperateProperty() const = 0;

		virtual bool DefaultConstruction(void* target, MemLayout::ArrayLayout array_layout = {}) const;
		virtual bool CopyConstruction(void* target, void const* source, MemLayout::ArrayLayout target_array_layout = {}, MemLayout::ArrayLayout source_array_layout = {}) const;
		virtual bool MoveConstruction(void* target, void* source, MemLayout::ArrayLayout target_array_layout = {}, MemLayout::ArrayLayout source_array_layout = {}) const;
		virtual bool Destruction(void* target, MemLayout::ArrayLayout target_array_layout = {}) const;
		virtual bool CopyAssigned(void* target, void const* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const;
		virtual bool MoveAssigned(void* target, void* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const;


		struct CustomConstruct
		{
			CustomConstructOperator construct_operator = CustomConstructOperator::Default;
			union
			{
				void* construct_parameter_object = nullptr;
				void const* construct_parameter_const_object;
			}paramter_object;
			MemLayout::ArrayLayout array_layout = {};
		};

		virtual bool CustomConstruction(void* target, std::span<CustomConstruct const> custom_construct) const;

		template<typename Type>
		static Ptr GetStatic();

		struct MemberView
		{
			Ptr struct_layout;
			std::u8string_view name;
			MemLayout::MermberLayout member_layout;
			std::byte const* GetByte(void const* object_buffer, std::size_t array_index = 0) const { return member_layout.GetMember(object_buffer, array_index); }
			std::byte* GetByte(void* object_buffer, std::size_t array_index = 0) const { return member_layout.GetMember(object_buffer, array_index); }
			template<typename Type>
			Type* As(void* object_buffer, std::size_t array_index = 0) const {
				assert(struct_layout->IsStatic<Type>());
				return reinterpret_cast<Type*>(GetByte(object_buffer, array_index));
			};
			template<typename Type>
			Type const* As(void const* object_buffer, std::size_t array_index = 0) const {
				assert(struct_layout->IsStatic<Type>());
				return reinterpret_cast<Type const*>(GetByte(object_buffer, array_index));
			};
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
		MemberView operator[](std::size_t index) const { auto span = GetMemberView(); assert(span.size() > index);  return span[index]; }
		virtual std::u8string_view GetName() const = 0;
		std::optional<MemberView> FindMemberView(std::u8string_view member_name) const;
		std::optional<MemberView> FindMemberView(std::size_t index) const;
		virtual Layout GetLayout() const = 0;

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

		static Ptr DefaultConstruct(StructLayout::Ptr struct_layout, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr CopyConstruct(StructLayoutObject const& source, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return CopyConstruct(source.GetStructLayout(), static_cast<void const*>(source.GetBuffer()), resource);
		}
		static Ptr MoveConstruct(StructLayoutObject& source, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return MoveConstruct(source.GetStructLayout(), static_cast<void*>(source.GetBuffer()), resource);
		}

		static Ptr CopyConstruct(StructLayout::Ptr struct_layout, void const* source, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr struct_layout, void* source, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		static Ptr CustomConstruction(StructLayout::Ptr struct_layout, std::span<StructLayout::CustomConstruct const> construct_parameter, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		StructLayout::Ptr GetStructLayout() const { return struct_layout; };
		std::byte const* GetBuffer() const { return static_cast<std::byte const*>(buffer); }
		std::byte* GetBuffer() { return static_cast<std::byte*>(buffer); }
		template<typename Type>
		Type* As() {
			assert(struct_layout->IsStatic<Type>());
			return static_cast<Type*>(buffer);
		}
		template<typename Type>
		Type const* As() const {
			assert(struct_layout->IsStatic<Type>());
			return static_cast<Type const*>(buffer);
		}

		template<typename Type>
		Type* TryGetDataWithStaticCast();

		template<typename Type>
		Type* TryGetMemberDataWithStaticCast(std::size_t member_index);

		template<typename Type>
		Type const* TryGetDataWithStaticCast() const {
			return const_cast<StructLayoutObject*>(this)->TryGetDataWithStaticCast<Type>();
		}

		template<typename Type>
		Type const* TryGetMemberDataWithStaticCast(std::size_t member_index) const
		{
			return const_cast<StructLayoutObject*>(this)->TryGetMemberDataWithStaticCast<Type>(member_index);
		}

	protected:

		StructLayoutObject(MemoryResourceRecord record, void* buffer, StructLayout::Ptr struct_layout)
			: MemoryResourceRecordIntrusiveInterface(record), buffer(buffer), struct_layout(std::move(struct_layout)) {}

		virtual ~StructLayoutObject();

		void* buffer = nullptr;
		StructLayout::Ptr struct_layout;

		friend struct Pointer::DefaultIntrusiveWrapper;

		virtual void AddStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	template<typename Type>
	Type* StructLayoutObject::TryGetDataWithStaticCast()
	{
		if (struct_layout->IsStatic<Type>())
		{
			return reinterpret_cast<Type*>(GetBuffer());
		}
		return nullptr;
	}

	template<typename Type>
	Type* StructLayoutObject::TryGetMemberDataWithStaticCast(std::size_t member_index)
	{
		auto member = struct_layout->FindMemberView(member_index);
		if (member.has_value() && member->struct_layout->IsStatic<Type>())
		{
			return member->As<Type>(GetBuffer());
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

		virtual bool DefaultConstruction(void* target, MemLayout::ArrayLayout array_layout) const override
		{
			assert(target != nullptr);
			if constexpr (std::is_constructible_v<AtomicType>)
			{
				auto count = std::max(array_layout.count, std::size_t{1});
				for (std::size_t i = 0; i < count; ++i)
				{
					new (array_layout.GetElement(static_cast<std::byte*>(target), i)) AtomicType{};
				}
				return true;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool CopyConstruction(void* target, void const* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const override
		{
			if constexpr (std::is_constructible_v<AtomicType, AtomicType const&>)
			{
				assert(target != source && target != nullptr && source != nullptr);
				assert(target_array_layout.count == source_array_layout.count);
				if (target_array_layout.count == source_array_layout.count)
				{
					auto count = std::max(target_array_layout.count, std::size_t{ 1 });
					for (std::size_t i = 0; i < count; ++i)
					{
						new (target_array_layout.GetElement(target, i)) AtomicType{
							*reinterpret_cast<AtomicType const*>(source_array_layout.GetElement(source, i))
						};
					}
					return true;
				}
				return false;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool MoveConstruction(void* target, void* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const override
		{
			if constexpr (std::is_constructible_v<AtomicType, AtomicType&&>)
			{
				assert(target != source && target != nullptr && source != nullptr);
				assert(target_array_layout.count == source_array_layout.count);
				if (target_array_layout.count == source_array_layout.count)
				{
					auto count = std::max(target_array_layout.count, std::size_t{ 1 });
					for (std::size_t i = 0; i < count; ++i)
					{
						new (target_array_layout.GetElement(target, i)) AtomicType{
							std::move(*reinterpret_cast<AtomicType*>(source_array_layout.GetElement(source, i)))
						};
					}
					return true;
				}
				return false;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool CopyAssigned(void* target, void const* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const override
		{
			if constexpr (std::is_copy_assignable_v<AtomicType>)
			{
				assert(target != source && target != nullptr && source != nullptr);
				assert(target_array_layout.count == source_array_layout.count);
				if (target_array_layout.count == source_array_layout.count)
				{
					auto count = std::max(target_array_layout.count, std::size_t{ 1 });
					for (std::size_t i = 0; i < count; ++i)
					{
						*reinterpret_cast<AtomicType*>(target_array_layout.GetElement(target, i))
							=
							*reinterpret_cast<AtomicType const*>(source_array_layout.GetElement(source, i));
					}
					return true;
				}
				return false;
			}
			else {
				assert(false);
				return false;
			}
		}

		virtual bool MoveAssigned(void* target, void* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const override
		{
			if constexpr (std::is_move_assignable_v<AtomicType>)
			{
				assert(target != source && target != nullptr && source != nullptr);
				assert(target_array_layout.count == source_array_layout.count);
				if (target_array_layout.count == source_array_layout.count)
				{
					auto count = std::max(target_array_layout.count, std::size_t{ 1 });
					for (std::size_t i = 0; i < count; ++i)
					{
						*reinterpret_cast<AtomicType*>(target_array_layout.GetElement(target, i))
							=
							std::move(*reinterpret_cast<AtomicType*>(source_array_layout.GetElement(source, i)));
					}
					return true;
				}
				return false;
			}
			else {
				assert(false);
				return false;
			}
		}

		std::size_t GetHashCode() const override { return typeid(AtomicType).hash_code(); }

		virtual bool Destruction(void* target, MemLayout::ArrayLayout target_array_layout) const override
		{
			assert(target != nullptr);
			auto count = std::max(target_array_layout.count, std::size_t{ 1 });
			for (std::size_t i = 0; i < count; ++i)
			{
				reinterpret_cast<AtomicType*>(target_array_layout.GetElement(target, i))->~AtomicType();
			}
			return true;
		}
		virtual bool CustomConstruction(void* target, std::span<StructLayout::CustomConstruct const> custom_construct) const override { assert(false); return false; }
	};

	template<typename Type>
	StructLayout::Ptr StructLayout::GetStatic()
	{
		return StaticAtomicStructLayout<std::remove_cvref_t<Type>>::Create();
	}

	

}