export module Potato.TMP;

export import Potato.STD;

namespace Potato::TMP::Implement
{
	template<size_t Index, typename ...InputType> struct TypeTupleIndex;
	template<size_t Index, typename Cur, typename ...InputType> struct TypeTupleIndex<Index, Cur, InputType...> {
		using Type = typename TypeTupleIndex<Index - 1, InputType...>::Type;
	};
	template<typename Cur, typename ...InputType> struct TypeTupleIndex<0, Cur, InputType...> {
		using Type = Cur;
	};
}

export namespace Potato::TMP
{
	template<typename T, typename ...AT> struct IsOneOf {
		static constexpr bool Value = false;
	};
	template<typename T, typename I, typename ...AT> struct IsOneOf<T, I, AT...> : IsOneOf<T, AT...> {};
	template<typename T, typename ...AT> struct IsOneOf<T, T, AT...> {
		static constexpr bool Value = true;
	};
	template<typename T, typename ...AT> constexpr bool IsOneOfV = IsOneOf<T, AT...>::Value;

	template<typename T, typename ...AT> struct IsNotOneOf {
		static constexpr bool Value = !IsOneOf<T, AT...>::Value;
	};
	template<typename T, typename ...AT> constexpr bool IsNotOneOfV = IsNotOneOf<T, AT...>::Value;

	/* IsRepeat */
	template<typename ...AT> struct IsRepeat { static constexpr bool Value = false; };
	template<typename T, typename ...AT>
	struct IsRepeat <T, AT...> {
		static constexpr bool Value = IsOneOf<T, AT...>::Value || IsRepeat<AT...>::Value;
	};

	/* ItSelf */
	template<typename T>
	struct ItSelf
	{
		using Type = T;
	};

	//};

	template<template<typename ...> class Output, typename ...Input> struct Instant
	{
	private:
		template<typename ...OInput> struct AppendTWrapper { using Type = Output<Input..., OInput...>; };
		template<typename ...OInput> struct FrontTWrapper { using Type = Output<OInput..., Input...>; };
	public:
		template<typename ...OInput> using Append = Instant<Output, Input..., OInput...>;
		template<typename ...OInput> using AppendT = typename AppendTWrapper<OInput...>::Type;
		template<typename ...OInput> using Front = Instant<Output, OInput..., Input...>;
		template<typename ...OInput> using FrontT = typename FrontTWrapper<OInput...>::Type;


	};
	template<template<typename ...> class Output, typename ...Input> using InstantT = Output<Input...>;

	template<typename ...Type> struct TypeTuple {
		static constexpr size_t Size = sizeof...(Type);
		template<size_t Index> using Get = typename Implement::TypeTupleIndex<Index, Type...>::Type;
	};

	// Replace

	template<typename Type> struct Replace;
	template<template<typename ...> class Holder, typename ...Type>
	struct Replace<Holder<Type...>>
	{
		template<template<typename ...> class OutHolder> using With = OutHolder<Type...>;
	};

	// Exist
	template<typename DetectType, template<typename ...> class Role, typename ...RoleParameter> struct Exist
	{
	private:
		template<typename P> static std::true_type DetectFunc(typename Role<P, RoleParameter...>*);
		template<typename P> static std::false_type DetectFunc(...);
	public:
		static constexpr bool Value = decltype(DetectFunc<DetectType>(nullptr))::value;
	};

	template<typename Type, typename = std::void_t<decltype(&Type::operator())>>
	struct IsFunctionObjectRole {};

	template<typename DetectType> struct IsFunctionObject : Exist<DetectType, IsFunctionObjectRole> {};

	template<typename T> struct FunctionInfoWrapper { using Type = decltype(&T::operator()); };

	template<typename FT> struct FunctionInfo;
	template<typename Owner, typename FunctionType> struct FunctionInfo<FunctionType Owner::*> : FunctionInfo<FunctionType>
	{
		using OwnerType = Owner;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...)> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) const > {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) const noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) const> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) const noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)  volatile noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile const > {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile const noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile const> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile const noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};

	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...)&> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) & noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)& > {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) & noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile&> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile& noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile&> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)  volatile& noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...)&&> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) && noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)&& > {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) && noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile&&> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile&& noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile&&> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)  volatile&& noexcept> {
		using ReturnType = RT;
		using OwnerType = void;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		template<template<typename...> class Package> using PackReturnParameters = Package<RT, Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};

	template<typename Type> struct FunctionObjectInfo : FunctionInfo<decltype(&Type::operator())> {};

	template<typename Type> struct MemberFunctionInfo;
	template<typename Owner, typename FunctionType> struct MemberFunctionInfo<FunctionType Owner::*> : FunctionInfo<FunctionType>
	{
		using OwnerType = Owner;
	};

	template<typename InputType, typename ReturnValue, typename ...Parameter>
	struct IsFunction
	{

	};

	template<typename CharT, std::size_t N>
	struct TypeString
	{
		CharT Storage[N];
		template<typename CharT>
		constexpr TypeString(const CharT(&str)[N]) : Storage{}
		{
			std::copy_n(str, N, Storage);
		}
		template<typename CharT2, std::size_t N2>
		constexpr bool operator==(TypeString<CharT2, N2> const& ref) const
		{
			if constexpr (std::is_same_v<CharT, CharT> && N == N2)
			{
				return std::equal(Storage, Storage + N, ref.Storage, N);
			}
			else
				return false;
		}
		using Type = CharT;
		static constexpr std::size_t Len = N;
	};

	template<std::size_t N>
	TypeString(const char32_t(&str)[N]) -> TypeString<char32_t, N>;

	template<std::size_t N>
	TypeString(const char16_t(&str)[N])-> TypeString<char16_t, N>;

	template<std::size_t N>
	TypeString(const char8_t(&str)[N])-> TypeString<char8_t, N>;

	template<std::size_t N>
	TypeString(const wchar_t(&str)[N])-> TypeString<wchar_t, N>;

	template<std::size_t N>
	TypeString(const char(&str)[N])-> TypeString<char, N>;

}