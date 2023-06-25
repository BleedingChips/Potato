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
	static void AddRef(Type2* Type) { Type->AddRef2(); }
	static void SubRef(Type2* Type) { if(Type->SubRef2()) { delete Type; } }
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
	StrongWrapper(Type4 const* T) : Ref(new SWRef{}) { }
	StrongWrapper() {}
	void AddRef(Type4 const* T)
	{
		Ref->WRef.AddRef();
		Ref->SRef.AddRef();
	}
	void SubRef(Type4 const* T)
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
	StrongWrapper(Type4 const* T, WeakWrapper const& Wra);
	using DowngradeT = WeakWrapper;
};

struct WeakWrapper
{
	SWRef* Ref = nullptr;
	WeakWrapper(Type4 const* T) = delete;
	WeakWrapper() {}
	WeakWrapper(Type4 const* T, StrongWrapper const& Wra);
	
	void AddRef(Type4 const* T)
	{
		Ref->WRef.AddRef();
	}

	bool EnableDowngrade(Type4 const* Ptr)
	{
		//return
		return ture;
	}

	void SubRef(Type4 const* T)
	{
		if (Ref->WRef.SubRef())
		{
			delete Ref;
		}
		Ref = nullptr;
	}

	using UpgradeT = StrongWrapper;
	using ForbidPtrT = void;
};

StrongWrapper::StrongWrapper(Type4 const* T, WeakWrapper const& Wra)
{
	Ref = Wra.Ref;
}

WeakWrapper::WeakWrapper(Type4 const* T, StrongWrapper const& Wra)
{
	Ref = Wra.Ref;
}

int main()
{
	GobalIndex = 10086;

	IntrusivePtr<Type1> P1{ new Type1 };

	//IntrusivePtr<Type1> P2{(Type1*)(nullptr)};

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

	SmartPtr<Type4, WeakWrapper> Ptr2{new Type4{ State }};

	auto W = Ptr.Downgrade();

	auto Kw = W.Upgrade();

	Ptr.Reset();

	Kw.Reset();

	W.Reset();

	auto K = W.Upgrade();

	

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