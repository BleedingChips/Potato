module;

#include <cassert>

export module PotatoIR;

import std;
import PotatoPointer;
import PotatoMisc;

export namespace Potato::IR
{
	struct Layout
	{
		std::size_t Align = 1;
		std::size_t Size = 0;

		template<typename Type>
		static constexpr Layout Get() { return { alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>) }; }
		template<typename Type>
		static constexpr Layout GetArray(std::size_t array_count) { return { alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>) * array_count }; }
		bool operator==(Layout const& l) const noexcept = default;
		std::strong_ordering operator<=>(Layout const& l2) const noexcept = default;
	};

	inline constexpr std::size_t InsertLayoutCPP(Layout& Target, Layout const Inserted)
	{
		if (Target.Align < Inserted.Align)
			Target.Align = Inserted.Align;
		if (Target.Size % Inserted.Align != 0)
			Target.Size += Inserted.Align - (Target.Size % Inserted.Align);
		std::size_t Offset = Target.Size;
		Target.Size += Inserted.Size;
		return Offset;
	}

	inline constexpr std::size_t InsertLayoutCPP(Layout& Target, Layout const Inserted, std::size_t ArrayCount)
	{
		if (Target.Align < Inserted.Align)
			Target.Align = Inserted.Align;
		if (Target.Size % Inserted.Align != 0)
			Target.Size += Inserted.Align - (Target.Size % Inserted.Align);
		std::size_t Offset = Target.Size;
		Target.Size += Inserted.Size * ArrayCount;
		return Offset;
	}

	inline constexpr bool FixLayoutCPP(Layout& Target)
	{
		auto ModedSize = (Target.Size % Target.Align);
		if (ModedSize != 0)
		{
			Target.Size += Target.Align - ModedSize;
			return true;
		}
		return false;
	}

	inline constexpr Layout SumLayoutCPP(std::span<Layout> Layouts)
	{
		Layout Start;
		for (auto Ite : Layouts)
			InsertLayoutCPP(Start, Ite);
		FixLayoutCPP(Start);
		return Start;
	}

	struct MemoryResourceRecord
	{
		std::pmr::memory_resource* resource = nullptr;
		Layout layout;
		void* adress = nullptr;
		void* Get() const { return adress; }
		std::byte* GetByte() const { return static_cast<std::byte*>(adress); }

		std::pmr::memory_resource* GetMemoryResource() const{ return resource; }

		template<typename Type>
		Type* Cast() const {  assert(sizeof(Type) <= layout.Size && alignof(Type) <= layout.Align); return static_cast<Type*>(adress); }

		template<typename Type>
		std::span<Type> GetArray(std::size_t element_size, std::size_t offset) const
		{
			return std::span<Type>{ reinterpret_cast<Type*>(static_cast<std::byte*>(adress) + offset), element_size };
		}

		operator bool () const;
		static MemoryResourceRecord Allocate(std::pmr::memory_resource* resource, Layout layout);

		template<typename Type>
		static MemoryResourceRecord Allocate(std::pmr::memory_resource* resource) { return  Allocate(resource, Layout::Get<Type>()); }
		bool Deallocate();
	};

	struct TypeID
	{
		template<typename Type>
		static TypeID CreateTypeID() { return typeid(Type); }
		
		std::strong_ordering operator<=>(TypeID const& i) const;
		bool operator==(TypeID const& i) const { return ID == i.ID; }
		TypeID(TypeID const&) = default;
		TypeID& operator=(TypeID const& i) { ID.~type_index(); new (&ID) std::type_index{i.ID}; return *this; }
		std::size_t HashCode() const noexcept { return ID.hash_code(); }
	private:
		TypeID(std::type_info const& ID) : ID(ID) {}
		TypeID(std::type_index ID) : ID(ID) {}
		std::type_index ID;
	};

	struct StructLayout
	{

		struct StructLayoutConstruction
		{
			bool enable_default = true;
			bool enable_copy = true;
			bool enable_move = true;
			StructLayoutConstruction operator&&(StructLayoutConstruction const& i)
			{
				return {enable_default && i.enable_default, enable_copy && i.enable_copy, enable_move && i.enable_move};
			}
			operator bool() const { return enable_default || enable_copy || enable_move; }
		};

		struct Wrapper
		{
			void AddRef(StructLayout const* ptr) const { ptr->AddStructLayoutRef(); }
			void SubRef(StructLayout const* ptr) const { ptr->SubStructLayoutRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<StructLayout, Wrapper>;

		struct Member
		{
			StructLayout::Ptr type_id;
			std::u8string_view name;
			std::size_t array_count = 1;
			void* init_object = nullptr;
		};

		virtual StructLayoutConstruction GetConstructProperty() const = 0;
		virtual bool DefaultConstruction(void* target, std::size_t array_count = 1) const = 0;
		virtual bool CopyConstruction(void* target, void* source, std::size_t array_count = 1) const = 0;
		virtual bool MoveConstruction(void* target, void* source, std::size_t array_count = 1) const = 0;
		virtual bool Destruction(void* target, std::size_t array_count = 1) const = 0;

		struct MemberView
		{
			StructLayout::Ptr type_id;
			std::u8string_view name;
			std::size_t array_count;
			std::size_t offset;
			void* init_object = nullptr;
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
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

	protected:
		
		virtual void AddStructLayoutRef() const = 0;
		virtual void SubStructLayoutRef() const = 0;
	};

	struct StructLayoutObject : protected Pointer::DefaultIntrusiveInterface
	{

		using Ptr = Pointer::IntrusivePtr<StructLayoutObject>;


		static Ptr DefaultConstruct(StructLayout::Ptr layout, std::size_t array_count = 1,  std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr CopyConstruct(StructLayout::Ptr layout, StructLayoutObject::Ptr source, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr layout, StructLayoutObject::Ptr source, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		static Ptr CopyConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		static Ptr MoveConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		StructLayout::Ptr GetStructLayout() const { return layout; };
		std::size_t GetArrayCount() const { return array_count; }
		void* GetData() const { return start; }
		void* GetData(std::size_t index) const { return static_cast<std::byte*>(start) + layout->GetLayout().Size * index; }

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
		StructLayoutConstruction GetConstructProperty() const override { return construct_property; }
		bool DefaultConstruction(void* target, std::size_t array_count) const override;
		bool CopyConstruction(void* target, void* source, std::size_t array_count) const override;
		bool MoveConstruction(void* target, void* source, std::size_t array_count) const override;
		bool Destruction(void* target, std::size_t array_count) const override;
		virtual std::size_t GetHashCode() const override { return hash_code; }

	protected:

		DynamicStructLayout(StructLayoutConstruction construct_property, std::u8string_view name, Layout total_layout, std::span<MemberView> member_view, std::size_t hash_code, MemoryResourceRecord record)
			: total_layout(total_layout), hash_code(hash_code), member_view(member_view), record(record), construct_property(construct_property)
		{
			
		}

		std::u8string_view name;
		Layout total_layout;
		std::span<MemberView> member_view;
		MemoryResourceRecord record;
		StructLayoutConstruction construct_property;
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
		StructLayoutConstruction GetConstructProperty() const override
		{
			return {
				std::is_constructible_v<AtomicType>,
				 std::is_constructible_v<AtomicType, AtomicType const&>,
				 std::is_constructible_v<AtomicType, AtomicType &&>
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

	


}