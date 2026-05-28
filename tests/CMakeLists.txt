add_executable(avail_tests
    test_session.cc
    test_synthesize.cc
    test_render.cc
    test_screens.cc
    test_mocks.cc
    test_demo_walker.cc
    test_menu.cc
    test_input.cc
)

target_link_libraries(avail_tests PRIVATE
    avail_core
    GTest::gtest_main
    GTest::gmock
)

target_compile_options(avail_tests PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

include(GoogleTest)
gtest_discover_tests(avail_tests)
