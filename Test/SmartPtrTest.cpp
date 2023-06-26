#include <cassert>

import Potato.SmartPtr;

int GobalIndex = 0;

using namespace Potato;
using namespace Potato::Misc;

struct Type1
{
	mutable Potato::Misc::AtomicRefCount Ref;
	void AddRef() const{ return Ref.AddRef(); }
	void SubRef() const{
		if (Ref.SubRef())
		{
			delete this;
		}
	}
	~Type1(){ 
	GobalIndex = 100; 
	}
};

struct Type23
{
	void Release(){ delete this;}
};

struct Type2
{
	mutable Potato::Misc::AtomicRefCount Ref;
	void AddRef2() const { return Ref.AddRef(); }
	bool SubRef2() const { return Ref.SubRef(); }
	~Type2() { 
	GobalIndex = 200; 
	}
};

struct Type2Wrapper
{
	Type2Wrapper(Type2* IType) {
		if(IType != nullptr)
			IType->AddRef2();
	}
	Type2Wrapper(Type2* IType, Type2Wrapper const&) : Type2Wrapper(IType) {}
	void Reset(Type2* IType) {
		if (IType != nullptr)
		{
			if(IType->SubRef2())
				delete IType;
		}
	}
};

struct DefaultRef
{
	std::size_t SRef = 0;
	std::size_t WRef = 0;

	void AddStrongRef() { SRef += 1; AddWeakRef(); }
	bool SubStrongRef() { SRef -= 1; SubWeakRef(); return SRef == 0; }

	void AddWeakRef() { WRef += 1; }
	void SubWeakRef() { WRef -= 1; }
};

struct Type3
{
	int32_t C = 0;
	void Release() {
		delete this;
	}
};

struct Type4
{
	int32_t& RefValue;

	Type4(int32_t& Ref) : RefValue(Ref) { RefValue = 1; }
	~Type4() { RefValue = 2; }
};

struct SWRef
{
	mutable Potato::Misc::AtomicRefCount SRef;
	mutable Potato::Misc::AtomicRefCount WRef;
};

struct WeakWrapper;

struct StrongWrapper
{
	SWRef* Ref = nullptr;
	StrongWrapper(Type4 const* T){
		if (T != nullptr)
		{
			Ref = new SWRef{};
			Ref->SRef.AddRef();
			Ref->WRef.AddRef();
		}
	}
	StrongWrapper() {}

	StrongWrapper(Type4*& IP, WeakWrapper const& Wra);

	void Reset(Type4* T)
	{
		if (T != nullptr && Ref != nullptr)
		{
			if (Ref->SRef.SubRef())
			{
				delete T;
			}

			if (Ref->WRef.SubRef())
			{
				delete Ref;
			}

			Ref = nullptr;
		}
	}

	StrongWrapper(Type4* T, StrongWrapper const& Wra) {
		if (T != nullptr)
		{
			Ref = Wra.Ref;
			if (Ref != nullptr)
			{
				Ref->SRef.AddRef();
				Ref->WRef.AddRef();
			}
		}
	}

	Potato::Misc::SmartPtr<Type4, WeakWrapper> Downgrade(Type4* IPtr);

	using RequireExplicitPointConstructT = void;
};

struct WeakWrapper
{
	SWRef* Ref = nullptr;


	WeakWrapper() {}
	WeakWrapper(Type4* IP, WeakWrapper const& Wra) {
		if (IP != nullptr)
		{
			Ref = Wra.Ref;
			Ref->WRef.AddRef();
		}
	}

	void Reset(Type4* T)
	{
		if (T != nullptr && Ref != nullptr)
		{
			if (Ref->WRef.SubRef())
			{
				delete Ref;
			}

			Ref = nullptr;
		}
	}

	WeakWrapper(Type4* T, StrongWrapper const& Wra) {
		if (T != nullptr)
		{
			Ref = Wra.Ref;
			if (Ref != nullptr)
			{
				Ref->WRef.AddRef();
			}
		}
	}

	SmartPtr<Type4, StrongWrapper> Upgrade(Type4* IPtr) {
		if (IPtr != nullptr && Ref != nullptr)
			return SmartPtr<Type4, StrongWrapper>(IPtr, *this);
		return {};
	}

	using RequireExplicitPointConstructT = void;
	using ForbidPtrT = void;
};

StrongWrapper::StrongWrapper(Type4*& IP, WeakWrapper const& Wra) {
	auto Temp = Wra.Ref;
	if (Temp != nullptr)
	{
		assert(IP != nullptr);
		if (Temp->SRef.TryAddRefNotFromZero())
		{
			Temp->WRef.AddRef();
			Ref = Temp;
		}
		else {
			IP = nullptr;
		}
	}
}

Potato::Misc::SmartPtr<Type4, WeakWrapper> StrongWrapper::Downgrade(Type4* IPtr) {
	if (IPtr != nullptr && Ref != nullptr)
		return Misc::SmartPtr<Type4, WeakWrapper>{IPtr, static_cast<StrongWrapper const&>(*this)};
	return {};
}

struct K
{
	K(int32_t I) {}
};



int main()
{

	static_assert(std::is_constructible_v<K, int32_t&>, "Fuck");

	GobalIndex = 10086;


	IntrusivePtr<Type1> P1{ new Type1 };

	P1.Reset();

	if (GobalIndex != 100)
		return 1;

	IntrusivePtr<Type2, Type2Wrapper> P2{ new Type2 };

	P2.Reset();

	if (GobalIndex != 200)
		return 1;

	DefaultRef K;

	int32_t State = 0;

	SmartPtr<Type4, StrongWrapper> Ptr{new Type4{ State }};

	auto W = Ptr.Downgrade();

	auto Kw = W.Upgrade();

	Ptr.Reset();

	Kw.Reset();

	

	auto Kc = W.Upgrade();

	W.Reset();

	

	volatile int i = 0;

	/*
	{
		StrongPtr<Type3, DefaultRef> Ptr{ new Type3, &K };

		auto IO2 = Ptr->C;

		auto K = WeakPtr<Type3, DefaultRef>(Ptr);
	}

	{

		//static_assert(std::is_constructible_v<UniquePtrDefaultWrapper, UniquePtrDefaultWrapper const&>);

		UniquePtr<Type23> Pxx{ new Type23{} };

		static_assert(!std::is_constructible_v<UniquePtr<Type23>, UniquePtr<Type23> const&>);
	}
	*/

	return 0;
}