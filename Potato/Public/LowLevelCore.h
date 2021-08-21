#pragma once
#include <stdint.h>
#include <vector>
#include <optional>
namespace Potato
{

	enum class LowLevelScriptCommandType : uint8_t
	{
		ADD,
		SUB,
		MUL,
		DEV,
		MOD,

		AND,
		OR,
		NOT,

		CAST_UI8,
		CAST_UI16,
		CAST_UI32,
		CAST_UI64,

		CAST_I8,
		CAST_I16,
		CAST_I32,
		CAST_I64,

		CAST_F32,
		CAST_F64,

		EQUAL,

		CALL,
		RET,

		JMP,
		IFJUMP,
	};

	enum class LowLevelScriptSourceType : uint8_t
	{
		UnKnow,
		Const,
		Stack,
		Member,

		IA_UI8,
		IA_UI16,
		IA_UI32,
		IA_UI64,

		IA_I8,
		IA_I16,
		IA_I32,
		IA_I64,

		IA_F32,
		IA_F64,
	};

	struct LowLevelScriptSource
	{
		LowLevelScriptSourceType Type;
		uint64_t index;
	};

	struct LowLevelScriptTAC
	{
		LowLevelScriptCommandType type;
		LowLevelScriptSourceType p1t;
		LowLevelScriptSourceType p2t;
		LowLevelScriptSourceType rt;
		uint64_t P1;
		uint64_t P2;
		uint64_t R;
	};

	struct LowLevelScriptTable
	{
		std::vector<std::byte> const_buffer;
		std::vector<LowLevelScriptTAC> command;
		std::optional<uint64_t> start_ptr;
		void Link(const LowLevelScriptTable const&);
	};

	struct LowLevelScriptCore
	{
		
		bool operator()(LowLevelScriptTable const& Table);
		
	private:
		
		std::vector<std::byte> stack_buffer;
		std::vector<std::byte> member_buffer;

	};


}
