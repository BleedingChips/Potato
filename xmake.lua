add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.profile")
set_languages("cxxlatest")


target("Potato")
    set_kind("static")
    add_files("Potato/*.cpp")
    add_files("Potato/*.ixx", {public = true})
target_end()

if os.scriptdir() == os.projectdir() then
    add_requires("ctre")
    set_project("Potato")

    for _, file in ipairs(os.files("Test/*.cpp")) do
        local name = "ZTest_" .. path.basename(file)
        target(name)
            set_kind("binary")
            add_files(file)
            add_deps("Potato")
            add_packages("ctre", {public = true})
        target_end()
    end
end