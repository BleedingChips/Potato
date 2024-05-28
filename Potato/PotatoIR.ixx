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
			std::optional<std::size_t> array_count;
		};

		static Ptr CreateDynamicStructLayout(std::u8string_view name, std::span<Member const> members, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		template<typename AtomicType>
		static Ptr CreateAtomicStructLayout(std::u8string_view name, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual bool CopyConstruction(void* target, void* source) const = 0;
		virtual bool MoveConstruction(void* target, void* source) const = 0;
		virtual bool Destruction(void* target) const = 0;

		struct MemberView
		{
			StructLayout::Ptr type_id;
			std::u8string_view name;
			std::optional<std::size_t> array_count;
			std::size_t offset;
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;
		virtual std::u8string_view GetName() const = 0;
		std::optional<MemberView> FindMemberView(std::u8string_view member_name) const;
		virtual Layout GetLayout() const = 0;
		Layout GetArrayLayout(std::size_t array_count) const;
		Layout GetLayout(std::optional<std::size_t> array_count) const;
		static void* GetData(MemberView const&, void* target);
		template<typename Type>
		static Type* GetDataAs(MemberView const& view, void* target)
		{
			return static_cast<Type*>(GetData(view, target));
		}
		static std::span<std::byte> GetDataSpan(MemberView const&, void* target);
		//static bool MakeMemberView(std::span<Member const> in, std::span<MemberView> output);

	protected:
		
		virtual void AddStructLayoutRef() const = 0;
		virtual void SubStructLayoutRef() const = 0;
	};

	struct DynamicStructLayout : public StructLayout, public Pointer::DefaultIntrusiveInterface
	{
		virtual std::u8string_view GetName() const override { return name; }
		virtual void AddStructLayoutRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutRef() const override { DefaultIntrusiveInterface::SubRef(); }
		virtual void Release() override;
		Layout GetLayout() const override;
		std::span<MemberView const> GetMemberView() const override;
		bool CopyConstruction(void* target, void* source) const override;
		bool MoveConstruction(void* target, void* source) const override;
		bool Destruction(void* target) const override;

	protected:

		DynamicStructLayout(std::u8string_view name, Layout total_layout, std::span<MemberView> member_view, MemoryResourceRecord record)
			: total_layout(total_layout), member_view(member_view), record(record)
		{
			
		}

		std::u8string_view name;
		Layout total_layout;
		std::span<MemberView> member_view;
		MemoryResourceRecord record;

		friend struct StructLayout;
	};

	template<typename AtomicType>
	struct AtomicStructLayout : public StructLayout, public Pointer::DefaultIntrusiveInterface
	{
		virtual Layout GetLayout() const override { return Layout::Get<AtomicType>(); }
		virtual void AddStructLayoutRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutRef() const override { DefaultIntrusiveInterface::SubRef(); }
		virtual void Release() override { auto re = record; this->~AtomicStructLayout(); re.Deallocate(); }
		virtual std::u8string_view GetName() const override { return name; }
		std::span<MemberView const> GetMemberView() const override { return {}; }

		virtual bool CopyConstruction(void* target, void* source) const override
		{
			if constexpr (std::is_constructible_v<AtomicType, AtomicType const&>)
			{
				new (target) AtomicType{ *static_cast<AtomicType const*>(source) };
				return true;
			}else
			{
				return false;
			}
		}

		virtual bool MoveConstruction(void* target, void* source) const override
		{
			if constexpr (std::is_constructible_v<AtomicType, AtomicType &&>)
			{
				new (target) AtomicType{ std::move(*static_cast<AtomicType*>(source)) };
				return true;
			}else
			{
				return CopyConstruction(target, source);
			}
		}

		virtual bool Destruction(void* target) const override
		{
			static_cast<AtomicType*>(target)->~AtomicType();
			return true;
		}

	protected:

		AtomicStructLayout(std::u8string_view name, MemoryResourceRecord record)
			: name(name), record(record) {}
		

		std::u8string_view name;
		MemoryResourceRecord record;

		friend struct StructLayout;
	};

	template<typename AtomicType>
	auto StructLayout::CreateAtomicStructLayout(std::u8string_view name, std::pmr::memory_resource* resource)
		-> Ptr
	{
		std::size_t name_size = name.size();

		auto cur_layout = Layout::Get<AtomicStructLayout<AtomicType>>();
		std::size_t name_offset = InsertLayoutCPP(cur_layout, Layout::GetArray<char8_t>(name_size));
		FixLayoutCPP(cur_layout);
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if(re)
		{
			auto str_span = std::span(reinterpret_cast<char8_t*>(re.GetByte() + name_offset), name_size);
			std::memcpy(str_span.data(), name.data(), name.size());
			name = std::u8string_view{str_span.subspan(0, name.size())};
			return new (re.Get()) AtomicStructLayout<AtomicType>{
				name,
				re
			};
		}
		return {};
	}

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