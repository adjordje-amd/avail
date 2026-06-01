#include "avail/device.h"
#include "avail/menu.h"
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

TEST(render_test, main_menu_lists_create_and_inspect) {
    std::ostringstream os;
    avail::render_main_menu(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("Create profiling configuration"), std::string::npos);
    EXPECT_NE(out.find("Inspect what is available"), std::string::npos);
    // Exit row removed; q/quit keyword in footer covers exit.
    EXPECT_EQ(out.find("  Exit\n"), std::string::npos);
    EXPECT_EQ(out.find("> Exit\n"), std::string::npos);
}

TEST(render_test, main_menu_renders_banner_and_build_info) {
    std::ostringstream os;
    avail::render_main_menu(os, color_off());
    const auto out = os.str();
    // Banner glyphs (line 1 contains a recognisable run of underscores).
    EXPECT_NE(out.find("___  ____  _______"), std::string::npos);
    // AVAIL sub-banner.
    EXPECT_NE(out.find("|_|___/_/ |_/___/____/"), std::string::npos);
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("x86_64-linux-gnu, gcc 13.3.0, rocm 7.2.x"), std::string::npos);
}

TEST(render_test, main_menu_renders_title_and_footer_hint) {
    std::ostringstream os;
    avail::render_main_menu(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("Main Menu"), std::string::npos);
    EXPECT_NE(out.find("Enter select"), std::string::npos);
    EXPECT_NE(out.find("q quit"), std::string::npos);
}

TEST(render_test, main_menu_two_column_layout_aligns_descriptions) {
    std::ostringstream os;
    avail::render_main_menu(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("Configure and launch a profiling run"), std::string::npos);
    EXPECT_NE(out.find("Discover CPUs, GPUs, NICs on this system"), std::string::npos);
    // First item cursor "> " prefix on the selected (first) row.
    EXPECT_NE(out.find("> Create profiling configuration"), std::string::npos);
    // Second item unselected cursor "  ".
    EXPECT_NE(out.find("  Inspect what is available"), std::string::npos);
}

TEST(render_test, main_menu_color_paints_banner_cyan_and_build_dim) {
    std::ostringstream os;
    avail::render_main_menu(os, color_on());
    const auto out = os.str();
    EXPECT_NE(out.find("\x1b[36m"), std::string::npos);  // cyan banner
    EXPECT_NE(out.find("\x1b[2m"), std::string::npos);   // dim build-info / rule
}

TEST(render_test, menu_items_helper_renders_2col_with_cursor) {
    std::vector<avail::MenuItem> items{
        {"alpha-long-label", "a", "first desc"},
        {"bravo", "b", "second desc"},
        {"charlie", "c", ""},
    };
    std::ostringstream os;
    avail::render_menu_items(os, color_off(), items, 1);
    const auto out = os.str();
    EXPECT_NE(out.find("  alpha-long-label      first desc"), std::string::npos);
    EXPECT_NE(out.find("> bravo"), std::string::npos);
    EXPECT_NE(out.find("second desc"), std::string::npos);
    // Item with empty description has no trailing gutter.
    EXPECT_NE(out.find("  charlie\n"), std::string::npos);
}

TEST(render_test, mode_select_lists_online_offline) {
    std::ostringstream os;
    avail::render_mode_select(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("Online"), std::string::npos);
    EXPECT_NE(out.find("Offline"), std::string::npos);
    // Back row removed; q/b keyword in footer covers back.
    EXPECT_EQ(out.find("  Back\n"), std::string::npos);
    EXPECT_EQ(out.find("> Back\n"), std::string::npos);
}

// --- Uniform chrome wrapper (banner + build-info + title + footer hints) ---
//
// Every user-visible screen must paint the same chrome (banner, build-info,
// title, footer hints). These tests pin the contract per screen so a future
// refactor cannot regress to per-screen-only chrome.

TEST(render_test, chrome_open_emits_title_and_banner) {
    std::ostringstream os;
    avail::render_chrome_open(os, color_off(), "My Screen");
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("My Screen"), std::string::npos);
}

TEST(render_test, chrome_close_emits_footer_hints) {
    std::ostringstream os;
    avail::render_chrome_close(os, color_off(), "q quit");
    EXPECT_NE(os.str().find("q quit"), std::string::npos);
}

TEST(render_test, mode_select_wraps_in_chrome) {
    std::ostringstream os;
    avail::render_mode_select(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("choose catalog mode"), std::string::npos);
    EXPECT_NE(out.find("Enter select"), std::string::npos);
    EXPECT_NE(out.find("q back"), std::string::npos);
}

TEST(render_test, create_session_wraps_in_chrome) {
    avail::SessionState s;
    std::ostringstream os;
    avail::render_create_session(os, color_off(), avail::CatalogMode::Online, "this machine", s);
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("Create config session"), std::string::npos);
    EXPECT_NE(out.find("q back"), std::string::npos);
}

TEST(render_test, device_detail_wraps_in_chrome) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "Radeon";
    d.supported = true;
    d.measure = {sq_waves};
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr);
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("gpu0 - "), std::string::npos);
    EXPECT_NE(out.find("Enter toggle"), std::string::npos);
}

TEST(render_test, metric_detail_wraps_in_chrome) {
    const auto& m = lookup_or_die(0x2001);
    std::ostringstream os;
    avail::render_metric_detail(os, color_off(), m, false, true);
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("Metric: "), std::string::npos);
    EXPECT_NE(out.find("back"), std::string::npos);
}

TEST(render_test, preset_preview_wraps_in_chrome) {
    avail::SessionState s;
    std::vector<avail::Device> devs;
    std::ostringstream os;
    avail::render_preset_preview(os, color_off(), s, devs);
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("Preset preview"), std::string::npos);
    EXPECT_NE(out.find("q back"), std::string::npos);
}

TEST(render_test, confirm_wraps_in_chrome) {
    std::ostringstream os;
    avail::render_confirm(os, color_off(), "Clear ALL?");
    const auto out = os.str();
    EXPECT_NE(out.find("rocm-sys-avail v1.6.0"), std::string::npos);
    EXPECT_NE(out.find("Confirm"), std::string::npos);
    EXPECT_NE(out.find("y yes"), std::string::npos);
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

TEST(render_test, device_detail_checkbox_always_present_even_when_session_null) {
    // Post-polish: checkbox renders on every metric row in BOTH modes.
    // Null session = everything unchecked.
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
    EXPECT_NE(os.str().find("[ ]"), std::string::npos);
    EXPECT_EQ(os.str().find("[x]"), std::string::npos);
}

TEST(render_test, device_detail_no_measure_or_configure_label) {
    // Post-polish: the block labels "Measure:" / "Configure:" are removed;
    // metrics render flat with stage headings only.
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
    EXPECT_EQ(os.str().find("Measure:"), std::string::npos);
    EXPECT_EQ(os.str().find("Configure:"), std::string::npos);
}

TEST(render_test, device_detail_footer_hints_include_i_info) {
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
    EXPECT_NE(os.str().find("i info"), std::string::npos);
}

TEST(render_test, device_detail_screen_has_no_housekeeping_rows) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    std::uint32_t supported_arr[] = {sq_waves.id};
    avail::SessionState s;
    std::ostringstream os;
    avail::render_device_detail_screen(os, color_off(), d, &s, supported_arr,
                                       avail::PaginatedDetail{}, /*selected_index=*/0);
    const auto out = os.str();
    EXPECT_EQ(out.find("Clear this device's selections"), std::string::npos);
    // "Back" must not appear as a body row. The footer hint line still
    // contains "q back" so check for the row form "  Back\n" / "> Back\n".
    EXPECT_EQ(out.find("  Back\n"), std::string::npos);
    EXPECT_EQ(out.find("> Back\n"), std::string::npos);
}

TEST(render_test, device_detail_screen_cursor_on_selected_row_only) {
    const auto& sq_waves = lookup_or_die(0x2001);
    const auto& mem_busy = lookup_or_die(0x2002);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves, mem_busy};
    std::uint32_t supported_arr[] = {sq_waves.id, mem_busy.id};
    avail::SessionState s;
    std::ostringstream os;
    avail::render_device_detail_screen(os, color_off(), d, &s, supported_arr,
                                       avail::PaginatedDetail{}, /*selected_index=*/1);
    const auto out = os.str();
    // Exactly one "> " cursor in the screen body.
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = out.find("> ", pos)) != std::string::npos) {
        ++count;
        pos += 2;
    }
    EXPECT_EQ(count, 1u);
}

// Cursor-preservation regression: toggling the metric must not move the
// cursor away from the toggled row. The interactive loop rebuilds the menu
// every iteration, so this exercises the render-side invariant that the
// caller-supplied selected_index is what gets the "> " cursor regardless
// of which checkboxes the session marks as enabled.
TEST(render_test, device_detail_screen_cursor_stable_across_toggle) {
    const auto& sq_waves = lookup_or_die(0x2001);    // sorts after insts_valu
    const auto& insts_valu = lookup_or_die(0x2002);  // sorts before sq_waves
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves, insts_valu};
    std::uint32_t supported_arr[] = {sq_waves.id, insts_valu.id};
    avail::SessionState s;
    // Visible row 1 corresponds to sq_waves after stage_sorted_indices puts
    // insts_valu (alphabetically first within Stage::Gpu) at row 0.
    const std::size_t kRow = 1;

    std::ostringstream before;
    avail::render_device_detail_screen(before, color_off(), d, &s, supported_arr,
                                       avail::PaginatedDetail{}, kRow);
    // Toggle sq_waves (the metric at visible row 1 = cursor row).
    s.toggle_metric(d.slot, sq_waves.name);
    std::ostringstream after;
    avail::render_device_detail_screen(after, color_off(), d, &s, supported_arr,
                                       avail::PaginatedDetail{}, kRow);

    // The cursor row is the only line that starts with "> ". The line must
    // still be on row 2 (the toggled metric) after the toggle.
    const auto find_cursor_line = [](const std::string& frame) {
        std::size_t start = 0;
        while (start < frame.size()) {
            const std::size_t eol = frame.find('\n', start);
            const std::string line = frame.substr(start, eol - start);
            if (line.rfind("> ", 0) == 0) {
                return line;
            }
            if (eol == std::string::npos) {
                break;
            }
            start = eol + 1;
        }
        return std::string{};
    };
    const std::string cursor_before = find_cursor_line(before.str());
    const std::string cursor_after = find_cursor_line(after.str());
    EXPECT_FALSE(cursor_before.empty());
    EXPECT_FALSE(cursor_after.empty());
    // Same row index visible in both frames: line begins with "> 2." after
    // the cursor glyph (the renderer 1-bases the row number).
    EXPECT_NE(cursor_before.find("2."), std::string::npos);
    EXPECT_NE(cursor_after.find("2."), std::string::npos);
    // Checkbox state must have flipped from [ ] to [x].
    EXPECT_NE(cursor_before.find("[ ]"), std::string::npos);
    EXPECT_NE(cursor_after.find("[x]"), std::string::npos);
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
        make_dev("nic0", "AINIC", "Pensando", "200 Gb/s", "", true),
    };
}

}  // namespace

TEST(render_test, device_list_groups_by_kind_in_stable_order) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    const auto cpu = out.find("[CPU]\n");
    const auto gpu = out.find("[GPU]\n");
    const auto ainic = out.find("[AINIC]\n");
    ASSERT_NE(cpu, std::string::npos);
    ASSERT_NE(gpu, std::string::npos);
    ASSERT_NE(ainic, std::string::npos);
    EXPECT_LT(cpu, gpu);
    EXPECT_LT(gpu, ainic);
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

TEST(render_test, kind_sorted_device_indices_buckets_cpu_gpu_ainic) {
    const auto devs = make_three_kinds();
    const auto order = avail::kind_sorted_device_indices(devs);
    ASSERT_EQ(order.size(), 4U);
    EXPECT_EQ(devs[order[0]].kind, "CPU");
    EXPECT_EQ(devs[order[1]].kind, "GPU");
    EXPECT_EQ(devs[order[2]].kind, "GPU");
    EXPECT_EQ(devs[order[3]].kind, "AINIC");
}

TEST(render_test, preset_preview_empty_shows_hint) {
    avail::SessionState s;
    std::vector<avail::Device> devs;
    std::ostringstream os;
    avail::render_preset_preview(os, color_off(), s, devs);
    EXPECT_NE(os.str().find("empty"), std::string::npos);
}

// --- Uniform menu-item shape across every menu screen ---
//
// Mode select / create session / preset preview must all use the cursor +
// label + dim description shape that the main menu does. No leftover
// "  1." / "  2." numbered-list output anywhere.

TEST(render_test, mode_select_uses_cursor_and_descriptions) {
    std::ostringstream os;
    avail::render_mode_select(os, color_off());
    const auto out = os.str();
    EXPECT_NE(out.find("> Online"), std::string::npos);
    EXPECT_NE(out.find("This machine, real-ish data"), std::string::npos);
    EXPECT_NE(out.find("Generic device classes, aspirational"), std::string::npos);
    EXPECT_EQ(out.find("  1. "), std::string::npos);
    EXPECT_EQ(out.find("  2. "), std::string::npos);
    EXPECT_EQ(out.find("  3. "), std::string::npos);
}

TEST(render_test, create_session_empty_description_says_empty) {
    avail::SessionState s;
    std::ostringstream os;
    avail::render_create_session(os, color_off(), avail::CatalogMode::Online, "this machine", s);
    const auto out = os.str();
    EXPECT_NE(out.find("> Devices..."), std::string::npos);
    EXPECT_NE(out.find("Browse and toggle metrics"), std::string::npos);
    EXPECT_NE(out.find("View preset preview"), std::string::npos);
    EXPECT_NE(out.find("(empty - select metrics first)"), std::string::npos);
    EXPECT_EQ(out.find("  1. "), std::string::npos);
    EXPECT_EQ(out.find("  2. "), std::string::npos);
    EXPECT_EQ(out.find("  3. "), std::string::npos);
}

TEST(render_test, create_session_populated_description_reflects_summary) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "sq.waves");
    std::ostringstream os;
    avail::render_create_session(os, color_off(), avail::CatalogMode::Online, "this machine", s);
    const auto out = os.str();
    EXPECT_NE(out.find("View preset preview"), std::string::npos);
    EXPECT_NE(out.find(s.summary()), std::string::npos);
    EXPECT_EQ(out.find("(empty - select metrics first)"), std::string::npos);
}

TEST(render_test, preset_preview_uses_cursor_and_descriptions) {
    avail::SessionState s;
    std::vector<avail::Device> devs;
    std::ostringstream os;
    avail::render_preset_preview(os, color_off(), s, devs);
    const auto out = os.str();
    EXPECT_NE(out.find("> Clear preset"), std::string::npos);
    EXPECT_NE(out.find("Reset all metric selections"), std::string::npos);
    // Back row removed; q/b keyword in footer covers back.
    EXPECT_EQ(out.find("  Back\n"), std::string::npos);
    EXPECT_EQ(out.find("> Back\n"), std::string::npos);
    EXPECT_EQ(out.find("  1. "), std::string::npos);
    EXPECT_EQ(out.find("  2. "), std::string::npos);
}

// --- Device Detail pagination (kDeviceDetailPageSize == 10) ---

namespace {

avail::Device make_paginated_device(std::size_t measure_count, std::size_t configure_count) {
    static const std::uint32_t kMeasureIds[] = {
        0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009,
        0x200A, 0x200B, 0x200C, 0x4001, 0x4002, 0x4003, 0x4004, 0x4005, 0x4006,
        0x4007, 0x4008, 0x5001, 0x5002, 0x5003, 0x5004, 0x5005,
    };
    static const std::uint32_t kConfigureIds[] = {
        0x3001, 0x3002, 0x3003, 0x3004, 0x3005, 0x3006, 0x3007, 0x3008,
    };
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "Radeon RX 9070 XT";
    d.supported = true;
    for (std::size_t i = 0; i < measure_count && i < std::size(kMeasureIds); ++i) {
        d.measure.push_back(lookup_or_die(kMeasureIds[i]));
    }
    for (std::size_t i = 0; i < configure_count && i < std::size(kConfigureIds); ++i) {
        d.configure.push_back(lookup_or_die(kConfigureIds[i]));
    }
    return d;
}

}  // namespace

TEST(render_test, device_detail_total_pages_matches_combined_metric_count) {
    const auto d_small = make_paginated_device(2, 0);
    EXPECT_EQ(avail::device_detail_total_pages(d_small, avail::kDeviceDetailPageSize), 1u);
    const auto d_exact = make_paginated_device(10, 0);
    EXPECT_EQ(avail::device_detail_total_pages(d_exact, avail::kDeviceDetailPageSize), 1u);
    const auto d_two = make_paginated_device(15, 0);
    EXPECT_EQ(avail::device_detail_total_pages(d_two, avail::kDeviceDetailPageSize), 2u);
    const auto d_three = make_paginated_device(17, 6);
    EXPECT_EQ(avail::device_detail_total_pages(d_three, avail::kDeviceDetailPageSize), 3u);
}

TEST(render_test, device_detail_page_one_shows_indicator_and_first_ten_rows) {
    const auto d = make_paginated_device(17, 6);
    std::vector<std::uint32_t> supported;
    for (const auto& m : d.measure) {
        supported.push_back(m.id);
    }
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported, avail::PaginatedDetail{0});
    const auto out = os.str();
    EXPECT_NE(out.find("(Page 1/3)"), std::string::npos);
    EXPECT_NE(out.find("  1. "), std::string::npos);
    EXPECT_NE(out.find(" 10. "), std::string::npos);
    EXPECT_EQ(out.find(" 11. "), std::string::npos);
    EXPECT_EQ(out.find(" 21. "), std::string::npos);
}

TEST(render_test, device_detail_page_two_shows_rows_eleven_through_twenty) {
    const auto d = make_paginated_device(17, 6);
    std::vector<std::uint32_t> supported;
    for (const auto& m : d.measure) {
        supported.push_back(m.id);
    }
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported, avail::PaginatedDetail{1});
    const auto out = os.str();
    EXPECT_NE(out.find("(Page 2/3)"), std::string::npos);
    EXPECT_NE(out.find(" 11. "), std::string::npos);
    EXPECT_NE(out.find(" 20. "), std::string::npos);
    EXPECT_EQ(out.find("  1. "), std::string::npos);
    EXPECT_EQ(out.find(" 21. "), std::string::npos);
}

TEST(render_test, device_detail_page_three_shows_remaining_rows) {
    const auto d = make_paginated_device(17, 6);
    std::vector<std::uint32_t> supported;
    for (const auto& m : d.measure) {
        supported.push_back(m.id);
    }
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported, avail::PaginatedDetail{2});
    const auto out = os.str();
    EXPECT_NE(out.find("(Page 3/3)"), std::string::npos);
    EXPECT_NE(out.find(" 21. "), std::string::npos);
    EXPECT_NE(out.find(" 23. "), std::string::npos);
    EXPECT_EQ(out.find(" 11. "), std::string::npos);
    EXPECT_EQ(out.find(" 24. "), std::string::npos);
}

TEST(render_test, device_detail_single_page_suppresses_indicator) {
    const auto d = make_paginated_device(3, 0);
    std::vector<std::uint32_t> supported;
    for (const auto& m : d.measure) {
        supported.push_back(m.id);
    }
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported, avail::PaginatedDetail{0});
    const auto out = os.str();
    EXPECT_EQ(out.find("(Page "), std::string::npos);
}

TEST(render_test, device_detail_footer_includes_page_arrows) {
    const auto d = make_paginated_device(17, 6);
    std::vector<std::uint32_t> supported;
    for (const auto& m : d.measure) {
        supported.push_back(m.id);
    }
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported, avail::PaginatedDetail{0});
    const auto out = os.str();
    EXPECT_NE(out.find("page"), std::string::npos);
    EXPECT_NE(out.find("\xe2\x86\x90"), std::string::npos);
    EXPECT_NE(out.find("\xe2\x86\x92"), std::string::npos);
}

TEST(render_test, device_detail_out_of_range_page_clamps_to_last) {
    const auto d = make_paginated_device(17, 6);
    std::vector<std::uint32_t> supported;
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported, avail::PaginatedDetail{99});
    EXPECT_NE(os.str().find("(Page 3/3)"), std::string::npos);
}

// --- Slice: device-list grouped headings + flat numbering ---

TEST(render_test, device_list_uses_bracketed_dim_headings) {
    const auto devs = make_three_kinds();
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    // Bare-bucket labels without brackets must not appear as section
    // headings (they may still occur inside device names, hence \n suffix).
    EXPECT_EQ(out.find("  CPU\n"), std::string::npos);
    EXPECT_EQ(out.find("  GPU\n"), std::string::npos);
    EXPECT_EQ(out.find("  AINIC\n"), std::string::npos);
    EXPECT_NE(out.find("  [CPU]\n"), std::string::npos);
    EXPECT_NE(out.find("  [GPU]\n"), std::string::npos);
    EXPECT_NE(out.find("  [AINIC]\n"), std::string::npos);
}

TEST(render_test, device_list_numbering_is_flat_global) {
    const auto devs = make_three_kinds();  // 1 CPU, 2 GPU, 1 AINIC = 4 rows
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    // Flat global indices 1..4 must each appear exactly once as row prefix
    // "N. " preceded by cursor padding. No per-bucket reset (no second "1.").
    EXPECT_NE(out.find("1. "), std::string::npos);
    EXPECT_NE(out.find("2. "), std::string::npos);
    EXPECT_NE(out.find("3. "), std::string::npos);
    EXPECT_NE(out.find("4. "), std::string::npos);
    // Per-bucket reset would produce two "1. " rows; flat numbering must
    // produce exactly one occurrence.
    EXPECT_EQ(out.find("1. ", out.find("1. ") + 1), std::string::npos);
}

TEST(render_test, device_list_omits_empty_buckets) {
    // GPU-only device set: CPU and AINIC headings must not appear.
    std::vector<avail::Device> devs = {
        make_dev("gpu0", "GPU", "Radeon", "gfx1201", "16 GB", true),
    };
    std::ostringstream os;
    avail::render_device_list(os, color_off(), devs, 0, true);
    const auto out = os.str();
    EXPECT_NE(out.find("[GPU]"), std::string::npos);
    EXPECT_EQ(out.find("[CPU]"), std::string::npos);
    EXPECT_EQ(out.find("[AINIC]"), std::string::npos);
}

// --- Slice: Inspect Device Detail is read-only ---

TEST(render_test, device_detail_read_only_omits_checkbox) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr,
                                avail::PaginatedDetail{}, /*read_only=*/true);
    const auto out = os.str();
    EXPECT_EQ(out.find("[x]"), std::string::npos);
    EXPECT_EQ(out.find("[ ]"), std::string::npos);
}

TEST(render_test, device_detail_read_only_footer_drops_enter_toggle) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr,
                                avail::PaginatedDetail{}, /*read_only=*/true);
    const auto out = os.str();
    EXPECT_EQ(out.find("Enter toggle"), std::string::npos);
    EXPECT_NE(out.find("i info"), std::string::npos);
    EXPECT_NE(out.find("q back"), std::string::npos);
}

TEST(render_test, device_detail_write_mode_keeps_checkbox_and_enter_toggle) {
    const auto& sq_waves = lookup_or_die(0x2001);
    avail::Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "test";
    d.supported = true;
    d.measure = {sq_waves};
    std::uint32_t supported_arr[] = {sq_waves.id};
    std::ostringstream os;
    avail::render_device_detail(os, color_off(), d, nullptr, supported_arr,
                                avail::PaginatedDetail{}, /*read_only=*/false);
    const auto out = os.str();
    // Default (write) mode keeps the checkbox column and the toggle hint.
    EXPECT_NE(out.find("[ ]"), std::string::npos);
    EXPECT_NE(out.find("Enter toggle"), std::string::npos);
}

TEST(render_test, footer_hints_for_device_detail_read_only_accessor) {
    const auto hints = avail::footer_hints_for_device_detail_read_only();
    EXPECT_EQ(hints.find("Enter toggle"), std::string::npos);
    EXPECT_NE(hints.find("i info"), std::string::npos);
    EXPECT_NE(hints.find("q back"), std::string::npos);
}

TEST(render_test, footer_hints_for_metric_detail_advertises_i_and_q) {
    const auto hints = avail::footer_hints_for_metric_detail();
    EXPECT_NE(hints.find('i'), std::string::npos);
    EXPECT_NE(hints.find('q'), std::string::npos);
    EXPECT_NE(hints.find("back"), std::string::npos);
}

}  // namespace
