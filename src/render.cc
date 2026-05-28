#include "avail/render.h"

#include "avail/device.h"
#include "avail/menu.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <ostream>
#include <profcat/descriptor.hpp>
#include <profcat/enums.hpp>
#include <span>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace avail {

Ansi make_ansi(const RenderConfig& cfg) {
    if (!cfg.color) {
        return Ansi{};
    }
    return Ansi{
        .reset = "\x1b[0m",
        .bold = "\x1b[1m",
        .dim = "\x1b[2m",
        .yellow = "\x1b[33m",
        .cyan = "\x1b[36m",
        .green = "\x1b[32m",
        .red = "\x1b[31m",
    };
}

namespace {

constexpr std::string_view kBadgeAspirational = "[demo: proposed UX]";
constexpr std::string_view kRule = "----------------------------------------";
// U+2500 BOX DRAWINGS LIGHT HORIZONTAL, encoded as three UTF-8 bytes.
constexpr std::string_view kRuleGlyph = "\xe2\x94\x80";
// U+25CF BLACK CIRCLE (supported indicator).
constexpr std::string_view kGlyphOk = "\xe2\x97\x8f";
// U+26A0 WARNING SIGN (unsupported indicator).
constexpr std::string_view kGlyphWarn = "\xe2\x9a\xa0";
constexpr int kDefaultRuleCols = 55;
constexpr std::string_view kFooterHintsDevices = "\xe2\x86\x91/\xe2\x86\x93 move   "
                                                 "Enter select   q back";
constexpr std::string_view kFooterHintsDetail = "\xe2\x86\x91/\xe2\x86\x93 move   "
                                                "Enter toggle   q back";
constexpr std::string_view kFooterHintsMainMenu = "\xe2\x86\x91/\xe2\x86\x93 move   "
                                                  "Enter select   q quit";
// U+2191 UPWARDS ARROW + U+2193 DOWNWARDS ARROW for hints.

// Hardcoded product banner (ROCM-SYSTEMS / AVAIL ASCII art).
// 79 columns wide; rule width falls back to 81 (banner + 2 padding) off-TTY.
constexpr std::array<std::string_view, 9> kBannerLines = {
    "   ___  ____  _______  ___  ______  __________________  _______",
    "  / _ \\/ __ \\/ ___/  |/  / / __/\\ \\/ / __/_  __/ __/  |/  / __/",
    " / , _/ /_/ / /__/ /|_/ / _\\ \\   \\  /\\ \\  / / / _// /|_/ /\\ \\",
    "/_/|_|\\____/\\___/_/  /_/ /___/   /_/___/ /_/ /___/_/  /_/___/",
    "",
    "             ___ _   _____   ______",
    "            / _ | | / / _ | /  _/ /",
    "           / __ | |/ / __ |_/ // /__",
    "          /_/ |_|___/_/ |_/___/____/",
};

constexpr std::string_view kBuildInfo =
    "rocm-sys-avail v1.6.0   (tag: v1.6.0, x86_64-linux-gnu, gcc 13.3.0, rocm 7.2.x)";

constexpr int kMainMenuRuleFallback = 81;
constexpr std::string_view kMainMenuItemGutter = "      ";  // 6 spaces between label and desc

int resolve_rule_width(const RenderConfig& cfg) noexcept {
    if (cfg.rule_width > 0) {
        return cfg.rule_width;
    }
    winsize ws{};
    if (::isatty(STDOUT_FILENO) != 0 && ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0) {
        return ws.ws_col;
    }
    return kDefaultRuleCols;
}

int resolve_main_menu_rule_width(const RenderConfig& cfg) noexcept {
    if (cfg.rule_width > 0) {
        return cfg.rule_width;
    }
    winsize ws{};
    if (::isatty(STDOUT_FILENO) != 0 && ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0) {
        return ws.ws_col;
    }
    return kMainMenuRuleFallback;
}

std::string repeat_glyph(std::string_view glyph, int count) {
    std::string out;
    out.reserve(glyph.size() * static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        out.append(glyph);
    }
    return out;
}

void hr(std::ostream& out, const Ansi& ansi, const RenderConfig& cfg) {
    const int cols = resolve_rule_width(cfg);
    out << "  " << ansi.dim << repeat_glyph(kRuleGlyph, cols) << ansi.reset << '\n';
}

void main_menu_rule(std::ostream& out, const Ansi& ansi, const RenderConfig& cfg) {
    const int cols = resolve_main_menu_rule_width(cfg);
    out << ansi.dim << repeat_glyph(kRuleGlyph, cols) << ansi.reset << '\n';
}

void chrome_title(std::ostream& out, const Ansi& ansi, std::string_view title) {
    out << "  " << ansi.bold << title << ansi.reset << '\n';
}

// Display order for the Stage groupings on a device-detail screen.
constexpr profcat::Stage kStageOrder[] = {
    profcat::Stage::Cpu,        profcat::Stage::Memory, profcat::Stage::Interconnect,
    profcat::Stage::Submission, profcat::Stage::Gpu,    profcat::Stage::DeviceTelemetry,
    profcat::Stage::ApuFabric,
};

constexpr const char* stage_heading(profcat::Stage s) noexcept {
    switch (s) {
    case profcat::Stage::Cpu:
        return "CPU";
    case profcat::Stage::Memory:
        return "Memory";
    case profcat::Stage::Interconnect:
        return "Interconnect";
    case profcat::Stage::Submission:
        return "Submission";
    case profcat::Stage::Gpu:
        return "GPU";
    case profcat::Stage::DeviceTelemetry:
        return "Device telemetry";
    case profcat::Stage::ApuFabric:
        return "APU fabric";
    }
    return "Other";
}

void rule(std::ostream& out, const Ansi& ansi) {
    out << ansi.dim << kRule << ansi.reset << '\n';
}

void header(std::ostream& out, const Ansi& ansi, std::string_view title) {
    rule(out, ansi);
    out << ansi.bold << title << ansi.reset << '\n';
    rule(out, ansi);
}

bool id_in(std::span<const std::uint32_t> ids, std::uint32_t id) noexcept {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// --- Badge helpers ---
//
// Each function returns the rendered badge string (already colorised when
// the Ansi struct carries escape codes) or an empty string when the badge
// should not be shown for this state.

std::string badge_supported(const Ansi& ansi, bool supported) {
    std::string s;
    s.append(" ");
    if (supported) {
        s.append(ansi.green).append("[supported]").append(ansi.reset);
    } else {
        s.append(ansi.red).append("[unsupported]").append(ansi.reset);
    }
    return s;
}

std::string badge_enabled(const Ansi& ansi, bool enabled) {
    if (!enabled) {
        return {};
    }
    std::string s;
    s.append(" ").append(ansi.cyan).append("[enabled]").append(ansi.reset);
    return s;
}

std::string badge_aspirational(const Ansi& ansi, bool aspirational) {
    if (!aspirational) {
        return {};
    }
    std::string s;
    s.append(" ")
        .append(ansi.dim)
        .append(ansi.yellow)
        .append(kBadgeAspirational)
        .append(ansi.reset);
    return s;
}

// Combined badge run in the project's required order:
//   supported -> enabled -> aspirational
std::string badges_for(const Ansi& ansi, bool supported, bool enabled, bool aspirational) {
    std::string s;
    s.append(badge_supported(ansi, supported));
    s.append(badge_enabled(ansi, enabled));
    s.append(badge_aspirational(ansi, aspirational));
    return s;
}

// Format one metric line in a device-detail list. Returns the line WITHOUT
// the trailing newline so the caller can decide between '\n' (legacy demo)
// and any additional decoration.
void render_metric_row(std::ostream& out, const Ansi& ansi, int index,
                       const profcat::MetricDescriptor& m, bool aspirational, bool supported,
                       bool enabled, const char* checkbox) {
    out << "  " << index << ". " << checkbox << ' ' << m.display_name << " ("
        << profcat::to_string(m.unit) << ')' << badges_for(ansi, supported, enabled, aspirational)
        << '\n';
}

bool metric_enabled(const SessionState* session, std::string_view slot, std::string_view name,
                    bool is_configure) {
    if (session == nullptr) {
        return false;
    }
    return is_configure ? session->has_configure(slot, name) : session->has_metric(slot, name);
}

}  // namespace

std::vector<std::size_t> stage_sorted_indices(std::span<const profcat::MetricDescriptor> metrics) {
    std::vector<std::size_t> idx(metrics.size());
    std::iota(idx.begin(), idx.end(), std::size_t{0});

    const auto stage_rank = [](profcat::Stage s) -> int {
        for (int i = 0; i < static_cast<int>(std::size(kStageOrder)); ++i) {
            if (kStageOrder[i] == s) {
                return i;
            }
        }
        return static_cast<int>(std::size(kStageOrder));
    };

    std::stable_sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) {
        const auto ra = stage_rank(metrics[a].stage);
        const auto rb = stage_rank(metrics[b].stage);
        if (ra != rb) {
            return ra < rb;
        }
        return metrics[a].name < metrics[b].name;
    });
    return idx;
}

void render_welcome(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "avail - DEMO");
    out << "Prototype UX for rocprof-sys-avail. All catalog data is mocked.\n";
    out << "Press Enter to continue (or type 'quit').\n";
}

namespace {

std::vector<MenuItem> build_main_menu_items_local() {
    return {
        {"Create profiling configuration", "create", "Configure and launch a profiling run"},
        {"Inspect what is available", "inspect", "Discover CPUs, GPUs, NICs on this system"},
        {"Exit", "exit", ""},
    };
}

}  // namespace

void render_banner(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    for (const auto line : kBannerLines) {
        out << ansi.cyan << line << ansi.reset << '\n';
    }
    out << '\n';
    out << ansi.dim << kBuildInfo << ansi.reset << '\n';
}

void render_main_menu_items(std::ostream& out, const RenderConfig& cfg,
                            const std::vector<MenuItem>& items, std::size_t selected_index) {
    const auto ansi = make_ansi(cfg);
    std::size_t label_width = 0;
    for (const auto& item : items) {
        label_width = std::max(label_width, item.label.size());
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        const std::string_view cursor = (i == selected_index) ? "> " : "  ";
        out << cursor << item.label;
        if (!item.description.empty()) {
            if (label_width > item.label.size()) {
                out << std::string(label_width - item.label.size(), ' ');
            }
            out << kMainMenuItemGutter << ansi.dim << item.description << ansi.reset;
        }
        out << '\n';
    }
}

void render_main_menu(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    main_menu_rule(out, ansi, cfg);
    render_banner(out, cfg);
    main_menu_rule(out, ansi, cfg);
    out << '\n';
    out << ansi.bold << "Main Menu" << ansi.reset << '\n';
    out << '\n';
    const auto items = build_main_menu_items_local();
    render_main_menu_items(out, cfg, items, 0);
    out << '\n';
    main_menu_rule(out, ansi, cfg);
    out << ansi.dim << kFooterHintsMainMenu << ansi.reset << '\n';
    main_menu_rule(out, ansi, cfg);
}

void render_mode_select(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "Create config - choose catalog mode");
    out << "  1. Online  (this machine, real-ish data)\n";
    out << "  2. Offline (generic device classes, aspirational)\n";
    out << "  3. Back\n";
}

void render_create_session(std::ostream& out, const RenderConfig& cfg, CatalogMode mode,
                           std::string_view catalog_label, const SessionState& session) {
    const auto ansi = make_ansi(cfg);
    std::string title = "Create config session [";
    title.append(catalog_label);
    title.append("] - ");
    title.append(mode_label(mode));
    header(out, ansi, title);

    out << "  1. Devices...\n";
    if (!session.is_empty()) {
        out << "  2. View preset preview (" << ansi.cyan << session.summary() << ansi.reset
            << ")\n";
    } else {
        out << "  2. View preset preview " << ansi.dim << "(empty - select metrics first)"
            << ansi.reset << '\n';
    }
    out << "  3. Back\n";
}

namespace {

// Stable bucket order for the device-list section headers.
constexpr std::array<std::string_view, 3> kKindOrder = {"CPU", "GPU", "NIC"};

int kind_rank(std::string_view kind) noexcept {
    for (int i = 0; i < static_cast<int>(kKindOrder.size()); ++i) {
        if (kKindOrder[static_cast<std::size_t>(i)] == kind) {
            return i;
        }
    }
    return static_cast<int>(kKindOrder.size());
}

std::string_view bucket_heading(std::string_view kind) noexcept {
    return kind_rank(kind) < static_cast<int>(kKindOrder.size()) ? kind : std::string_view{"other"};
}

struct DeviceColWidths {
    std::size_t name = 0;
    std::size_t spec1 = 0;
    std::size_t spec2 = 0;
};

DeviceColWidths compute_device_widths(const std::vector<Device>& devices) {
    DeviceColWidths w;
    for (const auto& d : devices) {
        w.name = std::max(w.name, d.display_name.size());
        w.spec1 = std::max(w.spec1, d.spec1.size());
        w.spec2 = std::max(w.spec2, d.spec2.size());
    }
    return w;
}

void render_device_row(std::ostream& out, const Ansi& ansi, const Device& dev,
                       const DeviceColWidths& w, bool selected) {
    const std::string_view cursor = selected ? "> " : "  ";
    const std::string_view glyph = dev.supported ? kGlyphOk : kGlyphWarn;
    const std::string_view glyph_color = dev.supported ? ansi.green : ansi.yellow;
    constexpr std::string_view kGutter = "   ";

    out << cursor << glyph_color << glyph << ansi.reset << ' ';
    out << dev.display_name;
    if (w.name > dev.display_name.size()) {
        out << std::string(w.name - dev.display_name.size(), ' ');
    }
    if (w.spec1 > 0) {
        out << kGutter << dev.spec1;
        if (w.spec1 > dev.spec1.size() && w.spec2 > 0) {
            out << std::string(w.spec1 - dev.spec1.size(), ' ');
        }
    }
    if (w.spec2 > 0) {
        out << kGutter << dev.spec2;
    }
    out << '\n';
}

}  // namespace

std::vector<std::size_t> kind_sorted_device_indices(const std::vector<Device>& devices) {
    std::vector<std::size_t> idx(devices.size());
    std::iota(idx.begin(), idx.end(), std::size_t{0});
    std::stable_sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) {
        return kind_rank(devices[a].kind) < kind_rank(devices[b].kind);
    });
    return idx;
}

void render_footer_hints(std::ostream& out, const RenderConfig& cfg, std::string_view hints) {
    const auto ansi = make_ansi(cfg);
    hr(out, ansi, cfg);
    out << "  " << ansi.dim << hints << ansi.reset << '\n';
}

void render_device_list(std::ostream& out, const RenderConfig& cfg,
                        const std::vector<Device>& devices, std::size_t selected_index,
                        bool show_footer_hints) {
    const auto ansi = make_ansi(cfg);
    chrome_title(out, ansi, "Devices");
    hr(out, ansi, cfg);
    out << '\n';

    if (devices.empty()) {
        out << "  " << ansi.dim << "(no devices detected)" << ansi.reset << '\n';
        if (show_footer_hints) {
            render_footer_hints(out, cfg, kFooterHintsDevices);
        }
        return;
    }

    const auto order = kind_sorted_device_indices(devices);
    const auto widths = compute_device_widths(devices);

    std::string_view current_bucket;
    bool first_bucket = true;
    for (std::size_t i = 0; i < order.size(); ++i) {
        const auto& d = devices[order[i]];
        const auto bucket = bucket_heading(d.kind);
        if (bucket != current_bucket) {
            if (!first_bucket) {
                out << '\n';
            }
            first_bucket = false;
            current_bucket = bucket;
            out << "  " << ansi.bold << bucket << ansi.reset << '\n';
        }
        render_device_row(out, ansi, d, widths, i == selected_index);
    }
    out << '\n';

    if (show_footer_hints) {
        render_footer_hints(out, cfg, kFooterHintsDevices);
    }
}

void render_device_list_screen(std::ostream& out, const RenderConfig& cfg,
                               const std::vector<Device>& devices, std::size_t selected_index) {
    render_device_list(out, cfg, devices, selected_index, true);
}

namespace {

// Render a stage-grouped, alphabetically-sorted block of metric rows for a
// device-detail screen. Returns the next display index so the caller can
// continue numbering across multiple blocks.
int render_metric_block(std::ostream& out, const Ansi& ansi, int start_index,
                        std::span<const profcat::MetricDescriptor> metrics, const Device& device,
                        const SessionState* session, std::span<const std::uint32_t> supported_ids,
                        bool is_configure) {
    if (metrics.empty()) {
        return start_index;
    }
    const auto order = stage_sorted_indices(metrics);
    profcat::Stage current_stage = metrics[order.front()].stage;
    bool first = true;
    int n = start_index;
    for (std::size_t pos : order) {
        const auto& m = metrics[pos];
        if (first || m.stage != current_stage) {
            current_stage = m.stage;
            first = false;
            out << "  " << ansi.dim << "[" << stage_heading(m.stage) << "]" << ansi.reset << '\n';
        }
        const bool supported = id_in(supported_ids, m.id);
        const bool aspirational = is_aspirational(device, m);
        const bool enabled = metric_enabled(session, device.slot, m.name, is_configure);
        const char* checkbox = "  ";
        if (session != nullptr) {
            checkbox = enabled ? "[x]" : "[ ]";
        }
        render_metric_row(out, ansi, n, m, aspirational, supported, enabled, checkbox);
        ++n;
    }
    return n;
}

}  // namespace

void render_device_detail(std::ostream& out, const RenderConfig& cfg, const Device& device,
                          const SessionState* session_or_null,
                          std::span<const std::uint32_t> supported_ids) {
    const auto ansi = make_ansi(cfg);
    std::string title = device.slot;
    title.append(" - ");
    title.append(device.identity);
    chrome_title(out, ansi, title);
    hr(out, ansi, cfg);
    out << '\n';

    if (!device.supported) {
        out << "  " << ansi.red << "Unsupported: " << ansi.reset
            << device.unsupported_reason.value_or("no detail") << '\n';
        out << '\n';
        render_footer_hints(out, cfg, kFooterHintsDetail);
        return;
    }

    int idx = 1;
    out << "  " << ansi.bold << "Measure:" << ansi.reset << '\n';
    idx = render_metric_block(out, ansi, idx, device.measure, device, session_or_null,
                              supported_ids, false);
    out << "  " << ansi.bold << "Configure:" << ansi.reset << '\n';
    idx = render_metric_block(out, ansi, idx, device.configure, device, session_or_null,
                              supported_ids, true);
    out << '\n';
    render_footer_hints(out, cfg, kFooterHintsDetail);
}

void render_metric_detail(std::ostream& out, const RenderConfig& cfg,
                          const profcat::MetricDescriptor& metric, bool aspirational,
                          bool supported) {
    const auto ansi = make_ansi(cfg);
    std::string title = "Metric: ";
    title.append(metric.display_name);
    header(out, ansi, title);

    // Five-field primary block.
    out << "Name:        " << metric.name << '\n';
    out << "Description: " << metric.description << '\n';
    out << "Track:       " << metric.track << '\n';
    out << "Unit:        " << profcat::to_string(metric.unit) << '\n';
    out << "Group:       " << profcat::to_string(metric.stage) << '\n';

    // Secondary descriptor fields (still useful for the inspector).
    out << "Kind:        " << profcat::to_string(metric.kind) << '\n';
    out << "Source:      " << profcat::to_string(metric.source) << '\n';
    if (metric.needs_root) {
        out << "Caveat:      requires root (CAP_SYS_ADMIN)\n";
    }
    if (metric.multiplexed) {
        out << "Note:        multiplexed counter\n";
    }

    // State badges: supported -> enabled -> aspirational.
    // (Inspector view has no session, so "enabled" is always false here.)
    out << "State:      " << badges_for(ansi, supported, false, aspirational) << '\n';
    out << "  b. Back\n";
}

void render_preset_preview(std::ostream& out, const RenderConfig& cfg, const SessionState& session,
                           const std::vector<Device>& devices) {
    (void) devices;
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "Preset preview");
    if (session.is_empty()) {
        out << ansi.dim << "# Preset is empty. Select metrics to populate." << ansi.reset << '\n';
    } else {
        out << ansi.green << "# Preset built via avail demo - " << session.summary() << ansi.reset
            << '\n';
    }
    out << '\n';
    out << "  1. Clear preset\n";
    out << "  2. Back\n";
}

void render_confirm(std::ostream& out, const RenderConfig& cfg, std::string_view question) {
    const auto ansi = make_ansi(cfg);
    out << ansi.yellow << "? " << question << " [y/N]: " << ansi.reset;
}

void render_invalid_input(std::ostream& out, const RenderConfig& cfg, std::string_view hint) {
    const auto ansi = make_ansi(cfg);
    out << ansi.red << "Invalid input. " << ansi.reset << hint << '\n';
}

void render_welcome_header(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "avail - DEMO");
    out << "Prototype UX for rocprof-sys-avail. All catalog data is mocked.\n";
}

void render_main_menu_header(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    main_menu_rule(out, ansi, cfg);
    render_banner(out, cfg);
    main_menu_rule(out, ansi, cfg);
    out << '\n';
    out << ansi.bold << "Main Menu" << ansi.reset << '\n';
    out << '\n';
}

void render_mode_select_header(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "Create config - choose catalog mode");
}

void render_create_session_header(std::ostream& out, const RenderConfig& cfg, CatalogMode mode,
                                  std::string_view catalog_label, const SessionState& session) {
    const auto ansi = make_ansi(cfg);
    std::string title = "Create config session [";
    title.append(catalog_label);
    title.append("] - ");
    title.append(mode_label(mode));
    header(out, ansi, title);
    if (!session.is_empty()) {
        out << "Current preset: " << ansi.cyan << session.summary() << ansi.reset << '\n';
    } else {
        out << ansi.dim << "Preset is empty - select metrics first." << ansi.reset << '\n';
    }
}

void render_device_list_header(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    chrome_title(out, ansi, "Devices");
    hr(out, ansi, cfg);
    out << '\n';
}

namespace {

// Stage-grouped block, no checkbox numbering (used by interactive header so
// the trailing MenuController draws the choice list).
void render_metric_block_header(std::ostream& out, const Ansi& ansi,
                                std::span<const profcat::MetricDescriptor> metrics,
                                const Device& device, const SessionState* session,
                                std::span<const std::uint32_t> supported_ids, bool is_configure) {
    if (metrics.empty()) {
        return;
    }
    const auto order = stage_sorted_indices(metrics);
    profcat::Stage current_stage = metrics[order.front()].stage;
    bool first = true;
    for (std::size_t pos : order) {
        const auto& m = metrics[pos];
        if (first || m.stage != current_stage) {
            current_stage = m.stage;
            first = false;
            out << "  " << ansi.dim << "[" << stage_heading(m.stage) << "]" << ansi.reset << '\n';
        }
        const bool supported = id_in(supported_ids, m.id);
        const bool aspirational = is_aspirational(device, m);
        const bool enabled = metric_enabled(session, device.slot, m.name, is_configure);
        const char* checkbox = "  ";
        if (session != nullptr) {
            checkbox = enabled ? "[x]" : "[ ]";
        }
        out << "  " << checkbox << ' ' << m.display_name << " (" << profcat::to_string(m.unit)
            << ')' << badges_for(ansi, supported, enabled, aspirational) << '\n';
    }
}

}  // namespace

void render_device_detail_header(std::ostream& out, const RenderConfig& cfg, const Device& device,
                                 const SessionState* session_or_null,
                                 std::span<const std::uint32_t> supported_ids) {
    const auto ansi = make_ansi(cfg);
    std::string title = device.slot;
    title.append(" - ");
    title.append(device.identity);
    chrome_title(out, ansi, title);
    hr(out, ansi, cfg);
    out << '\n';

    if (!device.supported) {
        out << "  " << ansi.red << "Unsupported: " << ansi.reset
            << device.unsupported_reason.value_or("no detail") << '\n';
        return;
    }

    out << "  " << ansi.bold << "Measure:" << ansi.reset << '\n';
    render_metric_block_header(out, ansi, device.measure, device, session_or_null, supported_ids,
                               false);
    out << "  " << ansi.bold << "Configure:" << ansi.reset << '\n';
    render_metric_block_header(out, ansi, device.configure, device, session_or_null, supported_ids,
                               true);
    out << '\n';
}

void render_preset_preview_header(std::ostream& out, const RenderConfig& cfg,
                                  const SessionState& session) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "Preset preview");
    if (session.is_empty()) {
        out << ansi.dim << "# Preset is empty. Select metrics to populate." << ansi.reset << '\n';
    } else {
        out << ansi.green << "# Preset built via avail demo - " << session.summary() << ansi.reset
            << '\n';
    }
    out << '\n';
}

std::string strip_ansi(std::string_view input) {
    std::string out_buf;
    out_buf.reserve(input.size());
    for (std::size_t i = 0; i < input.size();) {
        if (input[i] == '\x1b' && i + 1 < input.size() && input[i + 1] == '[') {
            i += 2;
            while (i < input.size()) {
                const char c = input[i];
                ++i;
                if (std::isalpha(static_cast<unsigned char>(c))) {
                    break;
                }
            }
        } else {
            out_buf.push_back(input[i]);
            ++i;
        }
    }
    return out_buf;
}

}  // namespace avail
