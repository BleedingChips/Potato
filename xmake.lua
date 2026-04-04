add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.profile")
set_languages("cxxlatest")
add_requires("ctre")

target("Potato")
    set_kind("static")
    add_files("Potato/*.cpp")
    add_packages("ctre", {public = true})
    add_headerfiles("Potato/HeaderUnits/*.hpp", {public = true})
    add_files("Potato/*.ixx", {public = true})
target_end()

if os.scriptdir() == os.projectdir() then
    set_project("Potato")

    for _, file in ipairs(os.files("Test/*.cpp")) do
        local name = "ZTest_" .. path.basename(file)
        target(name)
            set_kind("binary")
            add_files(file)
            add_deps("Potato")
        target_end()
    end
end