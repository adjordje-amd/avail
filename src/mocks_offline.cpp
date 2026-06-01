#include "avail/mocks_offline.h"

#include "avail/device.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <profcat/catalog.hpp>
#include <profcat/descriptor.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace avail {

namespace {

constexpr std::uint32_t kCpu_SampleCpuTime = 0x4001;
constexpr std::uint32_t kCpu_SampleCallStack = 0x4003;
constexpr std::uint32_t kCpu_PmuCycles = 0x4004;
constexpr std::uint32_t kCpu_PmuInstructions = 0x4005;
constexpr std::uint32_t kCpu_DerivedIpc = 0x6004;

constexpr std::uint32_t kGpu_SqWaves = 0x2001;
constexpr std::uint32_t kGpu_GrbmGuiActive = 0x2006;
constexpr std::uint32_t kGpu_TccHit = 0x2008;
constexpr std::uint32_t kGpu_TccMiss = 0x2009;
constexpr std::uint32_t kGpu_DerivedValuUtil = 0x6001;
constexpr std::uint32_t kGpu_DerivedL2Hit = 0x6002;
constexpr std::uint32_t kGpu_DerivedSqUtil = 0x6003;

constexpr std::uint32_t kRocm_KernelDispatch = 0x1101;
constexpr std::uint32_t kRocm_CopyH2d = 0x1201;
constexpr std::uint32_t kRocm_CopyD2h = 0x1202;

constexpr std::uint32_t kNic_RxBytes = 0x3010;
constexpr std::uint32_t kNic_TxBytes = 0x3011;

const profcat::MetricDescriptor* lookup(std::uint32_t id) noexcept {
    for (const auto& d : profcat::catalog) {
        if (d.id == id) {
            return &d;
        }
    }
    return nullptr;
}

std::vector<profcat::MetricDescriptor> filter(std::span<const std::uint32_t> ids) {
    std::vector<profcat::MetricDescriptor> out;
    out.reserve(ids.size());
    for (std::uint32_t id : ids) {
        if (const auto* d = lookup(id)) {
            out.push_back(*d);
        }
    }
    return out;
}

Device build_cpu0() {
    static constexpr std::uint32_t measure_ids[] = {
        kCpu_DerivedIpc,    kCpu_PmuCycles,       kCpu_PmuInstructions,
        kCpu_SampleCpuTime, kCpu_SampleCallStack,
    };
    Device d;
    d.slot = "cpu0";
    d.kind = "CPU";
    d.identity = "Generic CPU class (no specific model)";
    d.display_name = "Generic CPU class";
    d.spec1 = "generic";
    d.spec2 = "aspirational";
    d.supported = true;
    d.measure = filter(measure_ids);
    d.aspirational_metric_ids.assign(std::begin(measure_ids), std::end(measure_ids));
    return d;
}

Device build_gpu0() {
    static constexpr std::uint32_t measure_ids[] = {
        kGpu_SqWaves,         kGpu_GrbmGuiActive, kGpu_TccHit,        kGpu_TccMiss,
        kGpu_DerivedValuUtil, kGpu_DerivedL2Hit,  kGpu_DerivedSqUtil,
    };
    static constexpr std::uint32_t configure_ids[] = {
        kRocm_KernelDispatch,
        kRocm_CopyH2d,
        kRocm_CopyD2h,
    };
    Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "Generic GPU class (no specific model)";
    d.display_name = "Generic GPU class";
    d.spec1 = "generic";
    d.spec2 = "aspirational";
    d.supported = true;
    d.measure = filter(measure_ids);
    d.configure = filter(configure_ids);
    d.aspirational_metric_ids.reserve(std::size(measure_ids) + std::size(configure_ids));
    d.aspirational_metric_ids.insert(d.aspirational_metric_ids.end(), std::begin(measure_ids),
                                     std::end(measure_ids));
    d.aspirational_metric_ids.insert(d.aspirational_metric_ids.end(), std::begin(configure_ids),
                                     std::end(configure_ids));
    return d;
}

Device build_nic0() {
    static constexpr std::uint32_t measure_ids[] = {kNic_RxBytes, kNic_TxBytes};
    Device d;
    d.slot = "nic0";
    d.kind = "AINIC";
    d.identity = "Generic AI NIC class (no specific model)";
    d.display_name = "Generic AI NIC class";
    d.spec1 = "generic";
    d.spec2 = "aspirational";
    d.supported = true;
    d.measure = filter(measure_ids);
    d.aspirational_metric_ids.assign(std::begin(measure_ids), std::end(measure_ids));
    return d;
}

const std::vector<Device>& cached_devices() {
    static const std::vector<Device> kDevices = [] {
        std::vector<Device> v;
        v.reserve(3);
        v.push_back(build_cpu0());
        v.push_back(build_gpu0());
        v.push_back(build_nic0());
        return v;
    }();
    return kDevices;
}

const Device* find_slot(std::string_view slot) {
    const auto& devs = cached_devices();
    const auto it =
        std::find_if(devs.begin(), devs.end(), [&](const Device& d) { return d.slot == slot; });
    return it == devs.end() ? nullptr : &(*it);
}

}  // namespace

std::string MockOfflineDeviceCatalog::label() const {
    return "generic offline";
}

std::vector<Device> MockOfflineDeviceCatalog::devices() const {
    return cached_devices();
}

std::span<const profcat::MetricDescriptor>
MockOfflineMetricCatalog::measure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? std::span<const profcat::MetricDescriptor>{d->measure}
             : std::span<const profcat::MetricDescriptor>{};
}

std::span<const profcat::MetricDescriptor>
MockOfflineMetricCatalog::configure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? std::span<const profcat::MetricDescriptor>{d->configure}
             : std::span<const profcat::MetricDescriptor>{};
}

std::span<const std::uint32_t>
MockOfflineMetricCatalog::aspirational_ids(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? std::span<const std::uint32_t>{d->aspirational_metric_ids}
             : std::span<const std::uint32_t>{};
}

namespace {

const std::vector<std::uint32_t>& supported_ids_for(std::string_view slot) {
    static const std::map<std::string, std::vector<std::uint32_t>> kCache = [] {
        std::map<std::string, std::vector<std::uint32_t>> m;
        for (const auto& d : cached_devices()) {
            std::vector<std::uint32_t> ids;
            ids.reserve(d.measure.size() + d.configure.size());
            for (const auto& md : d.measure) {
                ids.push_back(md.id);
            }
            for (const auto& md : d.configure) {
                ids.push_back(md.id);
            }
            m.emplace(d.slot, std::move(ids));
        }
        return m;
    }();
    static const std::vector<std::uint32_t> kEmpty;
    const auto it = kCache.find(std::string(slot));
    return it == kCache.end() ? kEmpty : it->second;
}

}  // namespace

std::span<const std::uint32_t>
MockOfflineMetricCatalog::supported_ids(std::string_view slot) const {
    return supported_ids_for(slot);
}

}  // namespace avail
