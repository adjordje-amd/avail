#include "avail/menu.h"
#include "avail/render.h"

#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<avail::MenuItem> three_items() {
    return {
        {"alpha", "a"},
        {"bravo", "b"},
        {"charlie", "c"},
    };
}

avail::RenderConfig color_off() {
    return avail::RenderConfig{.color = false};
}

avail::RenderConfig color_on() {
    return avail::RenderConfig{.color = true};
}

TEST(menu_test, initial_selection_is_zero) {
    avail::MenuController m(three_items());
    EXPECT_EQ(m.selected_index(), 0u);
    EXPECT_EQ(m.current().id, "a");
}

TEST(menu_test, initial_index_can_be_specified) {
    avail::MenuController m(three_items(), 2);
    EXPECT_EQ(m.selected_index(), 2u);
    EXPECT_EQ(m.current().id, "c");
}

TEST(menu_test, out_of_range_initial_index_clamps_to_zero) {
    avail::MenuController m(three_items(), 99);
    EXPECT_EQ(m.selected_index(), 0u);
}

TEST(menu_test, move_down_advances_selection) {
    avail::MenuController m(three_items());
    m.move_down();
    EXPECT_EQ(m.selected_index(), 1u);
    m.move_down();
    EXPECT_EQ(m.selected_index(), 2u);
}

TEST(menu_test, move_down_wraps_at_bottom) {
    avail::MenuController m(three_items(), 2);
    m.move_down();
    EXPECT_EQ(m.selected_index(), 0u);
}

TEST(menu_test, move_up_wraps_at_top) {
    avail::MenuController m(three_items(), 0);
    m.move_up();
    EXPECT_EQ(m.selected_index(), 2u);
}

TEST(menu_test, move_up_decreases_selection) {
    avail::MenuController m(three_items(), 2);
    m.move_up();
    EXPECT_EQ(m.selected_index(), 1u);
}

TEST(menu_test, empty_menu_is_safe_to_navigate) {
    avail::MenuController m({});
    EXPECT_TRUE(m.empty());
    m.move_up();
    m.move_down();
    EXPECT_EQ(m.selected_index(), 0u);
}

TEST(menu_test, render_no_color_marks_selected_with_arrow) {
    avail::MenuController m(three_items(), 1);
    std::ostringstream os;
    m.render(os, color_off());
    const std::string out = os.str();
    EXPECT_NE(out.find("  alpha\n"), std::string::npos);
    EXPECT_NE(out.find("> bravo\n"), std::string::npos);
    EXPECT_NE(out.find("  charlie\n"), std::string::npos);
}

TEST(menu_test, render_no_color_emits_no_ansi) {
    avail::MenuController m(three_items());
    std::ostringstream os;
    m.render(os, color_off());
    EXPECT_EQ(os.str().find('\x1b'), std::string::npos);
}

TEST(menu_test, render_color_uses_inverse_video_for_selected) {
    avail::MenuController m(three_items(), 1);
    std::ostringstream os;
    m.render(os, color_on());
    const std::string out = os.str();
    EXPECT_NE(out.find("\x1b[7mbravo\x1b[27m"), std::string::npos);
}

TEST(menu_test, render_color_does_not_invert_unselected) {
    avail::MenuController m(three_items(), 1);
    std::ostringstream os;
    m.render(os, color_on());
    const std::string out = os.str();
    // alpha and charlie should not be wrapped in 7m/27m
    EXPECT_EQ(out.find("\x1b[7malpha"), std::string::npos);
    EXPECT_EQ(out.find("\x1b[7mcharlie"), std::string::npos);
}

TEST(menu_test, render_empty_menu_emits_nothing) {
    avail::MenuController m({});
    std::ostringstream os;
    m.render(os, color_off());
    EXPECT_TRUE(os.str().empty());
}

TEST(menu_test, current_returns_selected_item) {
    avail::MenuController m(three_items());
    EXPECT_EQ(m.current().label, "alpha");
    m.move_down();
    EXPECT_EQ(m.current().label, "bravo");
}

}  // namespace
