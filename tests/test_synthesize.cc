#include "avail/device.h"
#include "avail/mocks_online.h"
#include "avail/session.h"
#include "avail/synthesize.h"

#include <gtest/gtest.h>
#include <string>

namespace {

TEST(synthesize_test, empty_session_returns_empty_hint) {
    avail::SessionState s;
    std::vector<avail::Device> devs;
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("empty"), std::string::npos);
}

TEST(synthesize_test, real_metric_has_no_aspirational_badge) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("SQ_WAVES"), std::string::npos);
    EXPECT_NE(out.find("# gpu0 metrics"), std::string::npos);
    EXPECT_EQ(out.find("[demo: proposed UX]"), std::string::npos);
}

TEST(synthesize_test, aspirational_metric_gets_badge) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", "Wave Occupancy %");
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("[demo: proposed UX]"), std::string::npos);
    EXPECT_NE(out.find("aspirational"), std::string::npos);
}

TEST(synthesize_test, mixed_real_and_aspirational_produces_two_lines) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    s.toggle_metric("gpu0", "Wave Occupancy %");
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("SQ_WAVES"), std::string::npos);
    EXPECT_NE(out.find("Wave Occupancy %"), std::string::npos);
    EXPECT_NE(out.find("[demo: proposed UX]"), std::string::npos);
}

TEST(synthesize_test, configure_line_has_full_enable_string) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_configure("gpu0", "Memory copy trace");
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("--rocm-domains memory_copy"), std::string::npos);
}

TEST(synthesize_test, last_line_is_app_placeholder) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("-- ./your_app"), std::string::npos);
}

TEST(synthesize_test, header_includes_summary) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    s.toggle_metric("cpu0", "IPC");
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("2 metrics"), std::string::npos);
    EXPECT_NE(out.find("2 devices"), std::string::npos);
}

}  // namespace
