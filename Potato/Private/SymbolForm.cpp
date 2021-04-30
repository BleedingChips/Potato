#include "../Public/SymbolForm.h"

#include <filesystem>
#include <set>

namespace Potato::Symbol
{
	Mask Form::InsertSymbol(std::u32string_view name, std::any property, Section section)
	{
		Mask mask{ mapping.size(), name };
		mapping.push_back({ true, active_scope.size() });
		active_scope.push_back({ mask, {}, section, std::move(property) });
		return mask;
	}

	Mask Form::InsertSearchArea(std::u32string_view name, Area area, Section section)
	{
		if(area)
			return InsertSymbol(name, InsideSearchReference{area}, section);
		else
			return {};
	}

	Area Form::PopSymbolAsUnactive(size_t count)
	{
		count = std::min(count, active_scope.size());
		Area current_area{ unactive_scope.size(), count };
		auto scope_start_ite = active_scope.begin() + (active_scope.size() - count);
		for (auto Ite = scope_start_ite; Ite != active_scope.end(); ++Ite)
			Ite->area = current_area;
		{
			size_t offset = 0;
			for (auto Ite = mapping.begin() + (mapping.size() - count); Ite != mapping.end(); ++Ite, ++offset)
			{
				Ite->is_active = false;
				Ite->index = current_area.Index() + offset;
			}
		}
		unactive_scope.insert(unactive_scope.end(), std::move_iterator(scope_start_ite), std::move_iterator(active_scope.end()));
		active_scope.erase(scope_start_ite, active_scope.end());
		return current_area;
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

	Mask Form::FindActiveSymbolAtLast(std::u32string_view name) const noexcept
	{
		auto result = FindActivePropertyAtLast(name);
		if(result)
			return result->mask;
	}

	auto Form::FindProperty(Mask InputMask) const noexcept -> Potato::ObserverPtr<Property const>
	{
		if (InputMask && InputMask.Index() < mapping.size())
		{
			auto map = mapping[InputMask.Index()];
			auto scope = map.is_active ? &active_scope : &unactive_scope;
			auto& Pro = (*scope)[map.index];
			if(Pro.mask.Name() == InputMask.Name())
				return {&Pro};
		}
		return {};
	}

	auto Form::FindProperty(Area Input) const noexcept -> std::span<Property const>
	{
		if (Input && Input.Index() + Input.Count() <= unactive_scope.size() && unactive_scope[Input.Index()].area == Input)
		{
			return {&unactive_scope[Input.Index()], Input.Count()};
		}
		return {};
	}

	/*

	MemoryTag MemoryTagStyle::Finalize(MemoryTag owner)
	{
		auto mod = owner.size % owner.align;
		if (mod == 0)
			return owner;
		else
			return {owner.align, (owner.size + owner.align - mod)};
	}

	auto CLikeMemoryTagStyle::InsertMember(MemoryTag owner, MemoryTag member) -> Result
	{
		auto align = std::max(owner.align, member.align);
		auto mod = owner.size % member.align;
		if (mod == 0)
		{
			return { owner.size, {align, owner.size + member.size}};
		}
		else {
			auto offset = owner.size + (member.align - mod);
			return {offset, {align, offset + member.size}};
		}
	}

	auto HlslMemoryTagStyle::InsertMember(MemoryTag owner, MemoryTag member) -> Result
	{
		assert(member.align == MinAlign());
		constexpr size_t AlignSize = MinAlign() * 4;
		MemoryTag temp{ owner };
		size_t rever_size = AlignSize - temp.size % AlignSize;
		if (member.size >= AlignSize || rever_size < member.size)
			temp.size += rever_size;
		return {temp.size, {temp.align, temp.size + member.size}};
	}
	*/

	/*
	size_t MemoryModelMaker::operator()(MemoryModel const& info_i)
	{
		history = info;
		finalize = std::nullopt;
		auto re = Handle(info, info_i);
		info.align = re.align;
		info.size += re.size_reserved;
		size_t offset = info.size;
		info.size += info_i.size;
		return offset;
	}

	size_t MemoryModelMaker::MaxAlign(MemoryModel out, MemoryModel in) noexcept
	{
		return out.align < in.align ? in.align : out.align;
	}

	size_t MemoryModelMaker::ReservedSize(MemoryModel out, MemoryModel in) noexcept
	{
		auto mod = out.size % in.align;
		if (mod == 0)
			return 0;
		else
			return out.align - mod;
	}

	MemoryModelMaker::HandleResult MemoryModelMaker::Handle(MemoryModel cur, MemoryModel input) const
	{
		return { MaxAlign(cur, input), ReservedSize(cur, input) };
	}

	MemoryModel MemoryModelMaker::Finalize(MemoryModel cur) const
	{
		auto result = cur;
		result.size += ReservedSize(result, result);
		return result;
	}
	*/
}