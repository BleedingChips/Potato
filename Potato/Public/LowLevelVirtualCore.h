#pragma once
#include <stdint.h>
#include <vector>
#include <optional>
namespace Potato::LowLevelVirtualCore
{
	enum class CommandType : uint8_t
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

	enum class DataType : uint8_t
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

	size_t operator*(DataType Type);

	enum class DataSource : uint8_t
	{
		UnKnow,
		Const,
		Stack,
		Memory, // do not support yet
		ImmediateAddressing,
	};

	struct ThreeAdressCode
	{
		CommandType Type;
		DataType SType1 : 4;
		DataType SType2 : 4;
		DataSource P1T : 4;
		DataSource P2T : 4;
		DataSource P3T : 4;
		uint64_t P1;
		uint64_t P2;
		uint64_t P3;
	};

	struct Table
	{
		using ThreeAdressCode = ThreeAdressCode;
		std::vector<uint64_t> ConstBuffer;
		std::vector<ThreeAdressCode> Commands;
		uint64_t EnterFunctionOutputStackLength = 0;
		uint64_t EnterFunctionTotalStack = 0;
	};

	struct ProgramCounter
	{
		uint64_t CommandOffset = 0;
		uint64_t LocalStackOffset = 0;
		uint64_t CurrentStackUsingLength = 0;
		uint64_t CurrentStackTotalLength = 0;
	};

	struct Executor
	{

		bool operator()(Table const& CommandTable);
		
	private:
		
		struct PCContent
		{
			size_t command_offset;
			size_t stack_offset;
		};

		std::vector<uint64_t> StackBuffer;
		//std::vector<uint64_t> MemberBuffer;
		std::vector<ProgramCounter> ProgramCounters;
		void ReadData(uint64_t& BufferTarget, size_t Length, DataSource Source, uint64_t Data, Table const& Table);
		void SaveData(uint64_t BufferSource, size_t Length, DataSource Source, uint64_t Data);
		void Command_Add(ThreeAdressCode const& Tac, Table const& CommandTable);
		void Command_Sub(ThreeAdressCode const& Tac, Table const& CommandTable);
	};

}
