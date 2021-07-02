#include "../Public/Lexical.h"
#include <map>

namespace Potato
{
	SectionPoint CalculateSectionPoint(std::u32string_view str)
	{
		SectionPoint Result;
		for (auto Ite : str)
		{
			++Result.total_index;
			if (Ite == U'\n')
			{
				++Result.line;
				Result.line_index = 0;
			}
			else
				++Result.line_index;
		}
		return Result;
	}
}

namespace Potato
{

	void MakeUpUnfaTable(UnfaTable& table)
	{
		for(auto& ite : table.nodes)
		{
			for(auto& ite2 : ite)
			{
				if(ite2.Is<UnfaTable::ECapture>())
					ite2.property = UnfaTable::EEpsilon{};
			}
		}
	}

	void LexicalTable::LexicalFilter(UnfaTable const& table, std::vector<UnfaTable::Edge>& edges)
	{
		using Edge = UnfaTable::Edge;
		std::optional<uint32_t> acception_jume_state;
		edges.erase(std::remove_if(edges.begin(), edges.end(), [&](Edge edge)
		{
			if (edge.Is<UnfaTable::EAcception>())
			{
				if (!acception_jume_state)
				{
					acception_jume_state = edge.jump_state;
					return false;
				}
				else {
					acception_jume_state = std::max(*acception_jume_state, edge.Get<UnfaTable::EAcception>().acception_index);
					return true;
				}
			}
			else if (acception_jume_state)
			{
				if (edge.jump_state < *acception_jume_state)
					return true;
			}
			return false;
		}), edges.end());
	}
	
	LexicalTable LexicalTable::CreateFromRegexs(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller)
	{
		std::vector<UnfaTable> unfas;
		unfas.reserve(length);
		for(uint32_t i = 0; i < length; ++i)
			unfas.push_back(CreateUnfaTableFromRegex(adress[i].regex, i, adress[i].mask));
		auto result = LinkUnfaTable(unfas.data(), unfas.size());
		if(ignore_controller)
			MakeUpUnfaTable(result);
		return { result.Simplify(LexicalFilter) };
	}

	LexicalTable LexicalTable::CreateFromRegexsReverse(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller)
	{
		std::vector<UnfaTable> unfas;
		unfas.reserve(length);
		for (uint32_t i = static_cast<uint32_t>(length); i> 0; --i)
			unfas.push_back(CreateUnfaTableFromRegex(adress[i - 1].regex, i - 1, adress[i - 1].mask));
		auto result = LinkUnfaTable(unfas.data(), unfas.size());
		if(ignore_controller)
			MakeUpUnfaTable(result);
		return { result.Simplify(LexicalFilter) };
	}

	std::optional<LexicalMarch> LexicalTable::ProcessOnce(std::u32string_view code) const
	{
		assert(!Empty());
		auto re = table.Mark(code);
		if(re)
			return LexicalMarch{ re->acception_state, re->acception_mask, re->capture, {code.begin() + re->capture.size(), code.end()}, {{}, CalculateSectionPoint(re->capture)} };
		return std::nullopt;
	}

	std::vector<LexicalMarch> LexicalTable::Process(std::u32string_view code, size_t ignore_mask) const
	{
		std::vector<LexicalMarch> result;
		Section current_section;
		while(!code.empty())
		{
			auto re = ProcessOnce(code);
			if(re)
			{
				current_section.start = current_section.end;
				current_section.end = current_section.end + re->section.end;
				if(re->mask != ignore_mask)
				{
					re->section = current_section;
					result.push_back(*re);
				}
				code = re->last_string;
			}else
				throw Exception::MakeExceptionTuple(Exception::LexicalUnaccaptableItem{ std::u32string( code.begin(),  code.begin() + std::min(static_cast<size_t>(8), code.size())), current_section });
		}
		return result;
	}

	std::optional<LexicalMarch> LexicalTable::Match(std::u32string_view code, std::size_t ignore_mask) const
	{
		auto re = ProcessOnce(code);
		if(re && re->last_string.empty() && re->mask != ignore_mask)
			return re;
		return {};
	}
}