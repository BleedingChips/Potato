
cmake_minimum_required(VERSION 3.26)

project(
	Potato
	DESCRIPTION "Personal Library By BleedingChips"
	VERSION 1.0
	LANGUAGES C CXX
)

macro(ENABLE_CPP_MODULE)
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
endmacro()

macro(ENABLE_CMAKE_CPP_MODULE)

	if (MSVC)
	  set(CMAKE_EXPERIMENTAL_CXX_SCANDEP_DEPFILE_FORMAT "msvc")
	  set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "msvc")
	  set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "@<MODULE_MAP_FILE>")
	endif ()

	message(STATUS "Current CMake Version :" ${CMAKE_VERSION})

	if(${CMAKE_VERSION} EQUAL 3.25.1)
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API  "3c375311-a3c9-4396-a187-3227ef642046")
	elseif(${CMAKE_VERSION} EQUAL 3.26)
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API  "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
	else()
		message(FATAL_ERROR "Unsupport CMAKE Version")
	endif()

endmacro()

ENABLE_CMAKE_CPP_MODULE()
ENABLE_CPP_MODULE()

add_library(PotatoModule STATIC)

if(MSVC)

	file(COPY $ENV{VCTOOLSINSTALLDIR}modules DESTINATION ${PROJECT_BINARY_DIR}/stdxx)
	#file(COPY "C:/Program Files(x86)/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.36.32532/module" DESTINATION ${PROJECT_BINARY_DIR}/stdxx)

	add_library(MSVCSTD STATIC)

	target_sources(
		MSVCSTD PUBLIC
		FILE_SET cxx_modules TYPE CXX_MODULES
		BASE_DIRS
			${PROJECT_BINARY_DIR}/stdxx/modules/
		FILES
			${PROJECT_BINARY_DIR}/stdxx/modules/std.ixx
			${PROJECT_BINARY_DIR}/stdxx/modules/std.compat.ixx
	)
	target_compile_features(MSVCSTD PUBLIC cxx_std_23)

	target_link_libraries(PotatoModule PUBLIC MSVCSTD)

endif()

target_sources(
	PotatoModule PUBLIC
	FILE_SET cxx_modules TYPE CXX_MODULES
	FILES
		#"Potato.ixx"
		"Potato/PotatoMisc.ixx"
		"Potato/PotatoSTD.ixx"
		"Potato/PotatoTMP.ixx"
		"Potato/PotatoSmartPtr.ixx"
		"Potato/PotatoInterval.ixx"
		"Potato/PotatoIR.ixx"
		"Potato/PotatoSLRX.ixx"
		"Potato/PotatoReg.ixx"
		"Potato/PotatoFormat.ixx"
		"Potato/PotatoEncode.ixx"
		"Potato/PotatoEBNF.ixx"
		"Potato/PotatoDocument.ixx"
		"Potato/PotatoTaskSystem.ixx"
)

target_sources(
	PotatoModule PRIVATE
		"Potato/PotatoIR.cpp"
		"Potato/PotatoReg.cpp"
		"Potato/PotatoSLRX.cpp"
		"Potato/PotatoEncode.cpp"
		"Potato/PotatoEBNF.cpp"
		"Potato/PotatoDocument.cpp"
		"Potato/PotatoTaskSystem.cpp"
)


target_compile_features(PotatoModule PUBLIC cxx_std_23)


#add_subdirectory("Potato")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

	add_executable(MiscUnitTest "Test/MiscTest.cpp")
	target_link_libraries(MiscUnitTest PUBLIC PotatoModule)

	add_executable(IntervalTest "Test/IntervalTest.cpp")
	target_link_libraries(IntervalTest PUBLIC PotatoModule)

	add_executable(SmartPtrTest "Test/SmartPtrTest.cpp")
	target_link_libraries(SmartPtrTest PUBLIC PotatoModule)

	add_executable(SLRXTest "Test/SLRXTest.cpp")
	target_link_libraries(SLRXTest PUBLIC PotatoModule)

	add_executable(EncodeTest "Test/EncodeTest.cpp")
	target_link_libraries(EncodeTest PUBLIC PotatoModule)

	add_executable(RegTest "Test/RegTest.cpp")
	target_link_libraries(RegTest PUBLIC PotatoModule)

	add_executable(FormatTest "Test/FormatTest.cpp")
	target_link_libraries(FormatTest PUBLIC PotatoModule)

	add_executable(EbnfTest "Test/EbnfTest.cpp")
	target_link_libraries(EbnfTest PUBLIC PotatoModule)

	add_executable(TaskSystemTest "Test/TaskSystemTest.cpp")
	target_link_libraries(TaskSystemTest PUBLIC PotatoModule)

endif()
