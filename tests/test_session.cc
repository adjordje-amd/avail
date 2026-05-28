#include "avail/session.h"

#include <gtest/gtest.h>

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

}  // namespace
