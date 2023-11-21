module;

#include <cassert>

export module PotatoPointer;

import std;
import PotatoMisc;




export namespace Potato::Pointer
{

	template<typename WrapperT, typename ...Par>
	concept EnableEqual = requires(WrapperT T, Par...P)
	{
		{T.Equal(P...)};
	};

	template<typename WrapperT, typename ...Par>
	concept EnableAvailable = requires(WrapperT T, Par...P)
	{
		{T.Available(P...)};
	};

	template<typename WrapperT, typename ...Par>
	concept ForbidGetPointer = requires(WrapperT T)
	{
		{WrapperT::ForbidGetPointerT};
	};

	template<typename PtrT>
	struct PointerHolderT
	{
		PtrT* Ptr = nullptr;
	};

	template<typename PtrT, typename WrapperT>
	struct SmartPtr : protected PointerHolderT<PtrT>, public WrapperT
	{
		static_assert(!std::is_reference_v<PtrT>, "SmartPtr : Type should not be reference Type");

		using Type = PtrT;

		template<typename PtrT2, typename ...AppendT>
		SmartPtr(PtrT2* InputPtr, AppendT&& ...Append)
			requires(std::is_convertible_v<PtrT2*, PtrT*> && std::is_constructible_v<WrapperT, PtrT*&, PtrT2*&, AppendT&&...>)
		: PointerHolderT<PtrT>{}, WrapperT(this->Ptr, static_cast<PtrT*>(InputPtr), std::forward<AppendT>(Append)...)
		{}

		template<typename ...AppendT>
		SmartPtr(PtrT* InputPtr, AppendT&& ...Append)
			requires(std::is_constructible_v<WrapperT, PtrT*&, PtrT*&, AppendT&&...>)
		: PointerHolderT<PtrT>{}, WrapperT(this->Ptr, InputPtr, std::forward<AppendT>(Append)...)
		{}

		template<typename PtrT2, typename WrapperT2, typename ...AppendT>
		SmartPtr(SmartPtr<PtrT2, WrapperT2> const& SPtr, AppendT&& ...Append)
			requires(std::is_constructible_v<WrapperT, PtrT*&, PtrT2*&, WrapperT2 const&, AppendT&&...>)
		: PointerHolderT<PtrT>{}, WrapperT(this->Ptr, SPtr.Ptr, static_cast<WrapperT2 const&>(SPtr), std::forward<AppendT>(Append)...)
		{}

		SmartPtr(SmartPtr const& SPtr)
			requires(std::is_constructible_v<WrapperT, PtrT*&, PtrT*&, WrapperT const&>)
		: PointerHolderT<PtrT>{}, WrapperT(this->Ptr, SPtr.Ptr, static_cast<WrapperT const&>(SPtr))
		{}

		template<typename PtrT2, typename WrapperT2, typename ...AppendT>
		SmartPtr(SmartPtr<PtrT2, WrapperT2>&& SPtr, AppendT&& ...Append)
			requires(std::is_constructible_v<WrapperT, PtrT*&, PtrT2*&, WrapperT2&&, AppendT&&...>)
		: PointerHolderT<PtrT>{}, WrapperT(this->Ptr, SPtr.Ptr, static_cast<WrapperT2&&>(SPtr), std::forward<AppendT>(Append)...)
		{}

		SmartPtr(SmartPtr&& SPtr)
			requires(std::is_constructible_v<WrapperT, PtrT*&, PtrT*&, WrapperT&&>)
		: PointerHolderT<PtrT>{}, WrapperT(this->Ptr, SPtr.Ptr, static_cast<WrapperT&&>(SPtr))
		{}

		SmartPtr() requires(std::is_constructible_v<WrapperT>) {};

		void Reset() {
			WrapperT::Clear(this->Ptr);
		}

		~SmartPtr() {
			Reset();
		}

		SmartPtr& operator=(SmartPtr const& SPtr) requires(EnableEqual<WrapperT, PtrT*&, PtrT*&, WrapperT const&>)
		{
			Reset();
			WrapperT::Equal(this->Ptr, SPtr.Ptr, static_cast<WrapperT const&>(SPtr));
			return *this;
		}

		SmartPtr& operator=(SmartPtr&& SPtr) requires(EnableEqual<WrapperT, PtrT*&, PtrT*&, WrapperT&&>)
		{
			Reset();
			WrapperT::Equal(this->Ptr, SPtr.Ptr, static_cast<WrapperT&&>(SPtr));
			return *this;
		}

		constexpr operator bool() const {
			if constexpr (EnableAvailable<WrapperT const&, PtrT*>)
				return WrapperT::Available(this->Ptr);
			else
				return this->Ptr != nullptr;
		}

		decltype(auto) GetPointer() requires(!ForbidGetPointer<WrapperT>) { return this->Ptr; }
		decltype(auto) GetPointer() const requires(!ForbidGetPointer<WrapperT>) { return this->Ptr; }
		constexpr decltype(auto) operator->() requires(!ForbidGetPointer<WrapperT>) { return this->Ptr; }
		constexpr decltype(auto) operator->() const requires(!ForbidGetPointer<WrapperT>) { return this->Ptr; }
		constexpr decltype(auto) operator*() requires(!ForbidGetPointer<WrapperT>) { return *this->Ptr; }
		constexpr decltype(auto) operator*() const requires(!ForbidGetPointer<WrapperT>) { return *this->Ptr; }

		template<typename P, typename W>
		friend struct SmartPtr;
	};

	template<typename SubWrapperT>
	struct SmartPtrDefaultWrapper : public SubWrapperT
	{
		template<typename PtrT>
		SmartPtrDefaultWrapper(PtrT*& Ptr, PtrT* Input)
		{
			Ptr = Input;
			if (Ptr != nullptr)
			{
				SubWrapperT::AddRef(Ptr);
			}
		};

		template<typename PtrT>
		explicit SmartPtrDefaultWrapper(PtrT*& Ptr, PtrT* InputPtr, SmartPtrDefaultWrapper const& SW)
			: SmartPtrDefaultWrapper(Ptr, InputPtr)
		{};

		template<typename PtrT>
		explicit SmartPtrDefaultWrapper(PtrT*& Ptr, PtrT*& InputPtr, SmartPtrDefaultWrapper&& SW)
		{
			Ptr = InputPtr;
			InputPtr = nullptr;
		};

		template<typename PtrT>
		void Clear(PtrT*& Ptr) {
			if (Ptr != nullptr)
			{
				SubWrapperT::SubRef(Ptr);
				Ptr = nullptr;
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr) {
			Ptr = InputPtr;
			if(Ptr != nullptr)
				SubWrapperT::AddRef(Ptr);
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr, SmartPtrDefaultWrapper const& SW) {
			Ptr = InputPtr;
			if (Ptr != nullptr) {
				SubWrapperT::AddRef(Ptr);
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT*& InputPtr, SmartPtrDefaultWrapper&& SW) {
			Ptr = InputPtr;
			InputPtr = nullptr;
		}

		SmartPtrDefaultWrapper() = default;
	};

	struct IntrusiveSubWrapperT {
		template<typename PtrT>
		void AddRef(PtrT* Ptr) { Ptr->AddRef(); }
		template<typename PtrT>
		void SubRef(PtrT* Ptr) { Ptr->SubRef(); }
	};

	struct DefaultIntrusiveInterface
	{
	protected:

		void AddRef() const { SRefCount.AddRef(); }
		void SubRef() const { if (SRefCount.SubRef()) const_cast<DefaultIntrusiveInterface*>(this)->Release(); }
		virtual void Release() = 0;
		mutable Potato::Misc::AtomicRefCount SRefCount;

		friend struct IntrusiveSubWrapperT;
	};

	struct ObserverSubWrapperT {
		template<typename PtrT>
		void AddRef(PtrT* Ptr) {}

		template<typename PtrT>
		void SubRef(PtrT* Ptr) {}
	};
	
	template<typename PtrT>
	using IntrusivePtr = SmartPtr<PtrT, SmartPtrDefaultWrapper<IntrusiveSubWrapperT>>;

	template<typename PtrT>
	using ObserverPtr = SmartPtr<PtrT, SmartPtrDefaultWrapper<ObserverSubWrapperT>>;

	template<typename SubWrapperT>
	struct UniqueDefaultWrapper : public SubWrapperT
	{
		template<typename PtrT>
		UniqueDefaultWrapper(PtrT*& Ptr, PtrT* Input)
		{
			Ptr = Input;
		};

		template<typename PtrT>
		explicit UniqueDefaultWrapper(PtrT*& Ptr, PtrT* InputPtr, UniqueDefaultWrapper const& SW) = delete;

		template<typename PtrT>
		explicit UniqueDefaultWrapper(PtrT*& Ptr, PtrT*& InputPtr, UniqueDefaultWrapper&& SW)
		{
			Ptr = InputPtr;
			InputPtr = nullptr;
		};

		template<typename PtrT>
		void Clear(PtrT*& Ptr) {
			if (Ptr != nullptr)
			{
				SubWrapperT::SubRef(Ptr);
				Ptr = nullptr;
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr) {
			Ptr = InputPtr;
			if (Ptr != nullptr)
				SubWrapperT::AddRef(Ptr);
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr, UniqueDefaultWrapper const& SW) = delete;

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT*& InputPtr, UniqueDefaultWrapper&& SW) {
			Ptr = InputPtr;
			InputPtr = nullptr;
		}

		UniqueDefaultWrapper() = default;
	};

	struct UniqueSubWrapperT {
		template<typename PtrT>
		void AddRef(PtrT* Ptr) {}

		template<typename PtrT>
		void SubRef(PtrT* Ptr) { Ptr->Release(); }
	};

	template<typename PtrT, typename SubWrapper = UniqueSubWrapperT>
	using UniquePtr = SmartPtr<PtrT, UniqueDefaultWrapper<SubWrapper>>;

	template<typename SubWrapperT>
	struct WeakPtrWrapperT;

	template<typename SubWrapperT>
	struct StrongPtrWrapperT : public SubWrapperT
	{
		template<typename PtrT>
		StrongPtrWrapperT(PtrT*& Ptr, PtrT* Input) {
			Ptr = Input;
			if (Ptr != nullptr)
			{
				SubWrapperT::AddWeakRef(Ptr);
				SubWrapperT::AddStrongRef(Ptr);
			}
		};

		template<typename PtrT>
		explicit StrongPtrWrapperT(PtrT*& Ptr, PtrT* InputPtr, StrongPtrWrapperT const&)
		{
			Ptr = InputPtr;
			if (Ptr != nullptr) {
				SubWrapperT::AddWeakRef(Ptr);
				SubWrapperT::AddStrongRef(Ptr);
			}
		};

		template<typename PtrT>
		explicit StrongPtrWrapperT(PtrT*& Ptr, PtrT*& InputPtr, StrongPtrWrapperT&&)
		{
			Ptr = InputPtr;
			InputPtr = nullptr;
		};

		template<typename PtrT>
		explicit StrongPtrWrapperT(PtrT*& Ptr, PtrT* InputPtr, WeakPtrWrapperT<SubWrapperT> const& OWra);

		template<typename PtrT>
		void Clear(PtrT*& Ptr) {
			if (Ptr != nullptr)
			{
				SubWrapperT::SubStrongRef(Ptr);
				SubWrapperT::SubWeakRef(Ptr);
				Ptr = nullptr;
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr) {
			Ptr = InputPtr;
			if (Ptr != nullptr) {
				SubWrapperT::AddWeakRef(Ptr);
				SubWrapperT::AddStrongRef(Ptr);
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr, StrongPtrWrapperT const&) {
			Ptr = InputPtr;
			if (Ptr != nullptr) {
				SubWrapperT::AddWeakRef(Ptr);
				SubWrapperT::AddStrongRef(Ptr);
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT*& InputPtr, StrongPtrWrapperT&&) {
			Ptr = InputPtr;
			InputPtr = nullptr;
		}

		StrongPtrWrapperT() = default;
	};

	template<typename SubWrapperT>
	struct WeakPtrWrapperT : public SubWrapperT
	{

		template<typename PtrT>
		explicit WeakPtrWrapperT(PtrT*& Ptr, PtrT* InputPtr, WeakPtrWrapperT const&)
		{
			if (InputPtr != nullptr)
			{
				Ptr = InputPtr;
				SubWrapperT::AddWeakRef(Ptr);
			}
		};

		template<typename PtrT>
		explicit WeakPtrWrapperT(PtrT*& Ptr, PtrT*& InputPtr, WeakPtrWrapperT&&)
		{
			Ptr = InputPtr;
			InputPtr = nullptr;
		};

		template<typename PtrT>
		explicit WeakPtrWrapperT(PtrT*& Ptr, PtrT* InputPtr, StrongPtrWrapperT<SubWrapperT> const& OWra)
		{
			if (InputPtr != nullptr)
			{
				Ptr = InputPtr;
				SubWrapperT::AddWeakRef(Ptr);
			}
		};

		template<typename PtrT>
		void Clear(PtrT*& Ptr) {
			if (Ptr != nullptr)
			{
				SubWrapperT::SubWeakRef(Ptr);
				Ptr = nullptr;
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT* InputPtr, WeakPtrWrapperT const&) {
			Ptr = InputPtr;
			if (Ptr != nullptr) {
				SubWrapperT::AddWeakRef(Ptr);
			}
		}

		template<typename PtrT>
		void Equal(PtrT*& Ptr, PtrT*& InputPtr, WeakPtrWrapperT&&) {
			Ptr = InputPtr;
			InputPtr = nullptr;
		}

		WeakPtrWrapperT() = default;
	};

	template<typename SubWrapperT>
	template<typename PtrT>
	StrongPtrWrapperT<SubWrapperT>::StrongPtrWrapperT(PtrT*& Ptr, PtrT* InputPtr, WeakPtrWrapperT<SubWrapperT> const& OWra)
	{
		if (InputPtr != nullptr && SubWrapperT::TryAddStrongRef(InputPtr))
		{
			Ptr = InputPtr;
			SubWrapperT::AddWeakRef(Ptr);
		}
	}

	struct SWSubWrapperT
	{
		template<typename PtrT>
		void AddWeakRef(PtrT* P) { P->AddWeakRef(); }
		template<typename PtrT>
		void SubWeakRef(PtrT* P) { P->SubWeakRef(); }
		template<typename PtrT>
		void AddStrongRef(PtrT* P) { P->AddStrongRef(); }
		template<typename PtrT>
		void SubStrongRef(PtrT* P) { P->SubStrongRef(); }
		template<typename PtrT>
		bool TryAddStrongRef(PtrT* P) { return P->TryAddStrongRef(); }
	};

	struct DefaultStrongWeakInterface
	{
	protected:

		void AddWeakRef() const {  WRefCount.AddRef(); }
		void SubWeakRef() const { if(WRefCount.SubRef()) const_cast<DefaultStrongWeakInterface*>(this)->WeakRelease(); }
		void AddStrongRef() const { SRefCount.AddRef(); }
		void SubStrongRef() const { if(SRefCount.SubRef()) const_cast<DefaultStrongWeakInterface*>(this)->StrongRelease(); }
		bool TryAddStrongRef() const { return SRefCount.TryAddRefNotFromZero(); }

		virtual void WeakRelease() = 0;
		virtual void StrongRelease() = 0;

		mutable Potato::Misc::AtomicRefCount SRefCount;
		mutable Potato::Misc::AtomicRefCount WRefCount;

		friend struct SWSubWrapperT;
	};

	template<typename PtrT, typename SubWrapperT = SWSubWrapperT>
	using StrongPtr = SmartPtr<PtrT, StrongPtrWrapperT<SubWrapperT>>;

	template<typename PtrT, typename SubWrapperT = SWSubWrapperT>
	using WeakPtr = SmartPtr<PtrT, WeakPtrWrapperT<SubWrapperT>>;
}