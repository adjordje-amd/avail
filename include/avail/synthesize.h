#pragma once

#include "avail/device.h"
#include "avail/session.h"

#include <string>
#include <vector>

namespace avail {

/**
 * Build a multi-line `rocprof-sys-run` command from the session selections.
 *
 * Aspirational metrics carry a `# [demo: proposed UX]` trailing comment.
 * Empty selections produce a single explanatory comment line.
 *
 * @param session    Preset accumulator.
 * @param devices    Full device catalog for the active mode.
 * @return Multi-line command string (no trailing newline).
 */
[[nodiscard]] std::string synthesize_command(const SessionState& session,
                                             const std::vector<Device>& devices);

}  // namespace avail
