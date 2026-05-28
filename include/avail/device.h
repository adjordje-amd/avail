#pragma once

#include <optional>
#include <string>
#include <vector>

namespace avail {

/**
 * A single measurable quantity or configurable knob.
 *
 * Same shape powers Measure-tab rows and Configure rows.
 */
struct Metric {
    std::string name;
    std::string group;
    std::string purpose;
    std::string enable;
    bool aspirational = false;
    std::optional<std::string> status;
    std::optional<std::string> unit;
    std::optional<std::string> caveat;
};

/**
 * A detected (mocked) piece of hardware.
 */
struct Device {
    std::string slot;
    std::string kind;
    std::string identity;
    bool supported = true;
    std::optional<std::string> unsupported_reason;
    std::vector<Metric> measure;
    std::vector<Metric> configure;
};

}  // namespace avail
