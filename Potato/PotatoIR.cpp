module;

#include <cassert>

module PotatoIR;
//import :Define;

namespace Potato::IR
{
	std::wstring TranslateTypeName(std::string_view type_name)
	{
		std::array<wchar_t, 1024> cache_name;
		auto info = Encode::StrEncoder<char, wchar_t>{}.Encode(type_name, cache_name);
		return std::wstring{ cache_name.data(), info.target_space };
	}

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
			if (mem[i].array_count != mem2[i].array_count)
				return false;
			/*
			if (*mem[i].struct_layout != *mem2[i].struct_layout)
				return false;
				*/
		}
		return true;
	}

	std::span<std::byte> StructLayout::GetMemberDataBuffer(MemberView const& view, void* target)
	{
		return {
			static_cast<std::byte*>(target) + view.offset,
			view.struct_layout->GetLayout(view.array_count).size
		};
	}

	auto StructLayout::FindMemberView(std::wstring_view member_name) const
	-> std::optional<MemberView>
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

	Layout StructLayout::GetLayout(std::size_t array_count) const 
	{
		auto layout = GetLayout();
		return {
			layout.align,
			layout.size * array_count
		};
	}

	

	struct DynamicStructLayout : public StructLayout, public MemoryResourceRecordIntrusiveInterface
	{
		virtual std::wstring_view GetName() const override { return name; }
		virtual void AddStructLayoutRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubStructLayoutRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }
		Layout GetLayout() const override;
		std::span<MemberView const> GetMemberView() const override;
		OperateProperty GetOperateProperty() const override { return construct_property; }
		virtual std::size_t GetHashCode() const override { return hash_code; }

	protected:

		DynamicStructLayout(OperateProperty construct_property, std::wstring_view name, Layout total_layout, std::span<MemberView> member_view, std::size_t hash_code, MemoryResourceRecord record)
			: MemoryResourceRecordIntrusiveInterface(record), name(name), total_layout(total_layout), hash_code(hash_code), member_view(member_view), construct_property(construct_property)
		{

		}

		~DynamicStructLayout();

		std::wstring_view name;
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

	StructLayout::Ptr StructLayout::CreateDynamic(std::wstring_view name, std::span<Member const> members, LayoutCategory category, std::pmr::memory_resource* resource)
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

		auto cur_layout_cpp = MemLayout::MemLayout::Get<DynamicStructLayout>(category);
		std::size_t member_offset = cur_layout_cpp.Insert(Layout::GetArray<MemberView>(members.size()));
		std::size_t name_offset = cur_layout_cpp.Insert(Layout::GetArray<wchar_t>(name_size));

		auto record_layout = cur_layout_cpp;

		auto cur_layout = cur_layout_cpp.Get();
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if (re)
		{
			MemLayout::MemLayout total_layout_cpp{};
			auto member_span = std::span(reinterpret_cast<MemberView*>(re.GetByte(member_offset)), members.size());
			auto str_span = std::span(reinterpret_cast<wchar_t*>(re.GetByte(name_offset)), name_size);
			std::memcpy(str_span.data(), name.data(), name.size() * sizeof(wchar_t));
			name = std::wstring_view{ str_span.subspan(0, name.size()) };
			str_span = str_span.subspan(name.size());

			for (std::size_t i = 0; i < members.size(); ++i)
			{
				auto& cur = members[i];
				auto& tar = member_span[i];

				std::size_t offset = 0;
				if (i == 0)
				{
					switch (category)
					{
					case LayoutCategory::CLike:
						total_layout_cpp = cur.struct_layout->GetLayout(cur.array_count);
						break;
					}
					
				}
				else
				{
					auto new_layout = cur.struct_layout->GetLayout(cur.array_count);
					offset = total_layout_cpp.Insert(new_layout);
				}

				std::memcpy(str_span.data(), cur.name.data(), cur.name.size() * sizeof(wchar_t));

				new (&tar) MemberView{
					cur.struct_layout,
					std::wstring_view{str_span.data(), cur.name.size()},
					cur.array_count,
					offset
				};
				str_span = str_span.subspan(cur.name.size());
			}
			return new (re.Get()) DynamicStructLayout{
				ope_property,
				name,
				total_layout_cpp.Get(),
				member_span,
				hash_code,
				re
			};
		}

		return {};
	}

	StructLayoutObject::~StructLayoutObject()
	{
		assert(struct_layout && start != nullptr);
		struct_layout->Destruction(start, array_count);
	}

	auto StructLayoutObject::DefaultConstruct(StructLayout::Ptr struct_layout, std::size_t array_count, std::pmr::memory_resource* resource)
		->Ptr
	{
		if (!struct_layout || array_count == 0)
			return {};

		if (!struct_layout->GetOperateProperty().construct_default)
			return {};

		MemLayout::MemLayout layout_cpp = Layout::Get<StructLayoutObject>();
		auto m_layout = struct_layout->GetLayout(array_count);
		auto offset = layout_cpp.Insert(m_layout);
		auto o_layout = layout_cpp.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = re.GetByte(offset);
				auto re2 = struct_layout->DefaultConstruction(start, array_count);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ re, start, array_count, std::move(struct_layout) };
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
		
		auto cpp_layout = MemLayout::MemLayout::Get<StructLayoutObject>();
		
		auto m_layout = struct_layout->GetLayout(array_count);
		
		auto offset = cpp_layout.Insert(m_layout);
		auto o_layout = cpp_layout.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = re.GetByte(offset);
				auto re2 = struct_layout->CopyConstruction(start, source, array_count);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ re, start, array_count, std::move(struct_layout) };
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

		auto cpp_layout = MemLayout::MemLayout::Get<StructLayoutObject>();
		
		auto m_layout = struct_layout->GetLayout(array_count);

		auto offset = cpp_layout.Insert(m_layout);
		auto o_layout = cpp_layout.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = static_cast<std::byte*>(re.Get()) + offset;
				auto re2 = struct_layout->MoveConstruction(start, source, array_count);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ re, start, array_count, std::move(struct_layout) };
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::Construct(StructLayout::Ptr struct_layout, std::span<MemberConstruct const> construct_parameter, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if (!struct_layout || struct_layout->GetMemberView().size() != construct_parameter.size())
			return {};
		auto member_view = struct_layout->GetMemberView();

		for (std::size_t i = 0; i < member_view.size(); ++i)
		{
			auto operate = member_view[i].struct_layout->GetOperateProperty();
			switch (construct_parameter[i].construct_operator)
			{
			case ConstructOperator::Default:
				if (!operate.construct_default)
					return {};
				break;
			case ConstructOperator::Copy:
				if (!operate.construct_copy || construct_parameter[i].construct_parameter_object == nullptr)
					return {};
				break;
			case ConstructOperator::Move:
				if (!operate.construct_move || construct_parameter[i].construct_parameter_object == nullptr)
					return {};
				break;
			default:
				return {};
			}
		}

		auto cpp_layout = MemLayout::MemLayout::Get<StructLayoutObject>();
		auto m_layout = struct_layout->GetLayout();
		auto offset = cpp_layout.Insert(m_layout);
		auto o_layout = cpp_layout.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			auto start = re.GetByte(offset);
			for (std::size_t i = 0; i < member_view.size(); ++i)
			{
				auto& mv = member_view[i];
				switch (construct_parameter[i].construct_operator)
				{
				case ConstructOperator::Default:
				{
					auto result = mv.struct_layout->DefaultConstruction(start + mv.offset);
					assert(result);
					break;
				}
				case ConstructOperator::Copy:
				{
					auto result = mv.struct_layout->CopyConstruction(start + mv.offset, construct_parameter[i].construct_parameter_object);
					assert(result);
					break;
				}
				case ConstructOperator::Move:
				{
					auto result = mv.struct_layout->MoveConstruction(start + mv.offset, construct_parameter[i].construct_parameter_object);
					assert(result);
					break;
				}
				default:
					break;
				}
			}

			return new (re.Get()) StructLayoutObject{ re, start, 1, std::move(struct_layout) };
		}
		return {};
	}

	bool StructLayout::CopyConstruction(void* target, void const* source, std::size_t array_count) const
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
				std::byte* tar_ite = tar + layout.size * i;
				std::byte const* sou_ite = sou + layout.size * i;
				for(auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto data2 = GetMemberData(ite, sou_ite);
					auto re = ite.struct_layout->CopyConstruction(data, data2, ite.array_count);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveConstruction(void* target, void* source, std::size_t array_count) const
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
				std::byte* tar_ite = tar + layout.size * i;
				std::byte* sou_ite = sou + layout.size * i;
				for(auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto data2 = GetMemberData(ite, sou_ite);
					auto re = ite.struct_layout->MoveConstruction(data, data2, ite.array_count);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::DefaultConstruction(void* target, std::size_t array_count) const
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
				std::byte* tar_ite = tar + layout.size * i;
				for (auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto re = ite.struct_layout->DefaultConstruction(data, ite.array_count);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::Destruction(void* target, std::size_t array_count) const
	{
		assert(target != nullptr);
		auto member_view = GetMemberView();
		auto layout = GetLayout();
		std::byte* tar = static_cast<std::byte*>(target);
		for(std::size_t i = 0; i < array_count; ++i)
		{
			std::byte* tar_ite = tar + layout.size * i;
			for(auto& ite : member_view)
			{
				auto data = GetMemberData(ite, tar_ite);
				auto re = ite.struct_layout->Destruction(data, ite.array_count);
				assert(re);
			}
		}
		return true;
	}

	bool StructLayout::CopyAssigned(void* target, void const* source, std::size_t array_count) const
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
				std::byte* tar_ite = tar + layout.size * i;
				std::byte const* sou_ite = sou + layout.size * i;
				for (auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto re = ite.struct_layout->CopyAssigned(data, sou_ite, ite.array_count);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveAssigned(void* target, void* source, std::size_t array_count) const
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
				std::byte* tar_ite = tar + layout.size * i;
				std::byte* sou_ite = sou + layout.size * i;
				for (auto& ite : member_view)
				{
					auto data = GetMemberData(ite, tar_ite);
					auto re = ite.struct_layout->MoveAssigned(data, sou_ite, ite.array_count);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}

	void* StructLayoutObject::GetArrayMemberData(StructLayout::MemberView const& member_view, std::size_t element_index)
	{
		return static_cast<std::byte*>(GetArrayData(element_index)) + member_view.offset;
	}

	void* StructLayoutObject::GetArrayMemberData(std::size_t member_index, std::size_t element_index)
	{
		auto member_view = struct_layout->FindMemberView(member_index);
		if (member_view.has_value())
			return GetArrayMemberData(*member_view, element_index);
		return nullptr;
	}
}
