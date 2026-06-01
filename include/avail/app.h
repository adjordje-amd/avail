#pragma once

#include "avail/input.h"
#include "avail/interfaces.h"
#include "avail/render.h"

#include <istream>
#include <ostream>

namespace avail {

/**
 * True if @p ev should dismiss the MetricDetail screen and return the user
 * to whichever screen launched it. Accepts: Enter, Esc, q/Q, b, i/I.
 *
 * 'i' acts as a toggle: it opens MetricDetail from Device Detail, and the
 * second press here closes it. q/b/Esc keyword routing is unchanged.
 */
[[nodiscard]] bool is_metric_detail_back_key(const KeyEvent& ev) noexcept;

/**
 * Top-level REPL driver. Holds non-owning references to provider interfaces.
 *
 * Lifetimes of provider instances are managed by the caller (typically
 * main.cpp wires concrete mocks on the stack).
 */
class App {
  public:
    App(IDeviceCatalog& online, IDeviceCatalog& offline, IMetricCatalog& online_metrics,
        IMetricCatalog& offline_metrics, IRocmVersion& rocm, ILibraryEnv& libs);

    /**
     * Run the interactive REPL.
     * @return process exit code (0 on clean exit).
     */
    int run_interactive(std::istream& in, std::ostream& out, const RenderConfig& cfg);

    /**
     * Print a canned walk of every screen, non-interactively.
     * @return process exit code (0 on success).
     */
    int run_demo_frames(std::ostream& out, const RenderConfig& cfg);

  private:
    IDeviceCatalog& m_online;
    IDeviceCatalog& m_offline;
    IMetricCatalog& m_online_metrics;
    IMetricCatalog& m_offline_metrics;
    IRocmVersion& m_rocm;
    ILibraryEnv& m_libs;
};

}  // namespace avail
