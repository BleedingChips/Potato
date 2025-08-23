module;

#include <cassert>

export module PotatoPointer;

import std;
import PotatoMisc;




export namespace Potato::Pointer
{

	struct DefaultIntrusiveWrapper
	{
		template<typename PtrT>
		void AddRef(PtrT* p) { p->AddRef(); }
		template<typename PtrT>
		void SubRef(PtrT* p) { p->SubRef(); }
	};

	template<typename WrapperT, typename PtrT>
	concept HasTryAddRef = requires(WrapperT wra)
	{
		{wra.TryAddRef(std::declval<PtrT*>())} -> std::convertible_to<bool>;
	};

	template<typename WrapperT>
	concept ForbidPointerAccess = requires(WrapperT wra)
	{
		typename WrapperT::PotatoPointerForbidPointerAccess;
	};

	template<typename WrapperT>
	concept EnablePointerReferenceAccess = requires(WrapperT wra)
	{
		typename WrapperT::PotatoPointerEnablePointerAccess;
	};

	template<typename WrapperT>
	concept ForbidPointerConstruct = requires(WrapperT wra)
	{
		typename WrapperT::PotatoPointerForbidPointerConstruct;
	};

	template<typename WrapperT>
	concept HasIsomer = requires(WrapperT wra)
	{
		typename WrapperT::PotatoPointerIsomer;
	};

	template<typename PtrT, class WrapperT = DefaultIntrusiveWrapper>
	struct IntrusivePtr : protected WrapperT
	{

		using CurrentWrapper = WrapperT;

		static_assert(!std::is_reference_v<PtrT>, "SmartPtr : Type should not be reference Type");

		using Type = PtrT;

		IntrusivePtr(PtrT* ptr) requires(!ForbidPointerConstruct<WrapperT>) : ptr(ptr)
		{
			if(ptr != nullptr)
			{
				WrapperT::AddRef(ptr);
			}
		}

		template<typename PtrT2>
		IntrusivePtr(PtrT2* InputPtr)
			requires(std::is_convertible_v<PtrT2*, PtrT*> && !std::is_same_v<std::remove_cvref_t<PtrT2>, std::remove_cvref_t<PtrT>> && !ForbidPointerConstruct<WrapperT>)
		: IntrusivePtr(static_cast<PtrT*>(InputPtr)) {}

		IntrusivePtr(IntrusivePtr const& iptr)
			requires(std::is_constructible_v<WrapperT, WrapperT const&>)
		: WrapperT(static_cast<WrapperT const&>(iptr)), ptr(iptr.ptr)
		{
			if(ptr != nullptr)
			{
				WrapperT::AddRef(ptr);
			}
		}

		IntrusivePtr(IntrusivePtr && iptr)
			requires(std::is_constructible_v<WrapperT, WrapperT &&>)
			: WrapperT(static_cast<WrapperT &&>(iptr)), ptr(iptr.ptr)
		{
			iptr.ptr = nullptr;
		}

		template<typename PtrT2, typename WrapperT2>
		IntrusivePtr(IntrusivePtr<PtrT2, WrapperT2> const& iptr)
			requires(std::is_convertible_v<PtrT2*, PtrT*> && std::is_constructible_v<WrapperT, WrapperT2 const&>)
			: WrapperT(static_cast<WrapperT2 const&>(iptr)), ptr(iptr.ptr)
		{
			if(ptr != nullptr)
			{
				if constexpr (HasTryAddRef<WrapperT, PtrT>)
				{
					if (!WrapperT::TryAddRef(ptr))
					{
						ptr = nullptr;
					}
				}
				else
				{
					WrapperT::AddRef(ptr);
				}
			}
		}

		IntrusivePtr() : ptr(nullptr) {};

		void Reset() {
			if(ptr != nullptr)
			{
				WrapperT::SubRef(ptr);
				ptr = nullptr;
			}
		}

		~IntrusivePtr() {
			Reset();
		}

		IntrusivePtr& operator=(IntrusivePtr iptr)
		{
			Reset();
			WrapperT::operator=(static_cast<WrapperT &&>(iptr));
			ptr = iptr.ptr;
			iptr.ptr = nullptr;
			return *this;
		}

		template<typename Ptr2, typename Wrapper2>
		IntrusivePtr& operator=(IntrusivePtr<Ptr2, Wrapper2> const& iptr)
		{
			Reset();
			WrapperT::operator=(static_cast<Wrapper2 const&>(iptr));
			ptr = iptr.ptr;
			if(ptr != nullptr)
			{
				if constexpr (HasTryAddRef<WrapperT, PtrT>)
				{
					if (!WrapperT::TryAddRef(ptr))
					{
						ptr = nullptr;
					}
				}
				else
				{
					WrapperT::AddRef(ptr);
				}
			}
			return *this;
		}

		constexpr operator bool() const {
			return ptr != nullptr;
		}

		decltype(auto) GetPointer() const requires(!ForbidPointerAccess<WrapperT>) { return ptr; }
		PtrT*& GetPointerReference() requires(EnablePointerReferenceAccess<WrapperT>) { return ptr; }
		PtrT* const& GetPointerReference() const requires(EnablePointerReferenceAccess<WrapperT>) { return ptr; }

		constexpr decltype(auto) operator->() const requires(!ForbidPointerAccess<WrapperT>) { return ptr; }
		constexpr decltype(auto) operator*() const requires(!ForbidPointerAccess<WrapperT>) { return *ptr; }

		constexpr decltype(auto) Isomer() const requires(HasIsomer<WrapperT>)
		{
			return IntrusivePtr<PtrT, typename WrapperT::PotatoPointerIsomer>{*this};
		}

		template<typename OP, typename OW>
		std::strong_ordering operator<=>(IntrusivePtr<OP, OW> const& i) const noexcept { return ptr <=> i.ptr; }
		bool operator==(IntrusivePtr const& i) const noexcept { return this->operator<=>(i) == std::strong_ordering::equal; }
		template<typename OP, typename OW>
		bool operator!=(IntrusivePtr<OP, OW> const& i) const noexcept { return (ptr <=> i.ptr) != std::strong_ordering::equal; };

	protected:

		PtrT* ptr;

		template<typename Ptr2, typename Wrapper2>
		friend struct IntrusivePtr;
	};

	struct DefaultIntrusiveInterface
	{
	protected:

		void AddRef() const { SRefCount.AddRef(); }
		void SubRef() const { if (SRefCount.SubRef()) const_cast<DefaultIntrusiveInterface*>(this)->Release(); }

		virtual void Release() = 0;
		mutable Potato::Misc::AtomicRefCount SRefCount;

		friend struct DefaultIntrusiveWrapper;
	};


	struct ObserverSubWrapperT {
		template<typename PtrT>
		void AddRef(PtrT* Ptr) {}

		template<typename PtrT>
		void SubRef(PtrT* Ptr) {}

		template<typename T>
		ObserverSubWrapperT(T t) {}
		ObserverSubWrapperT(ObserverSubWrapperT const&) = default;
		ObserverSubWrapperT() = default;
	};

	template<typename Type> using ObserverPtr = IntrusivePtr<Type, ObserverSubWrapperT>;

	struct DefaultUniqueWrapper
	{
		template<typename PtrT>
		void AddRef(PtrT* Ptr) {}

		template<typename PtrT>
		void SubRef(PtrT* Ptr) { Ptr->Release(); }

		DefaultUniqueWrapper() = default;
		DefaultUniqueWrapper(DefaultUniqueWrapper&&) = default;
		DefaultUniqueWrapper(DefaultUniqueWrapper const&) = delete;
		DefaultUniqueWrapper& operator=(DefaultUniqueWrapper&&) = default;
	};

	template<typename PtrT>
	using UniquePtr = IntrusivePtr<PtrT, DefaultUniqueWrapper>;

	struct StrongWeakIntrusiveWrapperT
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

	struct StrongIntrusiveWrapperT;

	struct WeakIntrusiveWrapperT : public StrongWeakIntrusiveWrapperT
	{
		template<typename PtrT>
		void AddRef(PtrT* ptr)
		{
			StrongWeakIntrusiveWrapperT::AddWeakRef(ptr);
		}

		template<typename PtrT>
		void SubRef(PtrT* ptr)
		{
			StrongWeakIntrusiveWrapperT::SubWeakRef(ptr);
		}

		WeakIntrusiveWrapperT() = default;
		WeakIntrusiveWrapperT(WeakIntrusiveWrapperT const&) = default;
		WeakIntrusiveWrapperT(WeakIntrusiveWrapperT&&) = default;
		WeakIntrusiveWrapperT(StrongIntrusiveWrapperT const& wra) {}

		WeakIntrusiveWrapperT& operator=(WeakIntrusiveWrapperT const&) = default;
		WeakIntrusiveWrapperT& operator=(WeakIntrusiveWrapperT&&) = default;
		WeakIntrusiveWrapperT& operator=(StrongIntrusiveWrapperT const&) { return *this; }

		

		using PotatoPointerForbidPointerAccess = void;
		using PotatoPointerIsomer = StrongIntrusiveWrapperT;
		using PotatoPointerForbidPointerConstruct = void;
	};

	struct StrongIntrusiveWrapperT : public StrongWeakIntrusiveWrapperT
	{
		template<typename PtrT>
		void AddRef(PtrT* ptr)
		{
			StrongWeakIntrusiveWrapperT::AddWeakRef(ptr);
			StrongWeakIntrusiveWrapperT::AddStrongRef(ptr);
		}

		template<typename PtrT>
		void SubRef(PtrT* ptr)
		{
			StrongWeakIntrusiveWrapperT::SubStrongRef(ptr);
			StrongWeakIntrusiveWrapperT::SubWeakRef(ptr);
		}

		template<typename PtrT>
		bool TryAddRef(PtrT* ptr)
		{
			if(StrongWeakIntrusiveWrapperT::TryAddStrongRef(ptr))
			{
				StrongWeakIntrusiveWrapperT::AddWeakRef(ptr);
				return true;
			}
			return false;
		}

		StrongIntrusiveWrapperT() = default;
		StrongIntrusiveWrapperT(StrongIntrusiveWrapperT const&) = default;
		StrongIntrusiveWrapperT(StrongIntrusiveWrapperT&&) = default;

		StrongIntrusiveWrapperT(WeakIntrusiveWrapperT const&) {}

		using PotatoPointerIsomer = WeakIntrusiveWrapperT;

		StrongIntrusiveWrapperT& operator=(StrongIntrusiveWrapperT const&) = default;
		StrongIntrusiveWrapperT& operator=(StrongIntrusiveWrapperT&&) = default;
		StrongIntrusiveWrapperT& operator=(WeakIntrusiveWrapperT const&) { return *this; }
	};

	template<typename PtrT>
	using StrongPtr = IntrusivePtr<PtrT, StrongIntrusiveWrapperT>;

	template<typename PtrT>
	using WeakPtr = IntrusivePtr<PtrT, WeakIntrusiveWrapperT>;

	struct DefaultStrongWeakInterface
	{
	protected:

		void AddWeakRef() const { WRefCount.AddRef(); }
		void SubWeakRef() const { if (WRefCount.SubRef()) const_cast<DefaultStrongWeakInterface*>(this)->WeakRelease(); }
		void AddStrongRef() const { SRefCount.AddRef(); }
		void SubStrongRef() const { if (SRefCount.SubRef()) const_cast<DefaultStrongWeakInterface*>(this)->StrongRelease(); }
		bool TryAddStrongRef() const { return SRefCount.TryAddRefNotFromZero(); }

		virtual void WeakRelease() = 0;
		virtual void StrongRelease() = 0;
		virtual ~DefaultStrongWeakInterface() = default;

		mutable Potato::Misc::AtomicRefCount SRefCount;
		mutable Potato::Misc::AtomicRefCount WRefCount;

		friend struct StrongWeakIntrusiveWrapperT;
	};


	struct ControllerViewerIntrusiveWrapperT
	{
		template<typename PtrT>
		void AddControllerRef(PtrT* P) { P->AddControllerRef(); }
		template<typename PtrT>
		void SubControllerRef(PtrT* P) { P->SubControllerRef(); }
		template<typename PtrT>
		void AddViewerRef(PtrT* P) { P->AddViewerRef(); }
		template<typename PtrT>
		void SubViewerRef(PtrT* P) { P->SubViewerRef(); }
	};

	struct ControllerIntrusiveWrapperT;

	struct ViewerIntrusiveWrapperT : public ControllerViewerIntrusiveWrapperT
	{
		template<typename PtrT>
		void AddRef(PtrT* ptr)
		{
			ControllerViewerIntrusiveWrapperT::AddViewerRef(ptr);
		}

		template<typename PtrT>
		void SubRef(PtrT* ptr)
		{
			ControllerViewerIntrusiveWrapperT::SubViewerRef(ptr);
		}

		ViewerIntrusiveWrapperT() = default;
		ViewerIntrusiveWrapperT(ViewerIntrusiveWrapperT const&) = default;
		ViewerIntrusiveWrapperT(ViewerIntrusiveWrapperT&&) = default;
		ViewerIntrusiveWrapperT(ControllerIntrusiveWrapperT const&) {}

		ViewerIntrusiveWrapperT& operator=(ViewerIntrusiveWrapperT const&) = default;
		ViewerIntrusiveWrapperT& operator=(ViewerIntrusiveWrapperT&&) = default;
		ViewerIntrusiveWrapperT& operator=(ControllerIntrusiveWrapperT const&) { return *this; }

	};

	struct ControllerIntrusiveWrapperT : public ControllerViewerIntrusiveWrapperT
	{
		template<typename PtrT>
		void AddRef(PtrT* ptr)
		{
			ControllerViewerIntrusiveWrapperT::AddViewerRef(ptr);
			ControllerViewerIntrusiveWrapperT::AddControllerRef(ptr);
		}

		template<typename PtrT>
		void SubRef(PtrT* ptr)
		{
			ControllerViewerIntrusiveWrapperT::SubControllerRef(ptr);
			ControllerViewerIntrusiveWrapperT::SubViewerRef(ptr);
		}

		ControllerIntrusiveWrapperT() = default;
		ControllerIntrusiveWrapperT(ControllerIntrusiveWrapperT const&) = default;
		ControllerIntrusiveWrapperT(ControllerIntrusiveWrapperT&&) = default;

		using PotatoPointerIsomer = ViewerIntrusiveWrapperT;

		ControllerIntrusiveWrapperT& operator=(ControllerIntrusiveWrapperT const&) = default;
		ControllerIntrusiveWrapperT& operator=(ControllerIntrusiveWrapperT&&) = default;
	};

	struct DefaultControllerViewerInterface
	{
	protected:

		void AddViewerRef() const { VRefCount.AddRef(); AddControllerRef(); }
		void SubViewerRef() const { if (VRefCount.SubRef()) const_cast<DefaultControllerViewerInterface*>(this)->ViewerRelease(); SubControllerRef(); }
		void AddControllerRef() const { CRefCount.AddRef(); }
		void SubControllerRef() const { if (CRefCount.SubRef()) const_cast<DefaultControllerViewerInterface*>(this)->ControllerRelease(); }

		std::size_t GetViewerRef() const { return CRefCount.Count(); }

		virtual void ControllerRelease() = 0;
		virtual void ViewerRelease() = 0;
		virtual ~DefaultControllerViewerInterface() = default;

		mutable Potato::Misc::AtomicRefCount CRefCount;
		mutable Potato::Misc::AtomicRefCount VRefCount;

		friend struct ControllerViewerIntrusiveWrapperT;
	};

	template<typename PtrT>
	using ControllerPtr = IntrusivePtr<PtrT, ControllerIntrusiveWrapperT>;

	template<typename PtrT>
	using ViewerPtr = IntrusivePtr<PtrT, ViewerIntrusiveWrapperT>;
}