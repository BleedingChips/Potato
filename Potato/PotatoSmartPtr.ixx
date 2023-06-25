module;

export module Potato.SmartPtr;

export import Potato.Misc;
export import Potato.STD;


namespace Potato::Misc
{
	template<typename T, typename PtrT, typename = decltype(std::declval<T>().IsAvailable(std::declval<PtrT*>()))> struct WrapperIsAvailableRole {};
	template<typename T, typename PtrT, typename = decltype(std::declval<T>().Upgrade(std::declval<PtrT*>()))> struct WrapperUpgradeRole {};
	template<typename T, typename PtrT, typename = decltype(std::declval<T>().Downgrade(std::declval<PtrT*>()))> struct WrapperDowngradeRole {};

	template<typename T, typename PtrT, typename = decltype(std::declval<T>().EqualMove(std::declval<PtrT*>(), std::declval<T&&>()))> struct WrapperEqualMoveRole {};
	template<typename T, typename PtrT, typename = decltype(std::declval<T>().Equal(std::declval<PtrT*>(), std::declval<T const&>()))> struct WrapperEqualRole {};
	template<typename T, typename PtrT, typename = decltype(std::declval<T>().EqualPointer(std::declval<PtrT*>()))> struct WrapperEqualPointerRole {};

	template<typename T, typename PtrT, typename = decltype(std::declval<T>().Reset(std::declval<PtrT*>()))> struct WrapperResetRole {};

	template<typename T, typename = typename T::ForbidPtrT> struct WrapperForbidPtrRole{};
	template<typename T, typename = typename T::RequireExplicitPointConstructT> struct WrapperRequireExplicitPointConstructPtrRole {};
}


export namespace Potato::Misc
{

	struct SmartPtrIgnoreAddReference
	{

	};

	template<typename PtrT, typename WrapperT>
	struct SmartPtr : public WrapperT
	{
		static_assert(!std::is_reference_v<PtrT>, "SmartPtr : Type should not be reference Type");
		static_assert(TMP::Exist<WrapperT, WrapperResetRole, PtrT>::Value, "Wrapper Need Define void XX::Reset(PtrT*) MemberFunction");

		constexpr SmartPtr() requires(std::is_constructible_v<WrapperT>) : Ptr(nullptr), WrapperT() {}
		
		template<typename ...AT>
		SmartPtr(PtrT* IPtr, AT&& ...at) requires(!TMP::Exist<WrapperT, WrapperRequireExplicitPointConstructPtrRole> ::Value && std::is_constructible_v<WrapperT, PtrT*, AT...>)
			: Ptr(nullptr), WrapperT(IPtr, std::forward<AT>(at)...)
		{
			Ptr = IPtr;
		}

		template<typename ...AT>
		explicit SmartPtr(PtrT* IPtr, AT&& ...at) requires(TMP::Exist<WrapperT, WrapperRequireExplicitPointConstructPtrRole> ::Value && std::is_constructible_v<WrapperT, PtrT*, AT...>)
			: Ptr(nullptr), WrapperT(IPtr, std::forward<AT>(at)...)
		{
			Ptr = IPtr;
		}

		template<typename PtrT2, typename ...AT>
		constexpr SmartPtr(PtrT2* IPtr, AT&& ...at) requires(!TMP::Exist<WrapperT, WrapperRequireExplicitPointConstructPtrRole> ::Value && std::is_constructible_v<PtrT*, PtrT2*>&& std::is_constructible_v<WrapperT, PtrT*, AT...>)
			: SmartPtr(static_cast<PtrT*>(IPtr), std::forward<AT>(at)...)
		{
		}

		template<typename PtrT2, typename ...AT>
		explicit constexpr SmartPtr(PtrT2* IPtr, AT&& ...at) requires(TMP::Exist<WrapperT, WrapperRequireExplicitPointConstructPtrRole> ::Value && std::is_constructible_v<PtrT*, PtrT2*> && std::is_constructible_v<WrapperT, PtrT*, AT...>)
			: SmartPtr(static_cast<PtrT*>(IPtr), std::forward<AT>(at)...)
		{
		}

		constexpr SmartPtr(SmartPtr const& IPtr) requires(std::is_constructible_v<WrapperT, PtrT*, WrapperT const&>)
			: SmartPtr(IPtr.Ptr, static_cast<WrapperT const&>(IPtr)) {}

		constexpr SmartPtr(SmartPtr&& IPtr) requires(std::is_constructible_v<WrapperT, PtrT*, WrapperT &&>)
			: Ptr(nullptr), WrapperT(IPtr.Ptr, static_cast<WrapperT&&>(IPtr))
		{
			Ptr = IPtr.Ptr;
			IPtr.Ptr = nullptr;
		}

		template<typename PtrT2, typename WrapperT2>
		constexpr SmartPtr(SmartPtr<PtrT2, WrapperT2> const& IPtr)
			requires(std::is_constructible_v<PtrT*, PtrT2*> && std::is_constructible_v<WrapperT, PtrT*, WrapperT2 const&>)
			: SmartPtr(static_cast<PtrT*>(IPtr.Ptr), static_cast<WrapperT2 const&>(IPtr))
		{
		};

		constexpr SmartPtr& operator=(SmartPtr const& IPtr)
			requires(TMP::Exist<WrapperT, WrapperEqualRole, PtrT>::Value)
		{
			Reset();
			Ptr = IPtr.Ptr;
			WrapperT::Equal(Ptr, static_cast<WrapperT const&>(IPtr));
			return *this;
		};

		constexpr SmartPtr& operator=(SmartPtr && IPtr)
			requires(TMP::Exist<WrapperT, WrapperEqualMoveRole, PtrT>::Value)
		{
			Reset();
			Ptr = IPtr.Ptr;
			WrapperT::EqualMove(IPtr.Ptr, static_cast<WrapperT &&>(IPtr));
			IPtr.Ptr = nullptr;
			return *this;
		};

		constexpr SmartPtr& operator=(PtrT* IPtr)
			requires(TMP::Exist<WrapperT, WrapperEqualMoveRole, PtrT>::Value)
		{
			Reset();
			WrapperT::EqualPointer(IPtr);
			Ptr = IPtr.Ptr;
			return *this;
		};

		/*
		constexpr SmartPtr& operator=(SmartPtr const& IPtr) requires(
			std::is_invocable_v<decltype(&WrapperT::Equal), WrapperT&, PtrT*, WrapperT const&>
			)
		{
			Reset();
			Ptr = IPtr.Ptr;
			WrapperT::Equal(Ptr, static_cast<WrapperT const&>(IPtr));
			return *this;
		};
		*/

		/*
		constexpr SmartPtr& operator=(PtrT* IPtr) requires(
			std::is_invocable_v<decltype(&WrapperT::EqualPointer), WrapperT&, PtrT*>
			)
		{
			Reset();
			Ptr = IPtr.Ptr;
			WrapperT::EqualPointer(Ptr);
			return *this;
		};
		*/

		constexpr std::strong_ordering operator<=>(SmartPtr const& IPtr) const { return Ptr <=> IPtr.Ptr; }

		constexpr ~SmartPtr() { Reset(); }

		constexpr void Reset() {
			WrapperT::Reset(Ptr);
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
		
		constexpr decltype(auto) Upgrade() requires(TMP::Exist<WrapperT, WrapperUpgradeRole, PtrT>::Value) 
		{
			return WrapperT::Upgrade(Ptr);
		}

		constexpr decltype(auto) Downgrade() const requires(TMP::Exist<WrapperT, WrapperDowngradeRole, PtrT>::Value)
		{
			return WrapperT::Downgrade(Ptr);
		}

	protected:

		PtrT* Ptr = nullptr;

		template<typename PT, typename WT>
		friend struct SmartPtr;
	};

	// intrustive_ptr ***********************************************************
	struct IntrusivePtrDefaultWrapper
	{
		IntrusivePtrDefaultWrapper() = default;
		template<typename PtrT>
		IntrusivePtrDefaultWrapper(PtrT* IPtr) { if(IPtr != nullptr) IPtr->AddRef(); }
		template<typename PtrT>
		IntrusivePtrDefaultWrapper(PtrT* IPtr, IntrusivePtrDefaultWrapper const&) : IntrusivePtrDefaultWrapper(IPtr) {}
		template<typename PtrT>
		IntrusivePtrDefaultWrapper(PtrT* IPtr, IntrusivePtrDefaultWrapper &&) {}
		template<typename PtrT>
		void Equal(PtrT* IPtr, IntrusivePtrDefaultWrapper) { if(IPtr != nullptr) IPtr->AddRef(); }
		template<typename PtrT>
		void Reset(PtrT* IPtr) { if(IPtr != nullptr) IPtr->SubRef(); }
	};

	template<typename Type, typename Wrapper = IntrusivePtrDefaultWrapper>
	using IntrusivePtr = SmartPtr<std::remove_reference_t<Type>, Wrapper>;

	// observer_ptr ****************************************************************************

	struct ObserverPtrDefaultWrapper
	{
		ObserverPtrDefaultWrapper() = default;
		template<typename PtrT>
		ObserverPtrDefaultWrapper(PtrT* IPtr) {}
		template<typename PtrT>
		ObserverPtrDefaultWrapper(PtrT* IPtr, ObserverPtrDefaultWrapper const&) {}
		template<typename PtrT>
		ObserverPtrDefaultWrapper(PtrT* IPtr, ObserverPtrDefaultWrapper&&) {}
		template<typename PtrT>
		void Equal(PtrT* IPtr, ObserverPtrDefaultWrapper const&) {}
		template<typename PtrT>
		void Reset(PtrT* IPtr) {}
	};

	template<typename Type> 
	using ObserverPtr = IntrusivePtr<Type, ObserverPtrDefaultWrapper>;

	struct UniquePtrDefaultWrapper
	{
		UniquePtrDefaultWrapper() = default;
		template<typename PtrT>
		UniquePtrDefaultWrapper(PtrT* IPtr) {}
		template<typename PtrT>
		UniquePtrDefaultWrapper(PtrT* IPtr, UniquePtrDefaultWrapper&&) {}
		template<typename PtrT>
		void Reset(PtrT* IPtr) { IPtr->Release(); }
	};

	template<typename Type>
	using UniquePtr = IntrusivePtr<Type, UniquePtrDefaultWrapper>;

}