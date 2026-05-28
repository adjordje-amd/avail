#include "avail/device.h"
#include "avail/render.h"
#include "avail/session.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <profcat/catalog.hpp>
#include <profcat/descriptor.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace {

avail::RenderConfig color_off() {
    return avail::RenderConfig{.color = false};
}

avail::RenderConfig color_on() {
    return avail::RenderConfig{.color = true};
}

const profcat::MetricDescriptor& lookup_or_die(std::uint32_t id) {
    for (const auto& d : profcat::catalog) {
        if (d.id == id) {
            return d;
        }
    }
    std::abort();
}

TEST(render_test, welcome_includes_demo_label) {
    std::ostringstream os;
    avail::render_welcome(os, color_off());
    EXPECT_NE(os.str().find("DEMO"), std::string::npos);
}

TEST(render_test, main_menu_includes_all_three_options) {
    std::ostringstream os;
    avail::render_main_menu(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("Create profiling configuration"), std::string::npos);
    EXPECT_NE(out.find("Inspect what is available"), std::string::npos);
    EXPECT_NE(out.find("Exit"), std::string::npos);
}

TEST(render_test, mode_select_lists_online_offline_back) {
    std::ostringstream os;
    avail::render_mode_select(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("Online"), std::string::npos);
    EXPECT_NE(out.find("Offline"), std::string::npos);
    EXPECT_NE(out.find("Back"), std::string::npos);
}

TEST(render_test, color_off_emits_no_escape_sequences) {
    std::ostringstream os;
    avail::render_main_menu(os, color_off());
    EXPECT_EQ(os.str().find('\x1b'), std::string::npos);
}

TEST(render_test, color_on_emits_some_escape_sequence) {
    std::ostringstream os;
    avail::render_main_menu(os, color_on());
    EXPECT_NE(os.str().find('\x1b'), std::string::npos);
}

TEST(render_test, strip_ansi_removes_csi_sequences) {
    const std::string colored = "\x1b[1mbold\x1b[0m";
    EXPECT_EQ(avail::strip_ansi(colored), "bold");
}

TEST(render_test, strip_ansi_passes_through_plain_text) {
    EXPECT_EQ(avail::strip_ansi("plain"), "plain");
}

TEST(render_test, aspirational_badge_present_when_metric_is_aspirational) {
    const auto& m = lookup_or_die(0x6001);  // derived valu_util
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, true);
    EXPECT_NE(os.str().find("[demo: proposed UX]"), std::string::npos);
}

TEST(render_test, non_aspirational_metric_has_no_badge) {
    const auto& m = lookup_or_die(0x6004);  // derived ipc
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, false);
    EXPECT_EQ(os.str().find("[demo: proposed UX]"), std::string::npos);
}

TEST(render_test, device_detail_shows_checkbox_when_session_provided) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    avail::SessionState s;
    s.toggle_metric("gpu0", sq_waves.name);
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, &s);
    EXPECT_NE(os.str().find("[x]"), std::string::npos);
}

TEST(render_test, device_detail_no_checkbox_when_session_null) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr);
    EXPECT_EQ(os.str().find("[x]"), std::string::npos);
    EXPECT_EQ(os.str().find("[ ]"), std::string::npos);
}

TEST(render_test, unsupported_device_renders_reason) {
    avail::Device d;
    d.slot = "gpu1";
    d.kind = "GPU";
    d.identity = "Integrated";
    d.supported = false;
    d.unsupported_reason = std::string("no counters");
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr);
    EXPECT_NE(os.str().find("no counters"), std::string::npos);
}

TEST(render_test, preset_preview_empty_shows_hint) {
    avail::SessionState s;
    std::vector<avail::Device> devs;
    std::ostringstream os;
    avail::render_preset_preview(os, color_off(), s, devs);
    EXPECT_NE(os.str().find("empty"), std::string::npos);
}

}  // namespace
