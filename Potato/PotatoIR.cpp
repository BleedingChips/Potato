module;

#include <cassert>

module PotatoIR;
//import :Define;

namespace Potato::IR
{
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

	auto SymbolTable::InsertSymbol(std::u32string Name, std::size_t FeedbackIndex, std::any Data) -> std::size_t
	{
		std::size_t Index = Mappings.size();
		std::size_t Area = 0;
		if(!MarkedAreaStack.empty())
			Area = MarkedAreaStack.rbegin()->AreaIndex;
		Mappings.emplace_back(Mapping::State::Active, ActiveProperty.size());
		ActiveProperty.emplace_back(std::move(Name), Index, Area, FeedbackIndex, std::move(Data));
		return Index;
	}

	std::size_t SymbolTable::MarkAsAreaBegin()
	{
		MarkedAreaStack.emplace_back(Mappings.size(), AreaMapping.size() + 1);
		return AreaMapping.size() + 1;
	}

	std::optional<std::size_t> SymbolTable::PopArea(bool Searchable, bool KeepProperty)
	{
		if (!MarkedAreaStack.empty())
		{
			auto Last = *MarkedAreaStack.rbegin();
			MarkedAreaStack.pop_back();
			auto Span = std::span(Mappings).subspan(Last.MappingIndex);
			if (Span.empty())
			{
				AreaMapping.push_back({UnactiveProperty.size(), 0});
			}
			else {
				auto Index = Span.begin()->Index;
				auto ActiveSpa = std::span(ActiveProperty).subspan(Index);
				std::size_t UnactivePropertyStart = UnactiveProperty.size();
				if (KeepProperty) {
					AreaMapping.push_back({ UnactivePropertyStart, ActiveSpa.size() });
					UnactiveProperty.insert(UnactiveProperty.end(), std::move_iterator(ActiveSpa.begin()), std::move_iterator(ActiveSpa.end()));
				}
				else {
					AreaMapping.push_back({ UnactivePropertyStart, 0 });
				}		
				ActiveProperty.resize(Index);
				for (auto& Ite : Span)
				{
					if (Ite.ActiveState == Mapping::State::Active)
					{
						if (KeepProperty)
						{
							Ite.ActiveState = Mapping::State::UnActive;
							Ite.Index = UnactivePropertyStart;
							++UnactivePropertyStart;
						}	
						else
							Ite.ActiveState = Mapping::State::Destoryed;
					}
				}
			}
			return Last.MappingIndex;
		}
		return {};
	}

	auto SymbolTable::FindLastActiveSymbol(std::u32string_view Name) ->Pointer::ObserverPtr<Property>
	{
		for (auto Ite = ActiveProperty.rbegin(); Ite != ActiveProperty.rend(); ++Ite)
		{
			if(Ite->Name == Name)
				return Pointer::ObserverPtr<Property>{&*Ite};
		}
		return {};
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

	void* StructLayout::GetData(MemberView const& view, void* target)
	{
		assert(target != nullptr);
		return static_cast<std::byte*>(target) + view.offset;
	}

	std::span<std::byte> StructLayout::GetDataSpan(MemberView const& view, void* target)
	{
		return {
			static_cast<std::byte*>(target) + view.offset,
			view.struct_layout->GetLayout(view.array_count).size
		};
	}

	auto StructLayout::FindMemberView(std::u8string_view member_name) const
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

	StructLayoutObject::~StructLayoutObject()
	{
		assert(layout && start != nullptr);
		layout->Destruction(start, array_count);
	}

	void StructLayoutObject::Release()
	{
		auto re = record;
		this->~StructLayoutObject();
		re.Deallocate();
	}

	auto StructLayoutObject::DefaultConstruct(StructLayout::Ptr layout, std::size_t array_count, std::pmr::memory_resource* resource)
		->Ptr
	{
		assert(layout && layout->GetOperateProperty().default_construct);
		MemLayout::MemLayoutCPP layout_cpp = Layout::Get<StructLayoutObject>();
		auto m_layout = layout->GetLayout(array_count);
		auto offset = layout_cpp.Insert(m_layout);
		auto o_layout = layout_cpp.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = static_cast<std::byte*>(re.Get()) + offset;
				auto re2 = layout->DefaultConstruction(start, array_count);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ start, array_count, std::move(layout), re };
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		assert(layout && layout->GetOperateProperty().copy_construct && source != nullptr);
		auto cpp_layout = MemLayout::MemLayoutCPP::Get<StructLayoutObject>();
		
		auto m_layout = layout->GetLayout(array_count);
		
		auto offset = cpp_layout.Insert(m_layout);
		auto o_layout = cpp_layout.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = static_cast<std::byte*>(re.Get()) + offset;
				auto re2 = layout->CopyConstruction(start, source, array_count);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ start, array_count, std::move(layout), re };
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr layout, StructLayoutObject const& source, std::pmr::memory_resource* resource)
		-> Ptr
	{
		return CopyConstruct(std::move(layout), source.GetData(), source.GetArrayCount(), resource);
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		assert(layout && layout->GetOperateProperty().move_construct && source != nullptr);
		auto cpp_layout = MemLayout::MemLayoutCPP::Get<StructLayoutObject>();
		
		auto m_layout = layout->GetLayout(array_count);

		auto offset = cpp_layout.Insert(m_layout);
		auto o_layout = cpp_layout.Get();
		auto re = MemoryResourceRecord::Allocate(resource, o_layout);
		if (re)
		{
			try
			{
				auto start = static_cast<std::byte*>(re.Get()) + offset;
				auto re2 = layout->MoveConstruction(start, source);
				assert(re2);
				return new (re.Get()) StructLayoutObject{ start, array_count, std::move(layout), re };
			}
			catch (...)
			{
				re.Deallocate();
				throw;
			}
		}
		return {};
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr layout, StructLayoutObject& source, std::pmr::memory_resource* resource)
		-> Ptr
	{
		return MoveConstruct(std::move(layout), source.GetData(), source.GetArrayCount());
	}


	void DynamicStructLayout::Release()
	{
		auto re = record;
		auto span = member_view;
		this->~DynamicStructLayout();
		for(auto& ite : span)
		{
			ite.~MemberView();
		}
		re.Deallocate();
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

	bool StructLayout::DefaultConstruction(void* target, std::size_t array_count) const
	{
		assert(target != nullptr);
		auto operate_property = GetOperateProperty();
		auto member_view = GetMemberView();
		if(operate_property.default_construct)
		{
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + layout.size * i;
				for(auto& ite : member_view)
				{
					auto data = GetData(ite, tar_ite);
					if(ite.init_object != nullptr)
					{
						auto re = ite.struct_layout->CopyConstruction(data, ite.init_object, ite.array_count);
						assert(re);
					}else
					{
						auto re = ite.struct_layout->DefaultConstruction(data, ite.array_count);
						assert(re);
					}
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::CopyConstruction(void* target, void* source, std::size_t array_count) const
	{
		assert(target != nullptr);
		auto operate_property = GetOperateProperty();
		if(operate_property.copy_construct)
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
					auto data = GetData(ite, tar_ite);
					auto data2 = GetData(ite, sou_ite);
					auto re = ite.struct_layout->CopyConstruction(data, data2, ite.array_count);
				}
			}
			return true;
		}
		return false;
	}

	bool StructLayout::MoveConstruction(void* target, void* source, std::size_t array_count) const
	{
		assert(target != source && target != nullptr && source != nullptr);
		auto operate_property = GetOperateProperty();
		if(operate_property.move_construct)
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
					auto data = GetData(ite, tar_ite);
					auto data2 = GetData(ite, sou_ite);
					auto re = ite.struct_layout->MoveConstruction(data, data2, ite.array_count);
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
				auto data = GetData(ite, tar_ite);
				auto re = ite.struct_layout->Destruction(data, ite.array_count);
				assert(re);
			}
		}
		return true;
	}

	bool StructLayout::CopyAssigned(void* target, void* source, std::size_t array_count) const
	{
		assert(target != nullptr && source != nullptr);
		auto member_view = GetMemberView();
		auto operate_property = GetOperateProperty();
		auto layout = GetLayout();
		std::byte* tar = static_cast<std::byte*>(target);
		std::byte* sou = static_cast<std::byte*>(source);
		if(operate_property.copy_assigned)
		{
			for (std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + layout.size * i;
				std::byte* sou_ite = sou + layout.size * i;
				for (auto& ite : member_view)
				{
					auto data = GetData(ite, tar_ite);
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
		if (operate_property.move_assigned)
		{
			for (std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + layout.size * i;
				std::byte* sou_ite = sou + layout.size * i;
				for (auto& ite : member_view)
				{
					auto data = GetData(ite, tar_ite);
					auto re = ite.struct_layout->MoveAssigned(data, sou_ite, ite.array_count);
					assert(re);
				}
			}
			return true;
		}
		return false;
	}


	auto DynamicStructLayout::Create(std::u8string_view name, std::span<Member const> members, std::pmr::memory_resource* resource)
		-> Ptr
	{
		std::size_t hash_code = 0;
		OperateProperty ope_property;

		std::size_t name_size = name.size();
		std::size_t index = 1;

		for(auto& ite : members)
		{
			auto cur_hash_code = ite.type_id->GetHashCode();
			hash_code += (cur_hash_code - (cur_hash_code % index)) * ite.array_count + (cur_hash_code % index);
			name_size += ite.name.size();
			auto tem_layout = ite.type_id->GetOperateProperty();
			if(!tem_layout.default_construct && ite.init_object != nullptr && tem_layout.copy_construct)
			{
				tem_layout.default_construct = true;
			}
			ope_property = ope_property && tem_layout;
			++index;
		}

		auto cur_layout_cpp = MemLayout::MemLayoutCPP::Get<DynamicStructLayout>();
		std::size_t member_offset = cur_layout_cpp.Insert(Layout::GetArray<MemberView>(members.size()));
		std::size_t name_offset = cur_layout_cpp.Insert(Layout::GetArray<char8_t>(name_size));

		auto record_layout = cur_layout_cpp;

		for(auto& ite : members)
		{
			if(ite.init_object != nullptr)
				cur_layout_cpp.Insert(ite.type_id->GetLayout(ite.array_count));
		}

		auto cur_layout = cur_layout_cpp.Get();
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if(re)
		{
			MemLayout::MemLayoutCPP total_layout_cpp;
			auto member_span = std::span(reinterpret_cast<MemberView*>(re.GetByte() + member_offset), members.size());
			auto str_span = std::span(reinterpret_cast<char8_t*>(re.GetByte() + name_offset), name_size);
			std::memcpy(str_span.data(), name.data(), name.size() * sizeof(char8_t));
			name = std::u8string_view{str_span.subspan(0, name.size())};
			str_span = str_span.subspan(name.size());
			for(std::size_t i = 0; i < members.size(); ++i)
			{
				auto& cur = members[i];
				auto& tar = member_span[i];
				
				std::size_t offset = 0;
				if(i == 0)
				{
					total_layout_cpp = cur.type_id->GetLayout(cur.array_count);
				}else
				{
					auto new_layout = cur.type_id->GetLayout(cur.array_count);
					offset = total_layout_cpp.Insert(new_layout);
				}

				std::memcpy(str_span.data(), cur.name.data(), cur.name.size() * sizeof(char8_t));

				void* init_object = nullptr;

				if(cur.init_object != nullptr)
				{
					auto init_offset = record_layout.Insert(cur.type_id->GetLayout(cur.array_count));
					init_object = static_cast<std::byte*>(re.Get()) + init_offset;
					auto re2 = cur.type_id->CopyConstruction(init_object, cur.init_object, cur.array_count);
					assert(re2);
				}

				new (&tar) MemberView{
					cur.type_id,
					std::u8string_view{str_span.data(), cur.name.size()},
					cur.array_count,
					offset,
					init_object
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

	void MemoryResourceRecordIntrusiveInterface::SubRef() const
	{
		if(ref_count.SubRef())
		{
			auto re = record;
			this->~MemoryResourceRecordIntrusiveInterface();
			re.Deallocate();
		}
	}
}
