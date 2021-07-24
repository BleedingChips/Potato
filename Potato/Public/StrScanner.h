#pragma once
#include <vector>
#include <variant>
#include <string>
#include <cassert>
#include <optional>
#include "Unfa.h"
namespace Potato
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
	void DirectScan(std::u32string_view code, TargetType& tar_type)
	{
		Scanner<std::remove_const_t<TargetType>>{}.Scan(code, tar_type);
	}

	struct ScanPattern
	{
		UnfaSerilizedTable table;
		template<typename ...TargetType>
		std::optional<size_t> operator()(std::u32string_view code, TargetType&... all_target) const;
	private:
		template<typename CurTarget, typename ...TargetType>
		size_t ProcessImplement(UnfaMarch::Sub const* marching, size_t last_marching, CurTarget& target, TargetType&... other) const
		{
			if(last_marching > 0)
			{
				DirectScan(marching[0].capture, target);
				return this->ProcessImplement(marching + 1, last_marching - 1, other...);
			}
			return 0;
		}
		size_t ProcessImplement(UnfaMarch::Sub const* marching, size_t last_marching) const{ return last_marching;  }
	};

	template<typename ...TargetType>
	std::optional<size_t> ScanPattern::operator()(std::u32string_view code, TargetType&... all_target) const
	{
		auto result = table.Mark(code);
		if(result)
		{
			auto last = this->ProcessImplement(result->sub_capture.data(), result->sub_capture.size(), all_target...);
			return result->sub_capture.size() - last;
		}
		return {};
	}

	inline ScanPattern CreateScanPattern(std::u32string_view pattern){ return { CreateUnfaTableFromRegex(pattern).Simplify().Serilized() }; }

	template<typename ...TargetType>
	std::optional<size_t> Scan(ScanPattern const& pattern, std::u32string_view in, TargetType& ... tar_type){ return pattern(in, tar_type...); }

	template<typename ...TargetType>
	std::optional<size_t> Scan(std::u32string_view pattern, std::u32string_view in, TargetType& ... tar_type)
	{
		ScanPattern cur_pattern{CreateUnfaTableFromRegex(pattern).Serilized()};
		return cur_pattern(in, tar_type...);
	}
	
}

namespace Potato
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
	
}