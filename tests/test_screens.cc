#include "avail/screens.h"

#include <gtest/gtest.h>

namespace {

TEST(screens_test, welcome_to_main_on_any_input) {
    EXPECT_EQ(avail::next_screen(avail::Screen::Welcome, ""), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::Welcome, "x"), avail::Screen::MainMenu);
}

TEST(screens_test, main_menu_1_to_create_mode_select) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "1"), avail::Screen::CreateModeSelect);
}

TEST(screens_test, main_menu_2_to_inspect_device_list) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "2"), avail::Screen::InspectDeviceList);
}

TEST(screens_test, main_menu_3_to_exit) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "3"), avail::Screen::Exit);
}

TEST(screens_test, main_menu_quit_keyword_to_exit) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "quit"), avail::Screen::Exit);
}

TEST(screens_test, main_menu_invalid_input_stays) {
    EXPECT_EQ(avail::next_screen(avail::Screen::MainMenu, "9"), avail::Screen::MainMenu);
}

TEST(screens_test, create_mode_back_returns_to_main) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateModeSelect, "3"), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateModeSelect, "back"), avail::Screen::MainMenu);
}

TEST(screens_test, create_session_back_returns_to_main) {
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateSession, "3"), avail::Screen::MainMenu);
    EXPECT_EQ(avail::next_screen(avail::Screen::CreateSession, "b"), avail::Screen::MainMenu);
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

}  // namespace
