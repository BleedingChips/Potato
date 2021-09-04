#include "../Public/LowLevelVirtualCore.h"
#include <assert.h>
namespace Potato::LowLevelVirtualCore
{

	size_t operator*(DataType Type)
	{
		switch (Type)
		{
		case DataType::I8:
			return sizeof(int8_t);
		case DataType::UI8:
			return sizeof(uint8_t);
		case DataType::I16:
			return sizeof(int16_t);
		case DataType::UI16:
			return sizeof(uint16_t);
		case DataType::I32:
			return sizeof(int32_t);
		case DataType::UI32:
			return sizeof(uint32_t);
		case DataType::F32:
			return sizeof(float);
		case DataType::I64:
			return sizeof(int64_t);
		case DataType::UI64:
			return sizeof(uint64_t);
		case DataType::F64:
			return sizeof(double);
		case DataType::POINTER:
			return sizeof(uint64_t);
		default:
			assert(false);
			return 0;
		}
	}

	void Executor::ReadData(uint64_t& BufferTarget, size_t Length, DataSource Source, uint64_t Data, Table const& Table)
	{
		assert(Length != 0 && Length <= sizeof(uint64_t));
		switch (Source)
		{
		case DataSource::Const:
			assert(Data + Length <= Table.ConstBuffer.size() * sizeof(decltype(Table.ConstBuffer)::value_type));
			std::memcpy(&BufferTarget, reinterpret_cast<std::byte const*>(Table.ConstBuffer.data()) + Data, Length);
			break;
		case DataSource::Stack:
		{
			assert(ProgramCounters.size() >= 1);
			auto Ref = *ProgramCounters.rbegin();
			assert(Ref.LocalStackOffset + Data + Length < StackBuffer.size());
			std::memcpy(&BufferTarget, reinterpret_cast<std::byte const*>(StackBuffer.data()) + Data + Ref.LocalStackOffset, Length);
			break;
		}
		case DataSource::ImmediateAddressing:
			std::memcpy(&BufferTarget, &Data, Length);
			break;
		default:
			assert(false);
			break;
		}
	}

	void Executor::SaveData(uint64_t BufferSource, size_t Length, DataSource Source, uint64_t Data)
	{
		assert(Length != 0 && Length <= sizeof(uint64_t));
		switch (Source)
		{
		case DataSource::Stack:
		{
			assert(ProgramCounters.size() >= 1);
			auto Ref = *ProgramCounters.rbegin();
			assert(Ref.LocalStackOffset + Data + Length < StackBuffer.size());
			std::memcpy(reinterpret_cast<std::byte*>(StackBuffer.data()) + Data + Ref.LocalStackOffset, &BufferSource, Length);
			break;
		}
		default:
			assert(false);
			break;
		}
	}

	template<typename Type> struct CommandAddExecuter
	{
		uint64_t operator()(uint64_t P1, uint64_t P2) {
			Type Result = static_cast<Type>(*reinterpret_cast<Type const*>(&P1) + *reinterpret_cast<Type const*>(&P2));
			uint64_t Data = 0;
			std::memcpy(&Data, &Result, sizeof(Type));
			return Data;
		}
	};

	void Executor::Command_Add(ThreeAdressCode const& Tac, Table const& CommandTable)
	{
		uint64_t R1 = 0, R2 = 0;
		assert(Tac.Type == CommandType::ADD);
		size_t SourceLength = *Tac.SType1;
		ReadData(R1, SourceLength, Tac.P1T, Tac.P1, CommandTable);
		ReadData(R2, SourceLength, Tac.P2T, Tac.P2, CommandTable);
		switch (Tac.SType1)
		{
		case DataType::I8:
			SaveData(CommandAddExecuter<int8_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::I16:
			SaveData(CommandAddExecuter<int16_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::I32:
			SaveData(CommandAddExecuter<int32_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::I64:
			SaveData(CommandAddExecuter<int64_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::UI8:
			SaveData(CommandAddExecuter<uint8_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::UI16:
			SaveData(CommandAddExecuter<uint16_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::UI32:
			SaveData(CommandAddExecuter<uint32_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::UI64:
			SaveData(CommandAddExecuter<uint64_t>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::F32:
			SaveData(CommandAddExecuter<float>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		case DataType::F64:
			SaveData(CommandAddExecuter<double>{}(R1, R2), SourceLength, Tac.P3T, Tac.P3);
			break;
		default:
			break;
		}
	}




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