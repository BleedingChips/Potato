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

	{
		StrongPtr<Type3, DefaultRef> Ptr{ new Type3, &K };

		auto C = Ptr.Switch();
	}

	return 0;
}