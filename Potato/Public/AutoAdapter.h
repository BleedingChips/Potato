#pragma once
#include <functional>
#include "TMP.h"
/*
namespace Potato::Tool
{
	class unorder_adapt
	{

		template<typename T, typename K> struct combine_implement
		{
			static_assert(!std::is_same<K, std::integer_sequence<size_t>>::value, "unable to handle unbindable function");
		};
		template<size_t ...s1, size_t s2, size_t... s2o> struct combine_implement<std::integer_sequence<size_t, s1...>, std::integer_sequence<size_t, s2, s2o...>>
		{
			using type = typename std::conditional_t<Tmp::is_one_of<std::integral_constant<size_t, s2>, std::integral_constant<size_t, s1>...>::value,
				Tmp::instant<unorder_adapt::template combine_implement, std::integer_sequence<size_t, s1...>, std::integer_sequence<size_t, s2o...>>,
				Tmp::instant<Tmp::itself, std::integer_sequence<size_t, s1..., s2>>
			>::template in_t_t<>;
		};

	public:
		template<typename T, typename K> using match = std::is_convertible<T, K>;
		template<typename ...T> struct combine
		{
			using type = TmpCall::call<TmpCall::append<std::integer_sequence<size_t>, T...>, TmpCall::combine<TmpCall::make_func_t<unorder_adapt::template combine_implement>>, TmpCall::self>;
		};
	};

	class order_adapt
	{
		template<typename out, typename in> struct reverser;
		template<size_t ...out, size_t t, size_t... in> struct reverser<std::integer_sequence<size_t, out...>, std::integer_sequence<size_t, t, in...>>
		{
			using type = typename reverser<std::integer_sequence<size_t, t, out...>, std::integer_sequence<size_t, in...>>::type;
		};
		template<size_t ...out> struct reverser<std::integer_sequence<size_t, out...>, std::integer_sequence<size_t>>
		{
			using type = std::integer_sequence<size_t, out...>;
		};

		template<typename T, typename K> struct combine_implement
		{
			static_assert(!std::is_same<K, std::integer_sequence<size_t>>::value, "unable to handle unbindable function");
		};
		template<size_t s2, size_t... s2o> struct combine_implement<std::integer_sequence<size_t>, std::integer_sequence<size_t, s2, s2o...>>
		{
			using type = std::integer_sequence<size_t, s2>;
		};
		template<size_t s1, size_t ...s1o, size_t s2, size_t... s2o> struct combine_implement<std::integer_sequence<size_t, s1, s1o...>, std::integer_sequence<size_t, s2, s2o...>>
		{
			using type = typename std::conditional_t <
				Tmp::is_one_of<std::integral_constant<size_t, s2>, std::integral_constant<size_t, s1>, std::integral_constant<size_t, s1o>...>::value || (s1 > s2),
				combine_implement<std::integer_sequence<size_t, s1, s1o...>, std::integer_sequence<size_t, s2o...>>,
				Tmp::itself<std::integer_sequence<size_t, s2, s1, s1o...>>
				>::type;
		};

		template<typename T, typename K> using combine_implement_t = typename combine_implement<T, K>::type;
	public:
		template<typename T, typename K> using match = std::is_convertible<T, K>;
		template<typename ...T> struct combine
		{
			using type = typename reverser<std::integer_sequence<size_t>, TmpCall::call<TmpCall::append<std::integer_sequence<size_t>, T...>, TmpCall::combine<TmpCall::make_func_t<order_adapt::template combine_implement>>, TmpCall::self>>::type;
		};
	};

	namespace Implement
	{
		template<typename T> struct add_one;
		template<size_t ...i> struct add_one<std::integer_sequence<size_t, i...>>
		{
			using type = std::integer_sequence<size_t, (i + 1)...>;
		};

		template<template<typename ...> class role> struct analyze_match_implement
		{
			template<typename T, typename ...AT> struct in
			{
				using type = TmpCall::call<TmpCall::append<AT...>, TmpCall::localizer<TmpCall::make_func<role, T>>, TmpCall::self>;
			};
		};

		template<bool, typename adapt_type, typename func, typename ...par> struct analyze_implement
		{
			using func_para_append = typename Tmp::pick_func<Tmp::degenerate_func_t<Tmp::extract_func_t<func>>>::template out<TmpCall::append>;
			using type = typename TmpCall::call<func_para_append,
				TmpCall::sperate_call<
				TmpCall::append<par...>,
				analyze_match_implement<adapt_type::template match>
				>,
				TmpCall::make_func_t<adapt_type::template combine>
			>;
		};

		template<typename adapt_type, typename func, typename member_ptr, typename ...par>
		struct analyze_implement<true, adapt_type, func, member_ptr, par...>
		{
			using func_para_append = typename Tmp::pick_func<Tmp::degenerate_func_t<Tmp::extract_func_t<func>>>::template out<TmpCall::append>;

			using type = typename TmpCall::call<
				func_para_append,
				TmpCall::sperate_call<
				TmpCall::append<par...>,
				analyze_match_implement<adapt_type::template match>
				>,
				TmpCall::make_func_t<adapt_type::template combine>//,
			>;
		};

		template<size_t ...i, typename fun, typename par> decltype(auto) auto_adapter_implement(std::true_type, std::integer_sequence<size_t, i...>, fun&& f, par pa)
		{
			return std::invoke(std::forward<fun>(f), std::get<0>(pa), std::get<(i + 1)>(pa)...);
		}

		template<size_t ...i, typename fun, typename par> decltype(auto) auto_adapter_implement(std::false_type, std::integer_sequence<size_t, i...>, fun&& f, par pa)
		{
			return std::invoke(std::forward<fun>(f), std::get<(i)>(pa)...);
		}
	}

	template<typename adapter_type, typename func_object, typename ...input>
	decltype(auto) auto_adapter(func_object&& fo, input&& ... in)
	{
		//cout << typeid(func_object).name() << " " << typeid(std::tuple<input&&...>).name() << endl;
		return statement_if<Tmp::is_callable<func_object, input...>::value>
			(
				[](auto&& fot, auto&& ...ini) { return std::invoke(std::forward<decltype(fot) && >(fot), std::forward<decltype(ini) && >(ini)...); },
				[](auto&& fot, auto&& ...ini)
		{
			using index = typename Implement::analyze_implement<Tmp::is_member_function_pointer<func_object>::value, adapter_type, decltype(fot) &&, decltype(ini) && ...>::type;
			return Implement::auto_adapter_implement(
				std::integral_constant<bool, Tmp::is_member_function_pointer<func_object>::value>{},
				index{},
				std::forward<decltype(fot) && >(fot), std::forward_as_tuple(std::forward<decltype(ini) && >(ini)...));
		},
				std::forward<func_object>(fo), std::forward<input>(in)...
			);
	}


	template<typename func_object, typename ...input>
	decltype(auto) auto_adapter_unorder(func_object&& fo, input&& ... in)
	{
		return auto_adapter<unorder_adapt>(std::forward<func_object>(fo), std::forward<input>(in)...);
	}

	template<typename func_object, typename ...input>
	decltype(auto) auto_adapter_order(func_object&& fo, input&& ... in)
	{
		return auto_adapter<order_adapt>(std::forward<func_object>(fo), std::forward<input>(in)...);
	}

	namespace Implement
	{
		template<typename ...AT> struct auto_bind_function_implement
		{
			template<typename adapter_type, typename func, typename ...AK> decltype(auto) bind(func&& f, AK&& ...ak) const noexcept
			{
				return [=](AT ...a) mutable { return auto_adapter<adapter_type>(f, ak..., a...); };
			}
		};

		template<typename ...AT> struct auto_bind_member_function_implement
		{
			template<typename adapter_type, typename func, typename owner, typename ...AK> decltype(auto) bind(func&& f, owner&& o, AK&& ...ak) const noexcept
			{
				return [=, &o](AT ...a) mutable { return auto_adapter<adapter_type>(f, o, ak..., a...); };
			}
		};
	}

	template<typename target, typename adapter_type, typename fun_obj, typename ...input> decltype(auto) auto_bind_function(fun_obj&& fo, input&& ... in)
	{
		static_assert(!std::is_member_function_pointer<fun_obj>::value || sizeof...(input) >= 1, "PO::Mail::Assistant::mail_create_funtion_ptr_execute need a ref of the owner of the member function");
		return typename Tmp::pick_func<Tmp::degenerate_func_t<Tmp::extract_func_t<target>>>::template out<Implement::auto_bind_function_implement>{}.template bind<adapter_type>
			(
				std::forward<fun_obj>(fo),
				std::forward<input>(in)...
				);
	}

}
*/

namespace Potato
{
	
	namespace Implement
	{
		template<typename Type, size_t index> struct TypeWrapper{};

		template<size_t Index, typename Input, typename Output>
		struct PackerImplement;

		template<size_t Index, typename Input, typename ...OInput, typename ...Output>
		struct PackerImplement<Index, TypeTuple<Input, OInput...>, TypeTuple<Output...>> {
			using Type = typename PackerImplement<Index + 1, TypeTuple<OInput...>, TypeTuple<Output..., TypeWrapper<Input, Index>>>::Type;
		};

		template<size_t Index, typename ...Output>
		struct PackerImplement<Index, TypeTuple<>, TypeTuple<Output...>> {
			using Type = TypeTuple<Output...>;
		};

		template<typename ...OInput>
		struct Package {
			using Type = typename PackerImplement<0, TypeTuple<OInput...>, TypeTuple<>>::Type;
		};

		template<size_t index, typename InputType> struct AdapterOneResult
		{
			static constexpr size_t Index = index;
			using Last = InputType;
		};

		template<typename ...Input>
		struct AdapterOneType;

		template<typename RequireType, typename ...Last, size_t Index, typename Input, typename ...OInput>
		struct AdapterOneType<RequireType, TypeTuple<Last...>, TypeTuple<TypeWrapper<Input, Index>, OInput...>>
		{
		public:
			using Type = typename std::conditional_t<
				std::is_convertible_v<Input, RequireType>,
				Instant<ItSelf, AdapterOneResult<Index, TypeTuple<Last..., OInput...>>>,
				Instant<AdapterOneType, RequireType, TypeTuple<Last..., TypeWrapper<Input, Index>>, TypeTuple<OInput...>>
			>::template AppendT<>;
		};

		template<typename RequireType, typename ...Last>
		struct AdapterOneType<RequireType, TypeTuple<Last...>, TypeTuple<>>
		{
		public:
			using Type = AdapterOneResult<std::numeric_limits<size_t>::max(), TypeTuple<Last...>>;
		};

		template<typename Index, typename Require, typename Provide>
		struct Adapter;

		template<typename ...Index, typename CurRequire, typename ...ORequire, typename ...Provide>
		struct Adapter<TypeTuple<Index...>, TypeTuple<CurRequire, ORequire...>, TypeTuple<Provide...>>
		{
			using Cur = typename AdapterOneType<CurRequire, TypeTuple<>, TypeTuple<Provide...>>::Type;
			using Type = std::conditional_t<
			Cur::Index < std::numeric_limits<size_t>::max(),
			Instant<Adapter, TypeTuple<Index..., std::integral_constant<size_t, Cur::Index>>, TypeTuple<ORequire...>, typename Cur::Last>,
			Instant<ItSelf, std::index_sequence<>>
			>::template AppenT<>::Type;
		};
	}

	inline void OP(float*){}

	template<typename FuncObject, typename ...Parameter>
	struct Adaptable {
		using FunRequire = typename FunctionObjectInfo<FuncObject>::template PackParameters<TypeTuple>;
		using ParProvide = typename Implement::Package<Parameter...>::Type;
		using PK = Instant<Implement::AdapterOneType, int64_t, TypeTuple<>>::template Append<TypeTuple<Implement::TypeWrapper<int64_t, 0>>>;
		//using K = typename Implement::AdapterOneType<int64_t, TypeTuple<>, TypeTuple<Implement::TypeWrapper<int64_t, 0>>>::Type;
		//using Index = typename Implement::Adapter<TypeTuple<>, FunRequire, ParProvide>::Type;
		//static constexpr value = (Index::Size == FunRequire::Size);
		Adaptable() {
			OP(PK{});
		}
	};

	/*
	template<typename FuncObject, typename ...Parameter>
	decltype(auto) AutoAdapter(FuncObject&& fo, Parameter&& ... par) 
	{
		using FunRequire = typename FunctionObjectInfo<FuncObject>::template PackParameters<TypeTuple>;
		using ParProvide = typename Implement::Package<Parameter...>::Type;
		using Index = typename Implement::Adapter<std::index_sequence<>, FunRequire, ParProvide>::Type;
	}
	*/
}