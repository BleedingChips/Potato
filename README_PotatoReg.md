# PotatoReg

为词法分析器特化的，基于DFA的，可单次同时匹配多条正则表达式的正则库。

```cpp
import PotatoReg;
using namespace Potato::Reg;
```

* 创建正则表达式：

	```cpp
	Dfa Table(u8R"(([0-9]+))", false, 2); 
	// 序列化版本
	std::vector<Reg::StandardT> Table2 = CreateDfaBinaryTable(Table); 
	```
	false 表示这个字符串是个正则字符串，Accept 表示当该正则表达式匹配成功后所返回的标记值。

	支持创建多个正则表达式：

	```cpp
	MulityRegCreater Crerator;
	Crerator.LowPriorityLink(u8R"(while)", false, 1); // 1
	Crerator.LowPriorityLink(u8R"(if)", false, 2); // 2
	Crerator.LowPriorityLink(u8R"([a-zA-Z]+)", false, 3); // 3

	// Dfa::FormatE::HeadMatch 为匹配策略，既只要有匹配任意一个正则表达式则立马返回
	std::optional<Dfa> Table = Crerator.CreateDfa(Dfa::FormatE::HeadMatch);
	```

	同时处理多个正则表达式，先创建的优先级高。当多个正则均能同时匹配时，选取优先级最高的。


	由于一些优化，类似于`(.*)*`与`.*aa.*`这几种正则表达式，会抛出异常，其特征为：

	>对于一个非元字符，在预查找下一个需要的非元字符时，只能经过有限次的不消耗非元字符的查找。

	要执行上述的正则查找，只能手动去拆分多个正则表达式。

* 对字符串进行检测：

	目前提供的匹配策略有急躁匹配，完全匹配，贪婪匹配

	* 整串匹配（Dfa::FormatE::Match）
  
  		指字符串要与正则表达式完全匹配。若有多个正则表达式，则只需要匹配其中一种即可。

	* 急躁匹配（Dfa::FormatE::HeadMatch）

		只需要字符串的前部能匹配一个正则即可。

	* 贪婪匹配（Dfa::FormatE::GreedyHeadMatch）

		总是尝试去匹配最长的字符串，直到所有正则都不匹配。