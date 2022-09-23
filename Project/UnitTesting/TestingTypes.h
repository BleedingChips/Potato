#pragma once
#include <exception>
#include <iostream>

struct UnpassedUnit : public std::exception
{
	char const* StrPtr = nullptr;
	UnpassedUnit(char const* StrPtr) : StrPtr(StrPtr) {}
	UnpassedUnit(UnpassedUnit const&) = default;
	virtual const char* what() const { return StrPtr; }
};

void TestingTMP();
void TestingMisc();
void TestingStrEncode();
void TestingSLRX();
void TestingStrFormat();
void TestingReg();
void TestingEbnf();
void TestingIntrusivePointer();