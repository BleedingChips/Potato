#include <cassert>

import std;
import PotatoPointer;
import PotatoMisc;

int GobalIndex = 0;

using namespace Potato;
using namespace Potato::Misc;
using namespace Potato::Pointer;

struct Type1
{
	mutable Potato::Misc::AtomicRefCount Ref;
	void AddRef() const
	{
		return Ref.AddRef();
	}
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

struct Type2 : public Pointer::DefaultStrongWeakInterface
{

	virtual void StrongRelease() override { GobalIndex = 10000; }
	virtual void WeakRelease() override { this->~Type2(); }
	~Type2() {
		GobalIndex = 100;
	}
};

struct Type3 : public Pointer::DefaultControllerViewerInterface
{

	virtual void ControllerRelease() override { GobalIndex = 10000; this->~Type3(); }
	virtual void ViewerRelease() override {  }
	~Type3() {
		GobalIndex = 100;
	}
};

/*
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
	std::int32_t C = 0;
	void Release() {
		delete this;
	}
};

struct Type4
{
	std::int32_t& RefValue;

	Type4(std::int32_t& Ref) : RefValue(Ref) { RefValue = 1; }
	~Type4() { RefValue = 2; }
};

struct K
{
	K(std::int32_t I) {}
};
*/

int main()
{


	GobalIndex = 10086;


	IntrusivePtr<Type1> P1{ new Type1 };

	P1.Reset();

	if (GobalIndex != 100)
		return 1;

	StrongPtr<Type2> P2 {new Type2};

	auto iso = P2.Isomer();

	P2.Reset();


	if (GobalIndex != 10000)
		return 1;

	iso.Reset();

	if (GobalIndex != 100)
		return 1;

	ControllerPtr<Type3> P3{ new Type3 };

	auto iso2 = P3.Isomer();

	P3.Reset();

	if (GobalIndex != 10000)
		return 1;

	iso2.Reset();

	if (GobalIndex != 100)
		return 1;

	return 0;
}