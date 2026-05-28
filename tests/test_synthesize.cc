#include "avail/device.h"
#include "avail/mocks_online.h"
#include "avail/session.h"
#include "avail/synthesize.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

// profcat ids encoded as canonical metric names used by synthesize_command.
constexpr const char* kSqWaves = "rocm.gpu.sq.waves";                   // 0x2001, real
constexpr const char* kValuUtil = "rocm.gpu.derived.valu_utilization";  // 0x6001, aspirational
constexpr const char* kCpuIpc = "cpu.derived.ipc";                      // 0x6004, real
constexpr const char* kCopyH2d = "rocm.copy.h2d";                       // 0x1201, configure

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
    s.toggle_metric("gpu0", kSqWaves);
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find(kSqWaves), std::string::npos);
    EXPECT_NE(out.find("# gpu0 metrics"), std::string::npos);
    EXPECT_EQ(out.find("[demo: proposed UX]"), std::string::npos);
}

TEST(synthesize_test, aspirational_metric_gets_badge) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", kValuUtil);
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("[demo: proposed UX]"), std::string::npos);
    EXPECT_NE(out.find("aspirational"), std::string::npos);
}

TEST(synthesize_test, mixed_real_and_aspirational_produces_two_lines) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", kSqWaves);
    s.toggle_metric("gpu0", kValuUtil);
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find(kSqWaves), std::string::npos);
    EXPECT_NE(out.find(kValuUtil), std::string::npos);
    EXPECT_NE(out.find("[demo: proposed UX]"), std::string::npos);
}

TEST(synthesize_test, configure_line_has_full_enable_string) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_configure("gpu0", kCopyH2d);
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("--rocm-domains memory_copy"), std::string::npos);
}

TEST(synthesize_test, last_line_is_app_placeholder) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", kSqWaves);
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("-- ./your_app"), std::string::npos);
}

TEST(synthesize_test, header_includes_summary) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    avail::SessionState s;
    s.toggle_metric("gpu0", kSqWaves);
    s.toggle_metric("cpu0", kCpuIpc);
    const auto out = avail::synthesize_command(s, devs);
    EXPECT_NE(out.find("2 metrics"), std::string::npos);
    EXPECT_NE(out.find("2 devices"), std::string::npos);
}

}  // namespace
