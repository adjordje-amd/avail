#pragma once

#include "avail/device.h"

#include <cstdint>
#include <profcat/descriptor.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

/**
 * Provides the set of hardware devices visible to the application.
 */
class IDeviceCatalog {
  public:
    virtual ~IDeviceCatalog() = default;

    /** @return Display label for this catalog (e.g. "this machine"). */
    [[nodiscard]] virtual std::string label() const = 0;

    /** @return All devices, in display order. */
    [[nodiscard]] virtual std::vector<Device> devices() const = 0;
};

/**
 * Provides the metrics offered by a specific device slot.
 *
 * Distinct from IDeviceCatalog so a real impl can lazily query
 * the per-device metric catalog without listing every device's
 * metrics upfront.
 */
class IMetricCatalog {
  public:
    virtual ~IMetricCatalog() = default;

    [[nodiscard]] virtual std::span<const profcat::MetricDescriptor>
    measure_metrics(std::string_view device_slot) const = 0;
    [[nodiscard]] virtual std::span<const profcat::MetricDescriptor>
    configure_metrics(std::string_view device_slot) const = 0;

    /**
     * Subset of metric ids for @p device_slot that are aspirational
     * (declared in profcat, but not yet implemented for this device).
     */
    [[nodiscard]] virtual std::span<const std::uint32_t>
    aspirational_ids(std::string_view device_slot) const = 0;
};

/**
 * Reports the active ROCm runtime version, if any.
 */
class IRocmVersion {
  public:
    virtual ~IRocmVersion() = default;

    /** @return Version string, or "unknown" when no ROCm runtime is detected. */
    [[nodiscard]] virtual std::string version() const = 0;
};

/**
 * Reports profiling-relevant shared libraries loaded into the process.
 */
class ILibraryEnv {
  public:
    virtual ~ILibraryEnv() = default;

    [[nodiscard]] virtual std::vector<std::string> loaded_libraries() const = 0;
};

}  // namespace avail
