#pragma once
#include "LowLevelVirtualCore.h"
#include "Misc.h"
#include <assert.h>
#include <span>
namespace Potato::IntermediateRepresentation
{
	using namespace LowLevelVirtualCore;

	struct UserTypeLayout
	{
		size_t Align = 1;
		size_t Size = 0;
		constexpr UserTypeLayout(size_t IAlign = 1, size_t ISize = 0)
			: Align(IAlign), Size(ISize)
		{
			assert(((IAlign - 1) & IAlign) == 0);
			assert(ISize % Align == 0);
		};
		constexpr UserTypeLayout(UserTypeLayout const&) = default;
		constexpr UserTypeLayout& operator=(UserTypeLayout const&) = default;
	};

	using InsideType = LowLevelVirtualCore::DataType;

	struct TypeIndex
	{
		explicit TypeIndex(InsideType InputType) : IsInsideType(true), Index(0) {}
		TypeIndex(TypeIndex const&) = default;
		TypeIndex& operator=(TypeIndex const&) = default;
	private:
		explicit TypeIndex(size_t InputIndex) : IsInsideType(false), Index(InputIndex) {}
		bool IsInsideType = true;
		union {
			size_t Index;
			InsideType Type;
		};
		friend struct TypeProducer;
	};

	using TypeMask = std::optional<TypeIndex>;

	struct TypeDescription
	{
		UserTypeLayout CustomTypeLayout;
		struct Member
		{
			TypeIndex Index;
			size_t ElementCount = 1;
			size_t Offset;
		};
		std::vector<Member> Members;
	};

	struct TypeProducer
	{
		struct MemberModel
		{
			size_t MinAlign = 1;
			std::optional<size_t> MemberFild;
		};
		TypeIndex ForwardDefineType();
		bool StartDefine(TypeIndex Index);
		TypeIndex StartDefine();
		TypeMask FinishDefine(MemberModel const& MM);
		TypeMask FinishDefine(){ MemberModel Default; return FinishDefine(Default); }
		TypeMask InsertMember(TypeIndex Index, size_t ElementCount = 1);
	private:
		struct DefineStack
		{
			TypeIndex Index;
			size_t MemberOffset;
		};
		std::vector<TypeDescription::Member> TemporaryMembers;
		std::vector<std::optional<TypeDescription>> AllDescriptions;
		std::vector<DefineStack> DefineStack;
		static std::tuple<UserTypeLayout, size_t> FixLayout(MemberModel const& MM, UserTypeLayout Main, UserTypeLayout Member, size_t ElementCount);
	};

	struct ConstBufferProducer
	{	
		//void InsertData();
		struct Element
		{
			TypeIndex Index;
			UserTypeLayout ElementLayout;
			size_t ElementCount = 1;
		};
		size_t Insert(Element Element, std::span<std::byte const> Data);
	private:
		struct ElementStorage
		{
			Element Element;
			Potato::IndexSpan<> Data;
		};
		std::vector<uint64_t> ConstBuffer;
		std::vector<ElementStorage> Elements;
		size_t CurrentUsedInByte = 0;
	};

	struct RegisterIndex
	{
		TypeIndex Type;
		DataSource Source;
		size_t Index;
	};

	struct FunctionProducer
	{
		struct TAC
		{
			CommandType Type;
			RegisterIndex P1;
			RegisterIndex P2;
			RegisterIndex P3;
		};
		std::vector<TAC> AllFunctionScore;

	};
}