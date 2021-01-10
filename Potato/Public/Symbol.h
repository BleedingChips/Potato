#pragma once
#include <string_view>
#include <map>
#include <vector>
#include <optional>
#include <assert.h>
#include <variant>
#include <typeindex>
#include <any>
#include "Lexical.h"
#include <optional>
namespace Potato::Symbol
{

	using Section = Lexical::Section;
	
	struct Table
	{
		struct Mask
		{
			Mask(const Mask&) = default;
			Mask(size_t inputindex) : index(inputindex + 1){}
			Mask() =default;
			Mask& operator=(Mask const&) = default;
			operator bool() const noexcept { return index != 0; }
		private:
			friend struct Table;
			size_t index = 0;
		};
		
		Mask FindActiveLast(std::u32string_view name) const noexcept;
		std::vector<Mask> FindActiveAll(std::u32string_view name) const;

		enum class ResultType
		{
			Inavailable,
			ExistButNotRequireType,
			Available
		};
		
		template<typename RequireType>
		struct Result
		{
			ResultType type = ResultType::Inavailable;
			std::u32string_view name;
			Section section;
			RequireType* require_data = nullptr;
			operator bool () const noexcept{return type == ResultType::Available;}
			RequireType* operator->(){return require_data;}
			bool Exist() const noexcept{return type != ResultType::Inavailable;}
		};

		Result<const std::any> FindRaw(Mask mask) const;

		Result<std::any> FindRaw(Mask mask)
		{
			auto result = static_cast<Table const*>(this)->FindRaw(mask);
			return {result.type, result.name, result.section, const_cast<std::any*>(result.require_data) };
		}

		template<typename RequireType>
		auto Find(Mask mask) ->Result<std::remove_reference_t<RequireType>>
		{
			auto result = static_cast<Table const*>(this)->Find<RequireType>(mask);
			auto ptr = const_cast<std::remove_reference_t<RequireType>*>(result.require_data);
			return {result.type, result.name, result.section, ptr};
		}

		template<typename RequireType>
		auto Find(Mask mask) const ->Result<std::add_const_t<std::remove_reference_t<RequireType>>>
		{
			auto result = FindRaw(mask);
			auto ptr = std::any_cast<std::add_const_t<std::remove_reference_t<RequireType>>>(result.require_data);
			if(ptr != nullptr)
				return { ResultType::Available, result.name, result.section, ptr };
			else if(result.type == ResultType::Available)
				return { ResultType::ExistButNotRequireType, result.name, result.section, ptr };
			else
				return { ResultType::Inavailable, {}, {}, nullptr };
		}

		size_t PopElementAsUnactive(size_t count);

		std::vector<Mask> PopAndReturnElementAsUnactive(size_t count);

		Mask Insert(std::u32string_view name, std::any property, Section section = {});

		Table(Table&&) = default;
		Table(Table const&) = default;
		Table() = default;

	private:

		struct Storage
		{
			std::u32string_view name;
			Mask index;
			Section section;
			std::any property;
		};
		
		struct Mapping
		{
			bool is_active;
			size_t index;
		};
		
		std::vector<Storage> unactive_scope;
		std::vector<Storage> active_scope;
		std::vector<Mapping> mapping;
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