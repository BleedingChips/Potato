export module Potato.SmartPtr;

export import Potato.TMP;
export import Potato.STD;
export import Potato.Misc;

namespace Potato::Misc
{
	template<typename T, typename = decltype(T::Available)> struct WrapperAvailableRole{};
	template<typename T, typename = typename T::ForbidPtrT> struct ForbidPtrRole{};
}


export namespace Potato::Misc
{

	template<typename PtrT, typename WrapperT>
	struct SmartPtr
	{
		static_assert(!std::is_reference_v<PtrT>, "SmartPtr : Type should not be reference Type");

		constexpr SmartPtr() = default;
		constexpr SmartPtr(PtrT* IPtr) : Ptr(IPtr) { if(Ptr != nullptr) WrapperT::AddRef(Ptr); }
		constexpr SmartPtr(SmartPtr const& IPtr) requires(std::is_constructible_v<WrapperT, WrapperT const&>) : SmartPtr(IPtr.Ptr) { }
		constexpr SmartPtr(SmartPtr&& IPtr) requires(std::is_constructible_v<WrapperT, WrapperT &&>) : Ptr(std::move(IPtr.Ptr)) { if(Ptr != nullptr) WrapperT::AddRef(Ptr); }

		template<typename WrapperT2>
		constexpr SmartPtr(SmartPtr<PtrT, WrapperT2> const& IPtr)
			requires(std::is_constructible_v<WrapperT, WrapperT2 const&>)
		{
			if (IPtr.GetPointer() != nullptr)
			{
				Ptr = IPtr.GetPointer();
				WrapperT::AddRef(Ptr);
			}
		};

		constexpr SmartPtr& operator=(SmartPtr IPtr) { Reset(); Ptr = IPtr.Ptr; IPtr.Ptr = nullptr; return *this; };
		constexpr std::strong_ordering operator<=>(SmartPtr const& IPtr) const { return Ptr <=> IPtr.Ptr; }

		decltype(auto) GetPointer() requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return Ptr; }
		decltype(auto) GetPointer() const requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return Ptr; }

		constexpr ~SmartPtr() { Reset(); }

		constexpr void Reset() { 
			if (Ptr != nullptr)
			{
				WrapperT::SubRef(Ptr);
				Ptr = nullptr;
			}
		}

		constexpr operator bool() const {
			if (Ptr != nullptr)
			{
				if constexpr (TMP::Exist<WrapperT, WrapperAvailableRole>::Value)
				{
					return WrapperT::Available(Ptr);
				}
				return true;
			}
			return false;
		}

		constexpr decltype(auto) operator->() requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator->() const requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator*() requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value)  { return *Ptr; }
		constexpr decltype(auto) operator*() const requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return *Ptr; }
		constexpr decltype(auto) Switch() requires(WrapperT::SwitchT) { return SmartPtr<PtrT, typename WrapperT::SwitchT>{*this}; }
		constexpr decltype(auto) Switch() const requires(WrapperT::SwitchT) { return SmartPtr<PtrT, typename WrapperT::SwitchT>{*this}; }

	protected:

		PtrT* Ptr = nullptr;

		template<typename PT, typename WT>
		friend struct SmartPtr;
	};

	template<typename PtrT, typename ReferenceT, typename PtrWrapperT, typename WrapperT>
	struct AppendReferenceSmartPtr
	{
		static_assert(!std::is_reference_v<PtrT>, "AppendReferenceSmartPtr : Type should not be reference Type");

		constexpr AppendReferenceSmartPtr() = default;
		constexpr AppendReferenceSmartPtr(PtrT* IPtr, ReferenceT* IRef) : Ptr(IPtr), RefPtr(IRef) { if (IRef != nullptr) WrapperT::AddRef(IRef); }
		constexpr AppendReferenceSmartPtr(AppendReferenceSmartPtr const& IPtr)
			requires(std::is_constructible_v<PtrWrapperT, PtrWrapperT const&> && std::is_constructible_v<WrapperT, WrapperT const&>)
			: AppendReferenceSmartPtr(IPtr.Ptr, IPtr.RefPtr) {}
		constexpr AppendReferenceSmartPtr(AppendReferenceSmartPtr&& IPtr) : Ptr(IPtr), RefPtr(IPtr.RefPtr) { IPtr.Ptr = nullptr; IPtr.RefPtr = nullptr; }

		template<typename PtrWrapperT2, typename WrapperT2>
		constexpr AppendReferenceSmartPtr(AppendReferenceSmartPtr<PtrT, ReferenceT, PtrWrapperT2, WrapperT2> const& IPtr)
			requires(std::is_constructible_v<PtrWrapperT, PtrWrapperT2 const&>&& std::is_constructible_v<WrapperT, WrapperT2 const&>)
		{
			if (IPtr.RefPtr != nullptr)
			{
				Ptr = IPtr.Ptr;
				RefPtr = IPtr.RefPtr;
				WrapperT::AddRef(RefPtr);
			}
		};

		constexpr AppendReferenceSmartPtr& operator=(AppendReferenceSmartPtr IPtr) {
			Reset();
			Ptr = IPtr.Ptr;
			RefPtr = IPtr.RefPtr;
			IPtr.Ptr = nullptr;
			IPtr.RefPtr = nullptr;
			return *this;
		};

		constexpr std::strong_ordering operator<=>(AppendReferenceSmartPtr const& IPtr) const {
			auto Re = (Ptr <=> IPtr.Ptr);
			return (Re == std::strong_ordering::equal) ? Re : RefPtr <=> IPtr.RefPtr;
		}

		decltype(auto) GetPointer() { return Ptr; }
		decltype(auto) GetPointer() const { return Ptr; }

		constexpr ~AppendReferenceSmartPtr() { Reset(); }

		constexpr void Reset() {
			if (RefPtr != nullptr)
			{
				if (WrapperT::SubRef(RefPtr) && Ptr != nullptr)
					PtrWrapperT::Release(Ptr);
				RefPtr = nullptr;
				Ptr = nullptr;
			}
		}

		constexpr operator bool() const { 
			if (RefPtr != nullptr)
			{
				if constexpr (TMP::Exist<WrapperT, WrapperAvailableRole>::Value)
				{
					if(!WrapperT::Available(RefPtr))
						return false;
				}
				if (Ptr != nullptr)
				{
					if constexpr (TMP::Exist<PtrWrapperT, WrapperAvailableRole>::Value)
					{
						if (!PtrWrapperT::Available(Ptr))
							return false;
					}
					return true;
				}
			}
			return false;
			
		}
		constexpr decltype(auto) operator->() requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator->() const requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator*() requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return *Ptr; }
		constexpr decltype(auto) operator*() const requires(!TMP::Exist<WrapperT, ForbidPtrRole>::Value) { return *Ptr; }

	protected:

		PtrT* Ptr = nullptr;
		ReferenceT* RefPtr = nullptr;

		template<typename PtrT2, typename ReferenceT2, typename PtrWrapperT2, typename WrapperT2>
		friend struct AppendReferenceSmartPtr;
	};


	template<typename DriverT>
	struct DefaultIntrusiveObjectInterface
	{
		void AddRef() { RefCount.AddRef(); }
		void SubRef() { if(RefCount.SubRef()) static_cast<DriverT*>(this)->Release(); }
	protected:
		AtomicRefCount RefCount;
	};

	// intrustive_ptr ***********************************************************
	struct IntrusivePtrDefaultWrapper
	{
		template<typename Type>
		static void AddRef(Type* t) noexcept { t->AddRef(); }
		template<typename Type>
		static void SubRef(Type* t) noexcept { t->SubRef(); }
		IntrusivePtrDefaultWrapper(IntrusivePtrDefaultWrapper const&) = default;
		IntrusivePtrDefaultWrapper(IntrusivePtrDefaultWrapper &&) = default;
		IntrusivePtrDefaultWrapper() = default;
	};

	template<typename Type, typename Wrapper = IntrusivePtrDefaultWrapper>
	using IntrusivePtr = SmartPtr<std::remove_reference_t<Type>, Wrapper>;

	// observer_ptr ****************************************************************************

	struct ObserverPtrDefaultWrapper
	{
		template<typename Type>
		static void AddRef(Type *t) { }
		template<typename Type>
		static void SubRef(Type* t) { }
		ObserverPtrDefaultWrapper(ObserverPtrDefaultWrapper const&) = default;
		ObserverPtrDefaultWrapper(ObserverPtrDefaultWrapper&&) = default;
		ObserverPtrDefaultWrapper() = default;
	};

	template<typename Type> 
	using ObserverPtr = IntrusivePtr<Type, ObserverPtrDefaultWrapper>;

	struct UniquePtrDefaultWrapper
	{
		template<typename Type>
		static void AddRef(Type* t) { }
		template<typename Type>
		static void SubRef(Type* t) { t->Release(); }
		UniquePtrDefaultWrapper(UniquePtrDefaultWrapper const&) = delete;
		UniquePtrDefaultWrapper(UniquePtrDefaultWrapper&&) = default;
		UniquePtrDefaultWrapper() = default;
	};

	template<typename Type>
	using UniquePtr = IntrusivePtr<Type, UniquePtrDefaultWrapper>;

	struct WeakPtrDefaultWrapper;

	struct DefaultPtrWrapper
	{
		template<typename PtrT>
		static void Release(PtrT* Ptr) { Ptr->Release(); }
	};

	struct StrongPtrDefaultWrapper
	{
		template<typename ReferencePtrT>
		static void AddRef(ReferencePtrT* t) { t->AddStrongRef(); }

		template<typename ReferencePtrT>
		static bool SubRef(ReferencePtrT* t) { return t->SubStrongRef(); }

		template<typename ReferencePtrT>
		static bool Available(ReferencePtrT const* t) noexcept { return t->StrongAvailable(); }

		StrongPtrDefaultWrapper(WeakPtrDefaultWrapper const&) {}
	};

	template<typename PtrT, typename ReferenceT, typename PtrWrapperT = DefaultPtrWrapper, typename WrapperT = StrongPtrDefaultWrapper>
	using StrongPtr = AppendReferenceSmartPtr<
		std::remove_reference_t<PtrT>,
		std::remove_reference_t<ReferenceT>,
		PtrWrapperT
	,WrapperT>;

	struct WeakPtrDefaultWrapper
	{
		template<typename ReferencePtrT>
		static void AddRef(ReferencePtrT* t) { t->AddWeakRef(); }

		template<typename ReferencePtrT>
		static bool SubRef(ReferencePtrT* t) { t->SubWeakRef(); return false; }

		using ForbidPtrT = void;

		WeakPtrDefaultWrapper(WeakPtrDefaultWrapper const&) = default;
		WeakPtrDefaultWrapper(WeakPtrDefaultWrapper&&) = default;
		WeakPtrDefaultWrapper(StrongPtrDefaultWrapper const&) {}
	};

	//inline StrongPtrDefaultWrapper::operator WeakPtrDefaultWrapper(){ return {}; }

	template<typename PtrT, typename ReferenceT, typename PtrWrapperT = DefaultPtrWrapper, typename WrapperT = WeakPtrDefaultWrapper>
	using WeakPtr = AppendReferenceSmartPtr<
		std::remove_reference_t<PtrT>,
		std::remove_reference_t<ReferenceT>,
		PtrWrapperT, WrapperT>;

}