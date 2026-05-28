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
 * Online device catalog: literal-translated MOCK_DEVICES from the Python
 * avali predecessor. Labelled "this machine".
 *
 * No real hardware detection - despite the name, this is mocked content
 * pinned to the audit machine (Ryzen 9 9950X3D + RX 9070 XT + integrated +
 * Pensando DSC 200G).
 */
class MockOnlineDeviceCatalog final : public IDeviceCatalog {
  public:
    [[nodiscard]] std::string label() const override;
    [[nodiscard]] std::vector<Device> devices() const override;
};

class MockOnlineMetricCatalog final : public IMetricCatalog {
  public:
    [[nodiscard]] std::span<const profcat::MetricDescriptor>
    measure_metrics(std::string_view slot) const override;
    [[nodiscard]] std::span<const profcat::MetricDescriptor>
    configure_metrics(std::string_view slot) const override;
    [[nodiscard]] std::span<const std::uint32_t>
    aspirational_ids(std::string_view slot) const override;
};

class MockOnlineRocmVersion final : public IRocmVersion {
  public:
    [[nodiscard]] std::string version() const override;
};

class MockOnlineLibraryEnv final : public ILibraryEnv {
  public:
    [[nodiscard]] std::vector<std::string> loaded_libraries() const override;
};

}  // namespace avail
