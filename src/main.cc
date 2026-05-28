#include "CLI11.hpp"
#include "avail/app.h"
#include "avail/input.h"
#include "avail/mocks_offline.h"
#include "avail/mocks_online.h"
#include "avail/render.h"
#include "avail/version.h"

#include <iostream>

int main(int argc, char** argv) {
    CLI::App app{"avail - prototype rocprof-sys-avail UX (skeleton)"};
    app.set_version_flag("--version", std::string("avail ") + avail::version_string());

    bool demo_frames = false;
    bool no_color = false;
    app.add_flag("--demo-frames", demo_frames, "Print canned walk of every screen and exit");
    app.add_flag("--no-color", no_color, "Disable ANSI color in output");

    CLI11_PARSE(app, argc, argv);

    avail::RenderConfig cfg;
    cfg.color = !no_color && avail::stdout_is_tty();
    if (demo_frames) {
        cfg.color = !no_color;  // demo mode keeps color when explicitly enabled
    }

    avail::MockOnlineDeviceCatalog online_dev;
    avail::MockOfflineDeviceCatalog offline_dev;
    avail::MockOnlineMetricCatalog online_metrics;
    avail::MockOfflineMetricCatalog offline_metrics;
    avail::MockOnlineRocmVersion rocm;
    avail::MockOnlineLibraryEnv libs;

    avail::App driver(online_dev, offline_dev, online_metrics, offline_metrics, rocm, libs);

    if (demo_frames) {
        return driver.run_demo_frames(std::cout, cfg);
    }
    if (!avail::stdin_is_tty() || !avail::stdout_is_tty()) {
        std::cerr
            << "avail interactive requires a TTY; use --demo-frames for non-interactive walk\n";
        return 2;
    }
    return driver.run_interactive(std::cin, std::cout, cfg);
}
