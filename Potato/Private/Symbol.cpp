#include "../Public/Symbol.h"
namespace Potato
{
	SymbolMask SymbolForm::InsertSymbol(std::u32string_view name, std::any property, Section section)
	{
		SymbolMask mask{ std::in_place, mapping.size(), name };
		mapping.push_back({ true, active_scope.size() });
		active_scope.push_back({ *mask, {}, section, std::move(property) });
		return mask;
	}

	SymbolMask SymbolForm::InsertSearchSymbolArea(std::u32string_view name, SymbolAreaMask area, Section section)
	{
		if (area)
			return InsertSymbol(name, InsideSearchAreaReference{ *area }, section);
		else
			return {};
	}

	SymbolMask SymbolForm::InsertSearchSymbol(std::u32string_view name, SymbolMask area, Section section)
	{
		if (area)
			return InsertSymbol(name, InsideSearchReference{ *area }, section);
		else
			return {};
	}

	void SymbolForm::MarkSymbolActiveScopeBegin()
	{
		activescope_start_index.push_back(active_scope.size());
	}

	SymbolAreaMask SymbolForm::PopSymbolActiveScope()
	{
		if (!activescope_start_index.empty())
		{
			auto count = *activescope_start_index.rbegin();
			assert(count <= active_scope.size());
			activescope_start_index.pop_back();
			AreaMapping current_area{ unactive_scope.size(), count };
			SymbolAreaIndex area_index(area_mapping.size());
			auto scope_start_ite = active_scope.begin() + (active_scope.size() - count);
			for (auto Ite = scope_start_ite; Ite != active_scope.end(); ++Ite)
				Ite->area = area_index;
			size_t offset = 0;
			for (auto Ite = mapping.begin() + (mapping.size() - count); Ite != mapping.end(); ++Ite, ++offset)
			{
				Ite->is_active = false;
				Ite->index = current_area.symbol_area.start + offset;
			}
			unactive_scope.insert(unactive_scope.end(), std::move_iterator(scope_start_ite), std::move_iterator(active_scope.end()));
			active_scope.erase(scope_start_ite, active_scope.end());
			area_mapping.push_back(current_area);
			return area_index;
		}
		else {
			return {};
		}
	}

	auto SymbolForm::SearchElement(IteratorTuple Input, std::u32string_view name) const noexcept -> std::variant<
		Potato::ObserverPtr<SymbolForm::Property const>,
		std::tuple<IteratorTuple, IteratorTuple>
	>
	{
		auto& Ite = Input.start;
		for (; Ite != Input.end; ++Ite)
		{
			if (Ite->mask.name == name)
				return Potato::ObserverPtr<SymbolForm::Property const>(&(*Ite));
			auto Inside = Ite->TryCast<InsideSearchAreaReference>();
			if (Inside != nullptr)
			{
				assert(area_mapping.size() > Inside->area.index);
				auto area_map = area_mapping[Inside->area.index];
				assert(unactive_scope.size() >= area_map.symbol_area.start + area_map.symbol_area.length);
				IteratorTuple SearchPair;
				SearchPair.start = unactive_scope.rbegin() + (unactive_scope.size() - area_map.symbol_area.start - area_map.symbol_area.length);
				SearchPair.end = unactive_scope.rbegin() + (unactive_scope.size() - area_map.symbol_area.start);
				++Ite;
				return std::tuple<IteratorTuple, IteratorTuple>{SearchPair, Input };
			}
			else {
				auto Inside2 = Ite->TryCast<InsideSearchReference>();
				if (Inside2 != nullptr)
				{
					assert(mapping.size() > Inside2->index.index);
					auto& map = mapping[Inside2->index.index];
					if (map.is_active)
					{
						assert(active_scope.size() > map.index);
						auto& ele = active_scope[map.index];
						if(ele.mask.name == name)
							return Potato::ObserverPtr<SymbolForm::Property const>(&ele);
					}
					else {
						assert(unactive_scope.size() > map.index);
						auto& ele = unactive_scope[map.index];
						if (ele.mask.name == name)
							return Potato::ObserverPtr<SymbolForm::Property const>(&ele);
					}
				}
			}
		}
		return Potato::ObserverPtr<SymbolForm::Property const>{};
	}

	Potato::ObserverPtr<SymbolForm::Property const> SymbolForm::FindActivePropertyAtLast(std::u32string_view name) const noexcept
	{
		auto result = SearchElement({ active_scope.rbegin(), active_scope.rend() }, name);
		if (std::holds_alternative<Potato::ObserverPtr<SymbolForm::Property const>>(result))
			return std::get<Potato::ObserverPtr<SymbolForm::Property const>>(result);
		else
		{
			auto [It1, It2] = std::get<std::tuple<IteratorTuple, IteratorTuple>>(result);
			std::vector<IteratorTuple> search_stack;
			if (It2.start != It2.end)
				search_stack.push_back(It2);
			if (It1.start != It1.end)
				search_stack.push_back(It1);
			while (!search_stack.empty())
			{
				auto Pop = *search_stack.rbegin();
				search_stack.pop_back();
				auto result = SearchElement(Pop, name);
				if (std::holds_alternative<Potato::ObserverPtr<SymbolForm::Property const>>(result))
					return std::get<Potato::ObserverPtr<SymbolForm::Property const>>(result);
				else {
					auto [It1, It2] = std::get<std::tuple<IteratorTuple, IteratorTuple>>(result);
					if (It2.start != It2.end)
						search_stack.push_back(It2);
					if (It1.start != It1.end)
						search_stack.push_back(It1);
				}
			}
		}
		return {};
	}

	SymbolMask SymbolForm::FindActiveSymbolAtLast(std::u32string_view name) const noexcept
	{
		auto result = FindActivePropertyAtLast(name);
		if (result)
			return result->mask;
		return {};
	}

	auto SymbolForm::FindProperty(SymbolMask InputMask) const noexcept -> Potato::ObserverPtr<Property const>
	{
		if (InputMask && InputMask->index < mapping.size())
		{
			auto map = mapping[InputMask->index];
			decltype(&active_scope) scopr_ptr = nullptr;
			if(map.is_active)
				scopr_ptr = &active_scope;
			else
				scopr_ptr = &unactive_scope;
			auto& Pro = (*scopr_ptr)[map.index];
			if (Pro.mask.name == InputMask->name)
				return { &Pro };
		}
		return {};
	}

	auto SymbolForm::FindProperty(SymbolAreaMask Input) const noexcept -> std::span<Property const>
	{
		if (Input)
		{
			assert(Input->index < area_mapping.size());
			auto map = area_mapping[Input->index];
			assert(map.symbol_area.start + map.symbol_area.length <= unactive_scope.size());
			return { &unactive_scope[map.symbol_area.start], map.symbol_area.length};
		}
		return {};
	}
}
