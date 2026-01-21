target ("test.utils")
    set_kind("static")
    add_files("test_utils.cppm", {public = true})

target("test.plugin_loader")
    set_kind("binary")
    set_group("Tests")
    add_files("unit/test_plugin_loader.cpp")
    add_deps("avalon.core", "test.utils")
