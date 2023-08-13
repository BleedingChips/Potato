option("PotatoMainProject", {default = false, description = "Enable Unit Test"})

if (has_config("PotatoMainProject")) then
    set_project("Potato")
end

add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")

target("Potato")
    set_kind("static")
    add_files("Potato/*.cpp")
    add_files("Potato/*.ixx")
    

if has_config("PotatoMainProject") then

    target("TaskSystemTest")
        set_kind("binary")
        add_files("Test/TaskSystemTest.cpp")
        add_deps("Potato")
        

    target("PointerTest")
        set_kind("binary")
        add_files("Test/PointerTest.cpp")
        add_deps("Potato")

    target("EbnfTest")
        set_kind("binary")
        add_files("Test/EbnfTest.cpp")
        add_deps("Potato")

     target("EncodeTest")
        set_kind("binary")
        add_files("Test/EbnfTest.cpp")
        add_deps("Potato")
end 