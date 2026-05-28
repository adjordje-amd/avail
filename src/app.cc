#include "avail/app.h"

#include "avail/render.h"
#include "avail/screens.h"
#include "avail/session.h"
#include "avail/synthesize.h"

#include <algorithm>
#include <cctype>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

namespace {

constexpr const char* kSeparator = "---";

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(std::string s) {
    const auto issp = [](unsigned char c) { return std::isspace(c) != 0; };
    const auto first = std::find_if_not(s.begin(), s.end(), issp);
    s.erase(s.begin(), first);
    const auto last = std::find_if_not(s.rbegin(), s.rend(), issp).base();
    s.erase(last, s.end());
    return s;
}

bool parse_index(std::string_view token, int& out_idx) {
    if (token.empty()) {
        return false;
    }
    int value = 0;
    for (char c : token) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
    }
    out_idx = value;
    return true;
}

void prompt(std::ostream& out, const RenderConfig& cfg) {
    const auto ansi = make_ansi(cfg);
    out << ansi.bold << "avail> " << ansi.reset;
}

void render_screen(std::ostream& out, const RenderConfig& cfg, Screen current,
                   IDeviceCatalog& catalog, CatalogMode mode, const SessionState& session,
                   const std::vector<Device>& devices, std::size_t active_device,
                   std::size_t active_metric_in_device) {
    switch (current) {
    case Screen::Welcome:
        render_welcome(out, cfg);
        break;
    case Screen::MainMenu:
        render_main_menu(out, cfg);
        break;
    case Screen::CreateModeSelect:
        render_mode_select(out, cfg);
        break;
    case Screen::CreateSession:
        render_create_session(out, cfg, mode, catalog.label(), session);
        break;
    case Screen::CreateDeviceList:
        render_device_list(out, cfg, devices, true);
        break;
    case Screen::CreateDeviceDetail:
        if (active_device < devices.size()) {
            render_device_detail(out, cfg, devices[active_device], &session);
        }
        break;
    case Screen::PresetPreview:
        render_preset_preview(out, cfg, session, devices);
        out << synthesize_command(session, devices) << '\n';
        break;
    case Screen::ConfirmClearAll:
        render_confirm(out, cfg, "Clear ALL preset selections?");
        break;
    case Screen::ConfirmClearDevice:
        render_confirm(out, cfg, "Clear this device's selections?");
        break;
    case Screen::InspectDeviceList:
        render_device_list(out, cfg, devices, true);
        break;
    case Screen::InspectDeviceDetail:
        if (active_device < devices.size()) {
            render_device_detail(out, cfg, devices[active_device], nullptr);
        }
        break;
    case Screen::MetricDetail:
        if (active_device < devices.size()) {
            const auto& dev = devices[active_device];
            const auto total_measure = dev.measure.size();
            if (active_metric_in_device < total_measure) {
                render_metric_detail(out, cfg, dev.measure[active_metric_in_device]);
            } else if (active_metric_in_device - total_measure < dev.configure.size()) {
                render_metric_detail(out, cfg,
                                     dev.configure[active_metric_in_device - total_measure]);
            }
        }
        break;
    case Screen::Exit:
        out << "Goodbye.\n";
        break;
    }
}

}  // namespace

App::App(IDeviceCatalog& online, IDeviceCatalog& offline, IMetricCatalog& online_metrics,
         IMetricCatalog& offline_metrics, IRocmVersion& rocm, ILibraryEnv& libs)
    : m_online(online), m_offline(offline), m_online_metrics(online_metrics),
      m_offline_metrics(offline_metrics), m_rocm(rocm), m_libs(libs) {
    (void) m_online_metrics;
    (void) m_offline_metrics;
    (void) m_rocm;
    (void) m_libs;
}

int App::run_interactive(std::istream& in, std::ostream& out, const RenderConfig& cfg) {
    Screen current = Screen::Welcome;
    Screen metric_detail_return = Screen::MainMenu;
    CatalogMode mode = CatalogMode::Online;
    SessionState session;
    std::size_t active_device = 0;
    std::size_t active_metric = 0;

    auto active_catalog = [&]() -> IDeviceCatalog& {
        if (current == Screen::InspectDeviceList || current == Screen::InspectDeviceDetail) {
            return m_online;
        }
        return mode == CatalogMode::Online ? m_online : m_offline;
    };

    while (current != Screen::Exit) {
        std::vector<Device> devices = active_catalog().devices();
        render_screen(out, cfg, current, active_catalog(), mode, session, devices, active_device,
                      active_metric);

        if (current == Screen::CreateDeviceDetail || current == Screen::InspectDeviceDetail) {
            // explicit listing of extra commands inline above; nothing else to render
        }

        prompt(out, cfg);
        std::string raw;
        if (!std::getline(in, raw)) {
            break;
        }
        const std::string token = lower(trim(raw));

        // CreateDeviceDetail: digit = toggle metric, "v <n>" = view metric, "c" = clear, "b" back.
        if (current == Screen::CreateDeviceDetail && active_device < devices.size()) {
            const auto& dev = devices[active_device];
            const auto total_measure = dev.measure.size();
            const auto total = total_measure + dev.configure.size();
            int idx = 0;
            if (parse_index(token, idx) && idx >= 1 && static_cast<std::size_t>(idx) <= total) {
                const std::size_t z = static_cast<std::size_t>(idx) - 1;
                if (z < total_measure) {
                    session.toggle_metric(dev.slot, dev.measure[z].name);
                } else {
                    session.toggle_configure(dev.slot, dev.configure[z - total_measure].name);
                }
                continue;
            }
            if (token.rfind("v ", 0) == 0) {
                int sub = 0;
                if (parse_index(std::string_view(token).substr(2), sub) && sub >= 1 &&
                    static_cast<std::size_t>(sub) <= total) {
                    active_metric = static_cast<std::size_t>(sub) - 1;
                    metric_detail_return = Screen::CreateDeviceDetail;
                    current = Screen::MetricDetail;
                    continue;
                }
            }
            if (token == "c" || token == "clear") {
                current = Screen::ConfirmClearDevice;
                continue;
            }
            if (token == "b" || token == "back" || token == "q" || token == "quit") {
                current = Screen::CreateDeviceList;
                continue;
            }
            render_invalid_input(out, cfg, "Type a metric number, 'v N' to view, 'c' clear, 'b'.");
            continue;
        }

        // InspectDeviceDetail: "v N" to view, "b" back.
        if (current == Screen::InspectDeviceDetail && active_device < devices.size()) {
            const auto& dev = devices[active_device];
            const auto total = dev.measure.size() + dev.configure.size();
            if (token.rfind("v ", 0) == 0) {
                int sub = 0;
                if (parse_index(std::string_view(token).substr(2), sub) && sub >= 1 &&
                    static_cast<std::size_t>(sub) <= total) {
                    active_metric = static_cast<std::size_t>(sub) - 1;
                    metric_detail_return = Screen::InspectDeviceDetail;
                    current = Screen::MetricDetail;
                    continue;
                }
            }
            if (token == "b" || token == "back" || token == "q" || token == "quit") {
                current = Screen::InspectDeviceList;
                continue;
            }
            render_invalid_input(out, cfg, "Type 'v N' to view a metric, 'b' to go back.");
            continue;
        }

        // Device lists: digit selects device, last index is Back.
        if (current == Screen::CreateDeviceList || current == Screen::InspectDeviceList) {
            int idx = 0;
            if (parse_index(token, idx) && idx >= 1 &&
                static_cast<std::size_t>(idx) <= devices.size()) {
                active_device = static_cast<std::size_t>(idx) - 1;
                current = (current == Screen::CreateDeviceList) ? Screen::CreateDeviceDetail
                                                                : Screen::InspectDeviceDetail;
                continue;
            }
            if (parse_index(token, idx) && static_cast<std::size_t>(idx) == devices.size() + 1) {
                current = (current == Screen::CreateDeviceList) ? Screen::CreateSession
                                                                : Screen::MainMenu;
                continue;
            }
            if (token == "b" || token == "back" || token == "q" || token == "quit") {
                current = (current == Screen::CreateDeviceList) ? Screen::CreateSession
                                                                : Screen::MainMenu;
                continue;
            }
            render_invalid_input(out, cfg, "Type a device number or 'b'.");
            continue;
        }

        // CreateModeSelect handled specially to set mode + clear session.
        if (current == Screen::CreateModeSelect) {
            if (token == "1") {
                mode = CatalogMode::Online;
                session.clear_all();
                current = Screen::CreateSession;
                continue;
            }
            if (token == "2") {
                mode = CatalogMode::Offline;
                session.clear_all();
                current = Screen::CreateSession;
                continue;
            }
            if (token == "3" || token == "b" || token == "back" || token == "q" ||
                token == "quit") {
                current = Screen::MainMenu;
                continue;
            }
            render_invalid_input(out, cfg, "Choose 1, 2 or 3.");
            continue;
        }

        // CreateSession: 1 -> device list, 2 -> preview (only when non-empty), 3 -> back to main.
        if (current == Screen::CreateSession) {
            if (token == "1") {
                current = Screen::CreateDeviceList;
                continue;
            }
            if (token == "2") {
                if (session.is_empty()) {
                    render_invalid_input(out, cfg, "Preset is empty - select metrics first.");
                    continue;
                }
                current = Screen::PresetPreview;
                continue;
            }
            if (token == "3" || token == "b" || token == "back" || token == "q" ||
                token == "quit") {
                session.clear_all();
                current = Screen::MainMenu;
                continue;
            }
            render_invalid_input(out, cfg, "Choose 1, 2 or 3.");
            continue;
        }

        if (current == Screen::PresetPreview) {
            if (token == "1" || token == "clear" || token == "clear_all") {
                current = Screen::ConfirmClearAll;
                continue;
            }
            if (token == "2" || token == "b" || token == "back") {
                current = Screen::CreateSession;
                continue;
            }
            render_invalid_input(out, cfg, "Choose 1 to clear or 2 to go back.");
            continue;
        }

        if (current == Screen::ConfirmClearAll) {
            if (token == "y" || token == "yes") {
                session.clear_all();
            }
            current = Screen::CreateSession;
            continue;
        }
        if (current == Screen::ConfirmClearDevice) {
            if (token == "y" || token == "yes") {
                if (active_device < devices.size()) {
                    session.clear_device(devices[active_device].slot);
                }
            }
            current = Screen::CreateDeviceDetail;
            continue;
        }
        if (current == Screen::MetricDetail) {
            current = metric_detail_return;
            continue;
        }

        // Main menu / Welcome.
        current = next_screen(current, token);
    }

    render_screen(out, cfg, Screen::Exit, active_catalog(), mode, session,
                  active_catalog().devices(), active_device, active_metric);
    return 0;
}

int App::run_demo_frames(std::ostream& out, const RenderConfig& cfg) {
    SessionState session;
    const auto online = m_online.devices();
    const auto offline = m_offline.devices();

    const auto sep = [&]() { out << kSeparator << '\n'; };

    render_welcome(out, cfg);
    sep();
    render_main_menu(out, cfg);
    sep();
    render_mode_select(out, cfg);
    sep();
    render_create_session(out, cfg, CatalogMode::Online, m_online.label(), session);
    sep();
    render_device_list(out, cfg, online, true);
    sep();
    if (!online.empty()) {
        render_device_detail(out, cfg, online[0], &session);
        sep();
    }
    // Toggle one metric to make preview interesting.
    if (!online.empty() && !online[0].measure.empty()) {
        session.toggle_metric(online[0].slot, online[0].measure[0].name);
    }
    render_preset_preview(out, cfg, session, online);
    out << synthesize_command(session, online) << '\n';
    sep();
    session.clear_all();
    render_create_session(out, cfg, CatalogMode::Offline, m_offline.label(), session);
    sep();
    render_device_list(out, cfg, offline, true);
    sep();
    if (!offline.empty()) {
        render_device_detail(out, cfg, offline[0], &session);
        sep();
    }
    render_main_menu(out, cfg);
    sep();
    render_device_list(out, cfg, online, true);
    sep();
    if (online.size() > 1) {
        render_device_detail(out, cfg, online[1], nullptr);
        sep();
    }
    if (online.size() > 1 && !online[1].measure.empty()) {
        render_metric_detail(out, cfg, online[1].measure[0]);
        sep();
    }
    out << "Goodbye.\n";
    return 0;
}

}  // namespace avail
