import PotatoTMP;
import std;

struct Type1 {};

struct Type2 {};

struct Type3 {};

template<typename ...T>
struct TTTuple{};

template<typename T1, typename T2, typename T3, typename T4> struct TTuple {};

template<typename T1, typename T2, typename T3, typename T4> struct TTuple2 {};

template<typename Type, typename = std::void_t<decltype(&Type::Func1)>>
struct ExistFun1Role {};

template<typename Type, typename = std::void_t<decltype(&Type::Value1)>>
struct ExistValue1Role {};

template<Potato::TMP::TypeString H1> struct CST {};

void NormalTest(Type1)
{
	
}

int main()
{
	using namespace Potato::TMP;

	// IsOneOf 查看类型是否重复
	static_assert(IsOneOfV<Type1, Type1, Type2, Type3>, "Tmp::IsOneOf Not Pass");
	static_assert(!IsOneOfV<Type1, Type2, Type3>, "Tmp::IsOneOf Not Pass");

	// IsNotOneOf IsOneOf 的否
	static_assert(!IsNotOneOfV<Type1, Type1, Type2, Type3>, "Tmp::IsNotOneOfV Not Pass");
	static_assert(IsNotOneOfV<Type1, Type2, Type3>, "Tmp::IsNotOneOfV Not Pass");
	
	// IsRepeat 类型是否重复
	static_assert(IsRepeat<Type1, Type1, Type2, Type3>::Value, "Tmp::IsRepeat Not Pass");
	static_assert(!IsRepeat<Type1, Type2, Type3>::Value, "Tmp::IsRepeat Not Pass");

	// Instant 对于一些template<typename ...> 类型的延迟构造，通常用在类型运算的中间
	{
		using T1 = Instant<TTuple, Type2>;
		using T2 = T1:: template Append<Type3>;

		static_assert(std::is_same_v<T2, Instant<TTuple, Type2, Type3>>, "Tmp::Instance Not Pass");

		using T3 = T2::template Front<Type1>;

		static_assert(std::is_same_v<T3, Instant<TTuple, Type1, Type2, Type3>>, "Tmp::Instance Not Pass");

		using T4 = T3::template AppendT<Type1>;

		static_assert(std::is_same_v<T4, TTuple<Type1, Type2, Type3, Type1>>, "Tmp::Instance Not Pass");
	}

	// Replace 对于一些template<typename ...> 类型的替换，通常用在类型运算的中间
	{
		using T1 = TTuple<Type1, Type2, Type3, Type1>;

		using T2 = Replace<T1>::template With<TTuple2>;

		static_assert(std::is_same_v<T2, TTuple2<Type1, Type2, Type3, Type1>>, "Tmp::Replace Not Pass");
	}

	// FunctionInfo 萃取函数类型
	{
		struct Sample1
		{
			std::int32_t Function(char32_t, ...) && noexcept;
		};

		std::int32_t Function1(char32_t, std::int64_t I);

		using Info = FunctionInfo<decltype(&Sample1::Function)>;

		static_assert(!Info::IsConst, "FunctionInfo Not Pass");
		static_assert(Info::IsEllipsis, "FunctionInfo Not Pass");
		static_assert(Info::IsMoveRef, "FunctionInfo Not Pass");
		static_assert(Info::IsNoException, "FunctionInfo Not Pass");
		static_assert(!Info::IsRef, "FunctionInfo Not Pass");
		static_assert(!Info::IsVolatile, "FunctionInfo Not Pass");

		static_assert(std::is_same_v<Info::OwnerType, Sample1>, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info::ReturnType, std::int32_t>, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info::PackParameters<TTTuple>, TTTuple<char32_t>>, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info::PackReturnParameters<TTTuple>, TTTuple<std::int32_t, char32_t>>, "FunctionInfo Not Pass");

		using Info2 = FunctionInfo<decltype(Function1)>;
		static_assert(!Info2::IsNoException, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info2::OwnerType, void>, "FunctionInfo Not Pass");

		auto lambda = [](Type1, Type2){};

		using Info3 = FunctionInfo<decltype(lambda)>;

		static_assert(std::is_same_v<Info3::OwnerType, decltype(lambda)>, "FunctionInfo Not Pass");

		using Info4 = FunctionInfo<decltype(NormalTest)>;

		static_assert(std::is_same_v<Info4::OwnerType, void>, "FunctionInfo Not Pass");
	}

	// TempString 用于模板参数的字符串常量
	{
		static_assert(std::is_same_v<CST<u"1234">, CST<u"1234">>, "ConstString Not Pass");
		static_assert(!std::is_same_v<CST<u"1234">, CST<"1234">>, "ConstString Not Pass");
		static_assert(!std::is_same_v<CST<"12345">, CST<u"1234">>, "ConstString Not Pass");
	}

	FunctionRef<void(int32_t)> function1{ [](int32_t cc) {
		volatile int k = 0;
		} };

	bool k1 = function1;

	function1(678);

	int io = 1233;

	FunctionRef<void(int32_t)> function2{ [&io](int32_t cc) {
		volatile int k = 0;
		} };

	bool k2 = function2;

	function2(678);

	FunctionRef<void(int32_t)> function3;

	bool k3 = function3;

	std::cout << "TMP Pass !" << std::endl;

	return 0;
}