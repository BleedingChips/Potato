# PotatoStrFormat

`std::format`类似物，提供数据与字符串之间的相互转化。

```cpp
import PotatoFormat;
using namespace Potato::Format;
```

## Sanner

提供直接根据字符串转换成内置数据的功能：

```cpp
std::size_t i = 0;
DirectScan(u8"123", i);
assert(i == 123);
```

若实现了对应的类型的派生，也能处理字符串到自定义数据的转换。

```cpp
template<typename SourceType, typename UnicodeType>
struct Scanner
{
	bool Scan(std::span<UnicodeType const> Par, SourceType& Input) = delete;
};

struct A { std::size_t i = 0; }

template<> struct <A, char8_t>
{
	bool Scan(std::span<char8_t const> Par, A& Input) {  return DirectScan(Par, Input.i);  }
}

A a;
DirectScan(u8"123", a);
assert(a.i == 123);
```

同时也根据正则表达式捕获字符串内容，然后再转换成对应数据类型的功能，如：

```cpp
std::size_t i = 0;
MatchScan(u8"aa([1-9]*)aa", u8"aa123aa", i);
assert(i == 123);
```

## Format

提供内置数据到字符串的功能：


```cpp
std::size_t i = 10086;
std::size_t i2 = 100862;
auto FormatStr = FormatToString(u8"aaa{}bbb{}", i, i2);
assert(FormatStr == u8"aaa10086bbb100862");
```

同理实现下边的模板后自定义数据结构也能自由生成：

```cpp
template<typename SourceType, typename UnicodeType>
struct Formatter
{
	bool operator()(FormatWritter<SourceType>& Writer, std::basic_string_view<UnicodeType> Parameter, SourceType const& Input) = delete;
};
```

