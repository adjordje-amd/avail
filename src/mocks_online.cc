#include "avail/mocks_online.h"

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

// ---------------------------------------------------------------------------
// Per-slot metric id selections (mapped to profcat catalog ids).
//
// "measure" = sample/counter/gauge metrics surfaced as -E options.
// "configure" = trace-style metrics surfaced as --use-rocm / --rocm-domains.
// "aspirational" = subset of either list that is declared in profcat but
//   not yet implemented for this device class today.
// ---------------------------------------------------------------------------

constexpr std::uint32_t kCpu_SampleCpuTime = 0x4001;
constexpr std::uint32_t kCpu_SampleCallStack = 0x4003;
constexpr std::uint32_t kCpu_PmuCycles = 0x4004;
constexpr std::uint32_t kCpu_PmuInstructions = 0x4005;
constexpr std::uint32_t kCpu_PmuCacheMisses = 0x4006;
constexpr std::uint32_t kCpu_PmuBranchMisses = 0x4007;
constexpr std::uint32_t kCpu_SchedSwitch = 0x4008;
constexpr std::uint32_t kCpu_DerivedIpc = 0x6004;

constexpr std::uint32_t kProc_MemRss = 0x5001;
constexpr std::uint32_t kProc_CpuUtil = 0x5003;
constexpr std::uint32_t kProc_ThreadCount = 0x5004;

constexpr std::uint32_t kGpu_SqWaves = 0x2001;
constexpr std::uint32_t kGpu_SqInstsValu = 0x2002;
constexpr std::uint32_t kGpu_SqInstsSalu = 0x2003;
constexpr std::uint32_t kGpu_SqBusyCycles = 0x2005;
constexpr std::uint32_t kGpu_GrbmGuiActive = 0x2006;
constexpr std::uint32_t kGpu_TccHit = 0x2008;
constexpr std::uint32_t kGpu_TccMiss = 0x2009;
constexpr std::uint32_t kGpu_SdmaBusy = 0x200B;
constexpr std::uint32_t kGpu_SdmaIdle = 0x200C;

constexpr std::uint32_t kGpu_DerivedValuUtil = 0x6001;
constexpr std::uint32_t kGpu_DerivedL2Hit = 0x6002;
constexpr std::uint32_t kGpu_DerivedSqUtil = 0x6003;

constexpr std::uint32_t kGpu_SmiPower = 0x3001;
constexpr std::uint32_t kGpu_SmiTempEdge = 0x3002;
constexpr std::uint32_t kGpu_SmiGfxUtil = 0x3005;
constexpr std::uint32_t kGpu_SmiVramUsed = 0x3008;
constexpr std::uint32_t kGpu_SmiSclk = 0x300D;

constexpr std::uint32_t kRocm_HipRuntime = 0x1001;
constexpr std::uint32_t kRocm_Marker = 0x1007;
constexpr std::uint32_t kRocm_KernelDispatch = 0x1101;
constexpr std::uint32_t kRocm_CopyH2d = 0x1201;
constexpr std::uint32_t kRocm_CopyD2h = 0x1202;
constexpr std::uint32_t kRocm_CopyD2d = 0x1203;

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
        kCpu_DerivedIpc,      kCpu_PmuCycles,     kCpu_PmuInstructions, kCpu_PmuCacheMisses,
        kCpu_PmuBranchMisses, kCpu_SampleCpuTime, kCpu_SampleCallStack, kProc_MemRss,
        kProc_CpuUtil,        kProc_ThreadCount,
    };
    static constexpr std::uint32_t configure_ids[] = {
        kCpu_SchedSwitch,
    };
    static constexpr std::uint32_t aspirational[] = {
        kCpu_SchedSwitch,
    };
    Device d;
    d.slot = "cpu0";
    d.kind = "CPU";
    d.identity = "AMD Ryzen 9 9950X3D (16C / 32T, Zen5)";
    d.display_name = "AMD Ryzen 9 9950X3D";
    d.spec1 = "16C / 32T";
    d.spec2 = "Zen5";
    d.supported = true;
    d.measure = filter(measure_ids);
    d.configure = filter(configure_ids);
    d.aspirational_metric_ids.assign(std::begin(aspirational), std::end(aspirational));
    return d;
}

Device build_gpu0() {
    static constexpr std::uint32_t measure_ids[] = {
        kGpu_SqWaves,       kGpu_SqInstsValu,     kGpu_SqInstsSalu,  kGpu_SqBusyCycles,
        kGpu_GrbmGuiActive, kGpu_TccHit,          kGpu_TccMiss,      kGpu_SdmaBusy,
        kGpu_SdmaIdle,      kGpu_DerivedValuUtil, kGpu_DerivedL2Hit, kGpu_DerivedSqUtil,
        kGpu_SmiPower,      kGpu_SmiTempEdge,     kGpu_SmiGfxUtil,   kGpu_SmiVramUsed,
        kGpu_SmiSclk,
    };
    static constexpr std::uint32_t configure_ids[] = {
        kRocm_KernelDispatch, kRocm_CopyH2d,    kRocm_CopyD2h,
        kRocm_CopyD2d,        kRocm_HipRuntime, kRocm_Marker,
    };
    static constexpr std::uint32_t aspirational[] = {
        kGpu_DerivedValuUtil, kGpu_DerivedL2Hit, kGpu_DerivedSqUtil, kGpu_SdmaBusy, kGpu_SdmaIdle,
    };
    Device d;
    d.slot = "gpu0";
    d.kind = "GPU";
    d.identity = "Radeon RX 9070 XT, gfx1201 (RDNA4, 16 GB)";
    d.display_name = "Radeon RX 9070 XT";
    d.spec1 = "gfx1201";
    d.spec2 = "16 GB";
    d.supported = true;
    d.measure = filter(measure_ids);
    d.configure = filter(configure_ids);
    d.aspirational_metric_ids.assign(std::begin(aspirational), std::end(aspirational));
    return d;
}

Device build_gpu1() {
    Device d;
    d.slot = "gpu1";
    d.kind = "GPU";
    d.identity = "Integrated display, gfx1036";
    d.display_name = "Integrated display";
    d.spec1 = "gfx1036";
    d.spec2 = "unsupported";
    d.supported = false;
    d.unsupported_reason =
        "gfx1036 is an integrated/display GPU. No profiling counters exposed by current ROCm.";
    return d;
}

Device build_nic0() {
    static constexpr std::uint32_t measure_ids[] = {kNic_RxBytes, kNic_TxBytes};
    static constexpr std::uint32_t aspirational[] = {kNic_RxBytes, kNic_TxBytes};
    Device d;
    d.slot = "nic0";
    d.kind = "NIC";
    d.identity = "AMD Pensando DSC 200G";
    d.display_name = "AMD Pensando DSC 200G";
    d.spec1 = "200 Gb/s";
    d.supported = true;
    d.measure = filter(measure_ids);
    d.aspirational_metric_ids.assign(std::begin(aspirational), std::end(aspirational));
    return d;
}

const std::vector<Device>& cached_devices() {
    static const std::vector<Device> kDevices = [] {
        std::vector<Device> v;
        v.reserve(4);
        v.push_back(build_cpu0());
        v.push_back(build_gpu0());
        v.push_back(build_gpu1());
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

std::string MockOnlineDeviceCatalog::label() const {
    return "this machine";
}

std::vector<Device> MockOnlineDeviceCatalog::devices() const {
    return cached_devices();
}

std::span<const profcat::MetricDescriptor>
MockOnlineMetricCatalog::measure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? std::span<const profcat::MetricDescriptor>{d->measure}
             : std::span<const profcat::MetricDescriptor>{};
}

std::span<const profcat::MetricDescriptor>
MockOnlineMetricCatalog::configure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? std::span<const profcat::MetricDescriptor>{d->configure}
             : std::span<const profcat::MetricDescriptor>{};
}

std::span<const std::uint32_t>
MockOnlineMetricCatalog::aspirational_ids(std::string_view slot) const {
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

std::span<const std::uint32_t> MockOnlineMetricCatalog::supported_ids(std::string_view slot) const {
    return supported_ids_for(slot);
}

std::string MockOnlineRocmVersion::version() const {
    return "6.4.0 (mock)";
}

std::vector<std::string> MockOnlineLibraryEnv::loaded_libraries() const {
    return {"librocprofiler-sdk.so.1", "libhsa-runtime64.so.1", "libamdhip64.so.6"};
}

}  // namespace avail
