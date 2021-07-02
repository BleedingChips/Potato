#pragma once

#include "Unfa.h"

namespace Potato
{
	struct SectionPoint
	{
		std::size_t total_index = 0;
		std::size_t line = 0;
		std::size_t line_index = 0;
		bool operator<(SectionPoint const& lp) const { return total_index < lp.total_index; }
		SectionPoint operator+(SectionPoint const& sp) const
		{
			return { total_index + sp.total_index, line + sp.line, (sp.line != 0 ? sp.line_index : line_index + sp.line_index) };
		}
		SectionPoint& operator+=(SectionPoint const& sp)
		{
			*this = *this + sp;
			return *this;
		}
		auto operator<=>(SectionPoint const& lp) const { return total_index <=> lp.total_index; }
	};

	SectionPoint CalculateSectionPoint(std::u32string_view view);

	using Section = Interval<SectionPoint>;
}


namespace Potato
{

	inline constexpr uint32_t LexicalDefaultMask() { return std::numeric_limits<uint32_t>::max() - 1; }
	inline constexpr uint32_t LexicalDefaultIgnoreMask() { return std::numeric_limits<uint32_t>::max(); }
	
	struct LexicalRegexInitTuple
	{
		LexicalRegexInitTuple(std::u32string_view input_regex = {}) : regex(input_regex), mask(LexicalDefaultMask()) {}
		LexicalRegexInitTuple(std::u32string_view input_regex, uint32_t input_mask) : regex(input_regex), mask(input_mask){}
		std::u32string_view regex;
		uint32_t mask;
	};
	
	struct LexicalMarch
	{
		uint32_t acception;
		uint32_t mask;
		std::u32string_view capture;
		std::u32string_view last_string;
		Section section;
	};

	struct LexicalTable
	{
		UnfaSerilizedTable table;
		std::optional<LexicalMarch> ProcessOnce(std::u32string_view code) const;
		std::vector<LexicalMarch> Process(std::u32string_view code, std::size_t ignore_mask = LexicalDefaultIgnoreMask()) const;
		std::optional<LexicalMarch> Match(std::u32string_view code, std::size_t ignore_mask = LexicalDefaultIgnoreMask()) const;
		bool Empty() const noexcept{return table.Empty();}
		static LexicalTable CreateFromRegexs(LexicalRegexInitTuple const* adress, std::size_t length, bool ignore_controller = true);
		static LexicalTable CreateFromRegexsReverse(LexicalRegexInitTuple const* adress, std::size_t length, bool ignore_controller = true);
		static void LexicalFilter(UnfaTable const& table, std::vector<UnfaTable::Edge>& list);
	};

	inline LexicalTable CreateLexicalFromRegexs(LexicalRegexInitTuple const* adress, std::size_t length, bool ignore_controller = true){ return LexicalTable::CreateFromRegexs(adress, length, ignore_controller); }
	inline LexicalTable CreateLexicalFromRegexsReverse(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller = true) { return LexicalTable::CreateFromRegexsReverse(adress, length, ignore_controller); }
}

namespace Potato::Exception
{

	struct LexicalUnaccaptableItem {
		std::u32string possible_token;
		Section section;
	};
}