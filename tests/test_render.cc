#include "avail/device.h"
#include "avail/render.h"
#include "avail/session.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <profcat/catalog.hpp>
#include <profcat/descriptor.hpp>
#include <span>
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
    avail::render_metric_detail(os, color_off(), m, true, true);
    EXPECT_NE(os.str().find("[demo: proposed UX]"), std::string::npos);
}

TEST(render_test, non_aspirational_metric_has_no_badge) {
    const auto& m = lookup_or_die(0x6004);  // derived ipc
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, false, true);
    EXPECT_EQ(os.str().find("[demo: proposed UX]"), std::string::npos);
}

TEST(render_test, metric_detail_unsupported_shows_red_badge_off_color) {
    const auto& m = lookup_or_die(0x6004);
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, false, false);
    EXPECT_NE(os.str().find("[unsupported]"), std::string::npos);
}

TEST(render_test, metric_detail_supported_shows_supported_badge) {
    const auto& m = lookup_or_die(0x6004);
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, false, true);
    EXPECT_NE(os.str().find("[supported]"), std::string::npos);
}

TEST(render_test, metric_detail_includes_five_field_block) {
    const auto& m = lookup_or_die(0x2001);  // sq.waves
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, false, true);
    const auto out = os.str();
    EXPECT_NE(out.find("Name:"), std::string::npos);
    EXPECT_NE(out.find("Description:"), std::string::npos);
    EXPECT_NE(out.find("Track:"), std::string::npos);
    EXPECT_NE(out.find("Unit:"), std::string::npos);
    EXPECT_NE(out.find("Group:"), std::string::npos);
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
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, &s, supported_arr);
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
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr);
    EXPECT_EQ(os.str().find("[x]"), std::string::npos);
    EXPECT_EQ(os.str().find("[ ]"), std::string::npos);
}

TEST(render_test, device_detail_renders_stage_heading) {
    const auto& sq_waves = lookup_or_die(0x2001);  // Stage::Gpu
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr);
    EXPECT_NE(os.str().find("[GPU]"), std::string::npos);
}

TEST(render_test, device_detail_supported_and_unsupported_badges_per_metric) {
    const auto& sq_waves = lookup_or_die(0x2001);
    const auto& sdma_busy = lookup_or_die(0x200B);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves, sdma_busy};
    std::uint32_t supported_arr[] = {sq_waves.id};  // sdma_busy missing
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr);
    const auto out = os.str();
    EXPECT_NE(out.find("[supported]"), std::string::npos);
    EXPECT_NE(out.find("[unsupported]"), std::string::npos);
}

TEST(render_test, unsupported_device_renders_reason) {
    avail::Device d;
    d.slot = "gpu1";
    d.kind = "GPU";
    d.identity = "Integrated";
    d.supported = false;
    d.unsupported_reason = std::string("no counters");
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, std::span<const std::uint32_t>{});
    EXPECT_NE(os.str().find("no counters"), std::string::npos);
}

TEST(render_test, stage_sorted_indices_groups_by_stage) {
    const auto& sq_waves = lookup_or_die(0x2001);    // Stage::Gpu
    const auto& cpu_cycles = lookup_or_die(0x4004);  // Stage::Cpu
    std::vector<profcat::MetricDescriptor> ms = {sq_waves, cpu_cycles};
    const auto idx = avail::stage_sorted_indices(ms);
    ASSERT_EQ(idx.size(), 2U);
    EXPECT_EQ(ms[idx[0]].id, 0x4004U);  // Cpu first
    EXPECT_EQ(ms[idx[1]].id, 0x2001U);  // Gpu second
}

// --- Section-grouped device-list layout (post-2026-05-28 chrome) ---

namespace {

avail::Device make_dev(std::string slot, std::string kind, std::string display, std::string s1,
                       std::string s2, bool supported) {
    avail::Device d;
    d.slot = std::move(slot);
    d.kind = std::move(kind);
    d.identity = "id";
    d.display_name = std::move(display);
    d.spec1 = std::move(s1);
    d.spec2 = std::move(s2);
    d.supported = supported;
    return d;
}

std::vector<avail::Device> make_three_kinds() {
    return {
        make_dev("cpu0", "CPU", "AMD Ryzen", "16C / 32T", "Zen5", true),
        make_dev("gpu0", "GPU", "Radeon RX", "gfx1201", "16 GB", true),
        make_dev("gpu1", "GPU", "Integrated", "gfx1036", "unsupported", false),
        make_dev("nic0", "NIC", "Pensando", "200 Gb/s", "", true),
    };
}

}  // namespace

TEST(render_test, device_list_groups_by_kind_in_stable_order) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    const auto cpu = out.find("CPU\n");
    const auto gpu = out.find("GPU\n");
    const auto nic = out.find("NIC\n");
    ASSERT_NE(cpu, std::string::npos);
    ASSERT_NE(gpu, std::string::npos);
    ASSERT_NE(nic, std::string::npos);
    EXPECT_LT(cpu, gpu);
    EXPECT_LT(gpu, nic);
}

TEST(render_test, device_list_renders_status_glyphs) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    EXPECT_NE(out.find("\xe2\x97\x8f"), std::string::npos);  // U+25CF supported
    EXPECT_NE(out.find("\xe2\x9a\xa0"), std::string::npos);  // U+26A0 unsupported
}

TEST(render_test, device_list_cursor_marks_selected_row) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 1, true);
    const auto out = os.str();
    EXPECT_NE(out.find("> "), std::string::npos);
}

TEST(render_test, device_list_no_color_keeps_unicode_glyphs) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    EXPECT_EQ(out.find('\x1b'), std::string::npos);
    EXPECT_NE(out.find("\xe2\x94\x80"), std::string::npos);  // U+2500 rule
    EXPECT_NE(out.find("\xe2\x97\x8f"), std::string::npos);
    EXPECT_NE(out.find("\xe2\x86\x91"), std::string::npos);  // U+2191 up arrow
}

TEST(render_test, device_list_color_emits_escape_sequences_and_glyphs) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_on(), devs, 0, true);
    const auto out = os.str();
    EXPECT_NE(out.find('\x1b'), std::string::npos);
    EXPECT_NE(out.find("\xe2\x97\x8f"), std::string::npos);
    EXPECT_NE(out.find("\xe2\x94\x80"), std::string::npos);
}

TEST(render_test, device_list_footer_hints_include_q_back) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    EXPECT_NE(os.str().find("q back"), std::string::npos);
}

TEST(render_test, device_list_omits_footer_when_disabled) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, false);
    EXPECT_EQ(os.str().find("q back"), std::string::npos);
}

TEST(render_test, kind_sorted_device_indices_buckets_cpu_gpu_nic) {
    const auto devs = make_three_kinds();
    const auto order = avail::kind_sorted_device_indices(devs);
    ASSERT_EQ(order.size(), 4U);
    EXPECT_EQ(devs[order[0]].kind, "CPU");
    EXPECT_EQ(devs[order[1]].kind, "GPU");
    EXPECT_EQ(devs[order[2]].kind, "GPU");
    EXPECT_EQ(devs[order[3]].kind, "NIC");
}

TEST(render_test, preset_preview_empty_shows_hint) {
    avail::SessionState s;
    std::vector<avail::Device> devs;
    std::ostringstream os;
    avail::render_preset_preview(os, color_off(), s, devs);
    EXPECT_NE(os.str().find("empty"), std::string::npos);
}

}  // namespace
