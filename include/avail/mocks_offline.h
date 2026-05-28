#pragma once

#include "avail/device.h"
#include "avail/interfaces.h"

#include <cstdint>
#include <profcat/descriptor.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

/**
 * Offline device catalog: one generic device per class (cpu / gpu / ainic).
 *
 * Every metric is aspirational (target machine unknown).
 */
class MockOfflineDeviceCatalog final : public IDeviceCatalog {
  public:
    [[nodiscard]] std::string label() const override;
    [[nodiscard]] std::vector<Device> devices() const override;
};

class MockOfflineMetricCatalog final : public IMetricCatalog {
  public:
    [[nodiscard]] std::span<const profcat::MetricDescriptor>
    measure_metrics(std::string_view slot) const override;
    [[nodiscard]] std::span<const profcat::MetricDescriptor>
    configure_metrics(std::string_view slot) const override;
    [[nodiscard]] std::span<const std::uint32_t>
    aspirational_ids(std::string_view slot) const override;
};

}  // namespace avail
