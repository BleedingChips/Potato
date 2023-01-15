cmake_minimum_required(VERSION 3.20)

add_executable(
	PotatoUnitTest

	EbnfSample.cpp
	IntrusivePointerSample.cpp
	MiscSample.cpp
	RegSample.cpp
	SLRXSample.cpp
	StrEncodeSample.cpp
	StrFormatSample.cpp
	TestingTypes.h
	TMPSample.cpp
	UnitTesting.cpp
)

target_compile_features(PotatoUnitTest PUBLIC cxx_std_23)
target_link_libraries(PotatoUnitTest PUBLIC PotatoLib)