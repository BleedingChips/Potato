#include "../Public/Symbol.h"
#include <set>

namespace Potato::Symbol
{

	auto Table::Insert(std::u32string_view name,  std::any property, Section section) -> Table::Mask
	{
		Mask mask{mapping.size()};
		active_scope.push_back({name, mask, section, std::move(property)});
		mapping.push_back(Mapping{true, active_scope.size() - 1});
		return mask;
	}

	auto Table::FindRaw(Mask mask) const ->Result<const std::any>
	{
		if (mask && (mask.AsIndex() < mapping.size()))
		{
			Storage const* str = nullptr;
			auto& mapp = mapping[mask];
			if (mapp.is_active)
				str = &active_scope[mapp.index];
			else
				str = &unactive_scope[mapp.index];
			assert(str != nullptr);
			return { ResultType::Available, str->name, str->section, &str->property };
		}
		return { ResultType::Inavailable, {}, {}, {} };
	}

	auto Table::FindActiveLast(std::u32string_view name) const noexcept -> Table::Mask
	{
		for (auto ite = active_scope.rbegin(); ite != active_scope.rend(); ++ite)
		{
			if(ite->name == name)
				return ite->index;
		}
		return {};
	}

	auto Table::FindActiveAll(std::u32string_view name) const -> std::vector<Table::Mask>
	{
		std::vector<Mask> result;
		for (auto ite = active_scope.rbegin(); ite != active_scope.rend(); ++ite)
		{
			if (ite->name == name)
				result.push_back(ite->index);
		}
		return std::move(result);
	}
	
	size_t Table::PopElementAsUnactive(size_t count)
	{
		assert(active_scope.size() >= count);
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
		return active_scope.size();
	}

	auto Table::PopAndReturnElementAsUnactive(size_t count) -> std::vector<Mask>
	{
		assert(active_scope.size() >= count);
		auto unactive_scope_size = unactive_scope.size();
		auto offset = active_scope.size() - count;
		auto offset_ite = active_scope.begin() + offset;
		unactive_scope.insert(unactive_scope.end(),
			std::move_iterator(offset_ite),
			std::move_iterator(active_scope.end())
		);
		active_scope.erase(offset_ite, active_scope.end());
		std::vector<Mask> result;
		for (size_t i = 0; i < count; ++i)
		{
			auto& mapp = *(mapping.rbegin() + i);
			mapp.is_active = false;
			mapp.index = unactive_scope_size + count - i - 1;
			result.push_back(Mask{mapping.size() - i - 1});
		}
		return std::move(result);
	}
	
	auto ConstDataTable::Insert(Table::Mask mask, std::byte const* data, size_t length) -> Mask
	{
		if(mask)
		{
			size_t start = datas_mapping.size();
			Mask result(start);
			const_data_buffer.insert(const_data_buffer.end(), data, data + length);
			datas_mapping.push_back({mask, start, length});
			return result;
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