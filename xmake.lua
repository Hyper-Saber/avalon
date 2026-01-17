---@diagnostic disable: undefined-global
set_project "avalon"
set_version "0.1.0"
set_languages "c++23"
set_toolchains "clang"
set_policy("build.c++.modules", true)
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })
add_requires("vulkansdk", "directx-shader-compiler", { system = true })
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

function add_avalon_api_rules()
    if is_plat("windows") then
        add_defines("AVALON_API=__declspec(dllexport)", {interface = false})
        add_defines("AVALON_API=__declspec(dllimport)", {interface = true})
    else
        add_defines("AVALON_API=__attribute__((visibility(\"default\")))", {public = true})
    end
end

function add_rhi_backend(api)
    local path = "src/avalon/rhi"
    local prefix = (api == "vulkan") and "vk" or api
    add_files(format("%s/%s/%s_*.cppm", path, api, prefix), { public = true })
    add_files(format("%s/%s/%s_*.cpp", path, api, prefix))
    add_defines("AVALON_ENABLE_" .. string.upper(api))

    if api == "vulkan" then
        if is_plat "linux" then
            add_defines "VK_USE_PLATFORM_WAYLAND_KHR"
            add_defines "VK_USE_PLATFORM_XCB_KHR"
        elseif is_plat "windows" then
            add_defines "VK_USE_PLATFORM_WIN32_KHR"
        elseif is_plat "macosx" then
            add_defines "VK_USE_PLATFORM_METAL_EXT"
        end
        add_packages "vulkansdk"
    end
end

function add_window_backend(name)
    local path = "src/avalon/window"
    add_files(format("%s/%s_window.cppm", path, name), { public = true })
    add_files(format("%s/%s_window.cpp", path, name))
    add_defines("AVALON_ENABLE_WINDOW_" .. string.upper(name))
    add_packages(name)
end

target "avalon.core"
    set_kind "shared"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"
    add_avalon_api_rules()
    add_files("src/avalon/core/*.cppm", {public = true})
    add_files("src/avalon/core/*.cpp")
    add_packages("spdlog")

target "avalon.window"
    set_kind "static"
    add_rules "c++.build.modules"

    if is_plat("windows") then
        add_defines("AVALON_API=__declspec(dllexport)", {interface = false})
        add_defines("AVALON_API=__declspec(dllimport)", {interface = true})
    else
        add_defines("AVALON_API=__attribute__((visibility(\"default\")))", {public = true})
    end

    add_deps("avalon.core")

    add_files("src/avalon/window/window.cppm", { public = true })
    add_files("src/avalon/window/window.cpp")

    if has_config "glfw" then
        add_window_backend "glfw"
    end

target "avalon.rhi"
    set_kind "static"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"

    if is_plat("windows") then
        add_defines("AVALON_API=__declspec(dllexport)", {interface = false})
        add_defines("AVALON_API=__declspec(dllimport)", {interface = true})
    else
        add_defines("AVALON_API=__attribute__((visibility(\"default\")))", {public = true})
    end

    add_deps("avalon.core", "avalon.window")
    add_files("src/avalon/rhi/rhi.cppm", { public = true })
    add_files("src/avalon/rhi/rhi.cpp")

    if has_config "vulkan" then
        add_rhi_backend "vulkan"
    end

    if has_config "dx12" then
        add_rhi_backend "dx12"
    end
    add_packages "glm"

-- target("avalon.ecs")
--     set_kind("static")
--
--     set_policy("build.c++.modules", true)
--     add_rules("c++.build.modules")
--
--     add_files("src/avalon/ecs/ecs.cppm", {public = true})
target "avalon.shader_compiler"
    set_kind "shared"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"

    add_deps("avalon.core")
    add_packages "directx-shader-compiler"

    add_files("src/avalon/renderer/shader_compiler.cppm", { public = true })
    add_files("src/avalon/renderer/shader_compiler.cpp")

target "avalon.engine"
    set_kind "shared"
    set_policy("build.c++.modules", true)
    add_rules "c++.build.modules"

    if is_plat("windows") then
        add_defines("AVALON_API=__declspec(dllexport)", {interface = false})
        add_defines("AVALON_API=__declspec(dllimport)", {interface = true})
    else
        add_defines("AVALON_API=__attribute__((visibility(\"default\")))", {public = true})
    end

    add_headerfiles("src/avalon/engine/engine.cppm", {install = true})

    add_deps("avalon.core", "avalon.rhi", "avalon.window")
    add_files("src/avalon/engine/engine.cppm", {public = true})
    add_files("src/avalon/engine/engine.cpp")


target "helloTriangle"
    set_kind "binary"

    add_deps "avalon.engine"
    add_files "samples/01_hello_triangle/main.cpp"
