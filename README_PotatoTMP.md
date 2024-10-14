# PotatoTMP 

模板元编程库

```cpp
import PotatoTMP;
using namespace Potato::TMP;
```

* `IsOneOf/IsNotOneOf`

	用来判断某个类型是否在后续的类型集中。

	```cpp
	static_assert(IsOneOf<A, A, B, C>::Value); // true
	static_assert(IsOneOf<A, D, B, C>::Value); // false
	static_assert(IsNotOneOf<A, A, B, C>::Value); // false
	static_assert(IsNotOneOf<A, D, B, C>::Value); // true
	```

* `IsRepeat`

	类型中是否有重复的类型。

	```cpp
	static_assert(IsRepeat<A, A, B, C>::Value); // true
	static_assert(IsRepeat<A, D, B, C>::Value); // false
	```

* `Instant`
	
	延迟构造类型，将一些固定参数数量的模板摊分到多个构造类型中进行构造。

	```CPP
	template<typename ...A> class TT;
	using T1 = Instant<TT, A>;
	static_assert(std::is_same_v<T1::template Append<B>, TT<A, B>>); // true
	static_assert(std::is_same_v<T1::template Front<B>, TT<B, A>>);// true
	```

* `Replace`

	将一些特定类型的模板通过萃取其参数，并使用参数构造新的类型。
	```cpp
	template<typename ...A> class TT;
	template<typename ...A> class TT2;
	static_assert(std::is_same_v<Replace<TT<A, B>::template With<TT2>, TT2<A, B>>); // true
	```

* `FunctionInfo`

	函数类型签名的萃取。
	```cpp
	struct Sample1
	{
		int32_t Function(char32_t, ...) && noexcept;
	};

	using Info = FunctionInfo<decltype(&Sample1::Function)>;

	static_assert(!Info::IsConst);// true
	static_assert(Info::IsEllipsis);// true
	static_assert(Info::IsMoveRef);// true
	static_assert(Info::IsNoException);// true
	static_assert(!Info::IsRef);// true
	static_assert(!Info::IsVolatile);// true

	template<typename ...A> class TT;

	static_assert(std::is_same_v<Info::OwnerType, Sample1>); // true
	static_assert(std::is_same_v<Info::ReturnType, int32_t>); // true
	static_assert(std::is_same_v<Info::PackParameters<TT>, TT<char32_t>>); // true
	static_assert(std::is_same_v<Info::PackReturnParameters<TT>, TT<int32_t, char32_t>>); // true
	```
* `TempString`

	字符串常量类型，可用作模板参数。

	```cpp
	static_assert(std::is_same_v<CST<u"1234">, CST<u"1234">>,
	```