module;

#include <cassert>

module PotatoIR;
//import :Define;

namespace Potato::IR
{

	std::strong_ordering TypeID::operator<=>(TypeID const& i) const
	{
		if(ID == i.ID)
		{
			return std::strong_ordering::equal;
		}else if(ID < i.ID)
		{
			return std::strong_ordering::less;
		}else
			return std::strong_ordering::greater;
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
			view.type_id->GetLayout(view.array_count).Size
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

	Layout StructLayout::GetArrayLayout(std::size_t array_count) const
	{
		auto layout = GetLayout();
		layout.Size *= array_count;
		return layout;
	}

	Layout StructLayout::GetLayout(std::optional<std::size_t> array_count) const 
	{
		if(array_count)
		{
			return GetArrayLayout(*array_count);
		}
		return GetLayout();
	}

	StructLayoutObject::~StructLayoutObject()
	{
		assert(layout && start != nullptr);
		layout->Destruction(start);
	}

	void StructLayoutObject::Release()
	{
		auto re = record;
		this->~StructLayoutObject();
		re.Deallocate();
	}

	auto StructLayoutObject::DefaultConstruct(StructLayout::Ptr layout, std::pmr::memory_resource* resource)
		->Ptr
	{
		if(layout && layout->GetConstructProperty().enable_default)
		{
			auto o_layout = Layout::Get<StructLayoutObject>();
			auto m_layout = layout->GetLayout();
			auto offset = InsertLayoutCPP(o_layout, m_layout);
			FixLayoutCPP(o_layout);
			auto re = MemoryResourceRecord::Allocate(resource, o_layout);
			if(re)
			{
				try
				{
					auto start = static_cast<std::byte*>(re.Get()) + offset;
					auto re2 = layout->DefaultConstruction(start);
					assert(re2);
					return new (re.Get()) StructLayoutObject{start, std::move(layout), re};
				}catch (...)
				{
					re.Deallocate();
					throw;
				}
			}
		}
		return {};
	}

	auto StructLayoutObject::CopyConstruct(StructLayout::Ptr layout, void* source, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if(layout && layout->GetConstructProperty().enable_copy && source != nullptr)
		{
			auto o_layout = Layout::Get<StructLayoutObject>();
			auto m_layout = layout->GetLayout();

			auto offset = InsertLayoutCPP(o_layout, m_layout);
			FixLayoutCPP(o_layout);
			auto re = MemoryResourceRecord::Allocate(resource, o_layout);
			if(re)
			{
				try
				{
					auto start = static_cast<std::byte*>(re.Get()) + offset;
					auto re2= layout->CopyConstruction(start, source);
					assert(re2);
					return new (re.Get()) StructLayoutObject{start, std::move(layout), re};
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
			return CopyConstruct(std::move(layout), source->GetData());
		}
		return {};
	}

	auto StructLayoutObject::MoveConstruct(StructLayout::Ptr layout, void* source, std::pmr::memory_resource* resource)
		-> Ptr
	{
		if(layout && layout->GetConstructProperty().enable_move && source != nullptr)
		{
			auto o_layout = Layout::Get<StructLayoutObject>();
			auto m_layout = layout->GetLayout();

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
					return new (re.Get()) StructLayoutObject{start, std::move(layout), re};
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
			return MoveConstruct(std::move(layout), source->GetData());
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

	bool DynamicStructLayout::DefaultConstruction(void* target) const
	{
		if(target != nullptr && construct_property.enable_default)
		{
			for(auto& ite : member_view)
			{
				auto tar = GetData(ite, target);
				void* sou = nullptr;
				if(!ite.array_count)
				{
					auto re = ite.type_id->DefaultConstruction(tar);
					assert(re);
				}else
				{
					auto layout = ite.type_id->GetLayout();
					for(std::size_t i = 0; i < *ite.array_count; ++i)
					{
						auto ite_tar = static_cast<std::byte*>(tar) + i * layout.Size;
						auto re = ite.type_id->DefaultConstruction(ite_tar);
						assert(re);
					}
				}
			}
			return true;
		}
		return false;
	}

	bool DynamicStructLayout::CopyConstruction(void* target, void* source) const
	{
		if(construct_property.enable_copy && target != source && target != nullptr && source != nullptr)
		{
			for(auto& ite : member_view)
			{
				auto tar = GetData(ite, target);
				auto sou = GetData(ite, source);;
				if(!ite.array_count)
				{
					auto re = ite.type_id->CopyConstruction(tar, sou);
					assert(re);
				}else
				{
					auto layout = ite.type_id->GetLayout();
					for(std::size_t i = 0; i < *ite.array_count; ++i)
					{
						auto ite_tar = static_cast<std::byte*>(tar) + i * layout.Size;
						auto ite_sou = static_cast<std::byte*>(sou) + i * layout.Size;
						auto re = ite.type_id->CopyConstruction(ite_tar, ite_sou);
						assert(re);
					}
				}
			}
			return true;
		}
		return false;
	}

	bool DynamicStructLayout::MoveConstruction(void* target, void* source) const
	{
		if(construct_property.enable_move && target != source && target != nullptr && source != nullptr)
		{
			for(auto& ite : member_view)
			{
				auto tar = GetData(ite, target);
				auto sou = GetData(ite, source);;
				if(!ite.array_count)
				{
					auto re = ite.type_id->MoveConstruction(tar, sou);
					assert(re);
				}else
				{
					auto layout = ite.type_id->GetLayout();
					for(std::size_t i = 0; i < *ite.array_count; ++i)
					{
						auto ite_tar = static_cast<std::byte*>(tar) + i * layout.Size;
						auto ite_sou = static_cast<std::byte*>(sou) + i * layout.Size;
						auto re = ite.type_id->MoveConstruction(ite_tar, ite_sou);
						assert(re);
					}
				}
			}
			return true;
		}
		return false;
	}

	bool DynamicStructLayout::Destruction(void* target) const
	{
		if(target != nullptr)
		{
			for(auto& ite : member_view)
			{
				auto tar = GetData(ite, target);
				if(!ite.array_count)
				{
					auto re = ite.type_id->Destruction(tar);
					assert(re);
				}else
				{
					auto layout = ite.type_id->GetLayout();
					for(std::size_t i = 0; i < *ite.array_count; ++i)
					{
						auto ite_tar = static_cast<std::byte*>(tar) + i * layout.Size;
						auto re = ite.type_id->DefaultConstruction(ite_tar);
						assert(re);
					}
				}
			}
			return true;
		}
		return false;
	}


	auto StructLayout::CreateDynamicStructLayout(std::u8string_view name, std::span<Member const> members, std::pmr::memory_resource* resource)
		-> Ptr
	{

		StructLayoutConstruction construct_pro;

		std::size_t name_size = name.size();
		for(auto& ite : members)
		{
			name_size += ite.name.size();
			construct_pro =  construct_pro && ite.type_id->GetConstructProperty();
		}

		auto cur_layout = Layout::Get<DynamicStructLayout>();
		std::size_t member_offset = InsertLayoutCPP(cur_layout, Layout::GetArray<MemberView>(members.size()));
		std::size_t name_offset = InsertLayoutCPP(cur_layout, Layout::GetArray<char8_t>(name_size));
		FixLayoutCPP(cur_layout);
		auto re = MemoryResourceRecord::Allocate(resource, cur_layout);
		if(re)
		{
			Layout total_layout;
			auto member_span = std::span(reinterpret_cast<MemberView*>(re.GetByte() + member_offset), members.size());
			auto str_span = std::span(reinterpret_cast<char8_t*>(re.GetByte() + name_offset), name_size);
			std::memcpy(str_span.data(), name.data(), name.size());
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

				std::memcpy(str_span.data(), cur.name.data(), cur.name.size());
				new (&tar) MemberView{
					cur.type_id,
					std::u8string_view{str_span.data(), cur.name.size()},
					cur.array_count,
					offset
				};
				str_span = str_span.subspan(cur.name.size());
			}
			FixLayoutCPP(total_layout);
			return new (re.Get()) DynamicStructLayout{
				construct_pro,
				name,
				total_layout,
				member_span,
				re
			};
		}

		return {};
	}
}
