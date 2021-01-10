#pragma once
#include "Unfa.h"

namespace Potato::Lexical
{

	inline constexpr uint32_t DefaultMask() { return std::numeric_limits<uint32_t>::max() - 1; }
	inline constexpr uint32_t DefaultIgnoreMask() { return std::numeric_limits<uint32_t>::max(); }
	
	struct SectionPoint
	{
		size_t total_index = 0;
		size_t line = 0;
		size_t line_index = 0;
		bool operator<(SectionPoint const& lp) const { return total_index < lp.total_index; }
		SectionPoint operator+(SectionPoint const& sp) const
		{
			return { total_index + sp.total_index, line + sp.line, (sp.line != 0 ? sp.line_index : line_index + sp.line_index)};
		}
		SectionPoint& operator+=(SectionPoint const& sp)
		{
			*this = *this + sp;
			return *this;
		}
		
	};

	SectionPoint CalculateSectionPoint(std::u32string_view view);

	using Section = ::Potato::Misc::Segment<SectionPoint>;

	namespace Error
	{
		struct UnaccaptableLexicalItem {
			std::u32string possible_token;
			Section section;
		};
	}

	struct LexicalRegexInitTuple
	{
		LexicalRegexInitTuple(std::u32string_view input_regex = {}) : regex(input_regex), mask(DefaultMask()) {}
		LexicalRegexInitTuple(std::u32string_view input_regex, uint32_t input_mask) : regex(input_regex), mask(input_mask){}
		std::u32string_view regex;
		uint32_t mask;
	};
	
	struct March
	{
		uint32_t acception;
		uint32_t mask;
		std::u32string_view capture;
		std::u32string_view last_string;
		Section section;
	};

	struct Table
	{
		Unfa::SerilizedTable table;
		std::optional<March> ProcessOnce(std::u32string_view code) const;
		std::vector<March> Process(std::u32string_view code, size_t ignore_mask = DefaultIgnoreMask()) const;
		bool Empty() const noexcept{return table.Empty();}
		static Table CreateFromRegexs(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller = true);
		static Table CreateFromRegexsReverse(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller = true);
		static void LexicalFilter(Unfa::Table const& table, std::vector<Unfa::Table::Edge>& list);
	};

	inline Table CreateLexicalFromRegexs(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller = true){ return Table::CreateFromRegexs(adress, length, ignore_controller); }
	inline Table CreateLexicalFromRegexsReverse(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller = true) { return Table::CreateFromRegexsReverse(adress, length, ignore_controller); }
}
