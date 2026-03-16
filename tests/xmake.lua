target ("test.utils")
    set_kind("static")
    add_files("test_utils.cppm", {public = true})

target ("test.geometry")
    set_kind("binary")
    add_files("unit/test_geometry/*.cppm")
    add_deps("avalon.engine", "test.utils")
