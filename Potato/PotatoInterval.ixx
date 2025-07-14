module;

#include <cassert>

export module PotatoInterval;

import std;
import PotatoMisc;


export namespace Potato::Misc
{

	template<typename Type> 
	struct DefaultIntervalWrapperT
	{
		constexpr static Type AddOnce(Type T) { return T + 1; }
		constexpr static std::strong_ordering Order(Type T1, Type T2) { return T1 <=> T2; }
	};

	enum class OrderingE
	{
		Left = 0,
		LeftEqual = 1,
		Middile = 2,
		RightEqual = 3,
		Right = 4,
	};

	struct OrderResultT
	{
		OrderingE StartOrder;
		OrderingE EndOrder;
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
		constexpr IntervalElementT Expand(IntervalElementT Input) const{
			return {
				(std::is_lt(Wrapper::Order(Start, Input.Start)) ? Start : Input.Start),
				(std::is_gt(Wrapper::Order(End, Input.End)) ? End : Input.End)
			};
		};

		constexpr bool operator==(IntervalElementT const& I1) const {
			return std::is_eq(Wrapper::Order(Start, I1.Start)) && std::is_eq(Wrapper::Order(End, I1.End));
		}

		Type Start;
		Type End;

		using StorageT = Type;
		using WrapperT = Wrapper;
	};

	template<typename Type, typename Wrapper>
	struct IntervalT;

	template<typename Type, typename Wrapper = DefaultIntervalWrapperT<Type>>
	struct IntervalWrapperT
	{

		using ElementT = IntervalElementT<Type, Wrapper>;

		static std::strong_ordering Order(Type T1, Type T2) { return Wrapper::Order(T1, T2); }

		static auto Ordering(std::initializer_list<ElementT> const& List, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			-> IntervalT<Type, Wrapper>;

		// Need Ordered in Span
		static auto Add(std::span<ElementT const> T1, std::span<ElementT const> T2, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			-> IntervalT<Type, Wrapper>;

		// Need Ordered in Span
		static auto Sub(std::span<ElementT const> T1, std::span<ElementT const> T2, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			-> IntervalT<Type, Wrapper>;
		
		// Need Ordered in Span
		static auto And(std::span<ElementT const> T1, std::span<ElementT const> T2, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			-> IntervalT<Type, Wrapper>;

		// Need Ordered in Span
		static bool IsInclude(std::span<ElementT const> T1, Type Input)
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

		struct LocateResult
		{
			std::size_t Index;
			bool InFront;
		};

		static std::tuple<LocateResult, LocateResult> Locate(std::span<ElementT const> Source, ElementT Value);

		static bool AddOne(std::pmr::vector<ElementT>&, ElementT Input);
	};



	template<typename Type, typename Wrapper = DefaultIntervalWrapperT<Type>>
	struct IntervalT
	{
		using ElementT = IntervalElementT<Type, Wrapper>;
		using WrapperT = IntervalWrapperT<Type, Wrapper>;
		
		struct NoDetectT {};

		IntervalT(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : Elements(resource) {}
		IntervalT(Type Single, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : Elements({ElementT{Single}}, resource) {};
		IntervalT(ElementT SingleElementT, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : Elements({ SingleElementT }, resource) {}
		IntervalT(IntervalT const&) = default;
		IntervalT(IntervalT &&) = default;
		IntervalT(std::initializer_list<ElementT> const& List, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : IntervalT(WrapperT::Ordering(List, resource)) {}
		ElementT& operator[](std::size_t Index) { return Elements[Index]; }
		ElementT const& operator[](std::size_t Index) const { return Elements[Index]; }
		std::size_t Size() const { return Elements.size(); }
		auto begin() const { return Elements.begin(); }
		auto end() const { return Elements.end(); }
		auto GetSpan() const { return std::span(Elements); }

		bool IsInclude(Type Input) const { return WrapperT::IsInclude(std::span(Elements), Input); }

		IntervalT Add(std::span<ElementT const> T1, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) const { return WrapperT::Add(std::span(Elements), T1, resource); };
		IntervalT Sub(std::span<ElementT const> T1, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) const { return WrapperT::Sub(std::span(Elements), T1, resource); };
		IntervalT And(std::span<ElementT const> T1, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) const { return WrapperT::And(std::span(Elements), T1, resource); };


		IntervalT operator+(std::span<ElementT const> T1) const { return this->Add(T1); };
		IntervalT operator-(std::span<ElementT const> T1) const { return this->Sub(T1); };
		IntervalT operator&(std::span<ElementT const> T1) const { return this->And(T1); };


		IntervalT& operator=(IntervalT const&) = default;
		IntervalT& operator=(IntervalT&&) = default;

		bool operator==(IntervalT const& T1) const { return Elements == T1.Elements; }

	protected:

		std::pmr::vector<ElementT> Elements;

		template<typename, typename>
		friend struct IntervalWrapperT;

		template<typename, typename>
		friend struct IntervalT;
	};

	template<typename Type, typename Wrapper>
	auto IntervalWrapperT<Type, Wrapper>::Ordering(std::initializer_list<ElementT> const& List, std::pmr::memory_resource* resource)
		-> IntervalT<Type, Wrapper>
	{
		IntervalT<Type, Wrapper> Result(resource);
		for (auto Ite : List)
		{
			AddOne(Result.Elements, Ite);
		}
		return Result;
	}

	template<typename Type, typename Wrapper>
	auto IntervalWrapperT<Type, Wrapper>::Locate(std::span<ElementT const> Source, ElementT Value) -> std::tuple<LocateResult, LocateResult>
	{
		bool F1Front = true;
		auto Find = std::find_if(Source.begin(), Source.end(), [&](ElementT const& Ele){
			if (std::is_lt(Order(Value.Start, Ele.Start)))
			{
				return true;
			}
			else if (std::is_lteq(Order(Value.Start, Ele.End)))
			{
				F1Front = false;
				return true;
			}else
				return false;
		});

		bool F2Front = true;
		auto Find2 = std::find_if(Find, Source.end(), [&](ElementT const& Ele) {
			if (std::is_lt(Order(Value.End, Ele.Start)))
			{
				return true;
			}
			else if (std::is_lteq(Order(Value.End, Ele.End)))
			{
				F2Front = false;
				return true;
			}
			else
				return false;
		});
		return {
			{Find != Source.end() ? Find - Source.begin() : Source.size(), F1Front},
			{Find2 != Source.end() ? Find2 - Source.begin() : Source.size(), F2Front},
		};
	}

	template<typename Type, typename Wrapper>
	bool IntervalWrapperT<Type, Wrapper>::AddOne(std::pmr::vector<ElementT>& Output, ElementT Input)
	{
		if (Input)
		{
			auto [B, E] = IntervalWrapperT::Locate(std::span(Output), Input);
			if (E.Index == 0)
			{
				if (E.InFront)
				{
					Output.insert(Output.begin(), Input);
					return true;
				}
				else if(B.InFront)
					Output[0].Start = Input.Start;
				return false;
			}
			else if (B.Index == Output.size())
			{
				Output.push_back(Input);
				return true;
			}
			else if (B.Index == E.Index)
			{
				if (B.InFront && E.InFront)
				{
					Output.insert(Output.begin() + B.Index, Input);
					return true;
				}
				else {
					if (B.InFront)
					{
						auto Ite = Output.begin() + B.Index;
						Ite->Start = Input.Start;
					}
					return false;
				}
			}
			else {
				auto BIte = Output.begin() + B.Index;
				if(B.InFront)
					BIte->Start = Input.Start;
				if(E.InFront)
					BIte->End = Input.End;
				else
					BIte->End = Output[E.Index].End;
				if(!E.InFront && E.Index < Output.size())
					Output.erase(BIte + 1, Output.begin() + E.Index + 1);
				else
					Output.erase(BIte + 1, Output.begin() + E.Index);
				return false;
			}
		}
		return false;
	}

	template<typename Type, typename Wrapper>
	auto IntervalWrapperT<Type, Wrapper>::Add(std::span<ElementT const> T1, std::span<ElementT const> T2, std::pmr::memory_resource* resource)
		-> IntervalT<Type, Wrapper>
	{
		IntervalT<Type, Wrapper> Result(resource);
		for (auto Ite : T1)
		{
			AddOne(Result.Elements, Ite);
		}
		for(auto Ite : T2)
		{
			AddOne(Result.Elements, Ite);
		}
		return Result;
	}

	template<typename Type, typename Wrapper>
	auto IntervalWrapperT<Type, Wrapper>::Sub(std::span<ElementT const> T1, std::span<ElementT const> T2, std::pmr::memory_resource* resource)
		-> IntervalT<Type, Wrapper>
	{
		IntervalT<Type, Wrapper> Result(resource);

		while (!T1.empty())
		{
			auto Cur = T1[0];
			T1 = T1.subspan(1);
			bool NeedAdd = true;
			while (!T2.empty())
			{
				auto Cur2 = T2[0];
				auto [B, E] = Locate({&Cur, 1}, Cur2);
				if (E.Index == 0)
				{
					if (E.InFront)
					{
						T2 = T2.subspan(1);
						continue;
					}
					else if (!B.InFront)
					{
						if (!std::is_eq(Order(Cur.Start, Cur2.Start)))
							Result.Elements.push_back({ Cur.Start, Cur2.Start });
					}
					if (std::is_eq(Order(Cur.End, Cur2.End)))
					{
						T2 = T2.subspan(1);
						NeedAdd = false;
						break;
					}
					else {
						Cur = { Cur2.End, Cur.End };
						T2 = T2.subspan(1);
					}
				}
				else if (E.Index == 1)
				{
					if(B.Index == 1)
						break;
					else if (!B.InFront && !std::is_eq(Order(Cur.Start, Cur2.Start)))
					{
						Result.Elements.push_back({Cur.Start, Cur2.Start});
					}
					NeedAdd = false;
					break;
				}
			}
			if (NeedAdd)
			{
				Result.Elements.push_back(Cur);
			}
		}
		return Result;
	}

	template<typename Type, typename Wrapper>
	auto IntervalWrapperT<Type, Wrapper>::And(std::span<ElementT const> T1, std::span<ElementT const> T2, std::pmr::memory_resource* resource)
		-> IntervalT<Type, Wrapper>
	{
		IntervalT<Type, Wrapper> Result(resource);

		while (!T1.empty())
		{
			auto Cur = T1[0];
			T1 = T1.subspan(1);
			while (!T2.empty())
			{
				auto Cur2 = T2[0];
				auto [B, E] = Locate({ &Cur, 1 }, Cur2);
				if (E.Index == 0)
				{
					if (!E.InFront)
					{
						if (!B.InFront)
						{
							Result.Elements.push_back(Cur2);
						}
						else if(!std::is_eq(Wrapper::Order(Cur.Start, Cur2.End))){
							Result.Elements.push_back({ Cur.Start, Cur2.End });
						}
					}
					T2 = T2.subspan(1);
					continue;
				}
				else if (E.Index == 1)
				{
					if (B.Index != 1)
					{
						if (B.InFront)
						{
							Result.Elements.push_back(Cur);
						}
						else if(!std::is_eq(Wrapper::Order(Cur2.Start, Cur.End)))
						{
							Result.Elements.push_back({ Cur2.Start, Cur.End });
						}
					}
					break;
				}
			}
		}
		return Result;
	}
}