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
		
		void Deallocate();
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

	template<typename Type>
	struct StructLayoutOverride
	{
		Layout GetLayout() const { return Layout::Get<Type>(); }
		Layout GetLayoutAsMember() const { return Layout::Get<Type>(); }
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
		};

		static StructLayout::Ptr CreateDynamic(std::u8string_view name, std::span<Member const> members, LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual OperateProperty GetOperateProperty() const = 0;

		virtual bool DefaultConstruction(void* target, MemLayout::ArrayLayout array_layout = {}) const;
		virtual bool CopyConstruction(void* target, void const* source, MemLayout::ArrayLayout target_array_layout = {}, MemLayout::ArrayLayout source_array_layout = {}) const;
		virtual bool MoveConstruction(void* target, void* source, MemLayout::ArrayLayout target_array_layout = {}, MemLayout::ArrayLayout source_array_layout = {}) const;
		virtual bool Destruction(void* target, MemLayout::ArrayLayout target_array_layout = {}) const;
		virtual bool CopyAssigned(void* target, void const* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const;
		virtual bool MoveAssigned(void* target, void* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const;
		virtual std::optional<std::size_t> LocateNativeType(std::span<std::type_index const>) const { return std::nullopt; }
		std::optional<std::size_t> LocateNativeType(std::type_index type_index) const { return LocateNativeType({&type_index, 1}); }

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

		template<typename Type, template<typename> class Override = StructLayoutOverride>
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
				assert(struct_layout->LocateNativeType(typeid(std::remove_reference_t<Type>)).has_value());
				return reinterpret_cast<Type*>(GetByte(object_buffer, array_index));
			};
			template<typename Type>
			Type const* As(void const* object_buffer, std::size_t array_index = 0) const {
				assert(struct_layout->LocateNativeType(typeid(std::remove_reference_t<Type>)).has_value());
				return reinterpret_cast<Type const*>(GetByte(object_buffer, array_index));
			};
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
		MemberView operator[](std::size_t index) const { auto span = GetMemberView(); assert(span.size() > index);  return span[index]; }
		virtual std::u8string_view GetName() const = 0;
		std::optional<MemberView> FindMemberView(std::u8string_view member_name) const;
		std::optional<MemberView> FindMemberView(std::size_t index) const;
		virtual Layout GetLayout() const = 0;
		virtual Layout GetLayoutAsMember() const = 0;

		virtual std::uint64_t GetHashCode() const = 0;
		virtual ~StructLayout() = default;
		
		bool operator== (StructLayout const& other) const;
		static std::uint64_t CalculateHashCode(std::u8string_view name, std::span<MemberView const> member);
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

		static Ptr DefaultConstruct(StructLayout::Ptr struct_layout, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return DefaultConstruct(std::move(struct_layout), 0, {}, resource);
		}
		static Ptr DefaultConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, MemLayout::LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr CopyConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, void const* source, MemLayout::ArrayLayout source_array_layout, MemLayout::LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr CopyConstruct(StructLayout::Ptr struct_layout, void const* source, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return CopyConstruct(std::move(struct_layout), std::size_t{0}, source, MemLayout::ArrayLayout{ 0, 0 }, MemLayout::LayoutPolicyRef{}, resource);
		}
		static Ptr CopyConstruct(StructLayoutObject const& source, MemLayout::LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return CopyConstruct(source.GetStructLayout(), source.GetArrayCount(), source.GetObject(), source.GetArrayLayout(), policy, resource);
		}
		static Ptr MoveConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, void* source, MemLayout::ArrayLayout source_array_layout, MemLayout::LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr struct_layout, void* source, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return MoveConstruct(std::move(struct_layout), std::size_t{ 0 }, source, MemLayout::ArrayLayout{ 0, 0 }, MemLayout::LayoutPolicyRef{}, resource);
		}
		static Ptr MoveConstruct(StructLayoutObject& source, MemLayout::LayoutPolicyRef policy = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			return MoveConstruct(source.GetStructLayout(), source.GetArrayCount(), source.GetObject(), source.GetArrayLayout(), policy, resource);
		}

		static Ptr CustomConstruction(StructLayout::Ptr struct_layout, std::span<StructLayout::CustomConstruct const> construct_parameter, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		StructLayout::Ptr GetStructLayout() const { return struct_layout; };
		std::span<StructLayout::MemberView const> GetMember() const { return struct_layout->GetMemberView(); }
		std::byte const* GetObject(std::size_t array_index = 0) const { return array_layout.GetElement(buffer.data(), array_index); }
		std::byte* GetObject(std::size_t array_index = 0) { return array_layout.GetElement(buffer.data(), array_index); }
		
		template<typename Type>
		Type* As(std::size_t array_index = 0) {
			assert(struct_layout->LocateNativeType(typeid(std::remove_reference_t<Type>)).has_value());
			return reinterpret_cast<Type*>(GetObject(array_index));
		}

		template<typename Type>
		Type const* As(std::size_t array_index = 0) const {
			return const_cast<StructLayoutObject*>(this)->As<Type>(array_index);
		}

		template<typename Type>
		Type* MemberAs(std::size_t member_index, std::size_t array_index = 0) {
			auto mvs = struct_layout->GetMemberView();
			assert(member_index < mvs.size());
			return reinterpret_cast<Type*>(struct_layout->GetMemberView()[member_index].As<Type>(GetObject(array_index)));
		}

		template<typename Type>
		Type const* MemberAs(std::size_t member_index, std::size_t array_index = 0) const {
			return const_cast<StructLayoutObject*>(this)->MemberAs(member_index, array_index);
		}

		MemLayout::ArrayLayout GetArrayLayout() const { return array_layout; }
		std::size_t GetArrayCount() const { return GetArrayLayout().count; }
		std::span<std::byte const> GetBuffer() const { return buffer; }
		std::span<std::byte> GetBuffer() { return buffer; }

	protected:

		static std::tuple<Layout, MemLayout::MermberLayout, std::size_t> CalculateMemberLayout(StructLayout const& struct_layout, std::size_t array_count, MemLayout::LayoutPolicyRef member_policy);

		StructLayoutObject(MemoryResourceRecord record, std::span<std::byte> buffer, StructLayout::Ptr struct_layout, MemLayout::ArrayLayout array_layout)
			: MemoryResourceRecordIntrusiveInterface(record), buffer(buffer), struct_layout(std::move(struct_layout)), array_layout(array_layout){}

		virtual ~StructLayoutObject();

		std::span<std::byte> buffer;
		StructLayout::Ptr struct_layout;
		MemLayout::ArrayLayout array_layout;

		friend struct Pointer::DefaultIntrusiveWrapper;

		virtual void AddStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutObjectRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	template<typename AtomicType, template<typename> class Override>
	struct StaticAtomicStructLayout : public StructLayout
	{

		static StructLayout::Ptr Create()
		{
			static StaticAtomicStructLayout<AtomicType, Override> layout;
			return &layout;
		}
		virtual Layout GetLayout() const override { return Override<AtomicType>{}.GetLayout(); }
		virtual Layout GetLayoutAsMember() const override { return Override<AtomicType>{}.GetLayoutAsMember(); }
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

		std::uint64_t GetHashCode() const override { return typeid(AtomicType).hash_code(); }

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
		virtual std::optional<std::size_t> LocateNativeType(std::span<std::type_index const> type_index) const override {
			std::type_index cur_locate = typeid(AtomicType);
			auto finded = std::find(type_index.begin(), type_index.end(), cur_locate);
			if (finded != type_index.end())
			{
				return std::distance(type_index.begin(), finded);
			}
			return std::nullopt;
		}
	};

	template<typename Type, template<typename> class Override>
	StructLayout::Ptr StructLayout::GetStatic()
	{
		return StaticAtomicStructLayout<std::remove_cvref_t<Type>, Override>::Create();
	}

	template<std::size_t MemberCount>
		requires(MemberCount >= 1)
	struct StaticReferenceStructLayout : public StructLayout
	{
		template<typename ...OMemberT>
		requires(
			sizeof...(OMemberT) == MemberCount
				&& (std::is_constructible_v<OMemberT, StructLayout::Member const&> && ... && true)
				)
		StaticReferenceStructLayout(std::u8string_view name, LayoutPolicyRef policy = {}, OMemberT&& ...omember)
			: name(name)
		{
			std::array<StructLayout::Member const*, MemberCount> member_span = { &omember... };
			for (std::size_t i = 0; i < MemberCount; ++i)
			{
				auto& ite_member = *member_span[i];
				operate_property = operate_property && ite_member.struct_layout->GetOperateProperty();
				auto offset = *policy.Combine(member_layout, ite_member.struct_layout->GetLayoutAsMember(), ite_member.array_count);
				member_view[i] = {ite_member.struct_layout, ite_member.name, offset };
			}
			hash_code = StructLayout::CalculateHashCode(name, member_view);
			total_layout = *policy.Complete(member_layout);
		}
		virtual OperateProperty GetOperateProperty() const override { return operate_property; }
		virtual std::uint64_t GetHashCode() const override { return hash_code; }
		virtual Layout GetLayout() const override { return total_layout; }
		virtual Layout GetLayoutAsMember() const override { return member_layout; }
		virtual std::span<MemberView const> GetMemberView() const override { return member_view; }
		virtual std::u8string_view GetName() const override { return name; }
	protected:
		std::u8string_view name;
		std::array<MemberView, MemberCount> member_view;
		OperateProperty operate_property;
		std::uint64_t hash_code = 0;
		Potato::MemLayout::Layout member_layout;
		Potato::MemLayout::Layout total_layout;
		virtual void AddStructLayoutRef() const {}
		virtual void SubStructLayoutRef() const {}
	};
	
	template <class First, class... Rest>
		requires(
			std::is_constructible_v<First, StructLayout::Member const&>
			 && (std::is_constructible_v<Rest, StructLayout::Member const&> && ... && true)
		)
		StaticReferenceStructLayout(std::u8string_view, LayoutPolicyRef, First, Rest...)->StaticReferenceStructLayout<1 + sizeof...(Rest)>;

}