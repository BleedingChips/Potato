#pragma once
#include <vector>
#include <variant>
#include <string>
#include <cassert>
#include "Unfa.h"
namespace Potato::StrScanner
{
	template<typename TargetType>
	struct Scanner
	{
		void Scan(std::u32string_view par, TargetType& Input)
		{
			static_assert(false, "Undefine Scanner");
		}
	};

	template<typename TargetType>
	void DirectProcess(std::u32string_view code, TargetType& tar_type)
	{
		Scanner<std::remove_const_t<TargetType>>{}.Scan(code, tar_type);
	}

	struct Pattern
	{
		UnfaSerilizedTable table;
		template<typename ...TargetType>
		size_t Process(std::u32string_view code, TargetType&... all_target);
	private:
		template<typename CurTarget, typename ...TargetType>
		void ProcessImplement(UnfaMarch::Sub const* marching, size_t& last_marching, CurTarget& target, TargetType&... other)
		{
			if(last_marching > 0)
			{
				DirectProcess(marching[0].capture, target);
				--last_marching;
				marching += 1;
				ProcessImplement(marching, last_marching, other...);
			}
		}
		void ProcessImplement(UnfaMarch::Sub const* marching, size_t& last_marching){}
	};

	template<typename ...TargetType>
	size_t Pattern::Process(std::u32string_view code, TargetType&... all_target)
	{
		auto result = table.Mark(code);
		if(result)
		{
			size_t used = result->sub_capture.size();
			ProcessImplement(result->sub_capture.data(), used, all_target...);
			return result->sub_capture.size() - used;
		}
		return 0;
	}

	inline Pattern CreatePattern(std::u32string_view pattern){ return { CreateUnfaTableFromRegex(pattern).Simplify() }; }

	template<typename ...TargetType>
	void Process(Pattern const& pattern, std::u32string_view in, TargetType& ... tar_type){ pattern(in, tar_type...); }

	template<typename ...TargetType>
	void Process(std::u32string_view pattern, std::u32string_view in, TargetType& ... tar_type)
	{
		Pattern cur_pattern{CreateUnfaTableFromRegex(pattern)};
		cur_pattern.Process(in, tar_type...);
	}
	
}

namespace Potato::StrScanner
{

	template<>
	struct Scanner<std::u32string>
	{
		void Scan(std::u32string_view par, std::u32string& Input);
	};

	template<>
	struct Scanner<int32_t>
	{
		void Scan(std::u32string_view par, int32_t& Input);
	};

	template<>
	struct Scanner<uint32_t>
	{
		void Scan(std::u32string_view par, uint32_t& Input);
	};

	template<>
	struct Scanner<int64_t>
	{
		void Scan(std::u32string_view par, int64_t& Input);
	};

	template<>
	struct Scanner<uint64_t>
	{
		void Scan(std::u32string_view par, uint64_t& Input);
	};

	template<>
	struct Scanner<float>
	{
		void Scan(std::u32string_view par, float& Input);
	};

	template<>
	struct Scanner<double>
	{
		void Scan(std::u32string_view par, double& Input);
	};

	/*
	template<>
	struct Scanner<int32_t>
	{
		void Scan(std::u32string_view par, int32_t& Input);
	};
	*/
	
}