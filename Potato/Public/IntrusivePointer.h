#pragma once
#include <type_traits>

#include "Misc.h"

namespace Potato
{
	// intrustive_ptr ***********************************************************
	struct IntrusivePtrDefaultWrapper
	{
		template<typename Type>
		static void AddRef(Type* t) noexcept { t->AddRef(); }
		template<typename Type>
		static void SubRef(Type* t) noexcept { t->SubRef(); }

		/*
		//overwrite operator();
		template<typename Type, typename ...parameters>
		decltype(auto) operator()(const Type*&, parameters&&... a);

		template<typename Type, typename ...parameters>
		decltype(auto) operator()(Type*&, parameters&&... a);

		//overwrite operator++(int);
		decltype(auto) sub_add_pos(Type*&) const;
		decltype(auto) sub_add_pos(Type*&);

		//overwrite operator--(int);
		decltype(auto) sub_sub_pos(Type*&) const;
		decltype(auto) sub_sub_pos(Type*&);

		//overwrite operator++();
		decltype(auto) sub_add(Type*&) const;
		decltype(auto) sub_add(Type*&);

		//overwrite operator--();
		decltype(auto) sub_sub(Type*&) const;
		decltype(auto) sub_sub(Type*&);
		*/
	};

	template<typename Type, typename Wrapper = IntrusivePtrDefaultWrapper>
	struct IntrusivePtr
	{
		static_assert(!std::is_reference_v<Type>, "IntrusivePtr : Type should not be reference Type");

		IntrusivePtr() noexcept : ptr(nullptr) {}
		IntrusivePtr(Type* t) noexcept : ptr(t) { if (t != nullptr) Wrapper::AddRef(ptr); }
		IntrusivePtr(IntrusivePtr&& ip) noexcept : ptr(ip.ptr) { ip.ptr = nullptr; }
		IntrusivePtr(const IntrusivePtr& ip) noexcept : IntrusivePtr(ip.ptr) {}
		~IntrusivePtr() noexcept { Reset(); }

		bool operator== (const IntrusivePtr& ip) const noexcept { return ptr == ip.ptr; }
		bool operator<(const IntrusivePtr& ip) const noexcept { return ptr < ip.ptr; }

		bool operator!= (const IntrusivePtr& ip) const noexcept { return !((*this) == ip); }
		bool operator<= (const IntrusivePtr& ip) const noexcept { return (*this) == ip || (*this) < ip; }
		bool operator> (const IntrusivePtr& ip) const noexcept { return !((*this) <= ip); }
		bool operator>= (const IntrusivePtr& ip) const noexcept { return !((*this) < ip); }

		operator Type* () const noexcept { return ptr; }

		template<typename require_type, typename = std::enable_if_t<std::is_convertible_v<std::add_const_t<Type*>, require_type*>>>
		operator IntrusivePtr<require_type, Wrapper>() const noexcept
		{
			return static_cast<require_type*>(ptr);
		}

		template<typename require_type, typename = std::enable_if_t<std::is_convertible_v<Type*, require_type*>>>
		operator IntrusivePtr<require_type, Wrapper>() noexcept
		{
			return static_cast<require_type*>(ptr);
		}

		IntrusivePtr& operator=(IntrusivePtr&& ip) noexcept;
		IntrusivePtr& operator=(const IntrusivePtr& ip) noexcept {
			IntrusivePtr tem(ip);
			return operator=(std::move(tem));
		}

		template<typename ...parameter_types>
		decltype(auto) operator()(parameter_types&& ... i)
		{
			return Wrapper{}(ptr, std::forward<parameter_types>(i)...);
		}

		template<typename ...parameter_types>
		decltype(auto) operator()(parameter_types&& ... i) const
		{
			return Wrapper{}(ptr, std::forward<parameter_types>(i)...);
		}

		/*
		decltype(auto) operator++(int) { return Wrapper{}.self_add_pos(ptr); }
		decltype(auto) operator++(int) const { return Wrapper{}.self_add_pos(ptr); }
		decltype(auto) operator--(int) { return Wrapper{}.self_sub_pos(ptr); }
		decltype(auto) operator--(int) const { return Wrapper{}.self_sub_pos(ptr); }
		decltype(auto) operator++() { return Wrapper{}.self_add(ptr); }
		decltype(auto) operator++() const { return Wrapper{}.self_add(ptr); }
		decltype(auto) operator--() { return Wrapper{}.self_sub(ptr); }
		decltype(auto) operator--() const { return Wrapper{}.self_sub(ptr); }
		*/

		Type& operator*() const noexcept { return *ptr; }
		Type* operator->() const noexcept { return ptr; }

		void Reset() noexcept { if (ptr != nullptr) { Wrapper::SubRef(ptr); ptr = nullptr; } }
		operator bool() const noexcept { return ptr != nullptr; }

		template<typename o_type, typename = std::enable_if_t<std::is_base_of_v<Type, o_type> || std::is_base_of_v<o_type, Type>>>
		IntrusivePtr<o_type, Wrapper> CastStatic() const noexcept { return IntrusivePtr<o_type, Wrapper>{static_cast<o_type*>(ptr)}; }

		template<typename TargetType> TargetType* CastDynamic() const noexcept {
			if constexpr (std::is_constructible_v<const Type*, TargetType*>)
				return *this;
			else
				return dynamic_cast<TargetType*>(ptr);
		}

		template<typename TargetType> TargetType* CastSafe() const noexcept { return static_cast<TargetType*>(ptr); }
		template<typename TargetType> TargetType* CastReinterpret() const noexcept { return reinterpret_cast<TargetType*>(ptr); }
		IntrusivePtr<std::remove_const_t<Type>, Wrapper> RemoveConstCast() const noexcept { return const_cast<std::remove_const_t<Type>*>(ptr); };
	private:
		friend Wrapper;
		Type* ptr;
	};

	template<typename Type, typename Wrapper>
	auto IntrusivePtr<Type, Wrapper>::operator=(IntrusivePtr&& ip) noexcept ->IntrusivePtr&
	{
		IntrusivePtr tem(std::move(ip));
		Reset();
		ptr = tem.ptr;
		tem.ptr = nullptr;
		return *this;
	}

	// observer_ptr ****************************************************************************

	struct ObserverPtrDefaultWrapper
	{
		template<typename Type>
		static void AddRef(Type* t) noexcept { }
		template<typename Type>
		static void SubRef(Type* t) noexcept { }
	};

	template<typename Type> using ObserverPtr = IntrusivePtr<Type, ObserverPtrDefaultWrapper>;
}