---@diagnostic disable: undefined-global
set_project "avalon"
set_version "0.1.0"
set_languages "c++23"
set_toolchains "clang"
set_policy("build.c++.modules", true)
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })
add_requires("vulkan","directx-shader-compiler", { system = true })
add_requires("glfw", "glm", "spdlog")

option "vulkan"
set_default(true)
set_showmenu(true)
set_description "Enable Vulkan renderer"

option "dx12"
set_default(false)
set_showmenu(true)
set_description "Enable dx12 renderer"

option "glfw"
set_default(true)
set_showmenu(true)
set_description "Enable GLFW window backend"

function add_avalon_api_rules(target_name)
    local prefix = string.upper(target_name):gsub("%.", "_")
    if is_plat("windows") then
        add_defines(prefix.."_API=__declspec(dllexport)", {interface = false})
        add_defines(prefix.."_API=__declspec(dllimport)", {interface = true})
    else
        add_defines(prefix.."_API=__attribute__((visibility(\"default\")))", {public = true})
    end
end

function add_rhi_backend(api)
    local path = "src/avalon/rhi"
    local prefix = (api == "vulkan") and "vk" or api
    add_files(format("%s/%s/%s_*.cppm", path, api, prefix), { public = true })
    add_files(format("%s/%s/%s_*.cpp", path, api, prefix))
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/plugins")
    add_packages(api)
end

function add_window_backend(name)
    local path = "src/avalon/window"
    add_files(format("%s/%s/%s_window.cppm", path, name, name), { public = true })
    add_files(format("%s/%s/%s_window.cpp", path, name, name))
    add_defines("AVALON_ENABLE_WINDOW_" .. string.upper(name))
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/plugins")
    add_packages(name)
end

target "avalon.core"
    set_kind "shared"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"
    add_avalon_api_rules("avalon.core")
    add_files("src/avalon/core/*.cppm", {public = true})
    add_files("src/avalon/core/*.cpp")
    add_files("src/avalon/rhi/rhi.cppm", {public = true})
    add_files("src/avalon/window/window.cppm", {public = true})

    add_includedirs("src", {public = true})
    add_packages("spdlog")


target "avalon.rhi.vulkan"
    set_kind "shared"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"
    add_avalon_api_rules("avalon.rhi.vulkan")
    add_deps("avalon.core")

    if is_plat "linux" then
    add_defines "VK_USE_PLATFORM_WAYLAND_KHR"
    add_defines "VK_USE_PLATFORM_XCB_KHR"
    elseif is_plat "windows" then
    add_defines "VK_USE_PLATFORM_WIN32_KHR"
    elseif is_plat "macosx" then add_defines "VK_USE_PLATFORM_METAL_EXT"
    end

    add_rhi_backend("vulkan")

target "avalon.window.glfw"
    set_kind "static"
    add_rules "c++.build.modules"
    add_avalon_api_rules("avalon.window.glfw")
    add_deps("avalon.core")

-- target("avalon.ecs")
--     set_kind("static")
--
--     set_policy("build.c++.modules", true)
--     add_rules("c++.build.modules")
--
--     add_files("src/avalon/ecs/ecs.cppm", {public = true})
-- target "avalon.shader_compiler"
--     set_kind "shared"
--     set_policy("build.c++.modules", true)
--     add_rules "c++.build.modules"
--
--     add_deps("avalon.core")
--     add_packages "directx-shader-compiler"
--
--     add_files("src/avalon/renderer/shader_compiler.cppm", { public = true })
--     add_files("src/avalon/renderer/shader_compiler.cpp")

target "avalon.engine"
    set_kind "shared"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"
    add_avalon_api_rules("avalon.engine")

    add_deps("avalon.core")
    add_files("src/avalon/engine/engine.cppm", {public = true})
    add_files("src/avalon/engine/engine.cpp")

target "avalon.mock.plugin"
    set_kind("shared")
    add_files("src/avalon/tests/mocks/plugin_mock.cpp")
    add_deps("avalon.core")
    add_avalon_api_rules("avalon.mock.plugin")

target "helloPlugin"
    set_kind "binary"
    add_deps "avalon.core"
    add_files("samples/00_hello_plugin/main.cpp")

target "helloTriangle"
    set_kind "binary"
    add_deps "avalon.engine"
    add_files "samples/01_hello_triangle/main.cpp"
