#pragma once
#include <utility>
#include <type_traits>
#include <concepts>
#include <any>
#include <atomic>
#include <cassert>
#include <span>
#include <tuple>

#include "TMP.h"

namespace Potato
{
	namespace Exception
	{
		template<typename ...InterfaceT>
		struct DefineInterface {};

		template<typename ...Interface>
		struct ExceptionInterfaceTuple {};

		template<typename CurInterface, typename ...Interface>
		struct ExceptionInterfaceTuple<CurInterface, Interface...> : public CurInterface, ExceptionInterfaceTuple<Interface...>
		{
		};

		template<typename Storage, std::default_initializable ...Interface>
		struct ExceptionTuple : public ExceptionInterfaceTuple<Interface...>, public Storage
		{
			ExceptionTuple(Storage&& stro) : Storage(std::move(stro)) {};
			ExceptionTuple(Storage const& stro) : Storage(stro) {}
			ExceptionTuple(ExceptionTuple&&) = default;
			ExceptionTuple(ExceptionTuple const&) = default;
		};

		template<typename Type> struct IsDefineInterface { static constexpr bool Value = false; };
		template<typename ...Type> struct IsDefineInterface<DefineInterface<Type...>> { static constexpr bool Value = true; };
		template<typename Type, typename = std::enable_if_t< IsDefineInterface<typename Type::ExceptionInterface>::Value>>
		struct HasExceptionInterfaceTupleRole {};

		template<typename ExceptionT>
		auto MakeExceptionTuple(ExceptionT&& ET)
		{
			using RequireType = std::remove_all_extents_t<ExceptionT>;
			if constexpr (Exist<RequireType, HasExceptionInterfaceTupleRole>::Value)
			{
				using RealType = typename Replace<typename RequireType::ExceptionInterface>::template With<Instant<ExceptionTuple, RequireType>::template AppendT>;
				return RealType{ std::forward<ExceptionT>(ET) };
			}
			else {
				return ExceptionTuple<RequireType>{std::forward<ExceptionT>(ET)};
			}
		}



		/*
		template<typename ...RequireType>
		struct AnyViewerImplementation
		{
			template<typename Funuction>
			void UniteCall(Funuction&& Func){ return false; }
			void SperateCall() { return false; }
		};

		template<typename CurType, typename ...RequireType>
		struct AnyViewerImp
		{
			template<typename >
		};

		export template<typename ExceptionT>
			struct AnyViewer
		{
		};
		*/
	}

	template<typename RequireType>
	struct SelfCompare
	{
		auto operator()(RequireType const& i1, RequireType const& i2) const { return i1 <=> i2; }
	};


	/*
	export template<typename ...RequireType>
	struct AnyOnlyViewer {
		template<typename ViewerFunction>
		AnyOnlyViewer(std::any& any, ViewerFunction&& function) : result(false) {}
		template<typename ViewerFunction>
		AnyOnlyViewer(std::any const& any, ViewerFunction&& function) : result (false) {}
		operator bool() const noexcept { return result; }
	private:
		bool result;
	};

	export template<typename ReuqireType, typename ...LastRequireType>
	struct AnyOnlyViewer<ReuqireType, LastRequireType...> {
		template<typename ViewerFunction>
		AnyOnlyViewer(std::any& any, ViewerFunction&& function)
		{
			if (auto re = std::any_cast<ReuqireType>(&any); re != nullptr)
			{
				result = true;
				std::forward<ViewerFunction>(function)(*re);
			}
			else {
				result = AnyOnlyViewer<LastRequireType...>{any, std::forward<ViewerFunction>(function)};
			}
		}
		template<typename ViewerFunction>
		AnyOnlyViewer(std::any const& any, ViewerFunction&& function)
		{
			if (auto re = std::any_cast<ReuqireType>(&any); re != nullptr)
			{
				result = true;
				std::forward<ViewerFunction>(function)(*re);
			}
			else {
				result = AnyOnlyViewer<LastRequireType...>{any, std::forward<ViewerFunction>(function)};
			}
		}
		operator bool() const noexcept { return result; }
	private:
		bool result;
	};
	*/

	inline bool AnyViewer(std::any const& ref)
	{
		return false;
	}

	template<typename CurObject, typename ...FunctionObject>
	bool AnyViewer(std::any const& ref, CurObject&& co, FunctionObject&& ...input_fo)
	{
		using FunT = std::remove_cvref_t<CurObject>;
		if constexpr (std::is_function_v<FunT>)
		{
			using RequireType = std::remove_reference_t<typename FunctionInfo<FunT>::template PackParameters<ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else if constexpr (IsFunctionObject<FunT>::Value)
		{
			using RequireType = std::remove_reference_t<typename FunctionObjectInfo<FunT>::template PackParameters<ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else
			static_assert(false, "wrong function type");
	}

	inline bool AnyViewer(std::any& ref)
	{
		return false;
	}

	template<typename CurObject, typename ...FunctionObject>
	bool AnyViewer(std::any& ref, CurObject&& co, FunctionObject&& ...input_fo)
	{
		using FunT = std::remove_cvref_t<CurObject>;
		if constexpr (std::is_function_v<FunT>)
		{
			using RequireType = std::remove_reference_t<typename FunctionInfo<FunT>::template PackParameters<ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else if constexpr (IsFunctionObject<FunT>::Value)
		{
			using RequireType = std::remove_reference_t<typename FunctionObjectInfo<FunT>::template PackParameters<ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else
			static_assert(false, "wrong function type");
	}

	template<typename ...RequireT>
	struct AnyViewerTemplate
	{
		template<typename Func>
		bool operator()(std::any& ref, Func&& func) { return false; }
		template<typename Func>
		bool operator()(std::any const& ref, Func&& func) { return false; }
	};

	template<typename RequireTThis, typename ...RequireT>
	struct AnyViewerTemplate<RequireTThis, RequireT...>
	{
		template<typename Func>
		bool operator()(std::any& ref, Func&& func) {
			if (auto P = std::any_cast<RequireTThis>(&ref); P != nullptr)
			{
				std::forward<Func>(func)(*P);
				return true;
			}
			else {
				return AnyViewerTemplate<RequireT...>{}(ref, std::forward<Func>(func));
			}
		}
		template<typename Func>
		bool operator()(std::any const& ref, Func&& func) { 
			if (auto P = std::any_cast<RequireTThis>(&ref); P != nullptr)
			{
				std::forward<Func>(func)(*P);
				return true;
			}
			else {
				return AnyViewerTemplate<RequireT...>{}(ref, std::forward<Func>(func));
			}
		}
	};

	/*
	struct Any : public std::any
	{
		using std::any::any;

		Any& operator=(Any const&) = default;
		Any& operator=(Any &&) = default;

		template<typename Type>
		Type GetData() { return std::any_cast<Type>(data); }
		template<typename Type>
		Type* TryGetDataPtr() { return std::any_cast<Type>(&data); }
		template<typename Type>
		std::remove_reference_t<Type> MoveData() { return std::move(std::any_cast<std::add_lvalue_reference_t<Type>>(data)); }
		std::any&& MoveRawData() { return std::move(*this); }
	};
	*/

	template<typename StorageType = std::size_t>
	struct IndexSpan
	{
		StorageType start;
		StorageType length;
		template<template<typename ...> class Output = std::span, typename ArrayType>
		auto operator()(ArrayType& Type) const -> Output<std::remove_reference_t<decltype(*Type.data())>>
		{
			return Output<std::remove_reference_t<decltype(*Type.begin())>>(Type.data() + start, Type.data() + start + length);
		};
	};

	struct AtomicRefCount
	{
		void WaitTouch(size_t targe_value) const noexcept;
		bool TryAndRef() const noexcept;
		void AddRef() const noexcept
		{
			assert(static_cast<std::ptrdiff_t>(ref.load(std::memory_order_relaxed)) >= 0);
			ref.fetch_add(1, std::memory_order_relaxed);
		}
		bool SubRef() const noexcept
		{
			assert(static_cast<std::ptrdiff_t>(ref.load(std::memory_order_relaxed)) >= 0);
			return ref.fetch_sub(1, std::memory_order_relaxed) == 1;
		}

		size_t Count() const noexcept { return ref.load(std::memory_order_relaxed); }

		AtomicRefCount() noexcept : ref(0) {}
		AtomicRefCount(AtomicRefCount const&) = delete;
		AtomicRefCount& operator= (AtomicRefCount const&) = delete;
		~AtomicRefCount() { assert(ref.load(std::memory_order_relaxed) == 0); }
	private:
		mutable std::atomic_size_t ref = 0;
	};

	namespace Implement
	{
		template<typename Type, size_t index> struct TypeWithIndex {};

		template<size_t Index, typename Input, typename Output>
		struct TypeWithIndexPacker;

		template<size_t Index, typename Input, typename ...OInput, typename ...Output>
		struct TypeWithIndexPacker<Index, TypeTuple<Input, OInput...>, TypeTuple<Output...>> {
			using Type = typename TypeWithIndexPacker<Index + 1, TypeTuple<OInput...>, TypeTuple<Output..., TypeWithIndex<Input, Index>>>::Type;
		};

		template<size_t Index, typename ...Output>
		struct TypeWithIndexPacker<Index, TypeTuple<>, TypeTuple<Output...>> {
			using Type = TypeTuple<Output...>;
		};

		template<typename ...OInput>
		struct PackageTypeWithIndex {
			using Type = typename TypeWithIndexPacker<0, TypeTuple<OInput...>, TypeTuple<>>::Type;
		};

		template<size_t index, typename InputType> struct AdapteResult
		{
			static constexpr size_t Index = index;
			using Last = InputType;
		};

		template<typename Result, typename Last, typename Input>
		struct AdapteCurrentType;

		template<typename RequireType, typename ...Last, typename Input, size_t Index, typename ...OInput>
		struct AdapteCurrentType<RequireType, TypeTuple<Last...>, TypeTuple<TypeWithIndex<Input, Index>, OInput...>>
		{
		public:
			using Type = typename std::conditional_t<
				std::is_convertible_v<Input, RequireType>,
				Instant<ItSelf, AdapteResult<Index, TypeTuple<Last..., OInput...>>>,
				//Instant<ItSelf, AdapterOneResult<Index, TypeTuple<Last..., OInput...>>>
				Instant<Implement::AdapteCurrentType, RequireType, TypeTuple<Last..., TypeWithIndex<Input, Index>>, TypeTuple<OInput...>>
			>::template AppendT<>::Type;
		};

		template<typename RequireType, typename ...Last>
		struct AdapteCurrentType<RequireType, TypeTuple<Last...>, TypeTuple<>>
		{
		public:
			using Type = AdapteResult<std::numeric_limits<size_t>::max(), TypeTuple<Last...>>;
		};

		template<typename Index, typename Require, typename Provide>
		struct AutoInvoteAdapter;

		template<typename ...Index, typename CurRequire, typename ...ORequire, typename ...Provide>
		struct AutoInvoteAdapter<TypeTuple<Index...>, TypeTuple<CurRequire, ORequire...>, TypeTuple<Provide...>>
		{
			using PreExecute = typename AdapteCurrentType<CurRequire, TypeTuple<>, TypeTuple<Provide...>>::Type;
			using Type = typename std::conditional_t <
				PreExecute::Index < std::numeric_limits<size_t>::max(),
				Instant<Implement::AutoInvoteAdapter, TypeTuple<Index..., std::integral_constant<size_t, PreExecute::Index>>, TypeTuple<ORequire...>, typename PreExecute::Last>,
				Instant<ItSelf, TypeTuple<>>
				>::template AppendT<>::Type;
		};

		template<typename ...Index, typename ...Provide>
		struct AutoInvoteAdapter<TypeTuple<Index...>, TypeTuple<>, TypeTuple<Provide...>>
		{
			using Type = TypeTuple<Index...>;
		};

		template<typename Index> struct AutoInvoteExeceuter;
		template<size_t ...Index> struct AutoInvoteExeceuter<TypeTuple<std::integral_constant<size_t, Index>...>>
		{
			template<typename FuncObject, typename ...Par>
			decltype(auto) operator()(FuncObject&& FO, Par&& ...par) {
				auto fortup = std::forward_as_tuple(par...);
				return std::forward<FuncObject>(FO)(std::get<Index>(fortup)...);
			}
		};
	}

	template<typename FuncObject, typename ...Parameter>
	struct AutoInvocable {
		using FunRequire = typename FunctionObjectInfo<std::remove_reference_t<FuncObject>>::template PackParameters<TypeTuple>;
		using ParProvide = typename Implement::PackageTypeWithIndex<Parameter...>::Type;
		using Index = typename Implement::AutoInvoteAdapter<TypeTuple<>, FunRequire, ParProvide>::Type;
		static constexpr bool Value = (Index::Size == FunRequire::Size);
	};

	template<typename FuncObject, typename ...Parameter> constexpr bool AutoInvocableV = AutoInvocable<FuncObject, Parameter...>::Value;

	template<typename FuncObject, typename ...Parameter>
	decltype(auto) AutoInvote(FuncObject&& fo, Parameter&& ... par) requires AutoInvocableV<FuncObject, Parameter...>
	{
		using Index = typename AutoInvocable<FuncObject, Parameter...>::Index;
		return Implement::AutoInvoteExeceuter<Index>{}(std::forward<FuncObject>(fo), std::forward<Parameter>(par)...);
	}

	//template<typename name>
}




/*
export{
	namespace Potato
	{
		template<typename InterfaceT, typename StorageT>
		struct ExceptionTuple : public InterfaceT, public StorageT
		{
			ExceptionTuple(StorageT Info) : StorageT(std::move(Info)) {}
			ExceptionTuple(InterfaceT Interface, StorageT Storage) : InterfaceT(std::move(Interface)), StorageT(std::move(Storage)) {}
			ExceptionTuple(ExceptionTuple const&) = default;
			ExceptionTuple(ExceptionTuple&&) = default;
		};

		template<typename InterfaceT, typename StorageT>
		auto MakeExceptionTuple(StorageT&& storage)->ExceptionTuple<InterfaceT, std::remove_cvref_t<StorageT>>
		{
			return { std::forward<StorageT>(storage) };
		};

		template<typename InterfaceT, typename StorageT>
		auto MakeExceptionTuple(InterfaceT&& inter, StorageT&& storage)->ExceptionTuple<std::remove_cvref_t<InterfaceT>, std::remove_cvref_t<StorageT>>
		{
			return { std::forward<InterfaceT>(inter), std::forward<StorageT>(storage) };
		};
	}
}
*/