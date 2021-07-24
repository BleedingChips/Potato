#include "../Public/MiniCore.h"

#include <filesystem>
#include <set>

namespace Potato
{

	bool TypeProperty::IsPointer() const noexcept
	{
		for (auto& Ite : modifier)
		{
			if(Ite.type == TypeModifier::Type::POINTER)
				return true;
		}
		return false;
	}

	namespace Implement
	{
		RegisterMask ConstDataStorageTable::InserConstData(TypeProperty desc, TypeLayout layout, std::span<std::byte const> data)
		{
			if (!data.empty())
			{
				RegisterIndex reg{ RegisterIndex::Category::CONST, MemoryDescription::CUSTOM, elements.size() };
				IndexSpan<> span(datas.size(), data.size());
				datas.resize(datas.size() + data.size());
				std::memcpy(datas.data() + span.start, data.data(), span.length);
				elements.push_back({ std::move(desc), span, layout });
				return reg;
			}
			return {};
		}
	}

	TypeMask MiniCore::ForwardDefineType()
	{
		TypeMask tag{ std::in_place, MemoryDescription::CUSTOM, defined_types.size() };
		defined_types.push_back({});
		return tag;
	}

	

	/*

	TypeMask HIRForm::ForwardDefineType()
	{
		TypeMask tag{ std::in_place, MemoryDescription::CUSTOM, defined_types.size()};
		defined_types.push_back({});
		return tag;
	}

	bool HIRForm::MarkTypeDefineStart(TypeMask Input)
	{
		if (Input && Input->IsCustomType() && defined_types.size() > Input->index && !defined_types[Input->index].has_value())
		{
			define_stack_record.push_back({Input, temporary_member_type.size()});
			return true;
		}
		return false;
	}

	RegisterMask HIRForm::InserConstData(TypeProperty desc, TypeLayout layout, std::span<std::byte const> data)
	{
		RegisterIndex reg{RegisterIndex::Category::CONST, MemoryDescription::CUSTOM, elements.size() };
		IndexSpan<> span(datas.size(), data.size());
		datas.resize(datas.size() + data.size());
		std::memcpy(datas.data() + span.start, data.data(), span.length);
		elements.push_back({ std::move(desc), span, layout });
		return reg;
	}
	*/

	/*

	bool TypeForm::InsertMember(TypeReference type_reference, std::u32string_view name)
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
		if (ref.memory_mode.has_value())
		{
			switch (*ref.memory_mode)
			{
			case MemberMode::UINT8:
			case MemberMode::INT8:
				return Layout{ alignof(uint8_t), sizeof(alignof(uint8_t)) };
				break;
			case MemberMode::UINT16:
			case MemberMode::INT16:
				return Layout{ alignof(uint16_t), sizeof(alignof(uint16_t)) };
				break;
			case MemberMode::UINT32:
			case MemberMode::INT32:
			case MemberMode::FLOAT32:
				return Layout{ alignof(uint32_t), sizeof(alignof(uint32_t)) };
				break;
			case MemberMode::INT64:
			case MemberMode::UINT64:
			case MemberMode::FLOAT64:
				return Layout{ alignof(uint64_t), sizeof(alignof(uint64_t)) };
				break;
			case MemberMode::CUSTOM:
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
	*/

	/*
	std::span<std::byte> StackValueForm::TryAllocateMember(Layout layout)
	{
		while (true)
		{
			if()
		}
	}
	*/



	
}