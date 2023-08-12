set_project("Potato")
add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")

target("Potato")
    set_kind("static")
    add_files("Potato/*.cpp")
    add_files("Potato/*.ixx")

target("PotatoTaskSystemTest")
    set_kind("binary")
    add_files("Test/TaskSystemTest.cpp")
    add_deps("Potato")

target("PotatoPointerTest")
    set_kind("binary")
    add_files("Test/PointerTest.cpp")
    add_deps("Potato")

target("PotatoEbnfTest")
    set_kind("binary")
    add_files("Test/EbnfTest.cpp")
    add_deps("Potato")


