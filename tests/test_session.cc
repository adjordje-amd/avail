#include "avail/session.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <profcat/catalog.hpp>
#include <profcat/descriptor.hpp>
#include <span>
#include <vector>

namespace {

TEST(session_test, default_state_is_empty) {
    avail::SessionState s;
    EXPECT_TRUE(s.is_empty());
    EXPECT_EQ(s.metric_count(), 0U);
    EXPECT_EQ(s.configure_count(), 0U);
    EXPECT_EQ(s.device_count(), 0U);
}

TEST(session_test, toggle_metric_adds_then_removes) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    EXPECT_TRUE(s.has_metric("gpu0", "SQ_WAVES"));
    EXPECT_EQ(s.metric_count(), 1U);
    EXPECT_EQ(s.device_count(), 1U);
    s.toggle_metric("gpu0", "SQ_WAVES");
    EXPECT_FALSE(s.has_metric("gpu0", "SQ_WAVES"));
    EXPECT_EQ(s.metric_count(), 0U);
}

TEST(session_test, toggle_configure_independent_of_metric) {
    avail::SessionState s;
    s.toggle_configure("gpu0", "Memory copy trace");
    EXPECT_TRUE(s.has_configure("gpu0", "Memory copy trace"));
    EXPECT_FALSE(s.has_metric("gpu0", "Memory copy trace"));
    EXPECT_EQ(s.configure_count(), 1U);
    EXPECT_EQ(s.metric_count(), 0U);
}

TEST(session_test, clear_device_drops_selections_for_one_slot_only) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    s.toggle_metric("cpu0", "IPC");
    s.clear_device("gpu0");
    EXPECT_FALSE(s.has_metric("gpu0", "SQ_WAVES"));
    EXPECT_TRUE(s.has_metric("cpu0", "IPC"));
}

TEST(session_test, clear_all_empties_everything) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    s.toggle_configure("nic0", "RDMA trace toggle");
    s.clear_all();
    EXPECT_TRUE(s.is_empty());
}

TEST(session_test, device_count_distinct_across_metric_and_configure) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    s.toggle_configure("gpu0", "Memory copy trace");
    s.toggle_metric("cpu0", "IPC");
    EXPECT_EQ(s.device_count(), 2U);
    EXPECT_EQ(s.metric_count(), 2U);
    EXPECT_EQ(s.configure_count(), 1U);
}

TEST(session_test, summary_uses_singular_for_one_device) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    EXPECT_NE(s.summary().find("1 device"), std::string::npos);
    EXPECT_EQ(s.summary().find("1 devices"), std::string::npos);
}

TEST(session_test, summary_uses_plural_for_multiple_devices) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "SQ_WAVES");
    s.toggle_metric("cpu0", "IPC");
    EXPECT_NE(s.summary().find("2 devices"), std::string::npos);
}

TEST(session_test, repeated_toggle_off_unknown_is_noop) {
    avail::SessionState s;
    s.toggle_metric("gpu0", "DoesNotExist");
    s.toggle_metric("gpu0", "DoesNotExist");
    EXPECT_TRUE(s.is_empty());
}

namespace {

const profcat::MetricDescriptor& lookup_or_die(std::uint32_t id) {
    for (const auto& d : profcat::catalog) {
        if (d.id == id) {
            return d;
        }
    }
    std::abort();
}

}  // namespace

TEST(session_test, seed_defaults_adds_only_default_enabled_and_supported) {
    const auto& sq_waves = lookup_or_die(0x2001);       // default_enabled = true
    const auto& sq_insts_valu = lookup_or_die(0x2002);  // default_enabled = false
    std::vector<profcat::MetricDescriptor> measure = {sq_waves, sq_insts_valu};
    std::vector<profcat::MetricDescriptor> configure;
    std::uint32_t supported_arr[] = {sq_waves.id, sq_insts_valu.id};
    avail::SessionState s;
    s.seed_defaults("gpu0", measure, configure, supported_arr);
    EXPECT_TRUE(s.has_metric("gpu0", sq_waves.name));
    EXPECT_FALSE(s.has_metric("gpu0", sq_insts_valu.name));
}

TEST(session_test, seed_defaults_skips_unsupported_even_when_default_enabled) {
    const auto& sq_waves = lookup_or_die(0x2001);  // default_enabled = true
    std::vector<profcat::MetricDescriptor> measure = {sq_waves};
    std::vector<profcat::MetricDescriptor> configure;
    avail::SessionState s;
    s.seed_defaults("gpu0", measure, configure, std::span<const std::uint32_t>{});
    EXPECT_FALSE(s.has_metric("gpu0", sq_waves.name));
}

TEST(session_test, seed_defaults_idempotent_after_user_toggle_off) {
    const auto& sq_waves = lookup_or_die(0x2001);
    std::vector<profcat::MetricDescriptor> measure = {sq_waves};
    std::vector<profcat::MetricDescriptor> configure;
    std::uint32_t supported_arr[] = {sq_waves.id};
    avail::SessionState s;
    s.seed_defaults("gpu0", measure, configure, supported_arr);
    EXPECT_TRUE(s.has_metric("gpu0", sq_waves.name));
    s.toggle_metric("gpu0", sq_waves.name);  // user turns off
    EXPECT_FALSE(s.has_metric("gpu0", sq_waves.name));
    s.seed_defaults("gpu0", measure, configure, supported_arr);  // revisit
    EXPECT_FALSE(s.has_metric("gpu0", sq_waves.name));           // stays off
}

TEST(session_test, is_seeded_reflects_seed_state) {
    avail::SessionState s;
    EXPECT_FALSE(s.is_seeded("gpu0"));
    std::vector<profcat::MetricDescriptor> empty;
    s.seed_defaults("gpu0", empty, empty, std::span<const std::uint32_t>{});
    EXPECT_TRUE(s.is_seeded("gpu0"));
}

TEST(session_test, clear_device_drops_seeded_flag) {
    avail::SessionState s;
    std::vector<profcat::MetricDescriptor> empty;
    s.seed_defaults("gpu0", empty, empty, std::span<const std::uint32_t>{});
    s.clear_device("gpu0");
    EXPECT_FALSE(s.is_seeded("gpu0"));
}

}  // namespace
