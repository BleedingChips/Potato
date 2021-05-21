#pragma once
#include <stdint.h>
#include "Types.h"
#include "Misc.h"
namespace Potato::IR
{
	struct IR
	{
		struct TAC
		{
			int64_t operator_enum;
			int64_t source1;
			int64_t source2;
			int64_t result;
		};
		std::vector<std::byte> const_buffer;
		std::vector<IndexSpan<>> const_register;
		std::vector<TAC> command;
	};
}