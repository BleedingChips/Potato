#pragma once
#include <vector>
#include <variant>
#include <string>
#include <cassert>
#include "../Public/StrScanner.h"
#include "../Public/StrEncode.h"
#include <sstream>
namespace Potato
{
	void Scanner<std::u32string>::Scan(std::u32string_view par, std::u32string& Input)
	{
		Input = par;
	}

	void Scanner<int32_t>::Scan(std::u32string_view par, int32_t& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
	}

	void Scanner<uint32_t>::Scan(std::u32string_view par, uint32_t& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
	}

	void Scanner<int64_t>::Scan(std::u32string_view par, int64_t& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
	}

	void Scanner<uint64_t>::Scan(std::u32string_view par, uint64_t& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
	}

	void Scanner<float>::Scan(std::u32string_view par, float& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
	}

	void Scanner<double>::Scan(std::u32string_view par, double& Input)
	{
		std::stringstream ss;
		auto str = AsStrWrapper(par).ToString<char8_t>();
		std::string_view sv(reinterpret_cast<char*>(str.data()), str.size());
		ss << sv;
		ss >> Input;
	}
	
	
}