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

std::vector<avail::MenuItem> n_items(std::size_t n) {
    std::vector<avail::MenuItem> items;
    items.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        items.push_back({"label-" + std::to_string(i), "id-" + std::to_string(i)});
    }
    return items;
}

TEST(menu_pagination_test, page_size_zero_keeps_legacy_wrap) {
    avail::MenuController m(three_items());
    EXPECT_EQ(m.page_size(), 0u);
    EXPECT_EQ(m.total_pages(), 1u);
    m.move_up();
    EXPECT_EQ(m.selected_index(), 2u);
}

TEST(menu_pagination_test, twenty_five_items_split_into_three_pages_of_ten) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    EXPECT_EQ(m.total_pages(), 3u);
    EXPECT_EQ(m.current_page(), 0u);
    EXPECT_EQ(m.page_begin(), 0u);
    EXPECT_EQ(m.page_end(), 10u);
    m.next_page();
    EXPECT_EQ(m.current_page(), 1u);
    EXPECT_EQ(m.page_begin(), 10u);
    EXPECT_EQ(m.page_end(), 20u);
    m.next_page();
    EXPECT_EQ(m.current_page(), 2u);
    EXPECT_EQ(m.page_begin(), 20u);
    EXPECT_EQ(m.page_end(), 25u);
}

TEST(menu_pagination_test, ten_items_yield_one_page) {
    avail::MenuController m(n_items(10));
    m.set_page_size(10);
    EXPECT_EQ(m.total_pages(), 1u);
    EXPECT_EQ(m.page_begin(), 0u);
    EXPECT_EQ(m.page_end(), 10u);
}

TEST(menu_pagination_test, empty_menu_reports_one_total_page) {
    avail::MenuController m({});
    m.set_page_size(10);
    EXPECT_EQ(m.total_pages(), 1u);
    EXPECT_EQ(m.page_begin(), 0u);
    EXPECT_EQ(m.page_end(), 0u);
}

TEST(menu_pagination_test, next_page_at_last_page_is_noop) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    m.next_page();
    m.next_page();
    EXPECT_EQ(m.current_page(), 2u);
    m.next_page();
    EXPECT_EQ(m.current_page(), 2u);
    EXPECT_EQ(m.selected_index(), 20u);
}

TEST(menu_pagination_test, prev_page_at_first_page_is_noop) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    EXPECT_EQ(m.current_page(), 0u);
    m.prev_page();
    EXPECT_EQ(m.current_page(), 0u);
    EXPECT_EQ(m.selected_index(), 0u);
}

TEST(menu_pagination_test, move_down_clamps_within_page) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    for (int i = 0; i < 20; ++i) {
        m.move_down();
    }
    EXPECT_EQ(m.current_page(), 0u);
    EXPECT_EQ(m.selected_index(), 9u);
}

TEST(menu_pagination_test, move_up_clamps_within_page) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    m.next_page();
    EXPECT_EQ(m.selected_index(), 10u);
    for (int i = 0; i < 20; ++i) {
        m.move_up();
    }
    EXPECT_EQ(m.current_page(), 1u);
    EXPECT_EQ(m.selected_index(), 10u);
}

TEST(menu_pagination_test, next_page_snaps_selection_to_first_row_of_new_page) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    m.move_down();
    m.move_down();
    m.move_down();
    EXPECT_EQ(m.selected_index(), 3u);
    m.next_page();
    EXPECT_EQ(m.selected_index(), 10u);
    m.prev_page();
    EXPECT_EQ(m.selected_index(), 0u);
}

TEST(menu_pagination_test, render_emits_only_current_page) {
    avail::MenuController m(n_items(25));
    m.set_page_size(10);
    m.next_page();
    std::ostringstream os;
    m.render(os, color_off());
    const std::string text = os.str();
    EXPECT_NE(text.find("label-10"), std::string::npos);
    EXPECT_NE(text.find("label-19"), std::string::npos);
    EXPECT_EQ(text.find("label-0\n"), std::string::npos);
    EXPECT_EQ(text.find("label-20"), std::string::npos);
}

}  // namespace
