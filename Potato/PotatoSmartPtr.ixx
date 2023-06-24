module;

export module Potato.SmartPtr;

export import Potato.Misc;
export import Potato.STD;


namespace Potato::Misc
{
	template<typename T, typename PtrT, typename = decltype(std::declval<T>().IsAvailable(std::declval<PtrT*>()))> struct WrapperIsAvailableRole {};
	template<typename T, typename = typename T::ForbidPtrT> struct WrapperForbidPtrRole{};
	template<typename T, typename = typename T::UpgradeT> struct WrapperUpgradeRole {};
	template<typename T, typename = typename T::DowngradeT> struct WrapperDowngradeRole {};
}


export namespace Potato::Misc
{

	template<typename PtrT, typename WrapperT>
	struct SmartPtr : public WrapperT
	{
		static_assert(!std::is_reference_v<PtrT>, "SmartPtr : Type should not be reference Type");

			
		constexpr SmartPtr() requires(!std::is_constructible_v<WrapperT, PtrT*> && std::is_constructible_v<WrapperT>) = default;
		constexpr SmartPtr() requires(std::is_constructible_v<WrapperT, PtrT*>) : Ptr(nullptr), WrapperT(static_cast<PtrT*>(nullptr)) {}

		template<typename ...AT>
		requires(std::is_constructible_v<WrapperT, PtrT*, AT...>)
		constexpr SmartPtr(PtrT* IPtr, AT&& ...at) : Ptr(IPtr), WrapperT(IPtr, std::forward<AT>(at)...)
		{
			if (*this)
			{
				WrapperT::AddRef(Ptr);
			}
		}

		template<typename ...AT>
		requires(!std::is_constructible_v<WrapperT, PtrT*, AT...> && std::is_constructible_v<WrapperT, AT...>)
		constexpr SmartPtr(PtrT* IPtr, AT&& ...at) : Ptr(IPtr), WrapperT(std::forward<AT>(at)...)
		{
			if (*this)
			{
				WrapperT::AddRef(Ptr);
			}
		}

		constexpr SmartPtr(SmartPtr const& IPtr) requires(!std::is_constructible_v<WrapperT, PtrT const*, WrapperT const&> && std::is_constructible_v<WrapperT, WrapperT const&>)
			: SmartPtr(IPtr.Ptr), WrapperT(static_cast<WrapperT const&>(IPtr))
		{
			if (*this)
			{
				WrapperT::AddRef(Ptr);
			}
		}

		constexpr SmartPtr(SmartPtr const& IPtr) requires(std::is_constructible_v<WrapperT, PtrT const*, WrapperT const&>)
			: Ptr(IPtr.Ptr), WrapperT(IPtr.Ptr, static_cast<WrapperT const&>(IPtr))
		{
			if (*this)
			{
				WrapperT::AddRef(Ptr);
			}
		}

		constexpr SmartPtr(SmartPtr&& IPtr) requires(!std::is_constructible_v<WrapperT, PtrT*, WrapperT&&> && std::is_constructible_v<WrapperT, WrapperT &&>)
			: Ptr(std::move(IPtr.Ptr)), WrapperT(static_cast<WrapperT &&>(IPtr))  
		{
			IPtr.Ptr = nullptr;
		}

		constexpr SmartPtr(SmartPtr&& IPtr) requires(std::is_constructible_v<WrapperT, PtrT*, WrapperT &&>)
			: Ptr(std::move(IPtr.Ptr)), WrapperT(IPtr.Ptr, static_cast<WrapperT&&>(IPtr))
		{
			IPtr.Ptr = nullptr;
		}

		template<typename WrapperT2>
		constexpr SmartPtr(SmartPtr<PtrT, WrapperT2> const& IPtr)
			requires(!std::is_constructible_v<WrapperT, PtrT const*, WrapperT2 const&> && std::is_constructible_v<WrapperT, WrapperT2 const&>)
			: Ptr(IPtr.GetPointer()), WrapperT(static_cast<WrapperT2 const&>(IPtr))
		{
			if (*this)
			{
				WrapperT::AddRef(Ptr);
			}
		};

		template<typename WrapperT2>
		constexpr SmartPtr(SmartPtr<PtrT, WrapperT2> const& IPtr)
			requires(std::is_constructible_v<WrapperT, PtrT const*, WrapperT2 const&>)
		: Ptr(IPtr.Ptr), WrapperT(IPtr.Ptr, static_cast<WrapperT2 const&>(IPtr))
		{
			if (*this)
			{
				WrapperT::AddRef(Ptr);
			}
		};

		constexpr SmartPtr& operator=(SmartPtr IPtr) 
		{
			this->~SmartPtr();
			new (this) SmartPtr(std::move(IPtr));
			return *this;
		};

		constexpr std::strong_ordering operator<=>(SmartPtr const& IPtr) const { return Ptr <=> IPtr.Ptr; }

		constexpr ~SmartPtr() { Reset(); }

		constexpr void Reset() {
			if (*this)
			{
				WrapperT::SubRef(Ptr);
			}
			Ptr = nullptr;
		}

		constexpr operator bool() const {
			if (Ptr != nullptr)
			{
				if constexpr (TMP::Exist<WrapperT, WrapperIsAvailableRole, PtrT>::Value)
				{
					return WrapperT::IsAvailable(Ptr);
				}
				return true;
			}
			return false;
		}

		decltype(auto) GetPointer() requires(!TMP::Exist<WrapperT, WrapperForbidPtrRole>::Value) { return Ptr; }
		decltype(auto) GetPointer() const requires(!TMP::Exist<WrapperT, WrapperForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator->() requires(!TMP::Exist<WrapperT, WrapperForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator->() const requires(!TMP::Exist<WrapperT, WrapperForbidPtrRole>::Value) { return Ptr; }
		constexpr decltype(auto) operator*() requires(!TMP::Exist<WrapperT, WrapperForbidPtrRole>::Value)  { return *Ptr; }
		constexpr decltype(auto) operator*() const requires(!TMP::Exist<WrapperT, WrapperForbidPtrRole>::Value) { return *Ptr; }
		
		constexpr decltype(auto) Upgrade() requires(TMP::Exist<WrapperT, WrapperUpgradeRole>::Value) { return SmartPtr<PtrT, typename WrapperT::UpgradeT>{*this}; }
		constexpr decltype(auto) Downgrade() const requires(TMP::Exist<WrapperT, WrapperDowngradeRole>::Value) { return SmartPtr<PtrT, typename WrapperT::DowngradeT>{*this}; }

	protected:

		PtrT* Ptr = nullptr;

		template<typename PT, typename WT>
		friend struct SmartPtr;
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
		//ObserverPtrDefaultWrapper() = default;
		template<typename Type>
		ObserverPtrDefaultWrapper(Type* Ptr) {}
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

	struct DefaultPtrWrapper
	{
		template<typename PtrT>
		static void Release(PtrT* Ptr) { Ptr->Release(); }
	};

}