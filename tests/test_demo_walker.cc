#include "avail/app.h"
#include "avail/mocks_offline.h"
#include "avail/mocks_online.h"
#include "avail/render.h"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace {

avail::RenderConfig color_off() {
    return avail::RenderConfig{.color = false};
}

class demo_walker_test : public ::testing::Test {
  protected:
    void SetUp() override {
        avail::App app(m_online, m_offline, m_online_m, m_offline_m, m_rocm, m_libs);
        std::ostringstream os;
        const int rc = app.run_demo_frames(os, color_off());
        ASSERT_EQ(rc, 0);
        m_out = os.str();
    }

    avail::MockOnlineDeviceCatalog m_online;
    avail::MockOfflineDeviceCatalog m_offline;
    avail::MockOnlineMetricCatalog m_online_m;
    avail::MockOfflineMetricCatalog m_offline_m;
    avail::MockOnlineRocmVersion m_rocm;
    avail::MockOnlineLibraryEnv m_libs;
    std::string m_out;
};

TEST_F(demo_walker_test, contains_product_banner) {
    EXPECT_NE(m_out.find("rocm-sys-avail"), std::string::npos);
}

TEST_F(demo_walker_test, first_frame_is_main_menu) {
    EXPECT_NE(m_out.find("Main Menu"), std::string::npos);
    const auto main_menu_pos = m_out.find("Main Menu");
    const auto sep_pos = m_out.find("---");
    EXPECT_LT(main_menu_pos, sep_pos);
}

TEST_F(demo_walker_test, contains_main_menu_options) {
    EXPECT_NE(m_out.find("Create profiling configuration"), std::string::npos);
    EXPECT_NE(m_out.find("Inspect what is available"), std::string::npos);
    // Exit row removed; q/quit keyword in footer covers exit.
    EXPECT_EQ(m_out.find("  Exit\n"), std::string::npos);
    EXPECT_EQ(m_out.find("> Exit\n"), std::string::npos);
}

TEST_F(demo_walker_test, contains_mode_select) {
    EXPECT_NE(m_out.find("Online"), std::string::npos);
    EXPECT_NE(m_out.find("Offline"), std::string::npos);
}

TEST_F(demo_walker_test, contains_both_catalog_labels) {
    EXPECT_NE(m_out.find("this machine"), std::string::npos);
    EXPECT_NE(m_out.find("generic offline"), std::string::npos);
}

TEST_F(demo_walker_test, contains_online_device_slots) {
    // cpu0 / gpu0 are reachable via device-detail screens which prefix the
    // slot. nic0 is shown only in the device-list row, whose visible label is
    // the display_name (Pensando), not the internal slot id.
    EXPECT_NE(m_out.find("cpu0"), std::string::npos);
    EXPECT_NE(m_out.find("gpu0"), std::string::npos);
    EXPECT_NE(m_out.find("Pensando"), std::string::npos);
}

TEST_F(demo_walker_test, contains_separators) {
    EXPECT_NE(m_out.find("---"), std::string::npos);
}

TEST_F(demo_walker_test, contains_aspirational_badge) {
    EXPECT_NE(m_out.find("[demo: proposed UX]"), std::string::npos);
}

TEST_F(demo_walker_test, contains_synthesized_command_placeholder) {
    EXPECT_NE(m_out.find("-- ./your_app"), std::string::npos);
}

TEST_F(demo_walker_test, contains_rocprof_sys_run) {
    EXPECT_NE(m_out.find("rocprof-sys-run"), std::string::npos);
}

TEST_F(demo_walker_test, contains_goodbye_footer) {
    EXPECT_NE(m_out.find("Goodbye."), std::string::npos);
}

TEST_F(demo_walker_test, no_color_escape_sequences) {
    EXPECT_EQ(m_out.find('\x1b'), std::string::npos);
}

}  // namespace
