add_executable(avail_tests
    test_session.cpp
    test_synthesize.cpp
    test_render.cpp
    test_screens.cpp
    test_mocks.cpp
    test_demo_walker.cpp
    test_menu.cpp
    test_input.cpp
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
