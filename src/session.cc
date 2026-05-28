#include "avail/session.h"

#include <algorithm>
#include <numeric>
#include <set>
#include <sstream>

namespace avail {

namespace {

bool toggle_in(std::map<std::string, std::set<std::string>>& store, std::string_view slot,
               std::string_view name) {
    auto& bucket = store[std::string(slot)];
    const std::string key(name);
    auto [it, inserted] = bucket.insert(key);
    if (!inserted) {
        bucket.erase(it);
        return false;
    }
    return true;
}

bool contains_in(const std::map<std::string, std::set<std::string>>& store, std::string_view slot,
                 std::string_view name) {
    const auto it = store.find(std::string(slot));
    if (it == store.end()) {
        return false;
    }
    return it->second.find(std::string(name)) != it->second.end();
}

}  // namespace

void SessionState::toggle_metric(std::string_view slot, std::string_view name) {
    toggle_in(m_metrics, slot, name);
}

void SessionState::toggle_configure(std::string_view slot, std::string_view name) {
    toggle_in(m_configures, slot, name);
}

bool SessionState::has_metric(std::string_view slot, std::string_view name) const {
    return contains_in(m_metrics, slot, name);
}

bool SessionState::has_configure(std::string_view slot, std::string_view name) const {
    return contains_in(m_configures, slot, name);
}

void SessionState::clear_device(std::string_view slot) {
    const std::string key(slot);
    m_metrics.erase(key);
    m_configures.erase(key);
}

void SessionState::clear_all() noexcept {
    m_metrics.clear();
    m_configures.clear();
}

std::size_t SessionState::metric_count() const noexcept {
    return std::accumulate(
        m_metrics.begin(), m_metrics.end(), static_cast<std::size_t>(0),
        [](std::size_t acc, const auto& entry) { return acc + entry.second.size(); });
}

std::size_t SessionState::configure_count() const noexcept {
    return std::accumulate(
        m_configures.begin(), m_configures.end(), static_cast<std::size_t>(0),
        [](std::size_t acc, const auto& entry) { return acc + entry.second.size(); });
}

std::size_t SessionState::device_count() const noexcept {
    std::set<std::string> slots;
    for (const auto& [slot, names] : m_metrics) {
        if (!names.empty()) {
            slots.insert(slot);
        }
    }
    for (const auto& [slot, names] : m_configures) {
        if (!names.empty()) {
            slots.insert(slot);
        }
    }
    return slots.size();
}

bool SessionState::is_empty() const noexcept {
    return metric_count() == 0 && configure_count() == 0;
}

std::string SessionState::summary() const {
    std::ostringstream os;
    const auto metrics = metric_count();
    const auto configures = configure_count();
    const auto devices = device_count();
    os << metrics << " metrics + " << configures << " configures across " << devices
       << (devices == 1 ? " device" : " devices");
    return os.str();
}

}  // namespace avail
