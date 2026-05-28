#include "avail/app.h"

#include "avail/input.h"
#include "avail/menu.h"
#include "avail/render.h"
#include "avail/screens.h"
#include "avail/session.h"
#include "avail/synthesize.h"

#include <cstddef>
#include <istream>
#include <ostream>
#include <profcat/descriptor.hpp>
#include <profcat/enums.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

namespace {

constexpr const char* kSeparator = "---";

// id tokens used by MenuItem.id; opaque to render code.
constexpr const char* kIdBack = "back";
constexpr const char* kIdYes = "yes";
constexpr const char* kIdNo = "no";
constexpr const char* kIdClear = "clear";
constexpr const char* kIdPreview = "preview";
constexpr const char* kIdDevices = "devices";
constexpr const char* kIdExit = "exit";
constexpr const char* kIdCreate = "create";
constexpr const char* kIdInspect = "inspect";
constexpr const char* kIdOnline = "online";
constexpr const char* kIdOffline = "offline";

std::vector<MenuItem> build_main_menu() {
    return {
        {"Create profiling configuration", kIdCreate},
        {"Inspect what is available", kIdInspect},
        {"Exit", kIdExit},
    };
}

std::vector<MenuItem> build_mode_select_menu() {
    return {
        {"Online  (this machine, real-ish data)", kIdOnline},
        {"Offline (generic device classes, aspirational)", kIdOffline},
        {"Back", kIdBack},
    };
}

std::vector<MenuItem> build_create_session_menu(const SessionState& session) {
    std::vector<MenuItem> items;
    items.push_back({"Devices...", kIdDevices});
    if (session.is_empty()) {
        items.push_back({"View preset preview (empty - select metrics first)", kIdPreview});
    } else {
        items.push_back({"View preset preview (" + session.summary() + ")", kIdPreview});
    }
    items.push_back({"Back", kIdBack});
    return items;
}

std::vector<MenuItem> build_device_list_menu(const std::vector<Device>& devices) {
    std::vector<MenuItem> items;
    items.reserve(devices.size() + 1);
    for (const auto& d : devices) {
        std::string label = d.slot + "  " + d.kind + "  " + d.identity;
        if (!d.supported) {
            label += "  [unsupported]";
        }
        items.push_back({std::move(label), d.slot});
    }
    items.push_back({"Back", kIdBack});
    return items;
}

// Per-device metric menu. Metric ids: "m:<index>" (measure) or "c:<index>"
// (configure). Housekeeping rows use kIdClear / kIdBack.
std::vector<MenuItem> build_device_detail_menu(const Device& device, const SessionState& session,
                                               bool with_session) {
    std::vector<MenuItem> items;
    if (!device.supported) {
        items.push_back({"Back", kIdBack});
        return items;
    }
    items.reserve(device.measure.size() + device.configure.size() + 2);
    for (std::size_t i = 0; i < device.measure.size(); ++i) {
        const auto& m = device.measure[i];
        const bool on = with_session && session.has_metric(device.slot, m.name);
        std::string label = std::string{with_session ? (on ? "[x] " : "[ ] ") : "    "};
        label.append(m.display_name);
        label.append(" (");
        label.append(profcat::to_string(m.unit));
        label.append(")");
        items.push_back({std::move(label), "m:" + std::to_string(i)});
    }
    for (std::size_t i = 0; i < device.configure.size(); ++i) {
        const auto& m = device.configure[i];
        const bool on = with_session && session.has_configure(device.slot, m.name);
        std::string label = std::string{with_session ? (on ? "[x] " : "[ ] ") : "    "};
        label.append(m.display_name);
        label.append(" (");
        label.append(profcat::to_string(m.unit));
        label.append(")");
        items.push_back({std::move(label), "c:" + std::to_string(i)});
    }
    if (with_session) {
        items.push_back({"Clear this device's selections", kIdClear});
    }
    items.push_back({"Back", kIdBack});
    return items;
}

std::vector<MenuItem> build_preset_preview_menu() {
    return {
        {"Clear preset", kIdClear},
        {"Back", kIdBack},
    };
}

std::vector<MenuItem> build_confirm_menu() {
    return {
        {"No", kIdNo},
        {"Yes", kIdYes},
    };
}

bool menu_has_id(const MenuController& menu, std::string_view id) noexcept {
    for (const auto& it : menu.items()) {
        if (it.id == id) {
            return true;
        }
    }
    return false;
}

// Move selection to the first item whose id matches @c id. No-op if absent.
void select_by_id(MenuController& menu, std::string_view id) noexcept {
    for (std::size_t i = 0; i < menu.items().size(); ++i) {
        if (menu.items()[i].id == id) {
            while (menu.selected_index() != i) {
                menu.move_down();
            }
            return;
        }
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
    (void) in;  // raw mode reads from STDIN_FILENO directly

    RawModeGuard raw_guard;
    if (!raw_guard.active()) {
        out << "avail interactive requires a TTY on stdin.\n";
        return 2;
    }

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

    auto render_header = [&](const std::vector<Device>& devices) {
        out << kClearScreen;
        switch (current) {
        case Screen::Welcome:
            render_welcome_header(out, cfg);
            out << '\n' << "Press Enter to continue (q to quit).\n";
            break;
        case Screen::MainMenu:
            render_main_menu_header(out, cfg);
            break;
        case Screen::CreateModeSelect:
            render_mode_select_header(out, cfg);
            break;
        case Screen::CreateSession:
            render_create_session_header(out, cfg, mode, active_catalog().label(), session);
            break;
        case Screen::CreateDeviceList:
        case Screen::InspectDeviceList:
            render_device_list_header(out, cfg);
            break;
        case Screen::CreateDeviceDetail:
            if (active_device < devices.size()) {
                render_device_detail_header(out, cfg, devices[active_device], &session);
            }
            break;
        case Screen::InspectDeviceDetail:
            if (active_device < devices.size()) {
                render_device_detail_header(out, cfg, devices[active_device], nullptr);
            }
            break;
        case Screen::PresetPreview:
            render_preset_preview_header(out, cfg, session);
            out << synthesize_command(session, devices) << '\n' << '\n';
            break;
        case Screen::ConfirmClearAll:
            render_confirm(out, cfg, "Clear ALL preset selections?");
            out << '\n';
            break;
        case Screen::ConfirmClearDevice:
            render_confirm(out, cfg, "Clear this device's selections?");
            out << '\n';
            break;
        case Screen::MetricDetail:
            if (active_device < devices.size()) {
                const auto& dev = devices[active_device];
                const auto total_measure = dev.measure.size();
                if (active_metric < total_measure) {
                    const auto& md = dev.measure[active_metric];
                    render_metric_detail(out, cfg, md, is_aspirational(dev, md));
                } else if (active_metric - total_measure < dev.configure.size()) {
                    const auto& md = dev.configure[active_metric - total_measure];
                    render_metric_detail(out, cfg, md, is_aspirational(dev, md));
                }
            }
            out << '\n' << "(Press Enter, Esc, q, or b to go back)\n";
            break;
        case Screen::Exit:
            break;
        }
    };

    while (current != Screen::Exit) {
        std::vector<Device> devices = active_catalog().devices();

        // Non-menu screens: render once and read a single key.
        if (current == Screen::Welcome) {
            render_header(devices);
            out.flush();
            const auto ev = read_key();
            if (ev.key == Key::Enter) {
                current = Screen::MainMenu;
            } else if (ev.key == Key::Esc ||
                       (ev.key == Key::Char && (ev.ch == 'q' || ev.ch == 'Q'))) {
                current = Screen::Exit;
            }
            continue;
        }
        if (current == Screen::MetricDetail) {
            render_header(devices);
            out.flush();
            const auto ev = read_key();
            if (ev.key == Key::Enter || ev.key == Key::Esc ||
                (ev.key == Key::Char && (ev.ch == 'q' || ev.ch == 'Q' || ev.ch == 'b'))) {
                current = metric_detail_return;
            }
            continue;
        }

        // Menu screens: build menu, then redraw on every keypress until commit.
        std::vector<MenuItem> items;
        switch (current) {
        case Screen::MainMenu:
            items = build_main_menu();
            break;
        case Screen::CreateModeSelect:
            items = build_mode_select_menu();
            break;
        case Screen::CreateSession:
            items = build_create_session_menu(session);
            break;
        case Screen::CreateDeviceList:
        case Screen::InspectDeviceList:
            items = build_device_list_menu(devices);
            break;
        case Screen::CreateDeviceDetail:
            if (active_device < devices.size()) {
                items = build_device_detail_menu(devices[active_device], session, true);
            }
            break;
        case Screen::InspectDeviceDetail:
            if (active_device < devices.size()) {
                items = build_device_detail_menu(devices[active_device], session, false);
            }
            break;
        case Screen::PresetPreview:
            items = build_preset_preview_menu();
            break;
        case Screen::ConfirmClearAll:
        case Screen::ConfirmClearDevice:
            items = build_confirm_menu();
            break;
        default:
            break;
        }

        MenuController menu(std::move(items));
        bool committed = false;
        bool exit_now = false;

        while (!committed && !exit_now) {
            render_header(devices);
            menu.render(out, cfg);
            out.flush();

            const auto ev = read_key();
            if (ev.key == Key::Up) {
                menu.move_up();
                continue;
            }
            if (ev.key == Key::Down) {
                menu.move_down();
                continue;
            }
            if (ev.key == Key::Enter) {
                committed = true;
                continue;
            }
            const bool esc_or_q =
                ev.key == Key::Esc || (ev.key == Key::Char && (ev.ch == 'q' || ev.ch == 'Q'));
            const bool back_char = ev.key == Key::Char && ev.ch == 'b';
            if (esc_or_q || back_char) {
                if (menu_has_id(menu, kIdBack)) {
                    select_by_id(menu, kIdBack);
                    committed = true;
                } else if (esc_or_q) {
                    exit_now = true;
                }
                continue;
            }
            // y / n shortcuts on confirm screens.
            if ((current == Screen::ConfirmClearAll || current == Screen::ConfirmClearDevice) &&
                ev.key == Key::Char) {
                if (ev.ch == 'y' || ev.ch == 'Y') {
                    select_by_id(menu, kIdYes);
                    committed = true;
                    continue;
                }
                if (ev.ch == 'n' || ev.ch == 'N') {
                    select_by_id(menu, kIdNo);
                    committed = true;
                    continue;
                }
            }
        }

        if (exit_now) {
            current = Screen::Exit;
            continue;
        }

        const std::string id = menu.current().id;

        if (current == Screen::MainMenu) {
            if (id == kIdCreate) {
                current = Screen::CreateModeSelect;
            } else if (id == kIdInspect) {
                current = Screen::InspectDeviceList;
            } else if (id == kIdExit) {
                current = Screen::Exit;
            }
            continue;
        }
        if (current == Screen::CreateModeSelect) {
            if (id == kIdOnline) {
                mode = CatalogMode::Online;
                session.clear_all();
                current = Screen::CreateSession;
            } else if (id == kIdOffline) {
                mode = CatalogMode::Offline;
                session.clear_all();
                current = Screen::CreateSession;
            } else if (id == kIdBack) {
                current = Screen::MainMenu;
            }
            continue;
        }
        if (current == Screen::CreateSession) {
            if (id == kIdDevices) {
                current = Screen::CreateDeviceList;
            } else if (id == kIdPreview) {
                if (!session.is_empty()) {
                    current = Screen::PresetPreview;
                }
            } else if (id == kIdBack) {
                session.clear_all();
                current = Screen::MainMenu;
            }
            continue;
        }
        if (current == Screen::CreateDeviceList || current == Screen::InspectDeviceList) {
            if (id == kIdBack) {
                current = (current == Screen::CreateDeviceList) ? Screen::CreateSession
                                                                : Screen::MainMenu;
                continue;
            }
            for (std::size_t i = 0; i < devices.size(); ++i) {
                if (devices[i].slot == id) {
                    active_device = i;
                    current = (current == Screen::CreateDeviceList) ? Screen::CreateDeviceDetail
                                                                    : Screen::InspectDeviceDetail;
                    break;
                }
            }
            continue;
        }
        if (current == Screen::CreateDeviceDetail) {
            if (id == kIdBack) {
                current = Screen::CreateDeviceList;
                continue;
            }
            if (id == kIdClear) {
                current = Screen::ConfirmClearDevice;
                continue;
            }
            if (active_device < devices.size() && id.size() > 2 && id[1] == ':') {
                const auto& dev = devices[active_device];
                const std::size_t k = std::stoull(id.substr(2));
                if (id[0] == 'm' && k < dev.measure.size()) {
                    session.toggle_metric(dev.slot, dev.measure[k].name);
                } else if (id[0] == 'c' && k < dev.configure.size()) {
                    session.toggle_configure(dev.slot, dev.configure[k].name);
                }
            }
            continue;
        }
        if (current == Screen::InspectDeviceDetail) {
            if (id == kIdBack) {
                current = Screen::InspectDeviceList;
                continue;
            }
            if (active_device < devices.size() && id.size() > 2 && id[1] == ':') {
                const auto& dev = devices[active_device];
                const std::size_t k = std::stoull(id.substr(2));
                const auto total_measure = dev.measure.size();
                if (id[0] == 'm' && k < total_measure) {
                    active_metric = k;
                    metric_detail_return = Screen::InspectDeviceDetail;
                    current = Screen::MetricDetail;
                } else if (id[0] == 'c' && k < dev.configure.size()) {
                    active_metric = total_measure + k;
                    metric_detail_return = Screen::InspectDeviceDetail;
                    current = Screen::MetricDetail;
                }
            }
            continue;
        }
        if (current == Screen::PresetPreview) {
            if (id == kIdClear) {
                current = Screen::ConfirmClearAll;
            } else if (id == kIdBack) {
                current = Screen::CreateSession;
            }
            continue;
        }
        if (current == Screen::ConfirmClearAll) {
            if (id == kIdYes) {
                session.clear_all();
            }
            current = Screen::CreateSession;
            continue;
        }
        if (current == Screen::ConfirmClearDevice) {
            if (id == kIdYes && active_device < devices.size()) {
                session.clear_device(devices[active_device].slot);
            }
            current = Screen::CreateDeviceDetail;
            continue;
        }
    }

    out << kClearScreen;
    out << "Goodbye.\n";
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
        const auto& md = online[1].measure[0];
        render_metric_detail(out, cfg, md, is_aspirational(online[1], md));
        sep();
    }
    out << "Goodbye.\n";
    return 0;
}

}  // namespace avail
