#pragma once
#include <type_traits>


namespace Potato
{
	/* IsOneOf */
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
	template<typename ...AT> struct IsPrpeat { static constexpr bool Value = false; };
	template<typename T, typename ...AT>
	struct IsPrpeat <T, AT...> {
		static constexpr bool Value = IsOneOf<T, AT...>::Value || IsPrpeat<AT...>::Value;
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
		template<typename ...OInput> using Append = Instant<Output, Input..., OInput...>;
		template<typename ...OInput> using AppendT = Output<Input..., OInput...>;
		template<typename ...OInput> using Front = Instant<Output, OInput..., Input...>;
		template<typename ...OInput> using FrontT = Output<OInput..., Input...>;
	};
	template<template<typename ...> class Output, typename ...Input> using InstantT = Output<Input...>;

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
	struct IsFunctionObjectRole{};

	template<typename DetectType> struct IsFunctionObject : Exist<DetectType, IsFunctionObjectRole> {};

	template<typename T> struct FunctionInfoWrapper { using Type = decltype(&T::operator()); };

	template<typename FT> struct FunctionInfo;
	template<typename Owner, typename FunctionType> struct FunctionInfo<FunctionType Owner::*> : FunctionInfo<FunctionType>
	{
		using OwnerType = Owner;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...)> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) const > {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) const noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) const> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) const noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)  volatile noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile const > {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile const noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile const> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile const noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = true;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};

	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...)&> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) & noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)& > {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) & noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile&> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile& noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile&> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)  volatile& noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = true;
		static constexpr bool IsMoveRef = false;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...)&&> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) && noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)&& > {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) && noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = false;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile&&> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter...) volatile&& noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = true;
		static constexpr bool IsEllipsis = false;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...) volatile&&> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
		static constexpr bool IsConst = false;
		static constexpr bool IsVolatile = true;
		static constexpr bool IsRef = false;
		static constexpr bool IsMoveRef = true;
		static constexpr bool IsNoException = false;
		static constexpr bool IsEllipsis = true;
	};
	template<typename RT, typename ...Parameter> struct FunctionInfo<RT(Parameter..., ...)  volatile&& noexcept> {
		using ReturnType = RT;
		template<template<typename...> class Package> using PackParameters = Package<Parameter...>;
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

	template<std::size_t N>
	struct ConstString
	{
		char stroage[N];
		constexpr ConstString(const char(&str)[N]) : stroage{}
		{
			std::copy_n(str, N, stroage);
		}
		template<std::size_t N2>
		constexpr bool operator==(ConstString<N2> const& ref) const
		{
			if constexpr (N == N2)
			{
				for (std::size_t i = 0; i < N; ++i)
					if (stroage[i] != ref.stroage[i])
						return false;
				return true;
			}
			else
				return false;
		}
	};

	template<ConstString str>
	struct ConstStringHolder {};
}