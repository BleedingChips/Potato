cmake_minimum_required(VERSION 3.25.1)

ENABLE_CPP_MODULE()

add_executable(MiscUnitTest MiscTest.cpp)
target_link_libraries(MiscUnitTest PUBLIC PotatoModule)

add_executable(SmartPtrTest SmartPtrTest.cpp)
target_link_libraries(SmartPtrTest PUBLIC PotatoModule)

add_executable(SLRXTest SLRXTest.cpp)
target_link_libraries(SLRXTest PUBLIC PotatoModule)

add_executable(EncodeTest EncodeTest.cpp)
target_link_libraries(EncodeTest PUBLIC PotatoModule)

add_executable(RegTest RegTest.cpp)
target_link_libraries(RegTest PUBLIC PotatoModule)

add_executable(FormatTest FormatTest.cpp)
target_link_libraries(FormatTest PUBLIC PotatoModule)

add_executable(EbnfTest EbnfTest.cpp)
target_link_libraries(EbnfTest PUBLIC PotatoModule)