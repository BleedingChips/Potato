#pragma once
#include <vector>
#include <cassert>
#include <optional>
#include <tuple>
#include <algorithm>
namespace Potato::Misc
{

	struct NoDetectT{};

	template<typename PointT, typename Less = std::less<PointT>>
	struct Segment
	{

		Segment(PointT p1, PointT p2) : start(std::move(p1)), end(std::move(p2)){
			if(!(Less{}(start, end)))
				std::swap(start, end);
		}
		Segment(PointT p1) : Segment(p1, p1 + 1, NoDetectT{}){}
		Segment(PointT p1, PointT p2, NoDetectT) : start(std::move(p1)), end(std::move(p2)) {}
		Segment() = default;
		Segment(Segment const&) = default;
		Segment(Segment &&) = default;
		Segment& operator=(Segment const&) = default;
		Segment& operator=(Segment &&) = default;

		struct Result
		{
			Segment and_small;
			Segment and_middle;
			Segment and_big;
			Segment or_shared;
			bool small_from_left;
			bool big_from_left;
		};
		
		bool IsInclude(PointT const& si) const { return (Less{}(start, si) || start == si) && Less{}(si, end); }
		operator bool() const { return Less{}(start, end); }

		Result Collision(Segment const& v2) const noexcept;
		Segment operator&(Segment const& si) const noexcept;
		std::tuple<Segment, Segment> operator-(Segment const& si) const noexcept;

		Segment Expand(Segment const& ref) const noexcept;

	public:
		
		PointT start;
		PointT end;
		
	};

	
	template<typename PointT, typename Less>
	auto Segment<PointT, Less>::Collision(Segment const& v2) const noexcept -> Result
	{
		assert(*this && v2);
		if (Less{}(start, v2.start))
		{
			if (Less{}(end, v2.start))
				return { *this, {}, v2, {}, true, false };
			else if (end == v2.start)
				return { *this, {}, v2, {start, v2.end}, true, false };
			else
			{
				if (Less{}(end, v2.end) || end == v2.end)
					return {
						{start, v2.start},
						{v2.start, end},
						{end, v2.end},
						{start, v2.end},
						true, false
				};
				else
					return {
						{start, v2.start},
						{v2.start, v2.end},
						{v2.end, end},
						* this,
						true, true
				};
			}
		}
		else if (start == v2.start)
		{
			if (Less{}(end, v2.end))
				return { {}, *this, {end, v2.end}, v2, false, false };
			else if (end == v2.end)
				return { {}, *this, {}, *this, false, false };
			else
				return { {}, v2, {v2.end, end}, *this, false, true };
		}
		else
		{
			if (Less{}(v2.end, start))
				return { v2, {}, *this, {}, false, true };
			else if (v2.end == start)
				return { v2, {}, *this, {v2.start, end}, false, true };
			else
			{
				if (Less{}(v2.end, end))
					return { {v2.start, start}, {start, v2.end}, {v2.end, end}, {v2.start, end}, false, true };
				else
					return { {v2.start, start}, *this, {end, v2.end}, v2, false, false };
			}
		}
	}

	template<typename PointT, typename Less>
	auto Segment<PointT, Less>::operator&(Segment const& si) const noexcept -> Segment
	{
		if(*this && si)
			return Collision(si).and_middle;
		else
			return {};
	}

	template<typename PointT, typename Less>
	auto Segment<PointT, Less>::operator-(Segment const& si) const noexcept -> std::tuple<Segment, Segment>
	{
		if (*this && si)
		{
			auto result = Collision(si);
			std::tuple<Segment, Segment> re{{}, {}};
			if(result.small_from_left)
				std::get<0>(re) = result.and_small;
			if(result.big_from_left)
			{
				if(std::get<0>(re))
					std::get<1>(re) = result.and_big;
				else
					std::get<0>(re) = result.and_big;
			}
			return re;
		}
		else
			return *this;
	}

	template<typename PointT, typename Less>
	auto Segment<PointT, Less>::Expand(Segment const& ref) const noexcept -> Segment
	{
		if (*this && ref)
			return { Less{}(start, ref.start) ? start : ref.start,  Less{}(end, ref.end) ? ref.end : end };
		else if (*this)
			return *this;
		else
			return ref;
	}

	template<typename PointT, typename Less = std::less<PointT>, typename Allocator = std::allocator<Segment<PointT, Less>>>
	struct Interval;
	
	template<typename PointT, typename Less = std::less<PointT>>
	struct IntervalWrapper
	{
		using Segment = Segment<PointT, Less>;

		IntervalWrapper(Segment const* using_start = nullptr, size_t using_length = 0) : start(using_start), length(using_length) {}
		IntervalWrapper(Segment const& ref) : start(&ref), length(1) {} 
		
		bool Empty() const noexcept {return length == 0 || start == nullptr;}

		template<typename Allocator>
		struct Result
		{
			Interval<PointT, Less, Allocator> left;
			Interval<PointT, Less, Allocator> middle;
			Interval<PointT, Less, Allocator> right;
		};

		template<typename Allocator = std::allocator<Segment>>
		Result<Allocator> Collision(IntervalWrapper wrapper) const { return CollisionImplement<true, true, true, Allocator>(wrapper); }

		template<typename Allocator = std::allocator<Segment>>
		Interval<PointT, Less, Allocator> Intersection(IntervalWrapper wrapper) const { return CollisionImplement<false, true, false, Allocator>(wrapper).middle; }

		template<typename Allocator = std::allocator<Segment>>
		Interval<PointT, Less, Allocator> Remove(IntervalWrapper wrapper) const { return CollisionImplement<true, false, false, Allocator>(wrapper).left; }

		template<typename Allocator = std::allocator<Segment>>
		Interval<PointT, Less, Allocator> Union(IntervalWrapper wrapper) const;

		Segment const* begin() const noexcept {return start;}
		Segment const* end() const noexcept { return start + length; }
		size_t size() const noexcept{ return length; }
		bool IsInclude(PointT const&) const noexcept;
		
	private:
		
		template<bool write_left, bool write_middle, bool write_right, typename Allocator>
		Result<Allocator> CollisionImplement(IntervalWrapper wrapper) const;

		Segment const* start = nullptr;
		size_t length = 0;
		
	};

	template<typename PointT, typename Less>
	bool IntervalWrapper<PointT, Less>::IsInclude(PointT const& P) const noexcept
	{
		for(size_t i =0; i <length; ++i)
		{
			if(start[i].IsInclude(P))
				return true;
		}
		return false;
	}

	template<typename PointT, typename Less>
	template<bool write_left, bool write_middle, bool write_right, typename Allocator>
	auto IntervalWrapper<PointT, Less>::CollisionImplement(IntervalWrapper v2) const -> Result<Allocator>
	{
		if (!Empty() && !v2.Empty())
		{
			std::vector<Segment, Allocator> output_left;
			std::vector<Segment, Allocator> output_middle;
			std::vector<Segment, Allocator> output_right;
			auto i1s = begin(), i1e = end();
			auto i2s = v2.begin(), i2e = v2.end();
			std::optional<Segment> left = *(i1s++);
			std::optional<Segment> right = *(i2s++);
			while (left.has_value() && right.has_value())
			{
				typename Segment::Result sem_result = left->Collision(*right);
				if constexpr (write_middle)
				{
					if (sem_result.and_middle)
						output_middle.push_back(sem_result.and_middle);
				}
				if constexpr (write_left || write_right)
				{
					if (sem_result.and_small)
					{
						if (sem_result.small_from_left)
						{
							if constexpr (write_left)
								output_left.push_back(sem_result.and_small);
						}
						else
						{
							if constexpr (write_right)
								output_right.push_back(sem_result.and_small);
						}
					}
				}
				if (sem_result.and_big)
				{
					if (sem_result.big_from_left)
					{
						left = sem_result.and_big;
						right = std::nullopt;
					}else
					{
						right = sem_result.and_big;
						left = std::nullopt;
					}
				}else
				{
					left = std::nullopt;
					right = std::nullopt;
				}
				if(!left.has_value() && i1s != i1e)
					left = *(i1s++);
				if(!right.has_value() && i2s != i2e)
					right = *(i2s++);
			}
			if constexpr (write_left)
			{
				if(left)
					output_left.push_back(*left);
				output_left.insert(output_left.end(), i1s, i1e);
			}
			if constexpr (write_right)
			{
				if (right)
					output_right.push_back(*right);
				output_right.insert(output_right.end(), i2s, i2e);
			}
			return Result<Allocator>{ {std::move(output_left), NoDetectT{}}, { std::move(output_middle), NoDetectT{} }, { std::move(output_right), NoDetectT{} }};
		}else
			return {*this, {}, v2};
	}


	template<typename PointT, typename Less>
	template<typename Allocator>
	auto IntervalWrapper<PointT, Less>::Union(IntervalWrapper v2) const -> Interval<PointT, Less, Allocator>
	{
		
		if(!Empty() && !v2.Empty())
		{
			std::vector<Segment, Allocator> result;
			auto i1s = begin(), i1e = end();
			auto i2s = v2.begin(), i2e = v2.end();
			std::optional<Segment> left = *(i1s++);
			std::optional<Segment> right = *(i2s++);
			while(left.has_value() && right.has_value())
			{
				typename Segment::Result seg_result = left->Collision(*right);
				if(seg_result.or_shared)
				{
					if(seg_result.and_big)
					{
						if(seg_result.big_from_left)
						{
							left = seg_result.or_shared;
							right = std::nullopt;
						}else
						{
							right = seg_result.or_shared;
							left = std::nullopt;
						}
					}else
					{
						result.push_back(seg_result.or_shared);
						left = std::nullopt;
						right = std::nullopt;
					}
				}else
				{
					result.push_back(seg_result.and_small);
					if(seg_result.big_from_left)
						right = std::nullopt;
					else
						left = std::nullopt;
				}
				if (!left.has_value() && i1s != i1e)
					left = *(i1s++);
				if (!right.has_value() && i2s != i2e)
					right = *(i2s++);
			}
			if (left)
			{
				result.push_back(*left);
				result.insert(result.end(), i1s, i1e);
			}
			if(right)
			{
				result.push_back(*right);
				result.insert(result.end(), i2s, i2e);
			}
			return Interval<PointT, Less, Allocator>(std::move(result), NoDetectT{});
		}else if(!Empty())
			return *this;
		else
			return v2;
	}
	
	template<typename PointT, typename Less, typename Allocator>
	struct Interval
	{
		using Segment = Segment<PointT, Less>;
		using StorageType = std::vector<Segment, Allocator>;

		Interval() = default;
		Interval(Interval&&) = default;
		Interval(Interval const&) = default;
		Interval& operator=(Interval&&) = default;
		Interval& operator=(Interval const&) = default;

		Interval(Segment const& si) : Interval(si ? Interval({ si }, NoDetectT{}) : Interval()) {}
		Interval(StorageType st, NoDetectT) : intervals(std::move(st)) {};
		Interval(Segment const* pointer, size_t length, NoDetectT) : intervals(pointer, pointer + length) {};
		Interval(StorageType st);
		Interval(Segment const* pointer, size_t length) : Interval(StorageType(pointer, pointer + length)) {};
		Interval(IntervalWrapper<PointT, Less> wrapper) : Interval(wrapper.begin(), wrapper.size(), NoDetectT{}){}
		operator IntervalWrapper<PointT, Less>() const noexcept{return { intervals.data(), intervals.size() };}
		IntervalWrapper<PointT, Less> AsWrapper() const noexcept{return *this;}
		
		IntervalWrapper<PointT, Less>::Result<Allocator> Collision(IntervalWrapper<PointT, Less> wrapper) const { return AsWrapper().Collision<Allocator>(wrapper); }
		Interval operator-(IntervalWrapper<PointT, Less> wrapper) const {return AsWrapper().Remove<Allocator>(wrapper);}
		Interval operator+(IntervalWrapper<PointT, Less> wrapper) const {return *this | wrapper;}
		Interval operator&(IntervalWrapper<PointT, Less> wrapper) const { return AsWrapper().Intersection<Allocator>(wrapper); }
		Interval operator|(IntervalWrapper<PointT, Less> wrapper) const { return AsWrapper().Union<Allocator>(wrapper); }
		Segment AsSegment() const noexcept;
		auto begin() const {return intervals.begin();}
		auto end() const {return intervals.end(); }
		auto rbegin() const { return intervals.rbegin(); }
		auto rend() const { return intervals.rend(); }
		size_t size() const noexcept { return intervals.size(); }
		bool Empty() const noexcept{ return intervals.empty(); }
	private:
		StorageType intervals;
	};

	
	template<typename PointT, typename Less, typename Allocator>
	Interval<PointT, Less, Allocator>::Interval(StorageType st)
	{
		for(auto& ite : st)
			if(ite)
				intervals = std::move(AsWrapper().Union<Allocator>(IntervalWrapper<PointT, Less>(&ite, 1)).intervals);
	}

	template<typename PointT, typename Less, typename Allocator>
	auto  Interval<PointT, Less, Allocator>::AsSegment() const noexcept -> Segment
	{
		if(!Empty())
			return {begin()->start, rbegin()->end};
		return {};
	}

}
