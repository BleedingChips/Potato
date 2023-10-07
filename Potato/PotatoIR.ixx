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
		std::strong_ordering operator<=>(TypeID const& i) const { return ID <=> i.ID; }
	private:
		TypeID(std::type_info const& ID) : ID(ID) {}
		TypeID(std::type_index ID) : ID(ID) {}
		std::type_index ID;
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


}