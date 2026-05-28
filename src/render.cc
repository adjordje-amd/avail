#include "avail/render.h"

#include "avail/device.h"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <profcat/descriptor.hpp>
#include <profcat/enums.hpp>
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

constexpr std::string_view kBadge = " [demo: proposed UX]";
constexpr std::string_view kRule = "----------------------------------------";

void rule(std::ostream& out, const Ansi& ansi) {
    out << ansi.dim << kRule << ansi.reset << '\n';
}

void header(std::ostream& out, const Ansi& ansi, std::string_view title) {
    rule(out, ansi);
    out << ansi.bold << title << ansi.reset << '\n';
    rule(out, ansi);
}

void render_metric_line(std::ostream& out, const Ansi& ansi, int index,
                        const profcat::MetricDescriptor& m, bool aspirational,
                        const char* checkbox) {
    out << "  " << index << ". " << checkbox << ' ' << m.display_name << " ("
        << profcat::to_string(m.unit) << ')';
    if (aspirational) {
        out << ansi.yellow << ansi.dim << kBadge << ansi.reset;
    }
    out << '\n';
}

}  // namespace

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

void render_device_detail(std::ostream& out, const RenderConfig& cfg, const Device& device,
                          const SessionState* session_or_null) {
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

    out << ansi.bold << "Measure:" << ansi.reset << '\n';
    int idx = 1;
    for (const auto& m : device.measure) {
        const char* box = "[ ]";
        if (session_or_null != nullptr &&
            session_or_null->has_metric(device.slot, std::string{m.name})) {
            box = "[x]";
        }
        render_metric_line(out, ansi, idx, m, is_aspirational(device, m),
                           session_or_null ? box : "  ");
        ++idx;
    }
    out << ansi.bold << "Configure:" << ansi.reset << '\n';
    for (const auto& m : device.configure) {
        const char* box = "[ ]";
        if (session_or_null != nullptr &&
            session_or_null->has_configure(device.slot, std::string{m.name})) {
            box = "[x]";
        }
        render_metric_line(out, ansi, idx, m, is_aspirational(device, m),
                           session_or_null ? box : "  ");
        ++idx;
    }
    out << '\n';
    out << "  Type a number to toggle. 'b' to go back.\n";
}

void render_metric_detail(std::ostream& out, const RenderConfig& cfg,
                          const profcat::MetricDescriptor& metric, bool aspirational) {
    const auto ansi = make_ansi(cfg);
    std::string title = "Metric: ";
    title.append(metric.display_name);
    header(out, ansi, title);
    out << "Id:          " << metric.name << '\n';
    out << "Description: " << metric.description << '\n';
    out << "Track:       " << metric.track << '\n';
    out << "Kind:        " << profcat::to_string(metric.kind) << '\n';
    out << "Unit:        " << profcat::to_string(metric.unit) << '\n';
    out << "Source:      " << profcat::to_string(metric.source) << '\n';
    out << "Stage:       " << profcat::to_string(metric.stage) << '\n';
    if (metric.needs_root) {
        out << "Caveat:      requires root (CAP_SYS_ADMIN)\n";
    }
    if (metric.multiplexed) {
        out << "Note:        multiplexed counter\n";
    }
    if (aspirational) {
        out << ansi.yellow << ansi.dim << kBadge << ansi.reset << '\n';
    }
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
    // Note: command lines themselves are produced by synthesize_command and
    // streamed into out by the caller; this renders the screen chrome only.
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

void render_device_detail_header(std::ostream& out, const RenderConfig& cfg, const Device& device,
                                 const SessionState* session_or_null) {
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
    for (const auto& m : device.measure) {
        const char* box = "[ ]";
        if (session_or_null != nullptr &&
            session_or_null->has_metric(device.slot, std::string{m.name})) {
            box = "[x]";
        }
        out << "  " << (session_or_null ? box : "  ") << ' ' << m.display_name << " ("
            << profcat::to_string(m.unit) << ')';
        if (is_aspirational(device, m)) {
            out << ansi.yellow << ansi.dim << kBadge << ansi.reset;
        }
        out << '\n';
    }
    out << ansi.bold << "Configure:" << ansi.reset << '\n';
    for (const auto& m : device.configure) {
        const char* box = "[ ]";
        if (session_or_null != nullptr &&
            session_or_null->has_configure(device.slot, std::string{m.name})) {
            box = "[x]";
        }
        out << "  " << (session_or_null ? box : "  ") << ' ' << m.display_name << " ("
            << profcat::to_string(m.unit) << ')';
        if (is_aspirational(device, m)) {
            out << ansi.yellow << ansi.dim << kBadge << ansi.reset;
        }
        out << '\n';
    }
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
