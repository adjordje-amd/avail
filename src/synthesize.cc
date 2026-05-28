#include "avail/synthesize.h"

#include "avail/device.h"
#include "avail/session.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string>
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

}  // namespace

std::string synthesize_command(const SessionState& session, const std::vector<Device>& devices) {
    if (session.is_empty()) {
        return "# Preset is empty. Select metrics or configures to populate.";
    }

    std::ostringstream os;
    os << "# Preset built via avail demo - " << session.summary() << '\n';
    os << "rocprof-sys-run \\";

    // Collect involved slots in sorted, deterministic order.
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
            std::vector<const Metric*> real_metrics;
            std::vector<const Metric*> aspirational_metrics;
            for (const auto& m : dev->measure) {
                if (metric_it->second.find(m.name) == metric_it->second.end()) {
                    continue;
                }
                if (m.aspirational) {
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
                if (cfg_it->second.find(m.name) == cfg_it->second.end()) {
                    continue;
                }
                os << '\n' << "    " << m.enable << " \\";
                if (m.aspirational) {
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
