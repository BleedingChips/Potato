#pragma once
#include <string_view>
#include <optional>
#include "Ebnf.h"
#include "IntrusivePointer.h"
namespace Potato
{

	struct SymbolIndex
	{
		size_t index = std::numeric_limits<size_t>::max();
		std::u32string_view name;
		std::strong_ordering operator <=> (SymbolIndex const& mask) const noexcept { 
			auto re = index <=> mask.index;
			if(re == std::strong_ordering::equal)
				return name <=> mask.name;
			return re;
		}
	};

	using SymbolMask = std::optional<SymbolIndex>;

	struct SymbolAreaIndex
	{
		size_t index = std::numeric_limits<size_t>::max();
	};

	using SymbolAreaMask = std::optional<SymbolAreaIndex>;

	struct SymbolForm
	{
		struct Property
		{
			SymbolIndex mask;
			SymbolAreaIndex area;
			Section section;
			std::any property;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&property); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const>(&property); }
			template<typename FunObject>
			bool operator()(FunObject&& fo) { return AnyViewer(property, std::forward<FunObject>(fo)); }
		};

		SymbolMask InsertSymbol(std::u32string_view name, std::any property, Section section = {});
		SymbolMask InsertSearchSymbolArea(std::u32string_view name, SymbolAreaMask area, Section section = {});
		SymbolMask InsertSearchSymbol(std::u32string_view name, SymbolMask area, Section section = {});

		void MarkSymbolActiveScopeBegin();
		SymbolAreaMask PopSymbolActiveScope();

		SymbolMask FindActiveSymbolAtLast(std::u32string_view name) const noexcept;

		Potato::ObserverPtr<Property const> FindActivePropertyAtLast(std::u32string_view name) const noexcept;
		Potato::ObserverPtr<Property> FindActivePropertyAtLast(std::u32string_view name) noexcept { return reinterpret_cast<SymbolForm const*>(this)->FindActivePropertyAtLast(name).RemoveConstCast(); }

		Potato::ObserverPtr<Property const> FindProperty(SymbolMask InputMask) const noexcept;
		Potato::ObserverPtr<Property> FindProperty(SymbolMask InputMask) noexcept { return reinterpret_cast<SymbolForm const*>(this)->FindProperty(InputMask).RemoveConstCast(); }

		std::span<Property const> FindProperty(SymbolAreaMask Input) const noexcept;
		std::span<Property> FindProperty(SymbolAreaMask Input) noexcept {
			auto Result = reinterpret_cast<SymbolForm const*>(this)->FindProperty(Input);
			return { const_cast<Property*>(Result.data()), Result.size() };
		}

		std::span<Property const> GetAllActiveProperty() const noexcept { return active_scope; }
		std::span<Property> GetAllActiveProperty() noexcept { return active_scope; }

		SymbolForm(SymbolForm&&) = default;
		SymbolForm(SymbolForm const&) = default;
		SymbolForm() = default;

	private:

		struct InsideSearchAreaReference { SymbolAreaIndex area; };
		struct InsideSearchReference { SymbolIndex index; };

		struct IteratorTuple
		{
			std::vector<Property>::const_reverse_iterator start;
			std::vector<Property>::const_reverse_iterator end;
		};

		std::variant<
			Potato::ObserverPtr<SymbolForm::Property const>,
			std::tuple<IteratorTuple, IteratorTuple>
		> SearchElement(IteratorTuple Input, std::u32string_view name) const noexcept;

		struct Mapping
		{
			bool is_active;
			size_t index;
		};

		struct AreaMapping
		{
			IndexSpan<> symbol_area;
		};

		std::vector<size_t> activescope_start_index;
		std::vector<Property> unactive_scope;
		std::vector<Property> active_scope;
		std::vector<Mapping> mapping;
		std::vector<AreaMapping> area_mapping;
	};
}