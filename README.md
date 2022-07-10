[TOC]

# Potato

个人库，用来管理和存储一些常用的功能。

具体用法和单元测试，请参考Project/UnitTesting/XXXSample.cpp

## 1. PotatoTMP 

模板元编程库（namespace Potato::TMP）

其包含如下功能：

1. IsOneOf 用来判断某个类型是否在后续的类型集中。
2. IsNotOneOf IsOneOf的否。
3. IsRepeat 类型中是否有重复的类型。
4. ItSelf 占位符，自身无功能，一般用于一些特化。
5. Instant 对`template<typeanme ...> class`样式的模板类型的延迟构造。
6. TypeTuple `std::tuple`类似物，但本身不占用储存空间，实例化后也无任何逻辑。
7. Replace 对`template<typeanme ...> class`样式的模板类型的类型替换，比如将`T1<A, B, C>`替换成`T2<A, B, C>`。
8. Exist 存在判断，可以用来检测某个函数或者变量是否存在，SFINAE。
9. IsFunctionObjectRole 判断某个Object是不是CallableObject（标准是，是否存在唯一一个operator()）。
10. FunctionInfo 函数签名的萃取
11. TempString 字符串常量类型，可用作模板参数。

## 2. PotatoMisc

一些杂项，放置一些不便分类的功能类（namespace Potato::Misc）

1. IndexSpan 类似于std::span，但IndexSpan只储存了两个std::size_t（可定制），用于表示连续内存中的一段，方便在复制构造时，不需要重新绑定原数据。
2. AtomicRefCount 支持原子操作的引用计数器，主要用于智能指针。
3. AlignedSize 用于计算对齐。
4. namespace::SerilizerHelper 相关功能，用于执行读写序列化。将数据写入一段Buffer中，要求Buffer中的数据类型的对齐大于等于目标数据对齐。内部使用std::memocy实现，只支持StandaryLayout。

## 3. PotatoStrEncode

字符转换，用以处理char8_t，char16_t，char32_t和wchar_t之间的转化（namespace Potato::StrEncode）

1. CoreEncoder 对于单个字符的转化
2. StrCodeEncoder 对于字符串的转化
3. DocumentReader / DocumenetReaderWrapper 纯字符串文档阅读器（目前只支持utf8和utf8 with bom两种文本编码格式）
4. DocumentWriter 纯字符串文档写入器（目前支持写入utf8和utf8 with bom两种文本编码格式）

## 4. PotatoSLRX

基于LR(X)的语法分析器。（namespace Potato::SLRX）

SLRX是一款“柔性”的语法分析器，对于用户给定的X（默认为3），可以自动识别LR(X)的语法，并生成对应的分析表。

在此基础上，通过特定的储存方式，使得表本身有着更小的体积。

对于一些带有歧义的语法，也会在生成表格的时候，通过异常的方式抛出给用户，方便用户进行修改。

在性能上，当X为0时有着最小的体积与最高的效率。对于越大的X，其在生成表格和在处理语法时，性能越差。在最坏情况下，其性能复杂度为指数级。

为了方便构造分析表，其提供了一些功能，使得用户可以自定义运算符优先级（类似+-*/），以及防止深度为1的Reduce路径。

目前该分析器是单线程，不支持错误回退，并且是非异常安全的。

一般流程:

1. 将终结符与非终结符转换成内部专用的Symbol。
2. 使用Symbol，设置开始符，产生式，运算符优先级，构建分析表，并序列化成可读的Table。
3. 使用Table，对目标的Symbol序列进行分析，产生Step序列，Step序列的信息足够产生一颗AST树。
4. 对Step序列执行分析，产生目标语言。

该模块遇到错误会抛出异常，异常一般以`Potato::SLRX::Exception::Interface`为基类。

## 5. PotatoReg

基于`char32_t`的正则表达式库，依赖于`PotatoDLr`。（namespace Potato::Reg）

目前只支持以下几种元字符：

1. `.` `-`
2. `*` `*?`
3. `+` `+?`
4. `?` `??`
5. `|` `()` `[]` `[^]` `(?:)`
6. `{x,y}` `{X}`  `{X,}` `{,Y}` `{x,y}?` `{X}?`  `{X,}?` `{,Y}?`

由于一些优化，类似于`(.*)*`这种正则表达式，会抛出异常，其特征为：

>对于一个非元字符，在预查找下一个需要的非元字符时，只能经过有限次的不消耗非元字符的查找。

支持多个正则合并到一个正则上，合并的时候会有一次优化，会一同进行匹配。

在匹配时，支持以下三种操作：

1. Search 只需要字符串其中的一段满足正则既返回真。
2. March 需要字符串全段满足正则才返回。
3. FrontMarch 需要字符串的开头一段满足才返回。
4. GreedyFrontMatch 同 FrontMarch ，但会尝试匹配最长的匹配项。

一般的，GreedyFrontMatch 用在词法分析上。

该模块遇到错误时会抛出异常，异常一般以`Potato::Reg::Exception::Interface`为基类。

## 6. PotatoEbnf

依赖于`PotatoSLRX`与`PotatoReg`的`Ebnf`范式。（namespace Potato::Ebnf）

一般而言，除了支持从字符串构建语法分析表，并且支持`|` `()` `[]` `{}`等Ebnf范式的操作符，以及支持内置的一个词法分析器外，其余和 SLRX 无差别。

## 7. PotatoPath

文件路径相关库，施工中。

## 8. PotatoIR

编译器中间表示相关功能类，施工中。

## 9. PotatoInterval

用以表达数学上的集合概念的库，施工中。

## 10. PotatoIntrusivePointer

嵌入式智能指针。（namespace Potato）

一般用于类COM类型的指针的管理，例如一个类型实现有AddRef和SubRef两种函数，则IntrusivePointer可以直接管理。

又或者是其他类型的函数，比如AddRef1和SubRef1，则可以通过替换IntrusivePointer的Wrapper类型来进行管理。











