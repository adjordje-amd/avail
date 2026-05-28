#include "avail/synthesize.h"

#include "avail/device.h"
#include "avail/session.h"

#include <algorithm>
#include <cstdint>
#include <profcat/descriptor.hpp>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace avail {

namespace {

constexpr const char* kBadge = "  # [demo: proposed UX]";

const Device* find_device(const std::vector<Device>& devices, const std::string& slot) {
    const auto it = std::find_if(devices.begin(), devices.end(),
                                 [&](const Device& d) { return d.slot == slot; });
    return it == devices.end() ? nullptr : &(*it);
}

// Map a profcat configure-style metric to a rocprof-sys-run CLI fragment.
// Recognised families:
//   rocm.api.<x>           -> --rocm-domains <x>_api
//   rocm.copy.<x>          -> --rocm-domains memory_copy
//   rocm.gpu.<x>           -> --rocm-domains <x>
//   anything else          -> --enable <name>
std::string configure_fragment(const profcat::MetricDescriptor& m) {
    const std::string_view name = m.name;
    if (name.starts_with("rocm.copy.")) {
        return "--use-rocm --rocm-domains memory_copy";
    }
    if (name.starts_with("rocm.gpu.")) {
        return std::string{"--use-rocm --rocm-domains "} + std::string{name.substr(9)};
    }
    if (name.starts_with("rocm.api.")) {
        return std::string{"--use-rocm --rocm-domains "} + std::string{name.substr(9)} + "_api";
    }
    return std::string{"--enable "} + std::string{name};
}

}  // namespace

std::string synthesize_command(const SessionState& session, const std::vector<Device>& devices) {
    if (session.is_empty()) {
        return "# Preset is empty. Select metrics or configures to populate.";
    }

    std::ostringstream os;
    os << "# Preset built via avail demo - " << session.summary() << '\n';
    os << "rocprof-sys-run \\";

    std::set<std::string> slots;
    for (const auto& [slot, names] : session.selected_metrics()) {
        if (!names.empty()) {
            slots.insert(slot);
        }
    }
    for (const auto& [slot, names] : session.selected_configures()) {
        if (!names.empty()) {
            slots.insert(slot);
        }
    }

    for (const auto& slot : slots) {
        const Device* dev = find_device(devices, slot);
        if (dev == nullptr) {
            continue;
        }

        const auto metric_it = session.selected_metrics().find(slot);
        if (metric_it != session.selected_metrics().end() && !metric_it->second.empty()) {
            std::vector<const profcat::MetricDescriptor*> real_metrics;
            std::vector<const profcat::MetricDescriptor*> aspirational_metrics;
            for (const auto& m : dev->measure) {
                if (metric_it->second.find(std::string{m.name}) == metric_it->second.end()) {
                    continue;
                }
                if (is_aspirational(*dev, m)) {
                    aspirational_metrics.push_back(&m);
                } else {
                    real_metrics.push_back(&m);
                }
            }
            if (!real_metrics.empty()) {
                os << '\n' << "    -E ";
                for (std::size_t i = 0; i < real_metrics.size(); ++i) {
                    if (i > 0) {
                        os << ',';
                    }
                    os << real_metrics[i]->name;
                }
                os << " \\  # " << slot << " metrics";
            }
            if (!aspirational_metrics.empty()) {
                os << '\n' << "    -E ";
                for (std::size_t i = 0; i < aspirational_metrics.size(); ++i) {
                    if (i > 0) {
                        os << ',';
                    }
                    os << aspirational_metrics[i]->name;
                }
                os << " \\" << kBadge << "  " << slot << " aspirational";
            }
        }

        const auto cfg_it = session.selected_configures().find(slot);
        if (cfg_it != session.selected_configures().end() && !cfg_it->second.empty()) {
            for (const auto& m : dev->configure) {
                if (cfg_it->second.find(std::string{m.name}) == cfg_it->second.end()) {
                    continue;
                }
                os << '\n' << "    " << configure_fragment(m) << " \\";
                if (is_aspirational(*dev, m)) {
                    os << kBadge << "  " << slot << " aspirational";
                } else {
                    os << "  # " << slot << " configure";
                }
            }
        }
    }

    os << '\n' << "    -- ./your_app";
    return os.str();
}

}  // namespace avail
