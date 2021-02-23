#pragma once
#include <string_view>
#include <map>
#include <vector>
#include <optional>
#include <cassert>
#include <variant>
#include <typeindex>
#include <any>
#include "Lexical.h"
#include <optional>
#include "Misc.h"
#include "tmp.h"
#include <span>
namespace Potato::Grammar
{

	using Section = Lexical::Section;

	struct Mask
	{
		Mask(const Mask&) = default;
		Mask(size_t i_index) : index(i_index){ assert(i_index < std::numeric_limits<size_t>::max()); }
		Mask() = default;
		Mask& operator=(Mask const&) = default;
		operator bool() const noexcept { return index != std::numeric_limits<size_t>::max(); }
		operator size_t() const noexcept{ return index; }
		size_t AsIndex() const noexcept {return index; }
	private:
		size_t index = std::numeric_limits<size_t>::max();
	};

	template<Potato::Tmp::const_string>
	struct MaskWrapper : Mask
	{
		using Mask::Mask;
		MaskWrapper& operator=(MaskWrapper const&) = default;
	};

	using SymbolMask = MaskWrapper<"Symbol">;
	using SymbolAreaMask = MaskWrapper<"SymbolArea">;
	
	struct Symbol
	{

		struct Property
		{
			std::u32string_view name;
			SymbolMask index;
			SymbolAreaMask area;
			Section section;
			std::any property;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&property); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const*>(&property); }
		};
		
		SymbolMask FindActiveSymbolAtLast(std::u32string_view name) const noexcept;
		std::vector<SymbolMask> FindActiveAllSymbol(std::u32string_view name) const;
		Property* FindSymbol(SymbolMask mask) { return const_cast<Property*>(static_cast<Symbol const*>(this)->FindSymbol(mask));  }
		Property const* FindSymbol(SymbolMask mask) const;
		std::span<Property const> FindArea(SymbolAreaMask mask) const noexcept;
		std::span<Property> FindArea(SymbolAreaMask mask) noexcept
		{
			auto re = static_cast<Symbol const*>(this)->FindArea(mask);
			return {const_cast<Property*>(re.data()), re.size()};
		}

		SymbolAreaMask PopElementAsUnactive(size_t count);

		SymbolMask Insert(std::u32string_view name, std::any property, Section section = {});

		Symbol(Symbol&&) = default;
		Symbol(Symbol const&) = default;
		Symbol() = default;

	private:
		
		struct Mapping
		{
			bool is_active;
			size_t index;
		};
		std::vector<Property> unactive_scope;
		std::vector<Property> active_scope;
		std::vector<Mapping> mapping;
		std::vector<std::tuple<size_t, size_t>> areas;
	};

	struct TypeProperty
	{
		std::vector<SymbolMask> member;
	};

	using ValueMask = MaskWrapper<"Value">;
	
	struct Value
	{

		template<typename Data>
		ValueMask InsertValue(SymbolMask mask, Data const& data) { return InsertValue(mask, reinterpret_cast<std::byte const*>(&data), sizeof(data)); }
		ValueMask InsertValue(SymbolMask mask, std::byte const* data, size_t length);
		ValueMask ReservedLazyValue();
		bool InsertLazyValue(SymbolMask index, std::byte const* data, size_t length);

	private:

		enum class Style
		{
			Real,
			Lazy,
			LazyInited
		};
		
		struct Property
		{
			SymbolMask type;
			Style style;
			size_t start_index;
			size_t length;
		};
		
		std::vector<std::byte> value_buffer;
		std::vector<Property> value_mapping;
	};

	struct MemoryModel
	{
		size_t align = 0;
		size_t size = 0;
	};

	struct MemoryModelMaker
	{
		
		struct HandleResult
		{
			size_t align = 0;
			size_t size_reserved = 0;
		};

		size_t operator()(MemoryModel const& info);
		operator MemoryModel() const { return Finalize(info); }

		MemoryModelMaker(MemoryModel info = {}) : info(std::move(info)) {}
		MemoryModelMaker(MemoryModelMaker const&) = default;
		MemoryModelMaker& operator=(MemoryModelMaker const&) = default;

		static size_t MaxAlign(MemoryModel out, MemoryModel in) noexcept;
		static size_t ReservedSize(MemoryModel out, MemoryModel in) noexcept;

	private:
		
		virtual HandleResult Handle(MemoryModel cur, MemoryModel input) const;
		virtual MemoryModel Finalize(MemoryModel cur) const;
		MemoryModel info;
	};

}