#pragma once
#include <utility>
#include <type_traits>
#include <concepts>
#include <any>
#include <atomic>
#include <cassert>

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
			using RequireType = typename FunctionInfo<FunT>::template PackParameters<ItSelf>::Type;
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
			using RequireType = typename FunctionObjectInfo<FunT>::template PackParameters<ItSelf>::Type;
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
			using RequireType = typename FunctionInfo<FunT>::template PackParameters<ItSelf>::Type;
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
			using RequireType = typename FunctionObjectInfo<FunT>::template PackParameters<ItSelf>::Type;
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