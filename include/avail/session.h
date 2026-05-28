#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <profcat/descriptor.hpp>
#include <set>
#include <span>
#include <string>
#include <string_view>

namespace avail {

/**
 * Mutable preset-build state, one instance per Create-config entry.
 *
 * Cleared on Back-to-Main from the Create-config session.
 */
class SessionState {
  public:
    void toggle_metric(std::string_view slot, std::string_view name);
    void toggle_configure(std::string_view slot, std::string_view name);

    /**
     * Seed the starter selection for @p slot from the catalog defaults.
     *
     * For each descriptor in @p metrics whose id appears in @p supported AND
     * whose default_enabled flag is true, the metric/configure name is added
     * to the session if not already present. Manual selections are preserved.
     *
     * Idempotent: subsequent calls for the same slot are no-ops, so revisiting
     * a device-detail screen does not re-seed metrics the user toggled off.
     */
    void seed_defaults(std::string_view slot, std::span<const profcat::MetricDescriptor> measure,
                       std::span<const profcat::MetricDescriptor> configure,
                       std::span<const std::uint32_t> supported);

    [[nodiscard]] bool is_seeded(std::string_view slot) const;

    [[nodiscard]] bool has_metric(std::string_view slot, std::string_view name) const;
    [[nodiscard]] bool has_configure(std::string_view slot, std::string_view name) const;

    void clear_device(std::string_view slot);
    void clear_all() noexcept;

    [[nodiscard]] std::size_t metric_count() const noexcept;
    [[nodiscard]] std::size_t configure_count() const noexcept;
    [[nodiscard]] std::size_t device_count() const noexcept;
    [[nodiscard]] bool is_empty() const noexcept;

    [[nodiscard]] std::string summary() const;

    [[nodiscard]] const std::map<std::string, std::set<std::string>>&
    selected_metrics() const noexcept {
        return m_metrics;
    }
    [[nodiscard]] const std::map<std::string, std::set<std::string>>&
    selected_configures() const noexcept {
        return m_configures;
    }

  private:
    std::map<std::string, std::set<std::string>> m_metrics;
    std::map<std::string, std::set<std::string>> m_configures;
    std::set<std::string> m_seeded_slots;
};

}  // namespace avail
