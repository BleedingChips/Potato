set_project("Potato")

option("PotatoUnitTest", {default = false, description = "Enable Unit Test"})

target("Potato")
    set_kind("static")
    add_files("Potato/*.cpp")
    add_files("Potato/*.ixx")
    add_rules("mode.debug", "mode.release")
    set_languages("cxxlatest")

if has_config("PotatoUnitTest") then

    add_rules("mode.debug", "mode.release")
    set_languages("cxxlatest")

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
end 


--[[
target("PotatoPointerTest")
    set_kind("binary")
    add_files("Test/PointerTest.cpp")
    add_deps("Potato")
    

target("PotatoEbnfTest")
    set_kind("binary")
    add_files("Test/EbnfTest.cpp")
    add_deps("Potato")
    target:set_enable(has_config("PotatoUnitTest"))
--]]