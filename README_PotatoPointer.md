# PotatoPointer

侵入式智能指针

```cpp
import PotatoTMP;
using namespace Potato::Pointer;
```

智能指针的原型为：

```cpp
template<typename PtrT, typename WrapperT>
struct IntrusivePtr;
```

指针的行为通过给予的`WrapperT`类型进行规范。

基本的Wrapper类型：

```cpp
// 基础类型，一般情况下只拥有基本的引用计数功能
struct DefaultIntrusiveWrapper
{

	// 需要被指向的类提供引用计数接口
	template<typename PtrT>
	void AddRef(PtrT* p) { p->AddRef(); }
	template<typename PtrT>
	void SubRef(PtrT* p) { p->SubRef(); }
};

// 观察指针，功能等同于原始指针
struct ObserverSubWrapperT {
	template<typename PtrT>
	void AddRef(PtrT* Ptr) {}

	template<typename PtrT>
	void SubRef(PtrT* Ptr) {}

	// 可从任意其他任意的指针类型转成观察指针
	template<typename T>
	ObserverSubWrapperT(T t) {}
	ObserverSubWrapperT(ObserverSubWrapperT const&) = default;
	ObserverSubWrapperT() = default;
};

// Unique指针，功能等同于 std::unique_ptr
struct DefaultUniqueWrapper
{
	template<typename PtrT>
	void AddRef(PtrT* Ptr) {}

	// 不需要提供引用计数功能，但需要提供 Release() 接口
	template<typename PtrT>
	void SubRef(PtrT* Ptr) { Ptr->Release(); }

	DefaultUniqueWrapper() = default;
	DefaultUniqueWrapper(DefaultUniqueWrapper&&) = default;
	
	// 指针将不允许复制拷贝
	DefaultUniqueWrapper(DefaultUniqueWrapper const&) = delete;
	DefaultUniqueWrapper& operator=(DefaultUniqueWrapper&&) = default;
};

// 弱引用指针，等同于 std::weak_ptr
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
	
	// 允许从强引用指针构造
	WeakIntrusiveWrapperT(StrongIntrusiveWrapperT const& wra) {}

	WeakIntrusiveWrapperT& operator=(WeakIntrusiveWrapperT const&) = default;
	WeakIntrusiveWrapperT& operator=(WeakIntrusiveWrapperT&&) = default;
	WeakIntrusiveWrapperT& operator=(StrongIntrusiveWrapperT const&) { return *this; }

	

	using PotatoPointerForbidPointerAccess = void;

	// 允许下位转换成强引用指针
	using PotatoPointerIsomer = StrongIntrusiveWrapperT;
	using PotatoPointerForbidPointerConstruct = void;
};

// 强引用指针，功能等同于 std::shared_ptr
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

	// 需要判断强引用计数是否从0开始
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

	// 允许从弱引用指针进行构造
	StrongIntrusiveWrapperT(WeakIntrusiveWrapperT const&) {}

	// 允许下位转换成弱引用指针
	using PotatoPointerIsomer = WeakIntrusiveWrapperT;

	StrongIntrusiveWrapperT& operator=(StrongIntrusiveWrapperT const&) = default;
	StrongIntrusiveWrapperT& operator=(StrongIntrusiveWrapperT&&) = default;
	StrongIntrusiveWrapperT& operator=(WeakIntrusiveWrapperT const&) { return *this; }
};
```