#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <profcat/descriptor.hpp>
#include <string>
#include <vector>

namespace avail {

/**
 * A detected (mocked) piece of hardware and the metrics it can expose.
 *
 * Metrics come from the canonical profcat catalog. Aspirational metrics
 * (not yet implemented for this device) are tracked by id in
 * `aspirational_metric_ids`, separate from the descriptor itself, because
 * profcat has no UX-level notion of aspirational.
 */
struct Device {
    std::string slot;
    std::string kind;
    std::string identity;
    bool supported = true;
    std::optional<std::string> unsupported_reason;
    std::vector<profcat::MetricDescriptor> measure;
    std::vector<profcat::MetricDescriptor> configure;
    std::vector<std::uint32_t> aspirational_metric_ids;
};

/** Returns true if @p metric is in @p device's aspirational set. */
[[nodiscard]] inline bool is_aspirational(const Device& device,
                                          const profcat::MetricDescriptor& metric) noexcept {
    const auto& ids = device.aspirational_metric_ids;
    return std::find(ids.begin(), ids.end(), metric.id) != ids.end();
}

}  // namespace avail
