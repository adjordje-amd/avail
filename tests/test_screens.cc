#include "avail/app.h"
#include "avail/input.h"
#include "avail/screens.h"

#include <gtest/gtest.h>

namespace {

TEST(screens_test, main_menu_1_to_create_mode_select) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "1"), avail::Screen::CreateModeSelect);
}

TEST(screens_test, main_menu_2_to_inspect_device_list) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "2"), avail::Screen::InspectDeviceList);
}

TEST(screens_test, main_menu_q_keyword_exits) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "q"), avail::Screen::Exit);
}

TEST(screens_test, main_menu_quit_keyword_to_exit) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "quit"), avail::Screen::Exit);
}

TEST(screens_test, main_menu_invalid_input_stays) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "9"), avail::Screen::MainMenu);
}

TEST(screens_test, main_menu_past_last_row_is_invalid) {
    // Exit row is gone; "3" is no longer a valid index.
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "3"), avail::Screen::MainMenu);
}

TEST(screens_test, create_mode_back_keyword_returns_to_main) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateModeSelect, "back"), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateModeSelect, "q"), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateModeSelect, "b"), avail::Screen::MainMenu);
}

TEST(screens_test, create_mode_past_last_row_is_invalid) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateModeSelect, "3"),
              avail::Screen::CreateModeSelect);
}

TEST(screens_test, create_session_back_keyword_returns_to_main) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateSession, "back"), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateSession, "b"), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateSession, "q"), avail::Screen::MainMenu);
}

TEST(screens_test, create_session_past_last_row_is_invalid) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateSession, "3"), avail::Screen::CreateSession);
}

TEST(screens_test, create_device_list_back_to_session) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateDeviceList, "b"),
              avail::Screen::CreateSession);
}

TEST(screens_test, inspect_device_list_back_to_main) {
    EXPECT_EQ(avail::next_screen(avail::Screen::InspectDeviceList, "b"), avail::Screen::MainMenu);
}

TEST(screens_test, screen_name_round_trip_stable) {
    EXPECT_EQ(avail::screen_name(avail::Screen::MainMenu), "main_menu");
    EXPECT_EQ(avail::screen_name(avail::Screen::PresetPreview), "preset_preview");
    EXPECT_EQ(avail::screen_name(avail::Screen::MetricDetail), "metric_detail");
}

TEST(screens_test, mode_label_online_offline) {
    EXPECT_EQ(avail::mode_label(avail::CatalogMode::Online), "online");
    EXPECT_EQ(avail::mode_label(avail::CatalogMode::Offline), "offline");
}

TEST(screens_test, preset_preview_back_to_session) {
    EXPECT_EQ(avail::next_screen(avail::Screen::PresetPreview, "back"),
              avail::Screen::CreateSession);
}

TEST(screens_test, confirm_clear_all_yes_clears_to_session) {
    EXPECT_EQ(avail::next_screen(avail::Screen::ConfirmClearAll, "y"),
              avail::Screen::CreateSession);
}

TEST(screens_test, confirm_clear_all_no_stays_in_preview) {
    EXPECT_EQ(avail::next_screen(avail::Screen::ConfirmClearAll, "n"),
              avail::Screen::PresetPreview);
}

TEST(screens_test, metric_detail_i_key_dismisses) {
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'i'}));
}

TEST(screens_test, metric_detail_capital_i_key_dismisses) {
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'I'}));
}

TEST(screens_test, metric_detail_q_b_esc_enter_still_dismiss) {
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'q'}));
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'Q'}));
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'b'}));
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Esc, 0}));
    EXPECT_TRUE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Enter, 0}));
}

TEST(screens_test, metric_detail_unrelated_keys_do_not_dismiss) {
    EXPECT_FALSE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'a'}));
    EXPECT_FALSE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Char, 'x'}));
    EXPECT_FALSE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Up, 0}));
    EXPECT_FALSE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Down, 0}));
    EXPECT_FALSE(avail::is_metric_detail_back_key(avail::KeyEvent{avail::Key::Unknown, 0}));
}

}  // namespace
