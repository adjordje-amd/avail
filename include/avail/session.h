#pragma once

#include <cstddef>
#include <map>
#include <set>
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
};

}  // namespace avail
