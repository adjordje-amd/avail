#include "avail/screens.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace avail {

std::string screen_name(Screen s) {
    switch (s) {
    case Screen::MainMenu:
        return "main_menu";
    case Screen::CreateModeSelect:
        return "create_mode_select";
    case Screen::CreateSession:
        return "create_session";
    case Screen::CreateDeviceList:
        return "create_device_list";
    case Screen::CreateDeviceDetail:
        return "create_device_detail";
    case Screen::PresetPreview:
        return "preset_preview";
    case Screen::ConfirmClearAll:
        return "confirm_clear_all";
    case Screen::ConfirmClearDevice:
        return "confirm_clear_device";
    case Screen::InspectDeviceList:
        return "inspect_device_list";
    case Screen::InspectDeviceDetail:
        return "inspect_device_detail";
    case Screen::MetricDetail:
        return "metric_detail";
    case Screen::Exit:
        return "exit";
    }
    return "unknown";
}

std::string mode_label(CatalogMode m) {
    return m == CatalogMode::Online ? "online" : "offline";
}

namespace {

bool is_back(std::string_view input) {
    return input == "b" || input == "back" || input == "q" || input == "quit";
}

}  // namespace

Screen next_screen(Screen current, std::string_view input) {
    if (current == Screen::MainMenu) {
        if (input == "1")
            return Screen::CreateModeSelect;
        if (input == "2")
            return Screen::InspectDeviceList;
        if (input == "3" || is_back(input))
            return Screen::Exit;
        return current;
    }
    if (current == Screen::CreateModeSelect) {
        if (input == "1")
            return Screen::CreateSession;  // online
        if (input == "2")
            return Screen::CreateSession;  // offline (mode tracked elsewhere)
        if (input == "3" || is_back(input))
            return Screen::MainMenu;
        return current;
    }
    if (current == Screen::CreateSession) {
        if (input == "1")
            return Screen::CreateDeviceList;
        if (input == "2")
            return Screen::PresetPreview;
        if (input == "3" || is_back(input))
            return Screen::MainMenu;
        return current;
    }
    if (current == Screen::CreateDeviceList) {
        if (is_back(input))
            return Screen::CreateSession;
        return Screen::CreateDeviceDetail;
    }
    if (current == Screen::CreateDeviceDetail) {
        if (is_back(input))
            return Screen::CreateDeviceList;
        if (input == "view_metric")
            return Screen::MetricDetail;
        if (input == "clear_device")
            return Screen::ConfirmClearDevice;
        return current;
    }
    if (current == Screen::PresetPreview) {
        if (is_back(input))
            return Screen::CreateSession;
        if (input == "clear_all")
            return Screen::ConfirmClearAll;
        return current;
    }
    if (current == Screen::ConfirmClearAll) {
        if (input == "y" || input == "yes")
            return Screen::CreateSession;
        return Screen::PresetPreview;
    }
    if (current == Screen::ConfirmClearDevice) {
        return Screen::CreateDeviceDetail;
    }
    if (current == Screen::InspectDeviceList) {
        if (is_back(input))
            return Screen::MainMenu;
        return Screen::InspectDeviceDetail;
    }
    if (current == Screen::InspectDeviceDetail) {
        if (is_back(input))
            return Screen::InspectDeviceList;
        if (input == "view_metric")
            return Screen::MetricDetail;
        return current;
    }
    if (current == Screen::MetricDetail) {
        return Screen::InspectDeviceDetail;
    }
    return current;
}

}  // namespace avail
