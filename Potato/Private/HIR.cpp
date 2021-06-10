#include "../Public/HIR.h"

#include <filesystem>
#include <set>

namespace Potato::HIR
{

	size_t ConstForm::InserConstData(TypeReference desc, Layout layout, std::span<std::byte const> data)
	{
		IndexSpan<> span(const_form.size(), data.size());
		const_form.resize(const_form.size() + data.size());
		std::memcpy(const_form.data() + span.start, data.data(), span.length);
		size_t current = elements.size();
		elements.push_back(ConstForm::Element{std::move(desc), span, layout});
		return current;
	}


	Symbol Form::InsertSymbol(std::u32string_view name, std::any property, Section section)
	{
		Symbol mask{ mapping.size(), name };
		mapping.push_back({ Mapping::Category::ACTIVE, active_scope.size() });
		active_scope.push_back({ mask, {}, section, std::move(property) });
		return mask;
	}

	Symbol Form::InsertSearchArea(std::u32string_view name, SymbolArea area, Section section)
	{
		if(area)
			return InsertSymbol(name, InsideSearchReference{area}, section);
		else
			return {};
	}

	void Form::MarkSymbolActiveScopeBegin()
	{
		activescope_start_index.push_back(active_scope.size());
	}

	SymbolArea Form::PopSymbolActiveScope()
	{
		if (!activescope_start_index.empty())
		{
			auto count = *activescope_start_index.rbegin();
			assert(count <= active_scope.size());
			activescope_start_index.pop_back();
			SymbolArea current_area{ unactive_scope.size(), count };
			auto scope_start_ite = active_scope.begin() + (active_scope.size() - count);
			for (auto Ite = scope_start_ite; Ite != active_scope.end(); ++Ite)
				Ite->area = current_area;
			size_t offset = 0;
			for (auto Ite = mapping.begin() + (mapping.size() - count); Ite != mapping.end(); ++Ite, ++offset)
			{
				Ite->category = Mapping::Category::UNACTIVE;
				Ite->index = current_area.Index() + offset;
			}
			unactive_scope.insert(unactive_scope.end(), std::move_iterator(scope_start_ite), std::move_iterator(active_scope.end()));
			active_scope.erase(scope_start_ite, active_scope.end());
			return current_area;
		}
		else {
			return {};
		}
	}

	auto Form::SearchElement(IteratorTuple Input, std::u32string_view name) const noexcept -> std::variant<
		Potato::ObserverPtr<Form::Property const>,
		std::tuple<IteratorTuple, IteratorTuple>
	>
	{
		auto& Ite = Input.start;
		for (; Ite != Input.end; ++Ite)
		{
			if(Ite->mask.Name() == name)
				return Potato::ObserverPtr<Form::Property const>(&(*Ite));
			auto Inside = Ite->TryCast<InsideSearchReference>();
			if (Inside != nullptr)
			{
				assert(Inside->area);
				assert(unactive_scope.size() > Inside->area.Index() + Inside->area.Count());
				IteratorTuple SearchPair;
				SearchPair.start = unactive_scope.rbegin() + (unactive_scope.size() - Inside->area.Index() - Inside->area.Count());
				SearchPair.end = unactive_scope.rbegin() + (unactive_scope.size() - Inside->area.Index());
				++Ite;
				return std::tuple<IteratorTuple, IteratorTuple>{SearchPair, Input };
			}
		}
		return Potato::ObserverPtr<Form::Property const>{};
	}

	Potato::ObserverPtr<Form::Property const> Form::FindActivePropertyAtLast(std::u32string_view name) const noexcept
	{
		auto result = SearchElement({active_scope.rbegin(), active_scope.rend()}, name);
		if(std::holds_alternative<Potato::ObserverPtr<Form::Property const>>(result))
			return std::get<Potato::ObserverPtr<Form::Property const>>(result);
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
				if (std::holds_alternative<Potato::ObserverPtr<Form::Property const>>(result))
					return std::get<Potato::ObserverPtr<Form::Property const>>(result);
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

	Symbol Form::FindActiveSymbolAtLast(std::u32string_view name) const noexcept
	{
		auto result = FindActivePropertyAtLast(name);
		if(result)
			return result->mask;
		return {};
	}

	auto Form::FindProperty(Symbol InputMask) const noexcept -> Potato::ObserverPtr<Property const>
	{
		if (InputMask && InputMask.Index() < mapping.size())
		{
			auto map = mapping[InputMask.Index()];
			decltype(&active_scope) scopr_ptr = nullptr;
			switch (map.category)
			{
			case Mapping::Category::ACTIVE:
				scopr_ptr = &active_scope;
				break;
			case Mapping::Category::UNACTIVE:
				scopr_ptr = &unactive_scope;
				break;
			}
			assert(scopr_ptr != nullptr);
			auto& Pro = (*scopr_ptr)[map.index];
			if(Pro.mask.Name() == InputMask.Name())
				return {&Pro};
		}
		return {};
	}

	auto Form::FindProperty(SymbolArea Input) const noexcept -> std::span<Property const>
	{
		if (Input && Input.Index() + Input.Count() <= unactive_scope.size() && unactive_scope[Input.Index()].area == Input)
		{
			return {&unactive_scope[Input.Index()], Input.Count()};
		}
		return {};
	}
}