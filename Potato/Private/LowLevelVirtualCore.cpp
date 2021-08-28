#include "../Public/LowLevelVirtualCore.h"
namespace Potato
{
	/*
	bool LowLevelScriptCore::operator()(LowLevelScriptTable const& Table)
	{
		using TAC = LowLevelScriptTAC;
		using CommandT = LowLevelScriptCommandType;
		using DataT = LowLevelScriptDataType;
		using SourceT = LowLevelScriptSourceType;

		stack_buffer.clear();
		member_buffer.clear();
		pc_contents.clear();
		static_assert(std::is_same_v<decltype(stack_buffer)::value_type, decltype(Table.const_buffer)::value_type>);
		std::memcpy(stack_buffer.data(), member_buffer.data(), member_buffer.size() * sizeof(decltype(Table.const_buffer)::value_type));
		pc_contents.push_back({ 0, 0 });
		while (!pc_contents.empty())
		{
			auto pc_ite = *pc_contents.rbegin();
			pc_contents.pop_back();
			while (pc_ite.command_offset < Table.commands.size())
			{
				auto const command = Table.commands[pc_ite.command_offset];
				++pc_ite.command_offset;
				switch (command.type)
				{
				case CommandT::ADD:
					Command_Add(command);
					break;
				case CommandT::SUB:
					Command_Sub(command);
					break;
				}
			}
		}
		return false;
	}


	void LowLevelScriptCore::Command_Add(LowLevelScriptTAC const& Tac)
	{
		assert(Tac.type == CommandT::);

	}
	*/
}