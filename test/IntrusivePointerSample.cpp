#include "Potato/PotatoIntrusivePointer.h"
#include "Potato/PotatoMisc.h"
#include "TestingTypes.h"

int GobalIndex = 0;

using namespace Potato;
using namespace Misc;

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
	static void SubRef(Type2* Type) { if(Type->SubRef2()){ delete Type; } }
};

void TestingIntrusivePointer()
{

	GobalIndex = 10086;

	IntrusivePtr<Type1> P1 = new Type1;

	P1.Reset();

	if(GobalIndex != 100)
		throw UnpassedUnit("IntrusivePointer Not Pass! 1");

	IntrusivePtr<Type2, Type2Wrapper> P2 = new Type2;

	P2.Reset();

	if (GobalIndex != 200)
		throw UnpassedUnit("IntrusivePointer Not Pass! 2");

	std::cout<< "IntrusivePointer All Pass!" << std::endl;

}