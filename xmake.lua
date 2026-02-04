---@diagnostic disable: undefined-global
set_project "avalon"
set_version "0.1.0"
set_languages "c++23"
set_toolchains "clang"
add_cxxflags("-stdlib=libstdc++", {force = true})
add_ldflags("-stdlib=libstdc++", {force = true})
set_policy("build.c++.modules", true)
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })
add_requires("vulkan","directx-shader-compiler", { system = true })
add_requires("glfw", "glm", "spdlog")
set_config("mode", "debug")
set_runtimes(is_mode("debug") and "MDd" or "MD")

option("enable_tests")
    set_default(true)
    set_showmenu(true)

if has_config("enable_tests") then
    includes("tests")
end

includes("src/avalon")
includes("tests/mocks")

