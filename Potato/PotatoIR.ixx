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

		static Ptr CreateDynamicStructLayout(std::span<Member const> members, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual bool CopyConstruction(void* target, void* source) = 0;
		virtual bool MoveConstruction(void* target, void* source) = 0;
		virtual bool Destruction(void* target) = 0;

		struct MemberView
		{
			StructLayout::Ptr type_id;
			std::u8string_view name;
			std::optional<std::size_t> array_count;
			std::size_t offset;
		};

		virtual std::span<MemberView const> GetMemberView() const = 0;

		virtual Layout GetLayout() const = 0;

		Layout GetArrayLayout(std::size_t array_count) const
		{
			auto layout = GetLayout();
			layout.Size *= array_count;
			return layout;
		}

		Layout GetLayout(std::optional<std::size_t> array_count) const 
		{
			if(array_count)
			{
				return GetArrayLayout(*array_count);
			}
			return GetLayout();
		}
		

		static void* GetData(MemberView const&, void* target);
		static std::span<std::byte> GetDataSpan(MemberView const&, void* target);
		//static bool MakeMemberView(std::span<Member const> in, std::span<MemberView> output);

	protected:
		
		virtual void AddStructLayoutRef() const = 0;
		virtual void SubStructLayoutRef() const = 0;
	};

	struct Struct
	{
		
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

	struct TypeDescription
	{
		enum class TypeT : std::uint32_t
		{
			Unsighed,
			Sighed,
			Float
		};

		TypeT Type : 2;
		std::uint32_t Value : 30;
	};

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


}