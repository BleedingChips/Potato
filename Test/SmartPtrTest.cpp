#include <cassert>

import PotatoSmartPtr;

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

	void AddStrongRef(Type4* Ptr)
	{
		assert(Ptr != nullptr);
		SRef.AddRef();
	}
	void SubStrongRef(Type4* Ptr) {
		assert(Ptr != nullptr);
		if(SRef.SubRef())
			delete Ptr;
	}

	void AddWeakRef(Type4* Ptr)
	{
		assert(Ptr != nullptr);
		WRef.AddRef();
	}

	void SubWeakRef(Type4* Ptr)
	{
		if (WRef.SubRef())
		{
			delete this;
		}
	}

	bool TryAddStrongRef(Type4* Ptr)
	{
		return SRef.TryAddRefNotFromZero();
	}

	~SWRef() {
		volatile int i = 0;
	}
};

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

	StrongPtr<Type4, SWRef> Ptr{new Type4{ State }, new SWRef{}};

	auto W = Ptr.Downgrade();

	auto Kw = W.Upgrade();

	Ptr.Reset();

	Kw.Reset();

	

	auto Kc = W.Upgrade();

	W.Reset();

	IntrusivePtr2<Type1> L(new Type1{});

	

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