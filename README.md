# Potato

个人库，用来管理和存储一些常用的功能，采用xmake + c++ module

本库包含以下主要功能：

1. [PotatoTMP](README_PotatoTMP.md) —— 模板元编程功能库。

2. [PotatoPointer](PotatoPointer) —— 嵌入式智能指针功能库。

3. [PotatoStrEncode](PotatoStrEncode)	—— `Unicode`字符之间的编码相互转换的功能库。

4. [PotatoSLRX](PotatoSLRX) —— `LR(x)`的语法分析库，X为可配置。

5. [PotatoReg](PotatoReg) —— 为词法分析特化实现的，基于DFA的正则表达式分析库，可将多个正则表达式合并到同一个分析表中，以提高速度。

6. [PotatoStrFormat](PotatoStrFormat) —— `std::format`类似物，提供数据与`Unicode`字符之间的相互转化。

7. [PotatoEnbf](PotatoEnbf) —— `ENBF`词法/语法分析库。

8. [PotatoDocument](PotatoDocument) —— 处理带文件BOM头的纯字符串文件读写

9. [PotatoInterval](PotatoInterval) —— 数学概念中的区间运算

10. [PotatoIR](PotatoIR) —— 动态类型库，支持动态拼接数据类型，并提供动态类型的基础反射

11. [PotatoTaskSystem](PotatoTaskSystem) —— 基于线程池的多线程任务系统

12. [PotatoTaskFlow](PotatoTaskFlow) —— 基于有向无环图的多线程任务图

## Todo

1. PotatoPath        虚拟路径系统，映射代码中的路径和本地文件系统的路径

## 使用方式

从 Github 上将以下库Clone到本地：

```
https://github.com/BleedingChips/Potato.git
```

目前要求`C++23`以上，且只支持使用`xmake`在`windows`下进行构建，可以使用目录下的`xmake_install.ps1`获取最新版的`xmake`，并且使用目录下的`xmake_generate_vs_project.ps1`来生成vs的工程项目。

要作为第三方库引用，只需要在第三方库中的`xmake.lua`中加入

```lua
include("PotatoProectFile/Potato")
```

其中`PotatoProectFile`是该项目在本地的路径。然后在`xmake`的`target`中填入:

```lua
target(xxx)
	...
	add_deps("Potato")
target_end()
```

然后在`CPP`文件中`import`：

```cpp
import Potato; // 使用所有的Potato模块
import PotatoTaskSystem; // 只使用单独的模块
```

若不想使用`xmake`，直接将`Potato/`下的`Potato`文件夹复制粘贴到项目中即可。

## 目录包含

1. `Potato/Potato` 所有的代码。
2. `Potato/Test` 各模块的测试代码。
3. `README_XXXX.md` 各模块的基本介绍
