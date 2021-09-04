#include "../Public/IntermediateRepresentation.h"
namespace Potato::IntermediateRepresentation
{
	TypeIndex TypeProducer::ForwardDefineType()
	{
		TypeIndex Index{AllDescriptions.size()};
		AllDescriptions.push_back({});
		return Index;
	}

	bool TypeProducer::StartDefine(TypeIndex Index)
	{
		if (!Index.IsInsideType && Index.Index < AllDescriptions.size() && !AllDescriptions[Index.Index].has_value())
		{
			for (auto& Ite : DefineStack)
			{
				if(Ite.Index.Index == Index.Index)
					return false;
			}
			DefineStack.push_back({Index, TemporaryMembers.size()});
			return true;
		}
		return false;
	}

	TypeIndex TypeProducer::StartDefine()
	{
		TypeIndex Index{ AllDescriptions.size() };
		AllDescriptions.push_back({});
		DefineStack.push_back({ Index, TemporaryMembers.size() });
		return Index;
	}

	TypeMask TypeProducer::InsertMember(TypeIndex Index, size_t ElementCount)
	{
		if (!DefineStack.empty())
		{
			assert(Index.IsInsideType || (Index.Index < AllDescriptions.size() && AllDescriptions[Index.Index].has_value()));
			TemporaryMembers.push_back(TypeDescription::Member{Index, ElementCount, 0});
			return DefineStack.rbegin()->Index;
		}
		return {};
	}

	TypeMask TypeProducer::FinishDefine(MemberModel const& MM)
	{
		if (!DefineStack.empty())
		{
			auto Ite = *DefineStack.rbegin();
			DefineStack.pop_back();
			assert(Ite.Index.Index < AllDescriptions.size());
			assert(!AllDescriptions[Ite.Index.Index].has_value());
			assert(Ite.MemberOffset < TemporaryMembers.size());
			TypeDescription Description;
			Description.Members.insert(Description.Members.end(), std::move_iterator(TemporaryMembers.begin() + Ite.MemberOffset), std::move_iterator(TemporaryMembers.end()));
			TemporaryMembers.erase(TemporaryMembers.begin() + Ite.MemberOffset, TemporaryMembers.end());
			Description.CustomTypeLayout.Align = MM.MinAlign;
			for (auto& Ite : Description.Members)
			{
				UserTypeLayout MemberLayout;
				if (Ite.Index.IsInsideType)
				{
					MemberLayout.Align = *(Ite.Index.Type);
					MemberLayout.Size = MemberLayout.Align;
				}
				else {
					assert(Ite.Index.Index < AllDescriptions.size() && AllDescriptions[Ite.Index.Index].has_value());
					MemberLayout = AllDescriptions[Ite.Index.Index]->CustomTypeLayout;
				}
				std::tie(Description.CustomTypeLayout, Ite.Offset) = FixLayout(MM, Description.CustomTypeLayout, MemberLayout, Ite.ElementCount);
			}
			auto SubSpace = Description.CustomTypeLayout.Size % Description.CustomTypeLayout.Align;
			if (SubSpace != 0)
				Description.CustomTypeLayout.Size += Description.CustomTypeLayout.Align - SubSpace;
			AllDescriptions[Ite.Index.Index] = Description;
			return Ite.Index;
		}
		return {};
	}

	std::tuple<UserTypeLayout, size_t> TypeProducer::FixLayout(MemberModel const& MM, UserTypeLayout Main, UserTypeLayout Sub, size_t ElementCount)
	{
		if (MM.MemberFild.has_value())
			Sub.Align = std::max(Sub.Align, *MM.MemberFild);
		Main.Align = std::max(Main.Align, Sub.Align);
		auto SubSpace = Main.Size % Sub.Align;
		if (SubSpace != 0)
			Main.Size += Sub.Align - SubSpace;
		size_t offset = Main.Size;
		Main.Size += Sub.Size * ElementCount;
		return {Main, offset};
	}
}

