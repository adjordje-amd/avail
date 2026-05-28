#pragma once

#include "avail/device.h"
#include "avail/screens.h"
#include "avail/session.h"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <profcat/descriptor.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

struct MenuItem;

/**
 * Global render configuration shared by every render function.
 *
 * @c rule_width controls the horizontal rule length. Zero means the renderer
 * should auto-detect from the terminal (falling back to a fixed default when
 * stdout is not a TTY).
 */
struct RenderConfig {
    bool color = true;
    int rule_width = 0;
};

/** ANSI escape constants. Empty strings when config.color is false. */
struct Ansi {
    std::string reset;
    std::string bold;
    std::string dim;
    std::string yellow;
    std::string cyan;
    std::string green;
    std::string red;
};

[[nodiscard]] Ansi make_ansi(const RenderConfig& cfg);

void render_welcome(std::ostream& out, const RenderConfig& cfg);
void render_main_menu(std::ostream& out, const RenderConfig& cfg);

/**
 * Render the ASCII-art product banner + build-info line used at the top
 * of the main menu. Banner is cyan and build-info dim when colour is on;
 * stripped to plain text otherwise.
 */
void render_banner(std::ostream& out, const RenderConfig& cfg);

/**
 * Render the main-menu item list in 2-column layout: left-aligned label
 * padded to the widest label across @p items, then a fixed gutter, then
 * the per-item description (dim when colour is on, omitted entirely when
 * empty). The row matching @p selected_index is prefixed with "> ", all
 * others with "  ".
 */
void render_main_menu_items(std::ostream& out, const RenderConfig& cfg,
                            const std::vector<MenuItem>& items, std::size_t selected_index);
void render_mode_select(std::ostream& out, const RenderConfig& cfg);
void render_create_session(std::ostream& out, const RenderConfig& cfg, CatalogMode mode,
                           std::string_view catalog_label, const SessionState& session);

/**
 * Render the section-grouped device list with status glyphs and footer hints.
 *
 * Devices are bucketed by @c kind into stable groups (CPU > GPU > NIC > other).
 * Each row prefixes a status glyph (@c ● green for supported, @c ⚠ yellow for
 * unsupported), followed by three columns padded to the widest entry across all
 * devices. @c selected_index points at a device row in the bucket-sorted flat
 * order and decorates that row with a @c "> " cursor.
 *
 * @c show_footer_hints toggles the footer rule + key-hint line; pass false when
 * the caller draws its own footer (e.g. legacy demo).
 */
void render_device_list(std::ostream& out, const RenderConfig& cfg,
                        const std::vector<Device>& devices, std::size_t selected_index,
                        bool show_footer_hints);

/**
 * Bucket devices by kind in stable order (CPU > GPU > NIC > other). Within
 * each bucket the relative order from @c devices is preserved. Returns
 * indices into @c devices.
 */
[[nodiscard]] std::vector<std::size_t>
kind_sorted_device_indices(const std::vector<Device>& devices);
void render_device_detail(std::ostream& out, const RenderConfig& cfg, const Device& device,
                          const SessionState* session_or_null,
                          std::span<const std::uint32_t> supported_ids);
void render_metric_detail(std::ostream& out, const RenderConfig& cfg,
                          const profcat::MetricDescriptor& metric, bool aspirational,
                          bool supported);
void render_preset_preview(std::ostream& out, const RenderConfig& cfg, const SessionState& session,
                           const std::vector<Device>& devices);
void render_confirm(std::ostream& out, const RenderConfig& cfg, std::string_view question);
void render_invalid_input(std::ostream& out, const RenderConfig& cfg, std::string_view hint);

// --- Interactive-mode header renderers ---
//
// These print the static chrome (title + body) of each screen WITHOUT
// the trailing choice list. The choice list is supplied separately by
// MenuController so arrow-key navigation can highlight a row.
//
// The legacy `render_*` functions above remain unchanged so the demo
// walker still emits the original numbered-list layout bit-for-bit.

void render_welcome_header(std::ostream& out, const RenderConfig& cfg);
void render_main_menu_header(std::ostream& out, const RenderConfig& cfg);
void render_mode_select_header(std::ostream& out, const RenderConfig& cfg);
void render_create_session_header(std::ostream& out, const RenderConfig& cfg, CatalogMode mode,
                                  std::string_view catalog_label, const SessionState& session);
/**
 * Render the device-list screen for interactive mode.
 *
 * Emits title, top rule, section-grouped device rows with the cursor on
 * @c selected_index, bottom rule, and the footer hint line. The
 * MenuController in interactive mode supplies @c selected_index; the menu's
 * own @c render() is NOT called for this screen.
 */
void render_device_list_screen(std::ostream& out, const RenderConfig& cfg,
                               const std::vector<Device>& devices, std::size_t selected_index);
void render_device_list_header(std::ostream& out, const RenderConfig& cfg);
/** Footer rule + key-hint line shared by every chrome-style screen. */
void render_footer_hints(std::ostream& out, const RenderConfig& cfg, std::string_view hints);
void render_device_detail_header(std::ostream& out, const RenderConfig& cfg, const Device& device,
                                 const SessionState* session_or_null,
                                 std::span<const std::uint32_t> supported_ids);
void render_preset_preview_header(std::ostream& out, const RenderConfig& cfg,
                                  const SessionState& session);

/** Strip ANSI CSI sequences from input. Used by --no-color regression test. */
[[nodiscard]] std::string strip_ansi(std::string_view input);

/**
 * Return the metric indices (positions in @p metrics) ordered first by
 * Stage (Cpu, Memory, Interconnect, Submission, Gpu, DeviceTelemetry,
 * ApuFabric) then alphabetically by name. Used by both the menu builder
 * and the renderer so visible row N matches menu item N.
 */
[[nodiscard]] std::vector<std::size_t>
stage_sorted_indices(std::span<const profcat::MetricDescriptor> metrics);

}  // namespace avail
