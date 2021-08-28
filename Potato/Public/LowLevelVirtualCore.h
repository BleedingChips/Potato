#pragma once
#include <stdint.h>
#include <vector>
#include <optional>
namespace Potato::LowLevelVirtualCore
{

	enum class CommandT : uint8_t
	{
		ADD,
		SUB,
		MUL,
		DEV,
		MOD,

		AND,
		OR,
		NOT,

		CAST,

		ALLOCATE,
		RELEASE,

		READ,
		WRITE,

		MOVE,

		CALL,
		RET,

		JUMP,
		IFJUMP,
	};

	enum class DataT : uint8_t
	{
		UI8,
		UI16,
		UI32,
		UI64,

		I8,
		I16,
		I32,
		I64,

		F32,
		F64,

		POINTER,
	};

	enum class DataSourceT : uint8_t
	{
		UnKnow,
		Const,
		Stack,
		Member,
		ImmediateAddressing,
	};

	/*

	struct LowLevelScriptTAC
	{
		LowLevelScriptCommandType type;
		LowLevelScriptDataType sub_type;
		LowLevelScriptDataType sub_type2;
		LowLevelScriptSourceType p1t;
		LowLevelScriptSourceType p2t;
		LowLevelScriptSourceType p3t;
		uint64_t p1;
		uint64_t p2;
		uint64_t p3;
	};

	struct LowLevelScriptTable
	{
		using TAC = LowLevelScriptTAC;
		using CommandT = LowLevelScriptCommandType;
		using DataT = LowLevelScriptDataType;
		using SourceT = LowLevelScriptSourceType;
		std::vector<uint64_t> const_buffer;
		std::vector<LowLevelScriptTAC> commands;
		std::optional<uint64_t> start_ptr;
	};

	struct LowLevelScriptCore
	{
		
		bool operator()(LowLevelScriptTable const& Table);
		
	private:
		
		struct PCContent
		{
			size_t command_offset;
			size_t stack_offset;
		};

		std::vector<uint64_t> stack_buffer;
		std::vector<uint64_t> member_buffer;
		std::vector<PCContent> pc_contents;
		void Command_Add(LowLevelScriptTAC const& Tac);
		void Command_Sub(LowLevelScriptTAC const& Tac);
	};
	*/

}
