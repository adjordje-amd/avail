#include "avail/app.h"
#include "avail/input.h"
#include "avail/mocks_offline.h"
#include "avail/mocks_online.h"
#include "avail/render.h"
#include "avail/version.h"

#include <iostream>
#include <string_view>

namespace {

void print_usage(std::ostream& out) {
    out << "avail - prototype rocprof-sys-avail UX (skeleton)\n"
           "Usage: avail [OPTIONS]\n"
           "\n"
           "Options:\n"
           "  -h, --help      Print this help message and exit\n"
           "  --version       Display program version information and exit\n"
           "  --demo-frames   Print canned walk of every screen and exit\n"
           "  --no-color      Disable ANSI color in output\n";
}

}  // namespace

int main(int argc, char** argv) {
    bool demo_frames = false;
    bool no_color = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            return 0;
        }
        if (arg == "--version") {
            std::cout << "avail " << avail::version_string() << "\n";
            return 0;
        }
        if (arg == "--demo-frames") {
            demo_frames = true;
        } else if (arg == "--no-color") {
            no_color = true;
        } else {
            std::cerr << "avail: unrecognized option '" << arg << "'\n";
            print_usage(std::cerr);
            return 1;
        }
    }

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
