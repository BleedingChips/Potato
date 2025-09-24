module;

#include <cassert>

module PotatoIR;
//import :Define;

namespace Potato::IR
{

	MemoryResourceRecord MemoryResourceRecord::Allocate(std::pmr::memory_resource* resource, Layout layout)
	{
		if(resource != nullptr)
		{
			auto adress = resource->allocate(layout.size, layout.align);
			return { resource, layout, adress};
		}
		return {resource, layout, nullptr};
	}

	MemoryResourceRecord::operator bool() const
	{
		return resource != nullptr && adress != nullptr;
	}

	bool MemoryResourceRecord::Deallocate()
	{
		if(*this)
		{
			resource->deallocate(adress, layout.size, layout.align);
			resource = nullptr;
			adress = nullptr;
			return true;
		}
		return false;
	}

	void MemoryResourceRecordIntrusiveInterface::SubRef() const
	{
		if (ref_count.SubRef())
		{
			auto re = record;
			this->~MemoryResourceRecordIntrusiveInterface();
			re.Deallocate();
		}
	}

	bool StructLayout::operator== (StructLayout const& other) const
	{
		if (this == &other)
			return true;
		if (GetHashCode() != other.GetHashCode())
			return false;
		if (GetLayout() != other.GetLayout())
			return false;
		auto mem = GetMemberView();
		auto mem2 = other.GetMemberView();
		if (mem.size() != mem2.size())
			return false;
		for (std::size_t i = 0; i < mem.size(); ++i)
		{
			if (mem[i].name != mem2[i].name)
				return false;
			if (mem[i].offset != mem2[i].offset)
				return false;
			/*
			if (*mem[i].struct_layout != *mem2[i].struct_layout)
				return false;
				*/
		}
		return true;
	}

	auto StructLayout::FindMemberView(std::u8string_view member_name) const -> std::optional<MemberView>
	{
		auto span = GetMemberView();
		auto ite = std::find_if(span.begin(), span.end(), [=](MemberView const& ref) -> bool { return ref.name == member_name; });
		if(ite == span.end())
		{
			return std::nullopt;
		}else
		{
			return *ite;
		}
	}

	std::optional<StructLayout::MemberView> StructLayout::FindMemberView(std::size_t index) const
	{
		auto span = GetMemberView();
		if (index < span.size())
		{
			return span[index];
		}
		return std::nullopt;
	}

	

	struct DynamicStructLayout : public StructLayout, public MemoryResourceRecordIntrusiveInterface
	{
		virtual std::u8string_view GetName() const override { return name; }
		virtual void AddStructLayoutRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }
		Layout GetLayout() const override;
		std::span<MemberView const> GetMemberView() const override;
		OperateProperty GetOperateProperty() const override { return construct_property; }
		virtual std::size_t GetHashCode() const override { return hash_code; }

	protected:

		DynamicStructLayout(OperateProperty construct_property, std::u8string_view name, Layout total_layout, std::span<MemberView> member_view, std::size_t hash_code, MemoryResourceRecord record)
			: MemoryResourceRecordIntrusiveInterface(record), name(name), total_layout(total_layout), hash_code(hash_code), member_view(member_view), construct_property(construct_property)
		{

		}

		~DynamicStructLayout();

		std::u8string_view name;
		Layout total_layout;
		std::span<MemberView> member_view;
		OperateProperty construct_property;
		std::size_t hash_code;

		friend struct StructLayout;
		friend struct StructLayoutObject;
	};

	DynamicStructLayout::~DynamicStructLayout()
	{
		for (auto& ite : member_view)
		{
			ite.~MemberView();
		}
	}

	Layout DynamicStructLayout::GetLayout() const
	{
		return total_layout;
	}

	auto DynamicStructLayout::GetMemberView() const
		->std::span<MemberView const>
	{
		return member_view;
	}

	StructLayout::Ptr StructLayout::CreateDynamic(std::u8string_view name, std::span<Member const> members, LayoutPolicyRef layout_policy, std::pmr::memory_resource* resource)
	{
		std::size_t hash_code = 0;
		OperateProperty ope_property;

		std::size_t name_size = name.size();
		std::size_t index = 1;

		for (auto& ite : members)
		{
			auto cur_hash_code = ite.struct_layout->GetHashCode();
			hash_code += (cur_hash_code - (cur_hash_code % index)) * ite.array_count + (cur_hash_code % index);
			name_size += ite.name.size();
			ope_property = ope_property && ite.struct_layout->GetOperateProperty();
			++index;
		}

		auto cur_layout_cpp = Layout::Get<DynamicStructLayout>();
		auto member_offset = *layout_policy.Combine(cur_layout_cpp, Layout::Get<MemberView>(), members.size());
		auto name_offset = *layout_policy.Combine(cur_layout_cpp, Layout::Get<char>(), name_size);
		auto record_layout = cur_layout_cpp;
		auto cur_layout = *layout_policy.Complete(cur_layout_cpp);
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if (re)
		{
			Layout total_layout;
			auto member_span = std::span(reinterpret_cast<MemberView*>(re.GetByte(member_offset.buffer_offset)), members.size());
			auto str_span = std::span(reinterpret_cast<char8_t*>(re.GetByte(name_offset.buffer_offset)), name_size);
			std::memcpy(str_span.data(), name.data(), name.size() * sizeof(char8_t));
			name = std::u8string_view{ str_span.subspan(0, name.size()) };
			str_span = str_span.subspan(name.size());

			for (std::size_t i = 0; i < members.size(); ++i)
			{
				auto& cur = members[i];
				auto& tar = member_span[i];
				auto new_layout = cur.struct_layout->GetLayout();
				auto offset = *layout_policy.Combine(total_layout, new_layout, cur.array_count);

				std::memcpy(str_span.data(), cur.name.data(), cur.name.size() * sizeof(char8_t));

				new (&tar) MemberView{
					cur.struct_layout,
					std::u8string_view{str_span.data(), cur.name.size()},
					offset
				};
				str_span = str_span.subspan(cur.name.size());
			}
			return new (re.Get()) DynamicStructLayout{
				ope_property,
				name,
				*layout_policy.Complete(total_layout),
				member_span,
				hash_code,
				re
			};
		}

		return {};
	}

	StructLayoutObject::~StructLayoutObject()
	{
		assert(struct_layout && buffer != nullptr);
		struct_layout->Destruction(offset.GetMember(reinterpret_cast<std::byte*>(buffer)), offset.element_count, offset.next_element_offset);
	}

	auto StructLayoutObject::DefaultConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, std::pmr::memory_resource* resource)
		->Ptr
	{
		if (!struct_layout || array_count == 0)
			return {};

		if (!struct_layout->GetOperateProperty().construct_default)
			return {};

		Layout layout_cpp = Layout::Get<StructLayoutObject>();
		auto policy = MemLayout::GetCPPLikePolicy();
		auto m_layout = struct_layout->GetLayout();
		auto offset = *policy.Combine(layout_cpp, m_layout, array_count);
		auto o_layout = *policy.Complete(layout_cpp);
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = offset.GetMember(re.GetByte());
				auto re2 = struct_layout->DefaultConstruction(start, offset.element_count, offset.next_element_offset);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ re, re.Get(), offset, std::move(struct_layout) };
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr struct_layout, void const* source, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if (!struct_layout || !struct_layout->GetOperateProperty().construct_copy || source == nullptr || array_count == 0)
			return {};
		auto policy = MemLayout::GetCPPLikePolicy();
		auto cpp_layout = Layout::Get<StructLayoutObject>();
		auto m_layout = struct_layout->GetLayout();
		
		auto offset = *policy.Combine(cpp_layout, m_layout, array_count);
		auto o_layout = *policy.Complete(cpp_layout);
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = offset.GetMember(re.GetByte());
				auto re2 = struct_layout->CopyConstruction(start, source, offset.element_count, offset.next_element_offset);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ re, re.Get(), offset, std::move(struct_layout)};
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr struct_layout, void* source, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if (!struct_layout || !struct_layout->GetOperateProperty().construct_copy || source == nullptr || array_count == 0)
			return {};

		auto policy = MemLayout::GetCPPLikePolicy();

		auto cpp_layout = Layout::Get<StructLayoutObject>();
		auto m_layout = struct_layout->GetLayout();
		auto offset = *policy.Combine(cpp_layout, m_layout, array_count);
		auto o_layout = *policy.Complete(cpp_layout);
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = offset.GetMember(re.GetByte());
				auto re2 = struct_layout->MoveConstruction(start, source, offset.element_count, offset.next_element_offset);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ re, re.Get(), offset, std::move(struct_layout)};
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::CustomConstruction(StructLayout::Ptr struct_layout, std::span<StructLayout::CustomConstruct const> construct_parameter, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if (!struct_layout)
			return {};

		auto policy = MemLayout::GetCPPLikePolicy();
		auto cpp_layout = Layout::Get<StructLayoutObject>();
		auto m_layout = struct_layout->GetLayout();
		auto offset = *policy.Combine(cpp_layout, m_layout, array_count);
		auto o_layout = *policy.Complete(cpp_layout);
		auto member_view = struct_layout->GetMemberView();

		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			auto buffer = offset.GetMember(re.GetByte());
			if (struct_layout->CustomConstruction(buffer, construct_parameter, offset.element_count, offset.next_element_offset))
			{
				return new (re.Get()) StructLayoutObject{ re, re.Get(), offset, std::move(struct_layout)};
			}
			re.Deallocate();
		}
		return {};
	}

	bool StructLayout::CopyConstruction(void* target, void const* source, std::size_t array_count, std::size_t next_element_offset = 0) const
	{
		assert(target != nullptr && source != nullptr && array_count > 0);
		auto operate_property = GetOperateProperty();
		if(operate_property.construct_copy)
		{
			auto member_view = GetMemberView();
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			std::byte const* sou = static_cast<std::byte const*>(source);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + next_element_offset * i;
				std::byte const* sou_ite = sou + next_element_offset * i;
				for(auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto data2 = GetMemberData(ite, sou_ite);
					auto re = ite.struct_layout->CopyConstruction(data, data2, ite.offset.element_count, ite.offset.next_element_offset);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveConstruction(void* target, void* source, std::size_t array_count, std::size_t next_element_offset) const
	{
		assert(target != source && target != nullptr && source != nullptr && array_count >= 1);
		auto operate_property = GetOperateProperty();
		if(operate_property.construct_move)
		{
			auto member_view = GetMemberView();
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			std::byte* sou = static_cast<std::byte*>(source);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + next_element_offset * i;
				std::byte* sou_ite = sou + next_element_offset * i;
				for(auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto data2 = GetMemberData(ite, sou_ite);
					auto re = ite.struct_layout->MoveConstruction(data, data2, ite.offset.element_count, ite.offset.next_element_offset);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::DefaultConstruction(void* target, std::size_t array_count, std::size_t next_element_offset) const
	{
		assert(target != nullptr &&  array_count >= 1);
		auto operate_property = GetOperateProperty();
		if (operate_property.construct_default)
		{
			auto member_view = GetMemberView();
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			for (std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + next_element_offset * i;
				for (auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto re = ite.struct_layout->DefaultConstruction(data, ite.offset.element_count, ite.offset.next_element_offset);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::Destruction(void* target, std::size_t array_count, std::size_t next_element_offset) const
	{
		assert(target != nullptr);
		auto member_view = GetMemberView();
		auto layout = GetLayout();
		std::byte* tar = static_cast<std::byte*>(target);
		for(std::size_t i = 0; i < array_count; ++i)
		{
			std::byte* tar_ite = tar + next_element_offset * i;
			for(auto& ite : member_view)
			{
				auto data = GetMemberData(ite, tar_ite);
				auto re = ite.struct_layout->Destruction(data, ite.offset.element_count, ite.offset.next_element_offset);
				assert(re);
			}
		}
		return true;
	}

	bool StructLayout::CopyAssigned(void* target, void const* source, std::size_t array_count, std::size_t next_element_offset) const
	{
		assert(target != nullptr && source != nullptr);
		auto member_view = GetMemberView();
		auto operate_property = GetOperateProperty();
		auto layout = GetLayout();
		std::byte* tar = static_cast<std::byte*>(target);
		std::byte const* sou = static_cast<std::byte const*>(source);
		if(operate_property.assign_copy)
		{
			for (std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + next_element_offset * i;
				std::byte const* sou_ite = sou + next_element_offset * i;
				for (auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto re = ite.struct_layout->CopyAssigned(data, sou_ite, ite.offset.element_count, ite.offset.next_element_offset);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveAssigned(void* target, void* source, std::size_t array_count, std::size_t next_element_offset) const
	{
		assert(target != nullptr && source != nullptr);
		auto member_view = GetMemberView();
		auto operate_property = GetOperateProperty();
		auto layout = GetLayout();
		std::byte* tar = static_cast<std::byte*>(target);
		std::byte* sou = static_cast<std::byte*>(source);
		if (operate_property.assign_move)
		{
			for (std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + next_element_offset * i;
				std::byte* sou_ite = sou + next_element_offset * i;
				for (auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto re = ite.struct_layout->MoveAssigned(data, sou_ite, ite.offset.element_count, ite.offset.next_element_offset);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::CustomConstruction(void* target, std::span<CustomConstruct const> custom_construct, std::size_t array_count, std::size_t next_element_offset) const
	{
		auto mvs = GetMemberView();
		if (custom_construct.size() != mvs.size())
			return false;
		
		for (std::size_t mermeber_index = 0; mermeber_index < mvs.size(); ++mermeber_index)
		{
			auto& mv = mvs[mermeber_index];
			auto cp = custom_construct[mermeber_index];
			auto ope_property = mv.struct_layout->GetOperateProperty();
			switch (cp.construct_operator)
			{
			case CustomConstructOperator::Default:
				if (!ope_property.construct_default)
				{
					return false;
				}
				break;
			case CustomConstructOperator::Copy:
				if (!ope_property.construct_copy || cp.paramter_object.construct_parameter_const_object == nullptr)
					return false;
				break;
			case CustomConstructOperator::Move:
				if (!ope_property.construct_move || cp.paramter_object.construct_parameter_object == nullptr)
					return false;
				break;
			default:
				return false;
				break;
			}
		}

		for (std::size_t index = 0; index < array_count; ++index)
		{
			auto target_buffer = reinterpret_cast<std::byte*>(target) + index * next_element_offset;

			for (std::size_t mermeber_index = 0; mermeber_index < mvs.size(); ++mermeber_index)
			{
				auto& mv = mvs[mermeber_index];
				auto cp = custom_construct[mermeber_index];

				switch (cp.construct_operator)
				{
				case CustomConstructOperator::Default:
				{
					auto re = mv.struct_layout->DefaultConstruction(
						GetMemberData(mv, target_buffer),
						mv.offset.element_count, mv.offset.next_element_offset
					);
					assert(re);
					break;
				}
				case CustomConstructOperator::Copy:
				{
					auto re = mv.struct_layout->CopyConstruction(
						GetMemberData(mv, target_buffer),
						cp.paramter_object.construct_parameter_const_object,
						mv.offset.element_count, mv.offset.next_element_offset
					);
					assert(re);
					break;
				}
				case CustomConstructOperator::Move:
				{
					auto re = mv.struct_layout->MoveConstruction(
						GetMemberData(mv, target_buffer),
						cp.paramter_object.construct_parameter_object,
						mv.offset.element_count, mv.offset.next_element_offset
					);
					assert(re);
					break;
				}
				}
			}
		}

		return true;
	}

	void* StructLayoutObject::GetArrayMemberData(StructLayout::MemberView const& member_view, std::size_t element_index)
	{
		return member_view.offset.GetMember(offset.GetMember(reinterpret_cast<std::byte*>(buffer)), element_index);
	}

	void* StructLayoutObject::GetArrayMemberData(std::size_t member_index, std::size_t element_index)
	{
		auto member_view = struct_layout->FindMemberView(member_index);
		if (member_view.has_value())
			return GetArrayMemberData(*member_view, element_index);
		return nullptr;
	}
}
