
function add_avalon_api_rules(target_name)
    local prefix = string.upper(target_name):gsub("%.", "_")
    if is_plat("windows") then
        add_defines(prefix.."_API=__declspec(dllexport)", {interface = false})
        add_defines(prefix.."_API=__declspec(dllimport)", {interface = true})
    else
        add_defines(prefix.."_API=__attribute__((visibility(\"default\")))", {public = true})
    end
end

function add_window_backend(name)
    local path = "window"
    add_files("window/window.cppm", { public = true })
    add_files(format("%s/%s/%s_window.cpp", path, name, name))
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/plugins")
    add_packages(name)
end

target "avalon.core"
    set_kind "shared"
    add_avalon_api_rules("avalon.core")
    add_defines("AVALON_PLATFORM_DL_EXT=\""..(is_plat("windows") and ".dll" or ".so").."\"")
    add_headerfiles("core/debug/assert.hpp")
    add_files("core/**.cppm", {public = true})
    add_files("core/**.cpp")

    add_includedirs("..", {public = true})
    add_includedirs("core", {public = true})
    add_packages("spdlog")
    add_syslinks("stdc++exp")

target "avalon.rhi"
    set_kind "shared"
    add_avalon_api_rules("avalon.rhi")
    add_files("rhi/*.cppm", {public = true})
    add_deps("avalon.core")

target "avalon.window"
    set_kind "shared"
    add_avalon_api_rules("avalon.rhi")
    add_files("window/window.cppm", {public = true})
    add_deps("avalon.core")

target "avalon.rhi.vulkan"
    set_kind "shared"
    add_rules "c++.build.modules"
    add_avalon_api_rules("avalon.rhi.vulkan")
    add_deps("avalon.core", "avalon.rhi")
    add_files("rhi/vulkan/*.cppm", { public = true })
    add_files("rhi/vulkan/*.cpp")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/plugins")
    add_packages("vulkan")
    if is_plat "linux" then
    add_defines "VK_USE_PLATFORM_WAYLAND_KHR"
    add_defines "VK_USE_PLATFORM_XCB_KHR"
    elseif is_plat "windows" then
    add_defines "VK_USE_PLATFORM_WIN32_KHR"
    elseif is_plat "macosx" then add_defines "VK_USE_PLATFORM_METAL_EXT"
    end

target "avalon.window.glfw"
    set_kind "shared"
    add_avalon_api_rules("avalon.window.glfw")
    add_deps("avalon.core", "avalon.window")
    add_window_backend("glfw")
    if is_plat("linux") then
        add_defines "GLFW_EXPOSE_NATIVE_X11"
        add_defines "GLFW_EXPOSE_NATIVE_WAYLAND"
        add_syslinks("X11-xcb")
    end

target "avalon.ecs"
    set_kind "shared"
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/plugins")
    add_avalon_api_rules("avalon.ecs")
    add_deps("avalon.core", "avalon.rhi")
    add_files("ecs/*.cppm", {public = true})
    add_files("ecs/*.cpp")

target "avalon.shader"
    set_kind "shared"
    add_avalon_api_rules("avalon.shader")
    add_deps("avalon.core", "avalon.rhi")
    add_files("graphics/shader/*.cppm", {public = true})
    add_packages("spirv-reflect", "directx-shader-compiler", {public = true})
    add_includedirs("/usr/include/dxc/", {public = true})

target "avalon.graphics"
    set_kind "shared"
    add_avalon_api_rules("avalon.graphics")
    add_files("graphics/**.cppm|shader/**", {public = true})
    add_files("graphics/**.cpp|shader/**")
    add_deps("avalon.core", "avalon.rhi", "avalon.shader", "avalon.ecs")

target "avalon.scene"
    set_kind "shared"
    add_avalon_api_rules("avalon.scene")
    add_files ("scene/**.cppm", {public = true})
    add_files ("scene/**.cpp")
    add_deps("avalon.core", "avalon.ecs", "avalon.graphics", "avalon.rhi")


target "avalon.physics"
    set_kind "shared"
    add_avalon_api_rules("avalon.physics")
    add_files ("physics/*.cppm", {public = true})
    add_deps("avalon.core", "avalon.ecs")

target "avalon.engine"
    set_kind "shared"
    add_avalon_api_rules("avalon.engine")
    add_deps("avalon.core", "avalon.window", "avalon.rhi", "avalon.shader", "avalon.graphics", "avalon.ecs", "avalon.scene", "avalon.physics")
    add_deps("avalon.rhi.vulkan", "avalon.window.glfw")
    add_files("engine/*.cppm", {public = true})
    add_files("engine/*.cpp")
