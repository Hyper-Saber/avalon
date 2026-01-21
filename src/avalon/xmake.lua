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
    local path = "rhi"
    local prefix = (api == "vulkan") and "vk" or api
    add_files(format("%s/%s/%s_*.cppm", path, api, prefix), { public = true })
    add_files(format("%s/%s/%s_*.cpp", path, api, prefix))
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/plugins")
    add_packages(api)
end

function add_window_backend(name)
    local path = "window"
    add_files(format("%s/%s/%s_window.cppm", path, name, name), { public = true })
    add_files(format("%s/%s/%s_window.cpp", path, name, name))
    add_defines("AVALON_ENABLE_WINDOW_" .. string.upper(name))
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/plugins")
    add_packages(name)
end

set_policy("build.c++.modules", true)
add_rules "c++.build.modules"
set_kind "shared"

target "avalon.core"
    add_avalon_api_rules("avalon.core")
    add_defines("AVALON_PLATFORM_DL_EXT=\""..(is_plat("windows") and ".dll" or ".so").."\"")
    add_files("core/*.cppm", {public = true})
    add_files("core/*.cpp")
    add_files("rhi/rhi.cppm", {public = true})
    add_files("window/window.cppm", {public = true})

    add_includedirs("src", {public = true})
    add_packages("spdlog")


target "avalon.rhi.vulkan"
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
    add_avalon_api_rules("avalon.window.glfw")
    add_deps("avalon.core")

target "avalon.engine"
    add_avalon_api_rules("avalon.engine")

    add_deps("avalon.core")
    add_files("engine/engine.cppm", {public = true})
    add_files("engine/engine.cpp")
