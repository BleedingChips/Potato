#pragma once
#include <string_view>
#include <optional>
#include <vector>
#include "PotatoIntrusivePointer.h"
namespace Potato::IR
{
	struct ClassLayout
	{
		std::size_t Align = 1;
		std::size_t Size = 0;
		template<typename Type>
		static constexpr ClassLayout Get() { return {alignof(std::remove_cvref_t<Type>), sizeof(std::remove_cvref_t<Type>)}; }
	};

	struct ClassLayoutAssemblerCpp
	{
		ClassLayoutAssemblerCpp() = default;
		ClassLayoutAssemblerCpp(ClassLayoutAssemblerCpp const&) = default;
		ClassLayoutAssemblerCpp& operator=(ClassLayoutAssemblerCpp const&) = default;
		std::optional<std::size_t> InsertMember(ClassLayout MemberLayout);
		ClassLayout GetFinalLayout() const;
	private:
		ClassLayout CurrentLayout;
	};

	struct SymbolTable
	{

		struct Property
		{
			std::u32string Name;
			std::size_t Index;
			std::size_t Area;
			std::size_t FeedbackIndex;
			std::any Data;
			template<typename RequireType>
			RequireType* TryCast() noexcept { return std::any_cast<RequireType>(&Data); }
			template<typename RequireType>
			RequireType const* TryCast() const noexcept { return std::any_cast<RequireType const>(&Data); }
		};

	private:
		
		struct Mapping
		{

			enum State
			{
				UnDefine,
				Active,
				UnActive,
				Destoryed,
			};

			State ActiveState = State::UnDefine;
			std::size_t Index;
		};

		struct AreaStack
		{
			std::size_t MappingIndex;
			std::size_t AreaIndex;
		};

		std::vector<Property> ActiveProperty;
		std::vector<Property> UnactiveProperty;
		std::vector<Mapping> Mappings;
		std::vector<Misc::IndexSpan<>> AreaMapping;
		std::vector<AreaStack> MarkedAreaStack;

		std::size_t InsertSymbol(std::u32string Name, std::size_t FeedbackIndex, std::any Data);
		std::size_t MarkAsAreaBegin();
		std::optional<std::size_t> PopArea(bool Searchable = false, bool KeepProperty = true);
		Misc::ObserverPtr<Property> FindLastActiveSymbol(std::u32string_view Name);
	};

	struct TypeDescription
	{
		enum class TypeT : uint32_t
		{
			Unsighed,
			Sighed,
			Float
		};

		TypeT Type : 2;
		uint32_t Value : 30;
	};


}