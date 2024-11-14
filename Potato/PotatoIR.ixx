module;

#include <cassert>

export module PotatoIR;

import std;
import PotatoPointer;
import PotatoMisc;
export import PotatoMemLayout;

export namespace Potato::IR
{
	using Layout = MemLayout::Layout;

	struct MemoryResourceRecord
	{
		std::pmr::memory_resource* resource = nullptr;
		Layout layout;
		void* adress = nullptr;
		void* Get() const { return adress; }
		std::byte* GetByte() const { return static_cast<std::byte*>(adress); }

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

	struct StructLayout
	{

		struct OperateProperty
		{
			bool default_construct : 1 = true;
			bool copy_construct : 1 = true;
			bool move_construct : 1 = true;
			bool copy_assigned : 1 = true;
			bool move_assigned : 1 = true;

			OperateProperty operator&&(OperateProperty const& i) const
			{
				return {
					default_construct && i.default_construct,
					copy_construct&& i.copy_construct,
					move_construct&& i.move_construct,
					copy_assigned&& i.copy_assigned,
					move_assigned&& i.move_assigned
				};
			}

			bool EnableConstruct() const { return default_construct || copy_construct || move_construct; }
			bool EnableEqual() const { return copy_assigned || move_assigned; }
		};

		struct Wrapper
		{
			void AddRef(StructLayout const* ptr) const { ptr->AddStructLayoutRef(); }
			void SubRef(StructLayout const* ptr) const { ptr->SubStructLayoutRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<StructLayout const, Wrapper>;

		struct Member
		{
			StructLayout::Ptr type_id;
			std::u8string_view name;
			std::size_t array_count = 1;
			void* init_object = nullptr;
		};

		virtual OperateProperty GetOperateProperty() const = 0;
		virtual bool DefaultConstruction(void* target, std::size_t array_count = 1) const;
		virtual bool CopyConstruction(void* target, void* source, std::size_t array_count = 1) const;
		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count = 1) const;
		virtual bool Destruction(void* target, std::size_t array_count = 1) const;
		virtual bool CopyAssigned(void* target, void* source, std::size_t array_count = 1) const;
		virtual bool MoveAssigned(void* target, void* source, std::size_t array_count = 1) const;

		struct MemberView
		{
			StructLayout::Ptr struct_layout;
			std::u8string_view name;
			std::size_t array_count;
			std::size_t offset;
			void* init_object = nullptr;
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
		MemberView operator[](std::size_t index) const { auto span = GetMemberView(); assert(span.size() > index);  return span[index]; }
		virtual std::u8string_view GetName() const = 0;
		std::optional<MemberView> FindMemberView(std::u8string_view member_name) const;
		virtual Layout GetLayout() const = 0;
		Layout GetLayout(std::size_t array_count) const;
		static void* GetData(MemberView const&, void* target);
		template<typename Type>
		static Type* GetDataAs(MemberView const& view, void* target)
		{
			return static_cast<Type*>(GetData(view, target));
		}
		static std::span<std::byte> GetDataSpan(MemberView const&, void* target);
		template<typename Type>
		static std::span<Type> GetDataSpanAs(MemberView const& view, void* target)
		{
			return {
				GetDataAs<Type>(view, target),
				view.array_count
			};
		}
		virtual std::size_t GetHashCode() const = 0;
		virtual ~StructLayout() = default;
		virtual std::strong_ordering operator<=>(StructLayout const& layout) const;
		bool operator==(StructLayout const& layout) const { return operator<=>(layout) == std::strong_ordering::equal; }
		bool operator<(StructLayout const& layout) const { return operator<=>(layout) == std::strong_ordering::less; }

	protected:
		
		virtual void AddStructLayoutRef() const = 0;
		virtual void SubStructLayoutRef() const = 0;
	};

	struct StructLayoutObject : protected Pointer::DefaultIntrusiveInterface
	{

		using Ptr = Pointer::IntrusivePtr<StructLayoutObject>;


		static Ptr DefaultConstruct(StructLayout::Ptr layout, std::size_t array_count = 1,  std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr CopyConstruct(StructLayout::Ptr layout, StructLayoutObject& source, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr layout, StructLayoutObject& source, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		static Ptr CopyConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		StructLayout::Ptr GetStructLayout() const { return layout; };
		std::size_t GetArrayCount() const { return array_count; }
		void* GetData() const { return start; }
		void* GetData(std::size_t index) const { return static_cast<std::byte*>(start) + layout->GetLayout().size * index; }

	protected:

		virtual void Release() override;

		StructLayoutObject(void* start, std::size_t array_count, StructLayout::Ptr layout, MemoryResourceRecord record)
			: start(start), array_count(array_count), layout(std::move(layout)), record(record) {}

		virtual ~StructLayoutObject();

		void* start;
		std::size_t array_count;
		StructLayout::Ptr layout;
		MemoryResourceRecord record;

		friend struct Pointer::DefaultIntrusiveWrapper;
	};

	struct DynamicStructLayout : public StructLayout, public Pointer::DefaultIntrusiveInterface
	{
		static StructLayout::Ptr Create(std::u8string_view name, std::span<Member const> members, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual std::u8string_view GetName() const override { return name; }
		virtual void AddStructLayoutRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutRef() const override { DefaultIntrusiveInterface::SubRef(); }
		virtual void Release() override;
		Layout GetLayout() const override;
		std::span<MemberView const> GetMemberView() const override;
		OperateProperty GetOperateProperty() const override { return construct_property; }
		virtual std::size_t GetHashCode() const override { return hash_code; }

	protected:

		DynamicStructLayout(OperateProperty construct_property, std::u8string_view name, Layout total_layout, std::span<MemberView> member_view, std::size_t hash_code, MemoryResourceRecord record)
			: total_layout(total_layout), hash_code(hash_code), member_view(member_view), record(record), construct_property(construct_property)
		{
			
		}

		std::u8string_view name;
		Layout total_layout;
		std::span<MemberView> member_view;
		MemoryResourceRecord record;
		OperateProperty construct_property;
		std::size_t hash_code;

		friend struct StructLayout;
	};

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
			static std::string_view str = typeid(AtomicType).name();
			return std::u8string_view{reinterpret_cast<char8_t const*>(str.data()), str.size()};
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
			if constexpr (std::is_constructible_v<AtomicType, AtomicType const&>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				for(std::size_t i = 0; i < array_count; ++i)
				{
					new (tar + i) AtomicType{};
				}
				return true;
			}else
			{
				return false;
			}
		}

		virtual bool CopyConstruction(void* target, void* source, std::size_t array_count = 1) const override
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
			return false;
		}

		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count = 1) const override
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
			return false;
		}

		virtual bool CopyAssigned(void* target, void* source, std::size_t array_count = 1) const override
		{
			assert(target != source && target != nullptr && source != nullptr);
			if constexpr (std::is_copy_assignable_v<AtomicType>)
			{
				AtomicType* tar = static_cast<AtomicType*>(target);
				AtomicType const* sou = static_cast<AtomicType const*>(source);
				for (std::size_t i = 0; i < array_count; ++i)
				{
					tar[i].operator=(sou[i]);
				}
				return true;
			}
			return false;
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
					tar[i].operator=(std::move(sou[i]));
				}
				return true;
			}
			return false;
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

		virtual std::strong_ordering operator<=>(StructLayout const& layout) const
		{
			auto re = StructLayout::operator<=>(layout);
			if(re == std::strong_ordering::equal && this != &layout)
			{
				return GetName() <=> layout.GetName();
			}
			return re;
		}
	};

	struct SymbolTable
	{

		struct Property
		{
			std::u32string Name;
			std::size_t Index;
			std::size_t Area;
			std::size_t FeedbackIndex;
			std::any Data;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&Data); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const>(&Data); }
		};

	private:
		
		struct Mapping
		{

			enum State
			{
				UnDefine,
				Active,
				UnActive,
				Destoryed,
			};

			State ActiveState = State::UnDefine;
			std::size_t Index;
		};

		struct AreaStack
		{
			std::size_t MappingIndex;
			std::size_t AreaIndex;
		};

		std::vector<Property> ActiveProperty;
		std::vector<Property> UnactiveProperty;
		std::vector<Mapping> Mappings;
		std::vector<Misc::IndexSpan<>> AreaMapping;
		std::vector<AreaStack> MarkedAreaStack;

		std::size_t InsertSymbol(std::u32string Name, std::size_t FeedbackIndex, std::any Data);
		std::size_t MarkAsAreaBegin();
		std::optional<std::size_t> PopArea(bool Searchable = false, bool KeepProperty = true);
		Pointer::ObserverPtr<Property> FindLastActiveSymbol(std::u32string_view Name);
	};

	
	struct MemoryResourceRecordIntrusiveInterface
	{
		void AddRef() const { ref_count.AddRef(); }
		void SubRef() const;
	protected:
		MemoryResourceRecordIntrusiveInterface(MemoryResourceRecord record)
			: record(record) {}
		virtual ~MemoryResourceRecordIntrusiveInterface() = default;
		MemoryResourceRecord record;
		mutable Misc::AtomicRefCount ref_count;
	};

}