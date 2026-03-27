target ("test.utils")
    set_kind("static")
    add_files("test_utils.cppm", {public = true})

target ("test.lit")
    set_kind("binary")
    add_files("unit/test_lit/*.cppm")
    add_deps("avalon.engine", "test.utils")
