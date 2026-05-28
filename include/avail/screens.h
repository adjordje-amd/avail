#pragma once

#include <string>
#include <string_view>

namespace avail {

/**
 * All discrete screens addressable by the REPL.
 *
 * The state machine in states.md drives transitions between these.
 */
enum class Screen {
    Welcome,
    MainMenu,
    CreateModeSelect,
    CreateSession,
    CreateDeviceList,
    CreateDeviceDetail,
    PresetPreview,
    ConfirmClearAll,
    ConfirmClearDevice,
    InspectDeviceList,
    InspectDeviceDetail,
    MetricDetail,
    Exit,
};

/** Convert a Screen enum to a stable lowercase identifier. */
[[nodiscard]] std::string screen_name(Screen s);

/**
 * Catalog mode for the Create-config session.
 *
 * Inspect screens always use the online catalog.
 */
enum class CatalogMode { Online, Offline };

/** Convert a CatalogMode enum to a stable lowercase label. */
[[nodiscard]] std::string mode_label(CatalogMode m);

/**
 * Pure transition function: (current_screen, input_token) -> next_screen.
 *
 * Returns current unchanged when input_token does not produce a transition
 * from the current screen.
 *
 * @param current  Current screen.
 * @param input    Lowercased input token (digit string or keyword like "back").
 * @return         Next screen, or `current` if `input` is not recognised.
 */
[[nodiscard]] Screen next_screen(Screen current, std::string_view input);

}  // namespace avail
