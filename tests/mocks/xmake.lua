target "avalon.mock.plugin"
    set_kind("shared")
    add_files("plugin_mock.cpp")
    add_deps("avalon.core")
    add_avalon_api_rules("avalon.mock.plugin")
