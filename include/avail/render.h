#pragma once

#include "avail/device.h"
#include "avail/screens.h"
#include "avail/session.h"

#include <ostream>
#include <profcat/descriptor.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

/**
 * Global render configuration shared by every render function.
 */
struct RenderConfig {
    bool color = true;
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
void render_mode_select(std::ostream& out, const RenderConfig& cfg);
void render_create_session(std::ostream& out, const RenderConfig& cfg, CatalogMode mode,
                           std::string_view catalog_label, const SessionState& session);
void render_device_list(std::ostream& out, const RenderConfig& cfg,
                        const std::vector<Device>& devices, bool show_back);
void render_device_detail(std::ostream& out, const RenderConfig& cfg, const Device& device,
                          const SessionState* session_or_null);
void render_metric_detail(std::ostream& out, const RenderConfig& cfg,
                          const profcat::MetricDescriptor& metric, bool aspirational);
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
void render_device_list_header(std::ostream& out, const RenderConfig& cfg);
void render_device_detail_header(std::ostream& out, const RenderConfig& cfg, const Device& device,
                                 const SessionState* session_or_null);
void render_preset_preview_header(std::ostream& out, const RenderConfig& cfg,
                                  const SessionState& session);

/** Strip ANSI CSI sequences from input. Used by --no-color regression test. */
[[nodiscard]] std::string strip_ansi(std::string_view input);

}  // namespace avail
