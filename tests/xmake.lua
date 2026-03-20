target ("test.utils")
    set_kind("static")
    add_files("test_utils.cppm", {public = true})

target ("test.mvp")
    set_kind("binary")
    add_files("unit/test_mvp/*.cppm")
    add_deps("avalon.engine", "test.utils")

target ("test.lit")
    set_kind("binary")
    add_files("unit/test_lit/*.cppm")
    add_deps("avalon.engine", "test.utils")

target ("test.noise")
    set_kind("binary")
    add_files("unit/test_noise/*.cppm")
    add_deps("avalon.engine", "test.utils")
