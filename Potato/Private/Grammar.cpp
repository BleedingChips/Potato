#include "../Public/Grammar.h"

#include <filesystem>
#include <set>

namespace Potato::Grammar
{
	SymbolMask Table::InsertSymbol(std::u32string_view name, std::any property, Section section)
	{
		SymbolMask mask{ scope.size(), name };
		scope.push_back({ mask, {}, section, std::move(property) });
		active_scope.push_back(mask);
		return mask;
	}

	ValueMask Table::InsertValue(SymbolMask type, std::u32string_view type_name, std::span<std::byte const> datas)
	{
		ValueMask vmask{ data_mapping.size(), type, type_name, datas.size() };
		size_t start = data_buffer.size();
		data_buffer.insert(data_buffer.end(), datas.begin(), datas.end());
		data_mapping.push_back({ type, type_name, {start, datas.size()} });
		return vmask;
	}

	AreaMask Table::PopSymbolAsUnactive(size_t count)
	{
		count = std::min(count, active_scope.size());
		AreaMask current_area{ areas.size(), count };
		IndexSpan<> cur_areas_sub{0, 0};
		IndexSpan<> cur_areas{ areas.size(), 0};

		for (auto i = active_scope.end() - count; i != active_scope.end(); ++i)
		{
			auto& pro = scope[i->Index()];
			pro.area = current_area;
			if (cur_areas_sub.length == 0)
			{
				cur_areas_sub = { pro.mask.Index(), 1};
			}
			else {
				if(cur_areas_sub.start + count == pro.mask.Index())
					++cur_areas_sub.length;
				else {
					areas_sub.push_back(cur_areas_sub);
					++cur_areas.length;
					cur_areas_sub = {pro.mask.Index(), 1};
				}
			}
		}
		if (cur_areas_sub.length != 0)
		{
			areas_sub.push_back(cur_areas_sub);
			++cur_areas.length;
		}
		areas.push_back(cur_areas);
		active_scope.erase(active_scope.end() - count, active_scope.end());
		return current_area;
	}

	SymbolMask Table::FindActiveSymbolAtLast(std::u32string_view name) const noexcept
	{
		for (auto ite = active_scope.rbegin(); ite != active_scope.rend(); ++ite)
		{
			if(ite->Name() == name)
				return *ite;
		}
		return {};
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