module;

#include <cassert>

export module Potato.Interval;
export import Potato.Misc;

export namespace Potato::Misc
{

	template<typename Type> 
	struct DefaultIntervalWrapperT
	{
		constexpr static Type AddOnce(Type T) { return T + 1; }
		constexpr static std::strong_ordering Order(Type T1, Type T2) { return T1 <=> T2; }
	};

	template<typename Type, typename Wrapper = DefaultIntervalWrapperT<Type>>
	struct IntervalElementT
	{
		constexpr IntervalElementT(Type Single) : Start(Single), End(Wrapper::AddOnce(Single)) {}
		constexpr IntervalElementT(Type Start, Type End) : Start(Start), End(End) { assert(*this); }

		constexpr IntervalElementT(IntervalElementT const&) = default;
		constexpr IntervalElementT& operator=(IntervalElementT const&) = default;
		constexpr bool IsInclude(Type Input) const { return std::is_gteq(Wrapper::Order(Input, Start)) && std::is_lt(Wrapper::Order(Input, End)); }
		constexpr operator bool() const { return std::is_lt(Wrapper::Order(Start, End)); }

		Type Start;
		Type End;

		using StorageT = Type;
		using WrapperT = Wrapper;
	};

	template<typename Type, typename Wrapper, typename Allocator>
	struct IntervalT;

	template<typename Type, typename Wrapper>
	struct IntervalWrapperT
	{

		using ElementT = IntervalElementT<Type, Wrapper>;

		static std::strong_ordering Order(Type T1, Type T2) { return Wrapper::Order(T1, T2); }

		template<typename AllocatorT>
		static auto Ordering(std::initializer_list<ElementT> const& List, AllocatorT Allocator)
			-> IntervalT<Type, Wrapper, AllocatorT>;

		// Need Ordered in Span
		template<typename AllocatorT>
		static auto Add(std::span<ElementT const> T1, std::span<ElementT const> T2, AllocatorT Allocator)
			-> IntervalT<Type, Wrapper, AllocatorT>;

		// Need Ordered in Span
		template<typename AllocatorT>
		static auto Sub(std::span<ElementT const> T1, std::span<ElementT const> T2, AllocatorT Allocator)
			-> IntervalT<Type, Wrapper, AllocatorT>;
		
		// Need Ordered in Span
		template<typename AllocatorT>
		static auto And(std::span<ElementT const> T1, std::span<ElementT const> T2, AllocatorT Allocator)
			-> IntervalT<Type, Wrapper, AllocatorT>;

		// Need Ordered in Span
		static bool IsInclude(std::span<ElementT const> T1, ElementT::Type Input)
		{
			if (!T1.empty())
			{
				for (auto& Ite : T1)
				{
					if(Ite.IsInclude(Input))
						return true;
				}
			}
			return false;
		}

	protected:

		template<typename AllocatorT>
		static bool AddOne(IntervalT<Type, Wrapper, AllocatorT>& Output, ElementT Input);
	};

	template<typename Type, typename Wrapper = DefaultIntervalWrapperT<Type>, typename AllocatorT = std::allocator<IntervalElementT<Type, Wrapper>>>
	struct IntervalT
	{
		using ElementT = IntervalElementT<Type, Wrapper>;
		using WrapperT = IntervalWrapperT<Type, Wrapper>;
		
		struct NoDetectT {};

		IntervalT(AllocatorT Allocator = {}) : Elements(Allocator) {}
		IntervalT(Type Single, AllocatorT Allocator = {}) : Elements({ElementT{Single}}, std::move(Allocator)) {};
		IntervalT(ElementT SingleElementT, AllocatorT Allocator = {}) : Elements({ SingleElementT }, std::move(Allocator)) {}
		IntervalT(IntervalT const&) = default;
		IntervalT(IntervalT &&) = default;
		IntervalT(std::initializer_list<ElementT> const& List, AllocatorT Allocator = {}) : IntervalT(WrapperT::Ordering(List, std::move(Allocator))) {}

		bool IsInclude(Type Input) const { return WrapperT::IsInclude(std::span(Elements), Input); }

		template<typename OAllocatorT>
		IntervalT operator+(IntervalT<Type, Wrapper, OAllocatorT> const& T1) const { return WrapperT::Add(std::span(Elements), std::span(T1.Elements), AllocatorT{}); }
		template<typename OAllocatorT>
		IntervalT operator-(IntervalT<Type, Wrapper, OAllocatorT> const& T1) const { return WrapperT::Sub(std::span(Elements), std::span(T1.Elements), AllocatorT{}); }
		template<typename OAllocatorT>
		IntervalT operator&(IntervalT<Type, Wrapper, OAllocatorT> const& T1) const { return WrapperT::And(std::span(Elements), std::span(T1.Elements), AllocatorT{}); }

	protected:

		std::vector<ElementT, AllocatorT> Elements;

		template<typename Type, typename Wrapper>
		friend struct IntervalWrapperT;

		template<typename Type, typename Wrapper, typename AllocatorT>

		friend struct IntervalT;
	};

	template<typename Type, typename Wrapper>
	template<typename AllocatorT>
	static auto IntervalWrapperT<Type, Wrapper>::Ordering(std::initializer_list<ElementT> const& List, AllocatorT Allocator)
		-> IntervalT<Type, Wrapper, AllocatorT>
	{
		IntervalT<Type, Wrapper, AllocatorT> Result(Allocator);
		for (auto Ite : List)
		{
			AddExe(Result, Ite);
		}
		return Result;
	}

	template<typename Type, typename Wrapper>
	template<typename AllocatorT>
	static bool IntervalWrapperT<Type, Wrapper>::AddOne(IntervalT<Type, Wrapper, AllocatorT>& Output, ElementT Input)
	{
		if (Input)
		{
			auto LeftIte = std::find_if(Result.begin(), Result.end(), [=](ElementT Ele) { return std::is_lteq(Order(Ite.Start, Ele.Start)); });
			if (LeftIte == Result.end())
			{
				Result.push_back(Ite);
			}
			else {
				auto RigIte = std::find_if(Result.begin(), Result.end(), [=](ElementT Ele) { return std::is_lteq(Order(Ite.End, Ele.End)); });
				if (RigIte == LeftIte)
				{

				}
				else if (RigIte > LeftIte)
				{

				}else if()
				assert(RigIte >= LeftIte);
				if (LeftIte == RigIte)
				{
					LeftIte->Start = Ite.Start;
				}
				else {

				}
			}
			return true;
		}
		return false;
	}

	/*
	struct NoDetectT {};

	template<typename Type, typename Compare = SelfCompare<Type>>
	struct Interval
	{
		using StorageType = Type;
		using CompareType = Compare;

		Type start;
		Type end;
		Interval(Type P1) : Interval(P1, P1 + 1) {}
		Interval(Type p1, Type p2) : start(std::move(p1)), end(std::move(p2)) { assert(start <= end); }
		Interval() = default;
		Interval(Interval const&) = default;
		Interval(Interval&&) = default;
		Interval& operator=(Interval const&) = default;
		Interval& operator=(Interval&&) = default;
		std::strong_ordering operator<=>(Interval const&) const = default;
		bool IsInclude(Type const& si) const { return (Compare{}(start, si) <= 0) && (Compare{}(si, end) < 0); }
		operator bool() const { return Compare{}(start, end) < 0; }

		struct Result
		{
			Interval and_small;
			Interval and_middle;
			Interval and_big;
			Interval or_shared;
			bool small_from_left;
			bool big_from_left;
		};

		Result Collision(Interval const& input) const noexcept;
		Interval Intersection(Interval const& input) const noexcept;
		std::tuple<Interval, Interval> Remove(Interval const& input) const noexcept;
		Interval Union(Interval const& input) const noexcept;
	};

	template<typename Type, typename Compare>
	auto Interval<Type, Compare>::Collision(Interval const& v2) const noexcept -> Result
	{
		assert(*this && v2);
		auto r1 = Compare{}(start, v2.start);
		if (r1 < 0)
		{
			auto r2 = Compare{}(end, v2.start);
			if (r2 < 0)
				return { *this, {}, v2, {}, true, false };
			else if (r2 == 0)
				return { *this, {}, v2, {start, v2.end}, true, false };
			else
			{
				auto r3 = Compare{}(end, v2.end);
				if (r3 <= 0)
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
						*this,
						true, true
				};
			}
		}
		else if (r1 == 0)
		{
			auto r2 = Compare{}(end, v2.end);
			if (r2 < 0)
				return { {}, *this, {end, v2.end}, v2, false, false };
			else if (r2 == 0)
				return { {}, *this, {}, *this, false, false };
			else
				return { {}, v2, {v2.end, end}, *this, false, true };
		}
		else
		{
			auto r2 = Compare{}(v2.end, start);
			if (r2 < 0)
				return { v2, {}, *this, {}, false, true };
			else if (r2 == 0)
				return { v2, {}, *this, {v2.start, end}, false, true };
			else
			{
				auto r3 = Compare{}(v2.end, end);
				if (r3 < 0)
					return { {v2.start, start}, {start, v2.end}, {v2.end, end}, {v2.start, end}, false, true };
				else
					return { {v2.start, start}, *this, {end, v2.end}, v2, false, false };
			}
		}
	}

	template<typename Type, typename Compare>
	auto Interval<Type, Compare>::Intersection(Interval const& si) const noexcept -> Interval
	{
		if (*this && si)
			return Collision(si).and_middle;
		else
			return {};
	}

	template<typename Type, typename Compare>
	auto Interval<Type, Compare>::Remove(Interval const& si) const noexcept -> std::tuple<Interval, Interval>
	{
		if (*this && si)
		{
			auto result = Collision(si);
			std::tuple<Interval, Interval> re{ {}, {} };
			if (result.small_from_left)
				std::get<0>(re) = result.and_small;
			if (result.big_from_left)
			{
				if (std::get<0>(re))
					std::get<1>(re) = result.and_big;
				else
					std::get<0>(re) = result.and_big;
			}
			return re;
		}
		else
			return { *this, {} };
	}

	template<typename Type, typename Compare>
	auto Interval<Type, Compare>::Union(Interval const& ref) const noexcept -> Interval
	{
		if (*this && ref)
			return { (Compare{}(start, ref.start) < 0) ? start : ref.start,  (Compare{}(end, ref.end) < 0) ? ref.end : end };
		else if (*this)
			return *this;
		else
			return ref;
	}

	template<typename Type, typename Less = SelfCompare<Type>>
	struct SequenceIntervalWrapper : public std::span<Interval<Type, Less> const>
	{
		using IntervalT = Interval<Type, Less>;
		using StorageT = std::span<Interval<Type, Less> const>;

		//using StorageT::span;

		SequenceIntervalWrapper(StorageT input) : StorageT(input) {}
		template<typename A1>
		SequenceIntervalWrapper(std::vector<IntervalT, A1> const& input) : StorageT(input.begin(), input.end()) {}
		SequenceIntervalWrapper(IntervalT const& input) : StorageT(&input, 1) {};
		SequenceIntervalWrapper(IntervalT const* input, size_t count) : StorageT(input, count) {}
		SequenceIntervalWrapper() = default;


		struct Result
		{
			std::vector<IntervalT> left;
			std::vector<IntervalT> middle;
			std::vector<IntervalT> right;
		};

	private:

		template<bool write_left, bool write_middle, bool write_right, typename A1, typename A2, typename A3>
		void CollisionImplement(SequenceIntervalWrapper wrapper, std::vector<IntervalT, A1>& left, std::vector<IntervalT, A2>& middle, std::vector<IntervalT, A3>& right) const;

	public:

		IntervalT AsInterval() const { if (StorageT::empty()) return IntervalT{}; else return IntervalT{ StorageT::begin()->start, StorageT::rbegin()->end }; }

		template<typename A1, typename A2, typename A3>
		void Collision(SequenceIntervalWrapper wrapper, std::vector<IntervalT, A1>& left, std::vector<IntervalT, A2>& middle, std::vector<IntervalT, A3>& right) const {
			this->CollisionImplement<true, true, true>(wrapper, left, middle, right);
		}
		Result Collision(SequenceIntervalWrapper wrapper) const { Result result; Collision(wrapper, result.left, result.middle, result.right); return result; }

		template<typename A1>
		void Intersection(SequenceIntervalWrapper wrapper, std::vector<IntervalT, A1>& result) const {
			std::vector<IntervalT> temp;
			this->CollisionImplement<false, true, false>(wrapper, temp, result, temp);
		}
		std::vector<IntervalT> Intersection(SequenceIntervalWrapper wrapper) const { std::vector<IntervalT> re; Intersection(wrapper, re); return re; }

		template<typename A1>
		void Complementary(SequenceIntervalWrapper wrapper, std::vector<IntervalT, A1>& result) const {
			std::vector<IntervalT> temp;
			CollisionImplement<false, false, true>(wrapper, temp, temp, result);
		}
		std::vector<IntervalT> Complementary(SequenceIntervalWrapper wrapper) const { std::vector<IntervalT> re; Complementary(wrapper, re); return re; }

		template<typename A1>
		void Remove(SequenceIntervalWrapper wrapper, std::vector<IntervalT, A1>& result) const {
			std::vector<IntervalT> temp;
			CollisionImplement<true, false, false>(wrapper, result, temp, temp);
		}
		std::vector<IntervalT> Remove(SequenceIntervalWrapper wrapper) const { std::vector<IntervalT> re; Remove(wrapper, re); return re; }

		template<typename A1>
		void Union(SequenceIntervalWrapper wrapper, std::vector<IntervalT, A1>& result) const;
		std::vector<IntervalT> Union(SequenceIntervalWrapper wrapper) const { std::vector<IntervalT> re; Union(wrapper, re); return re; }

		template<typename A1>
		void Fix(std::vector<IntervalT, A1>& output) const;

		template<typename A1>
		std::vector<IntervalT> Fix() const { std::vector<IntervalT> output; this->Union({}, output); return std::move(output); }

		bool IsInclude(Type const&) const noexcept;
	};

	template<typename Type, typename Compare>
	template<typename A1>
	void SequenceIntervalWrapper<Type, Compare>::Fix(std::vector<IntervalT, A1>& output) const {
		for (auto& ite : *this)
		{
			auto Tem = std::move(output);
			SequenceIntervalWrapper<Type, Compare>{ ite }.Union(Tem, output);
		}
	}

	template<typename Type, typename Compare>
	bool SequenceIntervalWrapper<Type, Compare>::IsInclude(Type const& P) const noexcept
	{
		for (auto& ite : *this)
		{
			if (ite.IsInclude(P))
				return true;
		}
		return false;
	}

	template<typename Type, typename Compare>
	template<bool write_left, bool write_middle, bool write_right, typename A1, typename A2, typename A3>
	void SequenceIntervalWrapper<Type, Compare>::CollisionImplement(SequenceIntervalWrapper v2, std::vector<IntervalT, A1>& output_left, std::vector<IntervalT, A2>& output_middle, std::vector<IntervalT, A3>& output_right) const
	{
		if (!StorageT::empty() && !v2.empty())
		{
			auto i1s = this->begin(), i1e = this->end();
			auto i2s = v2.begin(), i2e = v2.end();
			std::optional<IntervalT> left = *(i1s++);
			std::optional<IntervalT> right = *(i2s++);
			while (left.has_value() && right.has_value())
			{
				typename IntervalT::Result sem_result = left->Collision(*right);
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
					}
					else
					{
						right = sem_result.and_big;
						left = std::nullopt;
					}
				}
				else
				{
					left = std::nullopt;
					right = std::nullopt;
				}
				if (!left.has_value() && i1s != i1e)
					left = *(i1s++);
				if (!right.has_value() && i2s != i2e)
					right = *(i2s++);
			}
			if constexpr (write_left)
			{
				if (left)
					output_left.push_back(*left);
				output_left.insert(output_left.end(), i1s, i1e);
			}
			if constexpr (write_right)
			{
				if (right)
					output_right.push_back(*right);
				output_right.insert(output_right.end(), i2s, i2e);
			}
		}
	}

	template<typename Type, typename Compare>
	template<typename A1>
	void SequenceIntervalWrapper<Type, Compare>::Union(SequenceIntervalWrapper v2, std::vector<IntervalT, A1>& result) const
	{

		if (!StorageT::empty() && !v2.empty())
		{
			auto i1s = this->begin(), i1e = this->end();
			auto i2s = v2.begin(), i2e = v2.end();
			std::optional<IntervalT> left = *(i1s++);
			std::optional<IntervalT> right = *(i2s++);
			while (left.has_value() && right.has_value())
			{
				typename IntervalT::Result seg_result = left->Collision(*right);
				if (seg_result.or_shared)
				{
					if (seg_result.and_big)
					{
						if (seg_result.big_from_left)
						{
							left = seg_result.or_shared;
							right = std::nullopt;
						}
						else
						{
							right = seg_result.or_shared;
							left = std::nullopt;
						}
					}
					else
					{
						result.push_back(seg_result.or_shared);
						left = std::nullopt;
						right = std::nullopt;
					}
				}
				else
				{
					result.push_back(seg_result.and_small);
					if (seg_result.big_from_left)
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
			if (right)
			{
				result.push_back(*right);
				result.insert(result.end(), i2s, i2e);
			}
		}
		else if (!StorageT::empty())
		{
			result.insert(result.end(), this->begin(), this->end());
		}
		else
			result.insert(result.end(), v2.begin(), v2.end());
	}

	template<typename Type, typename Compare = SelfCompare<Type>>
	struct SequenceInterval : std::vector<Interval<Type, Compare>>
	{
		using IntervalT = Interval<Type, Compare>;
		using StorageT = std::vector<IntervalT>;
		using WrapperT = SequenceIntervalWrapper<Type, Compare>;

		SequenceInterval(StorageT input, NoDetectT) : StorageT(std::move(input)) {}
		SequenceInterval(IntervalT input) : StorageT({ input }) {}
		SequenceInterval(SequenceIntervalWrapper<Type, Compare> input) { input.Fix(*this); }
		SequenceInterval(std::initializer_list<IntervalT> input) { SequenceIntervalWrapper<Type, Compare>(input).Fix(*this); }

		SequenceInterval(SequenceInterval&&) = default;
		SequenceInterval(SequenceInterval const&) = default;
		SequenceInterval& operator=(SequenceInterval const&) = default;
		SequenceInterval& operator=(SequenceInterval&&) = default;
		SequenceInterval() = default;

		std::strong_ordering operator<=>(SequenceInterval const&) const = default;
		bool operator==(SequenceInterval const&) const = default;


		IntervalT MinMax() const noexcept { return AsWrapper().AsInterval(); }
		SequenceIntervalWrapper<Type, Compare> AsWrapper() const noexcept { return SequenceIntervalWrapper<Type, Compare>(this->data(), this->size()); }
		operator SequenceIntervalWrapper<Type, Compare>() const noexcept { return AsWrapper(); }
		SequenceInterval Intersection(SequenceInterval const& input) const {
			return SequenceInterval{ AsWrapper().Intersection(input), NoDetectT {} };
		}
		SequenceInterval Intersection(IntervalT const& input) const {
			return SequenceInterval{ AsWrapper().Intersection(input), NoDetectT {} };
		}
		SequenceInterval Remove(SequenceInterval const& input) const {
			return SequenceInterval{ AsWrapper().Remove(input), NoDetectT {} };
		}
		SequenceInterval Remove(IntervalT const& input) const {
			return SequenceInterval{ AsWrapper().Remove(input), NoDetectT {} };
		}
		SequenceInterval Union(SequenceInterval const& input) const {
			return SequenceInterval{ AsWrapper().Union(input), NoDetectT {} };
		}
		SequenceInterval Union(IntervalT const& input) const {
			return SequenceInterval{ AsWrapper().Union(input), NoDetectT {} };
		}
		SequenceInterval Complementary(SequenceInterval const& input) const {
			return SequenceInterval{ AsWrapper().Complementary(input), NoDetectT {} };
		}
		SequenceInterval Complementary(IntervalT const& input) const {
			return SequenceInterval{ AsWrapper().Complementary(input), NoDetectT {} };
		}
	};
	*/
}