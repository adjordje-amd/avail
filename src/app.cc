#include "avail/app.h"

#include "avail/input.h"
#include "avail/menu.h"
#include "avail/render.h"
#include "avail/screens.h"
#include "avail/session.h"
#include "avail/synthesize.h"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <profcat/descriptor.hpp>
#include <profcat/enums.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

namespace {

constexpr const char* kSeparator = "---";

bool id_in_span(std::span<const std::uint32_t> ids, std::uint32_t id) noexcept {
    for (std::uint32_t v : ids) {
        if (v == id) {
            return true;
        }
    }
    return false;
}

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
        {"Create profiling configuration", kIdCreate, "Configure and launch a profiling run"},
        {"Inspect what is available", kIdInspect, "Discover CPUs, GPUs, NICs on this system"},
        {"Exit", kIdExit, ""},
    };
}

std::vector<MenuItem> build_mode_select_menu() {
    return {
        {"Online", kIdOnline, "This machine, real-ish data"},
        {"Offline", kIdOffline, "Generic device classes, aspirational"},
        {"Back", kIdBack, ""},
    };
}

std::vector<MenuItem> build_create_session_menu(const SessionState& session) {
    std::string preview_desc =
        session.is_empty() ? std::string{"(empty - select metrics first)"} : session.summary();
    std::vector<MenuItem> items;
    items.push_back({"Devices...", kIdDevices, "Browse and toggle metrics"});
    items.push_back({"View preset preview", kIdPreview, std::move(preview_desc)});
    items.push_back({"Back", kIdBack, ""});
    return items;
}

/**
 * Build the device-list menu in the bucket-sorted (kind_sorted_device_indices)
 * order. The label is just the slot since render_device_list_screen draws the
 * full visible row chrome; the label exists only so MenuController can fall
 * back to its own renderer if called.
 */
std::vector<MenuItem> build_device_list_menu(const std::vector<Device>& devices) {
    std::vector<MenuItem> items;
    items.reserve(devices.size());
    const auto order = kind_sorted_device_indices(devices);
    for (std::size_t pos : order) {
        const auto& d = devices[pos];
        items.push_back({d.slot, d.slot});
    }
    return items;
}

// Per-device metric menu. Metric ids: "m:<index>" (measure) or "c:<index>"
// (configure). Indices reference the ORIGINAL device.measure / device.configure
// vectors; rendering order is stage-sorted via stage_sorted_indices so visible
// row N maps to menu item N regardless of the catalog's storage order.
//
// Housekeeping rows use kIdClear / kIdBack.
std::vector<MenuItem> build_device_detail_menu(const Device& device, const SessionState& session,
                                               bool with_session) {
    std::vector<MenuItem> items;
    if (!device.supported) {
        items.push_back({"Back", kIdBack});
        return items;
    }
    items.reserve(device.measure.size() + device.configure.size() + 2);
    const auto measure_order = stage_sorted_indices(device.measure);
    for (std::size_t pos : measure_order) {
        const auto& m = device.measure[pos];
        const bool on = with_session && session.has_metric(device.slot, m.name);
        std::string label = std::string{with_session ? (on ? "[x] " : "[ ] ") : "    "};
        label.append(m.display_name);
        label.append(" (");
        label.append(profcat::to_string(m.unit));
        label.append(")");
        items.push_back({std::move(label), "m:" + std::to_string(pos)});
    }
    const auto configure_order = stage_sorted_indices(device.configure);
    for (std::size_t pos : configure_order) {
        const auto& m = device.configure[pos];
        const bool on = with_session && session.has_configure(device.slot, m.name);
        std::string label = std::string{with_session ? (on ? "[x] " : "[ ] ") : "    "};
        label.append(m.display_name);
        label.append(" (");
        label.append(profcat::to_string(m.unit));
        label.append(")");
        items.push_back({std::move(label), "c:" + std::to_string(pos)});
    }
    if (with_session) {
        items.push_back({"Clear this device's selections", kIdClear});
    }
    items.push_back({"Back", kIdBack});
    return items;
}

std::vector<MenuItem> build_preset_preview_menu() {
    return {
        {"Clear preset", kIdClear, "Reset all metric selections"},
        {"Back", kIdBack, ""},
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

    Screen current = Screen::MainMenu;
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

    auto active_metric_catalog = [&]() -> IMetricCatalog& {
        if (current == Screen::InspectDeviceList || current == Screen::InspectDeviceDetail ||
            (current == Screen::MetricDetail &&
             metric_detail_return == Screen::InspectDeviceDetail)) {
            return m_online_metrics;
        }
        return mode == CatalogMode::Online ? m_online_metrics : m_offline_metrics;
    };

    auto render_header = [&](const std::vector<Device>& devices) {
        out << kClearScreen;
        switch (current) {
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
            // Body drawn by render_device_list_screen in the per-iteration
            // render block below so the cursor can move with the MenuController.
            break;
        case Screen::CreateDeviceDetail:
            if (active_device < devices.size()) {
                const auto& dev = devices[active_device];
                const auto supported = active_metric_catalog().supported_ids(dev.slot);
                render_device_detail_header(out, cfg, dev, &session, supported);
            }
            break;
        case Screen::InspectDeviceDetail:
            if (active_device < devices.size()) {
                const auto& dev = devices[active_device];
                const auto supported = active_metric_catalog().supported_ids(dev.slot);
                render_device_detail_header(out, cfg, dev, nullptr, supported);
            }
            break;
        case Screen::PresetPreview:
            render_preset_preview_header(out, cfg, session);
            out << synthesize_command(session, devices) << '\n' << '\n';
            break;
        case Screen::ConfirmClearAll: {
            const auto ansi = make_ansi(cfg);
            render_chrome_open(out, cfg, "Confirm");
            out << ansi.yellow << "? Clear ALL preset selections? [y/N]" << ansi.reset << '\n';
            out << '\n';
            break;
        }
        case Screen::ConfirmClearDevice: {
            const auto ansi = make_ansi(cfg);
            render_chrome_open(out, cfg, "Confirm");
            out << ansi.yellow << "? Clear this device's selections? [y/N]" << ansi.reset << '\n';
            out << '\n';
            break;
        }
        case Screen::MetricDetail:
            if (active_device < devices.size()) {
                const auto& dev = devices[active_device];
                const auto supported = active_metric_catalog().supported_ids(dev.slot);
                const auto total_measure = dev.measure.size();
                if (active_metric < total_measure) {
                    const auto& md = dev.measure[active_metric];
                    render_metric_detail(out, cfg, md, is_aspirational(dev, md),
                                         id_in_span(supported, md.id));
                } else if (active_metric - total_measure < dev.configure.size()) {
                    const auto& md = dev.configure[active_metric - total_measure];
                    render_metric_detail(out, cfg, md, is_aspirational(dev, md),
                                         id_in_span(supported, md.id));
                }
            }
            break;
        case Screen::Exit:
            break;
        }
    };

    while (current != Screen::Exit) {
        std::vector<Device> devices = active_catalog().devices();

        // Non-menu screens: render once and read a single key.
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
        bool back_now = false;

        const bool is_device_list_screen =
            current == Screen::CreateDeviceList || current == Screen::InspectDeviceList;
        const bool uses_shared_menu_items =
            current == Screen::MainMenu || current == Screen::CreateModeSelect ||
            current == Screen::CreateSession || current == Screen::PresetPreview;

        while (!committed && !exit_now && !back_now) {
            render_header(devices);
            if (is_device_list_screen) {
                // render_device_list_screen already emits open+body+close.
                render_device_list_screen(out, cfg, devices, menu.selected_index());
            } else if (uses_shared_menu_items) {
                render_menu_items(out, cfg, menu.items(), menu.selected_index());
                out << '\n';
                std::string_view hints;
                switch (current) {
                case Screen::CreateModeSelect:
                    hints = footer_hints_for_mode_select();
                    break;
                case Screen::CreateSession:
                    hints = footer_hints_for_create_session();
                    break;
                case Screen::PresetPreview:
                    hints = footer_hints_for_preset_preview();
                    break;
                default:
                    hints = footer_hints_for_main_menu();
                    break;
                }
                render_chrome_close(out, cfg, hints);
            } else {
                menu.render(out, cfg);
                out << '\n';
                std::string_view hints;
                switch (current) {
                case Screen::CreateDeviceDetail:
                case Screen::InspectDeviceDetail:
                    hints = footer_hints_for_device_detail();
                    break;
                case Screen::ConfirmClearAll:
                case Screen::ConfirmClearDevice:
                    hints = footer_hints_for_confirm();
                    break;
                default:
                    hints = footer_hints_for_main_menu();
                    break;
                }
                render_chrome_close(out, cfg, hints);
            }
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
                if (is_device_list_screen) {
                    back_now = true;
                } else if (menu_has_id(menu, kIdBack)) {
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
        if (back_now) {
            // Footer-driven back affordance for device-list screens; the
            // MenuController has no Back item there.
            current =
                (current == Screen::CreateDeviceList) ? Screen::CreateSession : Screen::MainMenu;
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
                    const bool entering_create = (current == Screen::CreateDeviceList);
                    current =
                        entering_create ? Screen::CreateDeviceDetail : Screen::InspectDeviceDetail;
                    if (entering_create && !session.is_seeded(devices[i].slot)) {
                        auto& mc =
                            (mode == CatalogMode::Online) ? m_online_metrics : m_offline_metrics;
                        session.seed_defaults(devices[i].slot, mc.measure_metrics(devices[i].slot),
                                              mc.configure_metrics(devices[i].slot),
                                              mc.supported_ids(devices[i].slot));
                    }
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

    render_main_menu(out, cfg);
    sep();
    render_mode_select(out, cfg);
    sep();
    render_create_session(out, cfg, CatalogMode::Online, m_online.label(), session);
    sep();
    render_device_list(out, cfg, online, /*selected_index=*/0, /*show_footer_hints=*/true);
    sep();
    if (!online.empty()) {
        const auto supported = m_online_metrics.supported_ids(online[0].slot);
        render_device_detail(out, cfg, online[0], &session, supported);
        sep();
    }
    if (!online.empty() && !online[0].measure.empty()) {
        session.toggle_metric(online[0].slot, online[0].measure[0].name);
    }
    render_create_session(out, cfg, CatalogMode::Online, m_online.label(), session);
    sep();
    render_preset_preview(out, cfg, session, online);
    out << synthesize_command(session, online) << '\n';
    sep();
    session.clear_all();
    render_create_session(out, cfg, CatalogMode::Offline, m_offline.label(), session);
    sep();
    render_device_list(out, cfg, offline, /*selected_index=*/0, /*show_footer_hints=*/true);
    sep();
    if (!offline.empty()) {
        const auto supported = m_offline_metrics.supported_ids(offline[0].slot);
        render_device_detail(out, cfg, offline[0], &session, supported);
        sep();
    }
    render_main_menu(out, cfg);
    sep();
    render_device_list(out, cfg, online, /*selected_index=*/0, /*show_footer_hints=*/true);
    sep();
    if (online.size() > 1) {
        const auto supported = m_online_metrics.supported_ids(online[1].slot);
        render_device_detail(out, cfg, online[1], nullptr, supported);
        sep();
    }
    if (online.size() > 1 && !online[1].measure.empty()) {
        const auto& md = online[1].measure[0];
        const auto supported = m_online_metrics.supported_ids(online[1].slot);
        render_metric_detail(out, cfg, md, is_aspirational(online[1], md),
                             id_in_span(supported, md.id));
        sep();
    }
    out << "Goodbye.\n";
    return 0;
}

}  // namespace avail
