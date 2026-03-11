target ("test.utils")
    set_kind("static")
    add_files("test_utils.cppm", {public = true})

target ("test.triangle")
    set_kind("binary")
    add_files("unit/test_triangle.cpp")
    add_deps("avalon.core", "avalon.engine", "test.utils")
