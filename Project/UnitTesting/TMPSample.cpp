#include "Potato/Public/PotatoTMP.h"
#include <iostream>

struct Type1 {};

struct Type2 {};

struct Type3 {};

template<typename T1, typename T2, typename T3, typename T4> struct TTuple {};

template<typename T1, typename T2, typename T3, typename T4> struct TTuple2 {};

template<typename Type, typename = std::void_t<decltype(&Type::Func1)>>
struct ExistFun1Role {};

template<typename Type, typename = std::void_t<decltype(&Type::Value1)>>
struct ExistValue1Role {};

template<Potato::TMP::TempString H1> struct CST {};

void TestingTMP()
{
	using namespace Potato::TMP;

	// IsOneOf
	static_assert(IsOneOfV<Type1, Type1, Type2, Type3>, "Tmp::IsOneOf Not Pass");
	static_assert(!IsOneOfV<Type1, Type2, Type3>, "Tmp::IsOneOf Not Pass");

	// IsNotOneOf
	static_assert(!IsNotOneOfV<Type1, Type1, Type2, Type3>, "Tmp::IsNotOneOfV Not Pass");
	static_assert(IsNotOneOfV<Type1, Type2, Type3>, "Tmp::IsNotOneOfV Not Pass");

	// IsRepeat
	static_assert(IsRepeat<Type1, Type1, Type2, Type3>::Value, "Tmp::IsRepeat Not Pass");
	static_assert(!IsRepeat<Type1, Type2, Type3>::Value, "Tmp::IsRepeat Not Pass");

	// Instant
	{
		using T1 = Instant<TTuple, Type2>;
		using T2 = T1:: template Append<Type3>;

		static_assert(std::is_same_v<T2, Instant<TTuple, Type2, Type3>>, "Tmp::Instance Not Pass");

		using T3 = T2::template Front<Type1>;

		static_assert(std::is_same_v<T3, Instant<TTuple, Type1, Type2, Type3>>, "Tmp::Instance Not Pass");

		using T4 = T3::template AppendT<Type1>;

		static_assert(std::is_same_v<T4, TTuple<Type1, Type2, Type3, Type1>>, "Tmp::Instance Not Pass");
	}

	// Replace
	{
		using T1 = TTuple<Type1, Type2, Type3, Type1>;

		using T2 = Replace<T1>::template With<TTuple2>;

		static_assert(std::is_same_v<T2, TTuple2<Type1, Type2, Type3, Type1>>, "Tmp::Replace Not Pass");
	}

	// Exist
	{
		struct Sample1
		{
			int32_t Value1;
			void Func1(int32_t);
		};

		struct Sample2
		{

		};

		static_assert(Exist<Sample1, ExistValue1Role>::Value, "Exist Not Pass");
		static_assert(Exist<Sample1, ExistFun1Role>::Value, "Exist Not Pass");

		static_assert(!Exist<Sample2, ExistValue1Role>::Value, "Exist Not Pass");
		static_assert(!Exist<Sample2, ExistFun1Role>::Value, "Exist Not Pass");
	}
	
	// IsFunctionObjectRole
	{
		struct Sample1
		{
			void operator()();
		};

		struct Sample2
		{

		};

		static_assert(Exist<Sample1, IsFunctionObjectRole>::Value, "IsFunctionObjectRole Not Pass");
		static_assert(!Exist<Sample2, IsFunctionObjectRole>::Value, "IsFunctionObjectRole Not Pass");
	}

	// FunctionInfo
	{
		struct Sample1
		{
			int32_t Function(char32_t, ...) && noexcept;
		};

		int32_t Function1(char32_t, int64_t I);

		using Info = FunctionInfo<decltype(&Sample1::Function)>;

		static_assert(!Info::IsConst, "FunctionInfo Not Pass");
		static_assert(Info::IsEllipsis, "FunctionInfo Not Pass");
		static_assert(Info::IsMoveRef, "FunctionInfo Not Pass");
		static_assert(Info::IsNoException, "FunctionInfo Not Pass");
		static_assert(!Info::IsRef, "FunctionInfo Not Pass");
		static_assert(!Info::IsVolatile, "FunctionInfo Not Pass");

		static_assert(std::is_same_v<Info::OwnerType, Sample1>, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info::ReturnType, int32_t>, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info::PackParameters<TypeTuple>, TypeTuple<char32_t>>, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info::PackReturnParameters<TypeTuple>, TypeTuple<int32_t, char32_t>>, "FunctionInfo Not Pass");

		using Info2 = FunctionInfo<decltype(Function1)>;
		static_assert(!Info2::IsNoException, "FunctionInfo Not Pass");
		static_assert(std::is_same_v<Info2::OwnerType, void>, "FunctionInfo Not Pass");
	}

	// TempString
	{
		static_assert(std::is_same_v<CST<"1234">, CST<"1234">>, "ConstString Not Pass");
		static_assert(!std::is_same_v<CST<"12345">, CST<"1234">>, "ConstString Not Pass");
	}

	std::cout << "TMP Pass !" << std::endl;
}