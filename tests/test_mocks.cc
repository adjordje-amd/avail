#include "avail/mocks_offline.h"
#include "avail/mocks_online.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <string>

namespace {

TEST(mocks_online_test, label_is_this_machine) {
    avail::MockOnlineDeviceCatalog cat;
    EXPECT_EQ(cat.label(), "this machine");
}

TEST(mocks_online_test, has_expected_device_slots) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    ASSERT_EQ(devs.size(), 4U);
    EXPECT_EQ(devs[0].slot, "cpu0");
    EXPECT_EQ(devs[1].slot, "gpu0");
    EXPECT_EQ(devs[2].slot, "gpu1");
    EXPECT_EQ(devs[3].slot, "nic0");
}

TEST(mocks_online_test, gpu1_is_unsupported_with_reason) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    EXPECT_FALSE(devs[2].supported);
    ASSERT_TRUE(devs[2].unsupported_reason.has_value());
    EXPECT_NE(devs[2].unsupported_reason->find("integrated"), std::string::npos);
}

TEST(mocks_online_test, gpu0_has_mixed_aspirational_and_real_metrics) {
    avail::MockOnlineDeviceCatalog cat;
    const auto devs = cat.devices();
    const auto& gpu0 = devs[1];
    const bool has_real = std::any_of(gpu0.measure.begin(), gpu0.measure.end(),
                                      [](const avail::Metric& m) { return !m.aspirational; });
    const bool has_asp = std::any_of(gpu0.measure.begin(), gpu0.measure.end(),
                                     [](const avail::Metric& m) { return m.aspirational; });
    EXPECT_TRUE(has_real);
    EXPECT_TRUE(has_asp);
}

TEST(mocks_online_test, metric_catalog_returns_same_data_as_device_catalog) {
    avail::MockOnlineDeviceCatalog dev_cat;
    avail::MockOnlineMetricCatalog metric_cat;
    const auto gpu0 = dev_cat.devices()[1];
    const auto fetched = metric_cat.measure_metrics("gpu0");
    EXPECT_EQ(gpu0.measure.size(), fetched.size());
}

TEST(mocks_online_test, unknown_slot_returns_empty_metric_vector) {
    avail::MockOnlineMetricCatalog cat;
    EXPECT_TRUE(cat.measure_metrics("does_not_exist").empty());
    EXPECT_TRUE(cat.configure_metrics("does_not_exist").empty());
}

TEST(mocks_online_test, rocm_version_returns_non_empty_string) {
    avail::MockOnlineRocmVersion v;
    EXPECT_FALSE(v.version().empty());
}

TEST(mocks_online_test, library_env_lists_rocprofiler_sdk) {
    avail::MockOnlineLibraryEnv env;
    const auto libs = env.loaded_libraries();
    const bool found = std::any_of(libs.begin(), libs.end(), [](const std::string& l) {
        return l.find("rocprofiler-sdk") != std::string::npos;
    });
    EXPECT_TRUE(found);
}

TEST(mocks_offline_test, label_is_generic_offline) {
    avail::MockOfflineDeviceCatalog cat;
    EXPECT_EQ(cat.label(), "generic offline");
}

TEST(mocks_offline_test, has_three_devices_one_per_class) {
    avail::MockOfflineDeviceCatalog cat;
    const auto devs = cat.devices();
    ASSERT_EQ(devs.size(), 3U);
    EXPECT_EQ(devs[0].kind, "CPU");
    EXPECT_EQ(devs[1].kind, "GPU");
    EXPECT_EQ(devs[2].kind, "AINIC");
}

TEST(mocks_offline_test, every_metric_is_aspirational) {
    avail::MockOfflineDeviceCatalog cat;
    const auto devs = cat.devices();
    for (const auto& d : devs) {
        for (const auto& m : d.measure) {
            EXPECT_TRUE(m.aspirational) << "device " << d.slot << " metric " << m.name;
        }
        for (const auto& m : d.configure) {
            EXPECT_TRUE(m.aspirational) << "device " << d.slot << " configure " << m.name;
        }
    }
}

}  // namespace
