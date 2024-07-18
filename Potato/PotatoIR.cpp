module;

#include <cassert>

module PotatoIR;
//import :Define;

namespace Potato::IR
{
	std::strong_ordering StructLayout::operator<=>(StructLayout const& layout) const
	{
		auto re = (this <=> &layout);
		if(re == std::strong_ordering::equal)
			return re;
		re = GetHashCode() <=> layout.GetHashCode();
		if(re != std::strong_ordering::equal)
			return re;
		auto mem = GetMemberView();
		auto mem2 = layout.GetMemberView();
		re = (mem.size() <=> mem2.size());
		if(re != std::strong_ordering::equal)
			return re;
		for(std::size_t i = 0; i < mem.size();++i)
		{
			auto& ref = mem[i];
			auto& ref2 = mem2[i];
			re = (ref.offset <=> ref2.offset);
			if(re != std::strong_ordering::equal)
				return re;
			re = (ref.array_count <=> ref2.array_count);
			if(re != std::strong_ordering::equal)
				return re;
			re = (*ref.struct_layout) <=> (*ref2.struct_layout);
			if(re != std::strong_ordering::equal)
				return re;
		}
		return std::strong_ordering::equal;
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
			auto adress = resource->allocate(layout.Size, layout.Align);
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
			resource->deallocate(adress, layout.Size, layout.Align);
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
			view.struct_layout->GetLayout(view.array_count).Size
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

	Layout StructLayout::GetLayout(std::size_t array_count) const 
	{
		auto layout = GetLayout();
		return {
			layout.Align,
			layout.Size * array_count
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
		if(layout && layout->GetConstructProperty().enable_default)
		{
			auto o_layout = Layout::Get<StructLayoutObject>();
			auto m_layout = layout->GetLayout(array_count);
			auto offset = InsertLayoutCPP(o_layout, m_layout);
			FixLayoutCPP(o_layout);
			auto re = MemoryResourceRecord::Allocate(resource, o_layout);
			if(re)
			{
				try
				{
					auto start = static_cast<std::byte*>(re.Get()) + offset;
					auto re2 = layout->DefaultConstruction(start, array_count);
					assert(re2);
					return new (re.Get()) StructLayoutObject{start, array_count, std::move(layout), re};
				}catch (...)
				{
					re.Deallocate();
					throw;
				}
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if(layout && layout->GetConstructProperty().enable_copy && source != nullptr)
		{
			auto o_layout = Layout::Get<StructLayoutObject>();
			auto m_layout = layout->GetLayout(array_count);

			auto offset = InsertLayoutCPP(o_layout, m_layout);
			FixLayoutCPP(o_layout);
			auto re = MemoryResourceRecord::Allocate(resource, o_layout);
			if(re)
			{
				try
				{
					auto start = static_cast<std::byte*>(re.Get()) + offset;
					auto re2= layout->CopyConstruction(start, source, array_count);
					assert(re2);
					return new (re.Get()) StructLayoutObject{start, array_count, std::move(layout), re};
				}catch (...)
				{
					re.Deallocate();
					throw;
				}
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr layout, StructLayoutObject::Ptr source, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if(source && source->layout == layout)
		{
			return CopyConstruct(std::move(layout), source->GetData(), source->GetArrayCount());
		}
		return {};
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr layout, void* source, std::size_t array_count, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if(layout && layout->GetConstructProperty().enable_move && source != nullptr)
		{
			auto o_layout = Layout::Get<StructLayoutObject>();
			auto m_layout = layout->GetLayout(array_count);

			auto offset = InsertLayoutCPP(o_layout, m_layout);
			FixLayoutCPP(o_layout);
			auto re = MemoryResourceRecord::Allocate(resource, o_layout);
			if(re)
			{
				try
				{
					auto start = static_cast<std::byte*>(re.Get()) + offset;
					auto re2= layout->MoveConstruction(start, source);
					assert(re2);
					return new (re.Get()) StructLayoutObject{start, array_count, std::move(layout), re};
				}catch (...)
				{
					re.Deallocate();
					throw;
				}
			}
		}
		return {};
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr layout, StructLayoutObject::Ptr source, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if(source && source->layout == layout)
		{
			return MoveConstruct(std::move(layout), source->GetData(), source->GetArrayCount());
		}
		return {};
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
		auto construct_property = GetConstructProperty();
		auto member_view = GetMemberView();
		if(construct_property.enable_default)
		{
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + layout.Size * i;
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
		auto construct_property = GetConstructProperty();
		if(construct_property.enable_copy)
		{
			auto member_view = GetMemberView();
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			std::byte* sou = static_cast<std::byte*>(source);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + layout.Size * i;
				std::byte* sou_ite = sou + layout.Size * i;
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
		auto construct_property = GetConstructProperty();
		if(construct_property.enable_move)
		{
			auto member_view = GetMemberView();
			auto layout = GetLayout();
			std::byte* tar = static_cast<std::byte*>(target);
			std::byte* sou = static_cast<std::byte*>(source);
			for(std::size_t i = 0; i < array_count; ++i)
			{
				std::byte* tar_ite = tar + layout.Size * i;
				std::byte* sou_ite = sou + layout.Size * i;
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
			std::byte* tar_ite = tar + layout.Size * i;
			for(auto& ite : member_view)
			{
				auto data = GetData(ite, tar_ite);
				auto re = ite.struct_layout->Destruction(data, ite.array_count);
				assert(re);
			}
		}
		return true;
	}


	auto DynamicStructLayout::Create(std::u8string_view name, std::span<Member const> members, std::pmr::memory_resource* resource)
		-> Ptr
	{
		std::size_t hash_code = 0;
		StructLayoutConstruction construct_pro;

		std::size_t name_size = name.size();
		std::size_t index = 1;
		for(auto& ite : members)
		{
			auto cur_hash_code = ite.type_id->GetHashCode();
			hash_code += (cur_hash_code - (cur_hash_code % index)) * ite.array_count + (cur_hash_code % index);
			name_size += ite.name.size();
			auto tem_layout = ite.type_id->GetConstructProperty();
			if(!tem_layout.enable_default && ite.init_object != nullptr && tem_layout.enable_copy)
			{
				tem_layout.enable_default = true;
			}
			construct_pro =  construct_pro && tem_layout;
			++index;
		}

		if(!construct_pro)
			return {};

		auto cur_layout = Layout::Get<DynamicStructLayout>();
		std::size_t member_offset = InsertLayoutCPP(cur_layout, Layout::GetArray<MemberView>(members.size()));
		std::size_t name_offset = InsertLayoutCPP(cur_layout, Layout::GetArray<char8_t>(name_size));

		auto record_layout = cur_layout;

		for(auto& ite : members)
		{
			if(ite.init_object != nullptr)
				InsertLayoutCPP(cur_layout, ite.type_id->GetLayout(ite.array_count));
		}

		FixLayoutCPP(cur_layout);
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if(re)
		{
			Layout total_layout;
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
					total_layout = cur.type_id->GetLayout(cur.array_count);
				}else
				{
					auto new_layout = cur.type_id->GetLayout(cur.array_count);
					offset = InsertLayoutCPP(total_layout, new_layout);
				}

				std::memcpy(str_span.data(), cur.name.data(), cur.name.size() * sizeof(char8_t));

				void* init_object = nullptr;

				if(cur.init_object != nullptr)
				{
					auto init_offset = InsertLayoutCPP(record_layout, cur.type_id->GetLayout(cur.array_count));
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
			FixLayoutCPP(total_layout);
			return new (re.Get()) DynamicStructLayout{
				construct_pro,
				name,
				total_layout,
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
