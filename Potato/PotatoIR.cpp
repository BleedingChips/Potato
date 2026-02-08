module;

#include <cassert>

module PotatoIR;
//import :Define;

namespace Potato::IR
{

	MemoryResourceRecord MemoryResourceRecord::Allocate(std::pmr::memory_resource* resource, Layout layout)
	{
		assert(resource != nullptr);
		auto adress = resource->allocate(layout.size, layout.align);
		return { resource, layout, adress };
	}

	MemoryResourceRecord::operator bool() const
	{
		return resource != nullptr && adress != nullptr;
	}

	void MemoryResourceRecord::Deallocate()
	{
		assert(*this);
		resource->deallocate(adress, layout.size, layout.align);
		resource = nullptr;
		adress = nullptr;
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
			if (mem[i].member_layout != mem2[i].member_layout)
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
		Layout GetLayoutAsMember() const override;
		std::span<MemberView const> GetMemberView() const override;
		OperateProperty GetOperateProperty() const override { return construct_property; }
		virtual std::uint64_t GetHashCode() const override { return hash_code; }

	protected:

		DynamicStructLayout(
			OperateProperty construct_property, 
			std::u8string_view name, 
			Layout total_layout, 
			Layout layout_as_member,
			std::span<MemberView> member_view, 
			std::uint64_t hash_code,
			MemoryResourceRecord record
		)
			: 
			MemoryResourceRecordIntrusiveInterface(record), 
			name(name), 
			total_layout(total_layout), 
			layout_as_member(layout_as_member), 
			hash_code(hash_code), 
			member_view(member_view), 
			construct_property(construct_property)
		{

		}

		~DynamicStructLayout();

		std::u8string_view name;
		Layout total_layout;
		Layout layout_as_member;
		std::span<MemberView> member_view;
		OperateProperty construct_property;
		std::uint64_t hash_code;

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

	Layout DynamicStructLayout::GetLayoutAsMember() const
	{
		return layout_as_member;
	}

	auto DynamicStructLayout::GetMemberView() const
		->std::span<MemberView const>
	{
		return member_view;
	}

	std::uint64_t StructLayout::CalculateHashCode(std::u8string_view name, std::span<MemberView const> members)
	{
		std::uint64_t hash_code = 0;
		std::size_t index = 1;

		for (auto& ite : members)
		{
			auto cur_hash_code = ite.struct_layout->GetHashCode();
			hash_code += (cur_hash_code - (cur_hash_code % index)) * ite.member_layout.array_layout.count + (cur_hash_code % index);
			++index;
		}

		return hash_code;
	}

	StructLayout::Ptr StructLayout::CreateDynamic(std::u8string_view name, std::span<Member const> members, LayoutPolicyRef layout_policy, std::pmr::memory_resource* resource)
	{
		OperateProperty ope_property;

		std::size_t name_size = name.size();
		std::size_t index = 1;

		for (auto& ite : members)
		{
			name_size += ite.name.size();
			ope_property = ope_property && ite.struct_layout->GetOperateProperty();
			++index;
		}

		auto cpp_policy = MemLayout::LayoutPolicyRef{};
		auto cur_layout_cpp = Layout::Get<DynamicStructLayout>();
		auto member_offset = *cpp_policy.Combine(cur_layout_cpp, Layout::Get<MemberView>(), members.size());
		auto name_offset = *cpp_policy.Combine(cur_layout_cpp, Layout::Get<char>(), name_size);
		auto record_layout = cur_layout_cpp;
		auto cur_layout = *cpp_policy.Complete(cur_layout_cpp);
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if (re)
		{
			Layout total_layout;
			auto member_span = std::span(reinterpret_cast<MemberView*>(member_offset.GetMember(re.GetByte())), members.size());
			auto str_span = std::span(reinterpret_cast<char8_t*>(name_offset.GetMember(re.GetByte())), name_size);
			std::memcpy(str_span.data(), name.data(), name.size() * sizeof(char8_t));
			name = std::u8string_view{ str_span.subspan(0, name.size()) };
			str_span = str_span.subspan(name.size());

			for (std::size_t i = 0; i < members.size(); ++i)
			{
				auto& cur = members[i];
				auto& tar = member_span[i];
				auto new_layout = cur.struct_layout->GetLayoutAsMember();
				auto offset = *layout_policy.Combine(total_layout, new_layout, cur.array_count);

				std::memcpy(str_span.data(), cur.name.data(), cur.name.size() * sizeof(char8_t));

				new (&tar) MemberView{
					cur.struct_layout,
					std::u8string_view{str_span.data(), cur.name.size()},
					offset
				};
				str_span = str_span.subspan(cur.name.size());
			}

			std::uint64_t hash_code = StructLayout::CalculateHashCode(name, member_span);
			return new (re.Get()) DynamicStructLayout{
				ope_property,
				name,
				*layout_policy.Complete(total_layout),
				total_layout,
				member_span,
				hash_code,
				re
			};
		}

		return {};
	}

	StructLayoutObject::~StructLayoutObject()
	{
		assert(struct_layout && buffer.size() > 0);
		auto re = struct_layout->Destruction(GetObject(), array_layout);
		assert(re);
	}

	std::tuple<Layout, MemLayout::MermberLayout, std::size_t> StructLayoutObject::CalculateMemberLayout(StructLayout const& struct_layout, std::size_t array_count, MemLayout::LayoutPolicyRef array_policy)
	{
		Layout layout_cpp = Layout::Get<StructLayoutObject>();
		MemLayout::MermberLayout offset;
		std::size_t size = 0;
		if (array_count == 0)
		{
			auto mer_layout = struct_layout.GetLayout();
			offset = *LayoutPolicyRef{}.Combine(layout_cpp, mer_layout);
			size = mer_layout.size;
		}
		else {
			Layout total_layout;
			offset = *array_policy.Combine(total_layout, struct_layout.GetLayoutAsMember(), array_count);
			total_layout = *array_policy.Complete(total_layout);
			size = total_layout.size;
			auto offset2 = *LayoutPolicyRef{}.Combine(layout_cpp, total_layout);
			offset.offset += offset2.offset;
		}
		auto total_layout = *LayoutPolicyRef{}.Complete(layout_cpp);
		return { total_layout, offset, size};
	}

	auto StructLayoutObject::DefaultConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, MemLayout::LayoutPolicyRef policy, std::pmr::memory_resource* resource)
		-> Ptr
	{
		assert(struct_layout);
		assert(struct_layout->GetOperateProperty().construct_default);

		auto [o_layout, offset, size] = CalculateMemberLayout(*struct_layout, array_count, policy);
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto re2 = struct_layout->DefaultConstruction(offset.GetMember(re.GetByte()), offset.array_layout);
				assert(re2);
				std::span<std::byte> buffer{ offset.GetMember(re.GetByte()), size };
				return new (re.Get()) StructLayoutObject{ re, buffer, std::move(struct_layout), offset.array_layout };
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, void const* source, MemLayout::ArrayLayout source_array_layout, MemLayout::LayoutPolicyRef policy, std::pmr::memory_resource* resource)
		-> Ptr
	{
		assert(struct_layout && struct_layout->GetOperateProperty().construct_copy && source != nullptr);
		assert(array_count == source_array_layout.count);

		auto [o_layout, offset, size] = CalculateMemberLayout(*struct_layout, array_count, policy);
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto re2 = struct_layout->CopyConstruction(offset.GetMember(re.GetByte()), source, offset.array_layout, source_array_layout);
				assert(re2);
				std::span<std::byte> buffer{ offset.GetMember(re.GetByte()), size };
				return new (re.Get()) StructLayoutObject{ re, buffer, std::move(struct_layout), offset.array_layout};
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, void* source, MemLayout::ArrayLayout source_array_layout, MemLayout::LayoutPolicyRef policy, std::pmr::memory_resource* resource)
		-> Ptr
	{

		assert(struct_layout && struct_layout->GetOperateProperty().construct_move && source != nullptr);
		assert(array_count == source_array_layout.count);

		auto [o_layout, offset, size] = CalculateMemberLayout(*struct_layout, array_count, policy);
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto re2 = struct_layout->MoveConstruction(offset.GetMember(re.GetByte()), source, offset.array_layout, source_array_layout);
				assert(re2);
				std::span<std::byte> buffer{ offset.GetMember(re.GetByte()), size };
				return new (re.Get()) StructLayoutObject{ re, buffer, std::move(struct_layout), offset.array_layout};
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::CustomConstruction(StructLayout::Ptr struct_layout, std::span<StructLayout::CustomConstruct const> construct_parameter, std::pmr::memory_resource* resource)
		-> Ptr
	{
		assert(struct_layout);

		auto policy = MemLayout::LayoutPolicyRef{};
		auto cpp_layout = Layout::Get<StructLayoutObject>();
		auto m_layout = struct_layout->GetLayout();
		auto offset = *policy.Combine(cpp_layout, m_layout);
		std::size_t size = cpp_layout.size - offset.offset;
		auto o_layout = *policy.Complete(cpp_layout);
		auto member_view = struct_layout->GetMemberView();

		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			auto buffer = std::span<std::byte>{offset.GetMember(re.GetByte()), size};
			if (struct_layout->CustomConstruction(buffer.data(), construct_parameter))
			{
				return new (re.Get()) StructLayoutObject{ re, buffer, std::move(struct_layout), {0, o_layout.size} };
			}
			re.Deallocate();
		}
		return {};
	}

	bool StructLayout::CopyConstruction(void* target, void const* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const
	{
		auto operate_property = GetOperateProperty();
		assert(operate_property.construct_copy);
		assert(target != nullptr && source != nullptr && target != source);
		assert(target_array_layout.count == source_array_layout.count);
		if(operate_property.construct_copy)
		{
			auto count = std::max(target_array_layout.count, std::size_t{1});
			auto member_view = GetMemberView();

			for (std::size_t i = 0; i < count; ++i)
			{
				auto tar = target_array_layout.GetElement(target, i);
				auto sour = source_array_layout.GetElement(source, i);
				for (auto& ite : member_view)
				{
					auto re = ite.struct_layout->CopyConstruction(
						ite.member_layout.GetMember(tar),
						ite.member_layout.GetMember(sour),
						ite.member_layout.array_layout,
						ite.member_layout.array_layout
					);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveConstruction(void* target, void* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const
	{
		assert(target != nullptr && source != nullptr && target != source);
		auto operate_property = GetOperateProperty();
		assert(operate_property.construct_move);
		assert(target_array_layout.count == source_array_layout.count);
		if(operate_property.construct_move)
		{
			assert(target_array_layout.count == source_array_layout.count);
			auto count = std::max(target_array_layout.count, std::size_t{ 1 });
			auto member_view = GetMemberView();

			for (std::size_t i = 0; i < count; ++i)
			{
				auto tar = target_array_layout.GetElement(target, i);
				auto sour = source_array_layout.GetElement(source, i);
				for (auto& ite : member_view)
				{
					auto re = ite.struct_layout->MoveConstruction(
						ite.member_layout.GetMember(tar),
						ite.member_layout.GetMember(sour),
						ite.member_layout.array_layout,
						ite.member_layout.array_layout
					);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::DefaultConstruction(void* target, MemLayout::ArrayLayout target_array_layout) const
	{
		assert(target != nullptr);
		auto operate_property = GetOperateProperty();
		assert(operate_property.construct_default);
		if (operate_property.construct_default)
		{
			assert(target != nullptr);
			auto count = std::max(target_array_layout.count, std::size_t{ 1 });
			auto member_view = GetMemberView();

			for (std::size_t i = 0; i < count; ++i)
			{
				auto tar = target_array_layout.GetElement(target, i);
				for (auto& ite : member_view)
				{
					auto re = ite.struct_layout->DefaultConstruction(
						ite.member_layout.GetMember(tar),
						ite.member_layout.array_layout
					);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::Destruction(void* target, MemLayout::ArrayLayout target_array_layout) const
	{
		assert(target != nullptr);
		auto count = std::max(target_array_layout.count, std::size_t{ 1 });
		auto member_view = GetMemberView();

		for (std::size_t i = 0; i < count; ++i)
		{
			auto tar = target_array_layout.GetElement(target, i);
			for (auto& ite : member_view)
			{
				auto re = ite.struct_layout->Destruction(
					ite.member_layout.GetMember(tar),
					ite.member_layout.array_layout
				);
				assert(re);
			}
		}
		return true;
	}

	bool StructLayout::CopyAssigned(void* target, void const* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const
	{
		assert(target != nullptr && source != nullptr && target != source);
		auto operate_property = GetOperateProperty();
		assert(operate_property.assign_copy);
		if (operate_property.assign_copy)
		{
			assert(target != nullptr && source != nullptr && target != source);
			assert(target_array_layout.count == source_array_layout.count);
			auto count = std::max(target_array_layout.count, std::size_t{ 1 });
			auto member_view = GetMemberView();

			for (std::size_t i = 0; i < count; ++i)
			{
				auto tar = target_array_layout.GetElement(target, i);
				auto sour = source_array_layout.GetElement(source, i);
				for (auto& ite : member_view)
				{
					auto re = ite.struct_layout->CopyAssigned(
						ite.member_layout.GetMember(tar),
						ite.member_layout.GetMember(sour),
						ite.member_layout.array_layout,
						ite.member_layout.array_layout
					);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveAssigned(void* target, void* source, MemLayout::ArrayLayout target_array_layout, MemLayout::ArrayLayout source_array_layout) const
	{
		assert(target != nullptr && source != nullptr && target != source);
		auto operate_property = GetOperateProperty();
		assert(operate_property.assign_copy);
		if (operate_property.assign_copy)
		{
			assert(target != nullptr && source != nullptr && target != source);
			assert(target_array_layout.count == source_array_layout.count);
			auto count = std::max(target_array_layout.count, std::size_t{ 1 });
			auto member_view = GetMemberView();

			for (std::size_t i = 0; i < count; ++i)
			{
				auto tar = target_array_layout.GetElement(target, i);
				auto sour = source_array_layout.GetElement(source, i);
				for (auto& ite : member_view)
				{
					auto re = ite.struct_layout->MoveAssigned(
						ite.member_layout.GetMember(tar),
						ite.member_layout.GetMember(sour),
						ite.member_layout.array_layout,
						ite.member_layout.array_layout
					);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::CustomConstruction(void* target, std::span<CustomConstruct const> custom_construct) const
	{
		auto mvs = GetMemberView();
		if (custom_construct.size() != mvs.size())
		{
			assert(false);
			return false;
		}
		
		for (std::size_t mermeber_index = 0; mermeber_index < mvs.size(); ++mermeber_index)
		{
			auto& mv = mvs[mermeber_index];
			auto cp = custom_construct[mermeber_index];
			auto ope_property = mv.struct_layout->GetOperateProperty();
			
			if (cp.array_layout.count != mv.member_layout.array_layout.count)
			{
				assert(cp.array_layout.count == mv.member_layout.array_layout.count);
				return false;
			}
			switch (cp.construct_operator)
			{
			case CustomConstructOperator::Default:
				if (!ope_property.construct_default)
				{
					assert(false);
					return false;
				}
				break;
			case CustomConstructOperator::Copy:
				if (!ope_property.construct_copy || cp.paramter_object.construct_parameter_const_object == nullptr)
				{
					assert(false);
					return false;
				}
				break;
			case CustomConstructOperator::Move:
				if (!ope_property.construct_move || cp.paramter_object.construct_parameter_object == nullptr)
				{
					assert(false);
					return false;
				}
				break;
			default:
				assert(false);
				return false;
				break;
			}
		}

		for (std::size_t mermeber_index = 0; mermeber_index < mvs.size(); ++mermeber_index)
		{
			auto& ope = custom_construct[mermeber_index];
			auto& mv = mvs[mermeber_index];
			switch (ope.construct_operator)
			{
			case CustomConstructOperator::Default:
			{
				auto re = mv.struct_layout->DefaultConstruction(
					mv.member_layout.GetMember(target),
					mv.member_layout.array_layout
				);
				assert(re);
				break;
			}
			case CustomConstructOperator::Copy:
			{
				auto re = mv.struct_layout->CopyConstruction(
					mv.member_layout.GetMember(target),
					ope.paramter_object.construct_parameter_const_object,
					mv.member_layout.array_layout,
					ope.array_layout
				);
				assert(re);
				break;
			}
			case CustomConstructOperator::Move:
			{
				auto re = mv.struct_layout->MoveConstruction(
					mv.member_layout.GetMember(target),
					ope.paramter_object.construct_parameter_object,
					mv.member_layout.array_layout,
					ope.array_layout
				);
				assert(re);
				break;
			}
			default:
				assert(false);
				return false;
			}
		}
		return true;
	}
}
