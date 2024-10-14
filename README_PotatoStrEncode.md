# PotatoStrEncode

字符转换，用以处理`char8_t`，`char16_t`，`char32_t`和`wchar_t`之间的转化。

```cpp
import PotatoStrEncode;
using namespace Potato::Encode;
```

在windows下，只直接提供`char`到`wchar_t`的转换，且`wchar_t`等同于`char16_t`，在非windows下，`wchar_t`等同于`char32_t`。

转换逻辑分为两种，一种是逐字符转换，一种是字符串转换，字符串转换需要进行两次转换，第一次快速转换用以计算转换完成后的字符大小和字符数量，第二次转换直接进行不校验的转换。

* CharEncoder 
	
	对于单个字符的转化
	```cpp
	std::u8string_view Str;
	EncodeInfo Info = StrEncode<char8_t, char16_t>::RequireSpaceOnce(Str);
	std::u16string Str2;
	Str2.resize(Info.RequireSpace);
	StrEncode<char8_t, char16_t>::EncodeOnceUnSafe(Str, Str2);
	```

* StrEncoder 

	对于字符串的转化
	```cpp
	std::u8string_view Str;
	EncodeInfo I1 = StrEncoder<char8_t, char16_t>::RequireSpace(Str);
	std::u16string Str2;
	R1.resize(I1.TargetSpace);
	StrEncoder<ST, TT>::EncodeUnSafe(Str, Str2);
	```