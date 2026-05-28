#include "avail/render.h"

#include "avail/device.h"

#include <algorithm>
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

void render_main_menu(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "Main menu");
    out << "  1. Create profiling configuration\n";
    out << "  2. Inspect what is available\n";
    out << "  3. Exit\n";
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

void render_device_list(std::ostream& out, const RenderConfig& cfg,
                        const std::vector<Device>& devices, bool show_back) {
    const auto ansi = make_ansi(cfg);
    header(out, ansi, "Devices");
    int idx = 1;
    for (const auto& d : devices) {
        out << "  " << idx << ". " << ansi.bold << d.slot << ansi.reset << "  " << d.kind << "  "
            << d.identity;
        if (!d.supported) {
            out << "  " << ansi.red << "[unsupported]" << ansi.reset;
        }
        out << '\n';
        ++idx;
    }
    if (show_back) {
        out << "  " << idx << ". Back\n";
    }
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
    header(out, ansi, title);

    if (!device.supported) {
        out << ansi.red << "Unsupported: " << ansi.reset
            << device.unsupported_reason.value_or("no detail") << '\n';
        out << "  1. Back\n";
        return;
    }

    int idx = 1;
    out << ansi.bold << "Measure:" << ansi.reset << '\n';
    idx = render_metric_block(out, ansi, idx, device.measure, device, session_or_null,
                              supported_ids, false);
    out << ansi.bold << "Configure:" << ansi.reset << '\n';
    idx = render_metric_block(out, ansi, idx, device.configure, device, session_or_null,
                              supported_ids, true);
    out << '\n';
    out << "  Type a number to toggle. 'b' to go back.\n";
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
    header(out, ansi, "Main menu");
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
    header(out, ansi, "Devices");
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
    header(out, ansi, title);

    if (!device.supported) {
        out << ansi.red << "Unsupported: " << ansi.reset
            << device.unsupported_reason.value_or("no detail") << '\n';
        return;
    }

    out << ansi.bold << "Measure:" << ansi.reset << '\n';
    render_metric_block_header(out, ansi, device.measure, device, session_or_null, supported_ids,
                               false);
    out << ansi.bold << "Configure:" << ansi.reset << '\n';
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
