#include "../Public/HIR.h"

#include <filesystem>
#include <set>

namespace Potato::HIR
{

	bool TypeReference::IsPointer() const noexcept
	{
		for (auto& Ite : modifier)
		{
			if(Ite.type == Modifier::Type::POINTER)
				return true;
		}
		return false;
	}

	TypeTag TypeForm::ForwardDefineType()
	{
		TypeTag tag {StorageType::CUSTOM, defined_types.size()};
		defined_types.push_back({});
		return tag;
	}

	bool TypeForm::MarkTypeDefineStart(TypeTag Input)
	{
		if (Input.IsCustomType() && defined_types.size() > Input.AsIndex() && !defined_types[Input.AsIndex()].has_value())
		{
			define_stack_record.push_back({Input, temporary_member_type.size()});
			return true;
		}
		return false;
	}

	bool TypeForm::InsertMember(TypeReference type_reference, std::u32string name)
	{
		assert(!define_stack_record.empty());
		if (type_reference.type)
		{
			if (type_reference.type.AsIndex() < defined_types.size())
			{
				auto& ref = defined_types[type_reference.type.AsIndex()];
				if (ref.has_value() || type_reference.IsPointer())
				{
					temporary_member_type.push_back({ std::move(type_reference), std::move(name) });
					return true;
				}
			}
		}
		return false;
	}

	std::optional<Layout> TypeForm::CalculateTypeLayout(TypeTag const& ref) const
	{
		assert(ref);
		if (ref.storage_type.has_value())
		{
			switch (*ref.storage_type)
			{
			case StorageType::UINT8:
			case StorageType::INT8:
				return Layout{ alignof(uint8_t), sizeof(alignof(uint8_t)) };
				break;
			case StorageType::UINT16:
			case StorageType::INT16:
				return Layout{ alignof(uint16_t), sizeof(alignof(uint16_t)) };
				break;
			case StorageType::UINT32:
			case StorageType::INT32:
			case StorageType::FLOAT32:
				return Layout{ alignof(uint32_t), sizeof(alignof(uint32_t)) };
				break;
			case StorageType::INT64:
			case StorageType::UINT64:
			case StorageType::FLOAT64:
				return Layout{ alignof(uint64_t), sizeof(alignof(uint64_t)) };
				break;
			case StorageType::CUSTOM:
			{
				assert(defined_types.size() > ref.AsIndex());
				auto& ref2 = defined_types[ref.AsIndex()];
				if(ref2.has_value())
					return ref2->layout;
				return {};
			}break;
			default:
				return {};
			}
		}
		return {};
	}

	std::optional<Layout> TypeForm::CalculateTypeLayout(TypeReference const& ref, Setting const& setting) const
	{
		std::optional<Layout> OldLayout = CalculateTypeLayout(ref.type);
		for (auto Ite : ref.modifier)
		{
			switch (Ite.type)
			{
			case Modifier::Type::POINTER:
				OldLayout = setting.pointer_layout;
				break;
			case Modifier::Type::ARRAY:
				if(OldLayout.has_value())
					OldLayout->size *= Ite.parameter;
				break;
			default:
				break;
			}
		}
		return OldLayout;
	}

	std::optional<TypeTag> TypeForm::FinishTypeDefine(Setting const& setting)
	{
		if (!define_stack_record.empty())
		{
			auto [tag, size] = *define_stack_record.rbegin();
			define_stack_record.pop_back();
			assert(tag && defined_types.size() > tag.AsIndex() && !defined_types[tag.AsIndex()].has_value());
			TypeProperty property;
			auto& layout_ref = property.layout;
			layout_ref = {setting.min_alignas, 0};
			bool Error = false;
			for (auto ite = temporary_member_type.begin() + size; ite < temporary_member_type.end(); ++ite)
			{
				auto layout = CalculateTypeLayout(ite->type_reference, setting);
				if (!layout.has_value())
				{
					Error = true;
					break;
				}
				layout_ref.align = std::max(layout_ref.align, layout->align);
				auto alian_space = layout_ref.size % layout->align;
				if(alian_space != 0)
					layout_ref.size += layout->align - alian_space;
				if (setting.member_feild.has_value())
				{
					alian_space = layout_ref.size % *setting.member_feild;
					if(alian_space != 0 && alian_space + layout->size > *setting.member_feild)
						layout_ref.size += *setting.member_feild - alian_space;
				}
				ite->offset = layout_ref.size;
				ite->layout = *layout;
			}
			auto alian_size = layout_ref.size % layout_ref.align;
			if(alian_size != 0)
				layout_ref.size += layout_ref.align - alian_size;
			auto start_ite = temporary_member_type.begin() + size;
			if(!Error)
				property.members.insert(property.members.end(), std::move_iterator(start_ite), std::move_iterator(temporary_member_type.end()));
			temporary_member_type.erase(start_ite, temporary_member_type.end());
			if (!Error)
			{
				defined_types[tag.AsIndex()] = std::move(property);
				return tag;
			}
			return {};
		}
		return {};
	}

	Potato::ObserverPtr<std::optional<TypeProperty>> TypeForm::FindType(TypeTag tag) noexcept
	{
		if (tag && tag.IsCustomType() && tag.AsIndex() < defined_types.size())
		{
			return &defined_types[tag.AsIndex()];
		}
		return {};
	}

	/*
	std::span<std::byte> StackValueForm::TryAllocateMember(Layout layout)
	{
		while (true)
		{
			if()
		}
	}
	*/

	Register HIRForm::InserConstData(TypeReference desc, Layout layout, std::span<std::byte const> data)
	{
		Register reg{Register::Type::CONST, elements.size()};
		IndexSpan<> span(datas.size(), data.size());
		datas.resize(datas.size() + data.size());
		std::memcpy(datas.data() + span.start, data.data(), span.length);
		elements.push_back({std::move(desc), span, layout});
		return reg;
	}

	SymbolTag SymbolForm::InsertSymbol(std::u32string_view name, std::any property, Section section)
	{
		SymbolTag mask{ mapping.size(), name };
		mapping.push_back({ Mapping::Category::ACTIVE, active_scope.size() });
		active_scope.push_back({ mask, {}, section, std::move(property) });
		return mask;
	}

	SymbolTag SymbolForm::InsertSearchArea(std::u32string_view name, SymbolAreaTag area, Section section)
	{
		if(area)
			return InsertSymbol(name, InsideSearchReference{area}, section);
		else
			return {};
	}

	void SymbolForm::MarkSymbolActiveScopeBegin()
	{
		activescope_start_index.push_back(active_scope.size());
	}

	SymbolAreaTag SymbolForm::PopSymbolActiveScope()
	{
		if (!activescope_start_index.empty())
		{
			auto count = *activescope_start_index.rbegin();
			assert(count <= active_scope.size());
			activescope_start_index.pop_back();
			SymbolAreaTag current_area{ unactive_scope.size(), count };
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

	auto SymbolForm::SearchElement(IteratorTuple Input, std::u32string_view name) const noexcept -> std::variant<
		Potato::ObserverPtr<SymbolForm::Property const>,
		std::tuple<IteratorTuple, IteratorTuple>
	>
	{
		auto& Ite = Input.start;
		for (; Ite != Input.end; ++Ite)
		{
			if(Ite->mask.Name() == name)
				return Potato::ObserverPtr<SymbolForm::Property const>(&(*Ite));
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
		return Potato::ObserverPtr<SymbolForm::Property const>{};
	}

	Potato::ObserverPtr<SymbolForm::Property const> SymbolForm::FindActivePropertyAtLast(std::u32string_view name) const noexcept
	{
		auto result = SearchElement({active_scope.rbegin(), active_scope.rend()}, name);
		if(std::holds_alternative<Potato::ObserverPtr<SymbolForm::Property const>>(result))
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

	SymbolTag SymbolForm::FindActiveSymbolAtLast(std::u32string_view name) const noexcept
	{
		auto result = FindActivePropertyAtLast(name);
		if(result)
			return result->mask;
		return {};
	}

	auto SymbolForm::FindProperty(SymbolTag InputMask) const noexcept -> Potato::ObserverPtr<Property const>
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

	auto SymbolForm::FindProperty(SymbolAreaTag Input) const noexcept -> std::span<Property const>
	{
		if (Input && Input.Index() + Input.Count() <= unactive_scope.size() && unactive_scope[Input.Index()].area == Input)
		{
			return {&unactive_scope[Input.Index()], Input.Count()};
		}
		return {};
	}
}