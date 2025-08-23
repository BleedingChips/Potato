export module PotatoTMP;

import std;

namespace Implement
{

	template<std::size_t index, typename T, typename ...AT>
	struct LocateByTypeImp
	{
		static constexpr std::size_t Value = index + 1;
	};

	template<std::size_t index, typename T, typename CT, typename ...AT>
	struct LocateByTypeImp<index, T, CT, AT...>
	{
		static constexpr std::size_t Value = LocateByTypeImp<index + 1, T, AT...>::Value;
	};

	template<std::size_t index, typename T, typename ...AT>
	struct LocateByTypeImp<index, T, T, AT...>
	{
		static constexpr std::size_t Value = index;
	};

	template<std::size_t index, typename T>
	struct LocateByTypeImp<index, T, T>
	{
		static constexpr std::size_t Value = index;
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

	// FindByIndex

	template<size_t index, typename ...AT>
	struct FindByIndex;

	template<std::size_t index, typename CT, typename ...AT>
	struct FindByIndex<index, CT, AT...>
	{
		using Type = FindByIndex<index - 1, AT...>::Type;
	};

	template<typename CT>
	struct FindByIndex<0, CT>
	{
		using Type = CT;
	};

	template<typename CT, typename ...AT>
	struct FindByIndex<0, CT, AT...>
	{
		using Type = CT;
	};

	template<typename ...Type> struct TypeTuple {
		static constexpr size_t Size = sizeof...(Type);
		template<size_t Index> using Get = typename FindByIndex<Index, Type...>::Type;
	};

	// LocateByType

	template<typename T, typename ...AT>
	struct LocateByType
	{
		static constexpr std::size_t Value = Implement::LocateByTypeImp<0, T, AT...>::Value;
	};

	// Replace

	template<typename Type> struct Replace;
	template<template<typename ...> class Holder, typename ...Type>
	struct Replace<Holder<Type...>>
	{
		template<template<typename ...> class OutHolder> using With = OutHolder<Type...>;
	};

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

	template<typename FuncT>
	concept RequireMemberFunctionPointer = std::is_member_function_pointer_v<FuncT>;

	template<typename FuncT>
	struct SubFunctionInfo;

	template<typename FuntionType, typename Owner>
	struct SubFunctionInfo<FuntionType Owner::*> : public FunctionInfo<FuntionType>
	{
		using OwnerType = Owner;
	};

	template<RequireMemberFunctionPointer FuncT>
	struct FunctionInfo<FuncT> : SubFunctionInfo<FuncT>
	{
		
	};

	template<typename FuncT>
	concept RequireCertainlyOperatorParentheses = requires(FuncT fun)
	{
		{&FuncT::operator()};
	};

	template<RequireCertainlyOperatorParentheses FuncT>
	struct FunctionInfo<FuncT> : FunctionInfo<decltype(&FuncT::operator())>
	{

	};

	template<typename FuncT>
	concept  RequireDetectableFunction = std::is_function_v<std::remove_cvref_t<FuncT>> || std::is_member_function_pointer_v<std::remove_cvref_t<FuncT>> || RequireCertainlyOperatorParentheses<std::remove_cvref_t<FuncT>>;

	template<typename CharT, std::size_t N>
	struct TypeString
	{
		CharT string[N];
		constexpr std::basic_string_view<CharT> GetStringView() const { return {string}; }
		constexpr TypeString(const CharT(&str)[N]) : string{}
		{
			std::copy_n(str, N, string);
		}
		template<typename CharT2, std::size_t N2>
		constexpr bool operator==(TypeString<CharT2, N2> const& ref) const
		{
			if constexpr (std::is_same_v<CharT, CharT> && N == N2)
			{
				return std::equal(string, string + N, ref.string, N);
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

	template<typename FuncT>
	struct FunctionRef;

	template<typename ReturnT, typename ...ParameterT>
	struct FunctionRef<ReturnT(ParameterT...)>
	{
		using FunctionT = ReturnT(*)(ParameterT...);
		using CallableObjectFunctionT = ReturnT(*)(void*, ParameterT...);
		operator bool() const {
			return function_ptr.normal != nullptr;
		}
		FunctionRef() { function_ptr.normal = nullptr; }
		FunctionRef(FunctionT function) { function_ptr.normal = function; }
		template<typename CallableObjectT>
		FunctionRef(CallableObjectT&& object) requires(std::is_convertible_v<CallableObjectT, FunctionT>)
			: FunctionRef(static_cast<FunctionT>(object)) {}
		template<typename CallableObjectT>
		FunctionRef(CallableObjectT&& object) requires(
			!std::is_convertible_v<CallableObjectT, FunctionT> 
			&& std::is_invocable_r_v<ReturnT, CallableObjectT, ParameterT...>
			)
		{
			function_ptr.callable_object = [](void* pointer, ParameterT&&... parameter) -> ReturnT {
				return (*static_cast<CallableObjectT*>(pointer))(std::forward<ParameterT>(parameter)...);
				};
			callable_object = static_cast<void*>(&object);
		}
		FunctionRef(FunctionRef const&) = default;
		FunctionRef(FunctionRef&& other)
		{
			callable_object = other.callable_object;
			function_ptr.normal = other.function_ptr.normal;
			other.function_ptr.normal = nullptr;
			other.callable_object = nullptr;
		}
		template<typename ...OtherParameterT>
		ReturnT operator()(OtherParameterT&&... pars) const
		{
			if (callable_object != nullptr)
			{
				return (*function_ptr.callable_object)(callable_object, std::forward<OtherParameterT&&>(pars)...);
			}
			else
			{
				return (*function_ptr.normal)(std::forward<OtherParameterT&&>(pars)...);
			}
		}
	protected:
		void* callable_object = nullptr;
		union FunctionPointerT
		{
			FunctionT normal;
			CallableObjectFunctionT callable_object;
		}function_ptr;
	};
}