#include "../Public/Grammar.h"

#include <filesystem>
#include <set>

namespace Potato::Grammar
{

	SymbolMask Symbol::FindActiveSymbolAtLast(std::u32string_view name) const noexcept
	{
		for (auto ite = active_scope.rbegin(); ite != active_scope.rend(); ++ite)
		{
			if(ite->name == name)
				return ite->index;
		}
		return {};
	}

	std::vector<SymbolMask> Symbol::FindActiveAllSymbol(std::u32string_view name) const
	{
		std::vector<SymbolMask> result;
		for (auto ite = active_scope.rbegin(); ite != active_scope.rend(); ++ite)
		{
			if (ite->name == name)
				result.push_back(ite->index);
		}
		return std::move(result);
	}

	Symbol::Property const* Symbol::FindSymbol(SymbolMask mask) const
	{
		if (mask && (mask.AsIndex() < mapping.size()))
		{
			auto& mapp = mapping[mask];
			if (mapp.is_active)
				return &active_scope[mapp.index];
			else
				return &unactive_scope[mapp.index];
		}
		return nullptr;
	}

	std::span<Symbol::Property const> Symbol::FindArea(SymbolAreaMask mask) const noexcept
	{
		if(mask && (mask.AsIndex() < areas.size()))
		{
			auto [start, count] = areas[mask];
			return {unactive_scope.data() + start, count};
		}else
			return {};
	}

	SymbolAreaMask Symbol::PopElementAsUnactive(size_t count)
	{
		count = std::min(count, active_scope.size());
		assert(count <= active_scope.size());
		SymbolAreaMask current_area{area_index++};
		for(auto i = active_scope.end() - count; i != active_scope.end(); ++i)
			i->area = current_area;
		auto unactive_scope_size = unactive_scope.size();
		auto offset = active_scope.size() - count;
		auto offset_ite = active_scope.begin() + offset;
		unactive_scope.insert(unactive_scope.end(), 
			std::move_iterator(offset_ite),
			std::move_iterator(active_scope.end())
		);
		active_scope.erase(offset_ite, active_scope.end());
		for (size_t i = 0; i < count; ++i)
		{
			auto& mapp = *(mapping.rbegin() + i);
			mapp.is_active = false;
			mapp.index = unactive_scope_size + count - i - 1;
		}
		return current_area;
	}
	
	SymbolMask Symbol::Insert(std::u32string_view name,  std::any property, Section section)
	{
		SymbolMask mask{mapping.size()};
		active_scope.push_back({name, mask, {}, section, std::move(property)});
		mapping.push_back(Mapping{true, active_scope.size() - 1});
		return mask;
	}
	
	ValueMask Value::InsertValue(SymbolMask mask, std::byte const* data, size_t length)
	{
		if(mask)
		{
			ValueMask vmask{value_mapping.size()};
			size_t start = value_buffer.size();
			value_buffer.insert(value_buffer.end(), data, data + length);
			value_mapping.push_back({mask, Style::Real, start, length});
			return vmask;
		}
		return {};
	}

	ValueMask Value::ReservedLazyValue()
	{
		ValueMask res(value_mapping.size());
		value_mapping.push_back({{}, Style::Lazy, 0, 0});
		return res;
	}

	bool Value::InsertLazyValue(SymbolMask index, std::byte const* data, size_t length)
	{
		if(index && index.AsIndex() < value_mapping.size() && value_mapping[index].style == Style::Lazy)
		{
			auto& ref = value_mapping[index];
			if(ref.style == Style::Lazy)
			{
				value_buffer.insert(value_buffer.end(), data, data + length);
				ref.style = Style::LazyInited;
				return true;
			}
		}
		return false;
	}

	size_t MemoryModelMaker::operator()(MemoryModel const& info_i)
	{
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
}