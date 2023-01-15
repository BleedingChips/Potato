cmake_minimum_required(VERSION 3.20)

add_library(
	PotatoLib
	STATIC

	PotatoDocument.cpp
	PotatoEbnf.cpp
	PotatoIR.cpp
	PotatoMisc.cpp
	PotatoReg.cpp
	PotatoSLRX.cpp
	PotatoStrEncode.cpp
	PotatoStrFormat.cpp
	PotatoVirtualPath.cpp
)

target_compile_features(PotatoLib PUBLIC cxx_std_23)
target_include_directories(PotatoLib PUBLIC "../include/")