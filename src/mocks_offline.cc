#include "avail/mocks_offline.h"

#include "avail/device.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

namespace {

Metric aspirational(std::string name, std::string group, std::string purpose, std::string enable,
                    std::optional<std::string> unit = std::nullopt) {
    Metric m;
    m.name = std::move(name);
    m.group = std::move(group);
    m.purpose = std::move(purpose);
    m.enable = std::move(enable);
    m.aspirational = true;
    m.status = std::string("[demo]");
    m.unit = std::move(unit);
    return m;
}

std::vector<Device> build_devices() {
    std::vector<Device> out;
    out.reserve(3);

    Device cpu;
    cpu.slot = "cpu0";
    cpu.kind = "CPU";
    cpu.identity = "Generic CPU class (no specific model)";
    cpu.supported = true;
    cpu.measure = {
        aspirational("IPC", "Derived metrics", "Instructions retired per cycle",
                     "rocprof-sys-run --sample cputime -- ./app", "ratio"),
        aspirational("Cache miss rate %", "Derived metrics", "Last-level cache miss fraction",
                     "rocprof-sys-run --sample cputime -- ./app", "%"),
        aspirational("Branch miss rate %", "Derived metrics", "Branch mispredict fraction",
                     "rocprof-sys-run --sample cputime -- ./app", "%"),
    };
    cpu.configure = {
        aspirational("Sampling mode", "Sampling", "Wall-clock vs CPU-time trigger",
                     "rocprof-sys-run --sample cputime -- ./app", "cputime|realtime"),
    };
    out.push_back(std::move(cpu));

    Device gpu;
    gpu.slot = "gpu0";
    gpu.kind = "GPU";
    gpu.identity = "Generic GPU class (no specific model)";
    gpu.supported = true;
    gpu.measure = {
        aspirational("Wave Occupancy %", "Derived metrics", "Active waves vs hardware max",
                     "rocprof-sys-run --metric wave_occupancy -- ./app", "%"),
        aspirational("L2 Cache Hit %", "Derived metrics", "GPU L2 hit rate",
                     "rocprof-sys-run --metric l2_hit_rate -- ./app", "%"),
        aspirational("Compute Throughput", "Derived metrics", "Achieved FP throughput",
                     "rocprof-sys-run --metric compute_throughput -- ./app", "TFLOP/s"),
    };
    gpu.configure = {
        aspirational("Kernel dispatch trace", "Tracing", "Capture per-kernel dispatch",
                     "rocprof-sys-run --use-rocm --rocm-domains kernel_dispatch -- ./app", "bool"),
        aspirational("Memory copy trace", "Tracing", "Capture HIP / HSA memcpy",
                     "rocprof-sys-run --use-rocm --rocm-domains memory_copy -- ./app", "bool"),
    };
    out.push_back(std::move(gpu));

    Device nic;
    nic.slot = "nic0";
    nic.kind = "AINIC";
    nic.identity = "Generic AI NIC class (no specific model)";
    nic.supported = true;
    nic.measure = {
        aspirational("Packet RX rate", "Network counters", "Packets received per second",
                     "rocprof-sys-run --nic pkt_rx_rate -- ./app", "pkt/s"),
        aspirational("Packet TX rate", "Network counters", "Packets transmitted per second",
                     "rocprof-sys-run --nic pkt_tx_rate -- ./app", "pkt/s"),
    };
    nic.configure = {
        aspirational("RDMA trace toggle", "Tracing", "Record RDMA verbs as trace slices",
                     "rocprof-sys-run --nic-trace rdma -- ./app", "bool"),
    };
    out.push_back(std::move(nic));

    return out;
}

const std::vector<Device>& cached_devices() {
    static const std::vector<Device> kDevices = build_devices();
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

std::vector<Metric> MockOfflineMetricCatalog::measure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? d->measure : std::vector<Metric>{};
}

std::vector<Metric> MockOfflineMetricCatalog::configure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? d->configure : std::vector<Metric>{};
}

}  // namespace avail
