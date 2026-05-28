#include "avail/mocks_online.h"

#include "avail/device.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace avail {

namespace {

Metric make_metric(std::string name, std::string group, std::string purpose, std::string enable,
                   bool aspirational = false, std::optional<std::string> status = std::nullopt,
                   std::optional<std::string> unit = std::nullopt,
                   std::optional<std::string> caveat = std::nullopt) {
    return Metric{std::move(name), std::move(group),  std::move(purpose), std::move(enable),
                  aspirational,    std::move(status), std::move(unit),    std::move(caveat)};
}

std::vector<Device> build_devices() {
    std::vector<Device> out;
    out.reserve(4);

    Device cpu;
    cpu.slot = "cpu0";
    cpu.kind = "CPU";
    cpu.identity = "AMD Ryzen 9 9950X3D (16C / 32T, Zen5)";
    cpu.supported = true;
    cpu.measure = {
        make_metric("IPC", "Derived metrics", "Instructions retired per cycle",
                    "rocprof-sys-run --sample cputime --sampling-freq 100 -- ./app", false,
                    std::nullopt, "ratio"),
        make_metric("L3 miss rate %", "Derived metrics", "Fraction of L3 accesses that miss",
                    "rocprof-sys-run --sample cputime --sampling-freq 100 -- ./app", false,
                    std::nullopt, "%", "Requires hardware PMU access; some servers need root"),
        make_metric("Branch miss rate %", "Derived metrics", "Fraction of branches mispredicted",
                    "rocprof-sys-run --sample cputime --sampling-freq 100 -- ./app", false,
                    std::nullopt, "%"),
        make_metric(
            "Frontend stalls %", "Derived metrics", "Cycles stalled waiting on the frontend",
            "rocprof-sys-run --sample cputime --sampling-freq 1000 -- ./app", false,
            std::string("high overhead"), "%", "Higher sampling frequency increases probe effect"),
        make_metric("CPU freq (MHz)", "Derived metrics", "Per-core operating frequency over time",
                    "rocprof-sys-run --host -- ./app", false, std::nullopt, "MHz"),
        make_metric("Package thermal (C)", "Derived metrics", "CPU package temperature over time",
                    "rocprof-sys-run --host -- ./app", false, std::nullopt, "C"),
    };
    cpu.configure = {
        make_metric("Sampling mode", "Sampling", "Wall-clock vs CPU-time sample trigger",
                    "rocprof-sys-run --sample cputime -- ./app", false, std::nullopt,
                    "cputime|realtime"),
        make_metric("Sampling freq (Hz)", "Sampling", "Samples per second per thread",
                    "rocprof-sys-run --sample cputime --sampling-freq 100 -- ./app", false,
                    std::nullopt, "Hz", "Frequencies above 1000 Hz add measurable overhead"),
        make_metric("Sampling wait (sec)", "Sampling", "Delay before the first sample fires",
                    "rocprof-sys-run --sample cputime --sampling-wait 2 -- ./app", false,
                    std::nullopt, "sec"),
    };
    out.push_back(std::move(cpu));

    Device gpu0;
    gpu0.slot = "gpu0";
    gpu0.kind = "GPU";
    gpu0.identity = "Radeon RX 9070 XT, gfx1201 (RDNA4, 16 GB)";
    gpu0.supported = true;
    gpu0.measure = {
        make_metric("Wave Occupancy %", "Derived metrics", "Active waves vs hardware max per CU",
                    "rocprof-sys-run --metric wave_occupancy -- ./app", true,
                    std::string("experimental"), "%",
                    "Synthesized layer not yet implemented; today expose SQ_WAVES raw"),
        make_metric("VALU Utilization %", "Derived metrics",
                    "Vector ALU busy cycles vs active cycles",
                    "rocprof-sys-run --metric valu_utilization -- ./app", true,
                    std::string("experimental"), "%"),
        make_metric("L2 Cache Hit %", "Derived metrics", "GPU L2 hit rate across all CUs",
                    "rocprof-sys-run --metric l2_hit_rate -- ./app", true,
                    std::string("experimental"), "%"),
        make_metric("HBM Bandwidth (GB/s)", "Derived metrics", "Sustained device-memory throughput",
                    "rocprof-sys-run --metric hbm_bandwidth -- ./app", true,
                    std::string("experimental"), "GB/s"),
        make_metric("Compute Throughput (TFLOP/s)", "Derived metrics",
                    "Achieved FP throughput across all CUs",
                    "rocprof-sys-run --metric compute_throughput -- ./app", true,
                    std::string("experimental"), "TFLOP/s"),
        make_metric("SQ_WAVES", "Hardware counters", "Waves launched by the shader sequencer",
                    "ROCPROFSYS_ROCM_EVENTS=SQ_WAVES rocprof-sys-run -- ./app", false, std::nullopt,
                    "count"),
        make_metric("GRBM_GUI_ACTIVE", "Hardware counters", "Cycles the graphics block is active",
                    "ROCPROFSYS_ROCM_EVENTS=GRBM_GUI_ACTIVE rocprof-sys-run -- ./app", false,
                    std::nullopt, "count"),
        make_metric("SQ_INSTS_VALU", "Hardware counters", "Vector ALU instructions issued",
                    "ROCPROFSYS_ROCM_EVENTS=SQ_INSTS_VALU rocprof-sys-run -- ./app", false,
                    std::nullopt, "count"),
        make_metric("TCC_HIT", "Hardware counters", "L2 cache hits across all channels",
                    "ROCPROFSYS_ROCM_EVENTS=TCC_HIT rocprof-sys-run -- ./app", false, std::nullopt,
                    "count"),
        make_metric("TCC_MISS", "Hardware counters", "L2 cache misses across all channels",
                    "ROCPROFSYS_ROCM_EVENTS=TCC_MISS rocprof-sys-run -- ./app", false, std::nullopt,
                    "count"),
    };
    gpu0.configure = {
        make_metric("Kernel dispatch trace", "Tracing", "Capture per-kernel dispatch start / end",
                    "rocprof-sys-run --use-rocm --rocm-domains kernel_dispatch -- ./app", false,
                    std::nullopt, "bool",
                    "Conflicts with per-kernel PMC collection in single pass"),
        make_metric("Memory copy trace", "Tracing", "Capture HIP / HSA memcpy start / end",
                    "rocprof-sys-run --use-rocm --rocm-domains memory_copy -- ./app", false,
                    std::nullopt, "bool"),
        make_metric("Per-GPU buffer size (KB)", "Tracing", "Per-device trace buffer before flush",
                    "rocprof-sys-run --trace-buffer-size 4096 -- ./app", false, std::nullopt, "KB"),
    };
    out.push_back(std::move(gpu0));

    Device gpu1;
    gpu1.slot = "gpu1";
    gpu1.kind = "GPU";
    gpu1.identity = "Integrated display, gfx1036";
    gpu1.supported = false;
    gpu1.unsupported_reason =
        "gfx1036 is an integrated/display GPU. No profiling counters exposed by current ROCm.";
    out.push_back(std::move(gpu1));

    Device nic;
    nic.slot = "nic0";
    nic.kind = "NIC";
    nic.identity = "AMD Pensando DSC 200G";
    nic.supported = true;
    nic.measure = {
        make_metric("RDMA WQE rate", "Network counters", "Work queue entries posted per second",
                    "rocprof-sys-run --nic rdma_wqe_rate -- ./app", true,
                    std::string("high overhead"), "ops/s",
                    "Per-WQE sampling can saturate the management path"),
        make_metric("Packet RX rate", "Network counters", "Packets received per second",
                    "rocprof-sys-run --nic pkt_rx_rate -- ./app", true, std::string("[demo]"),
                    "pkt/s"),
        make_metric("Packet TX rate", "Network counters", "Packets transmitted per second",
                    "rocprof-sys-run --nic pkt_tx_rate -- ./app", true, std::string("[demo]"),
                    "pkt/s"),
        make_metric("Congestion notifications", "Network counters",
                    "ECN / CNP events from the fabric",
                    "rocprof-sys-run --nic congestion_notif -- ./app", true, std::string("[demo]"),
                    "count"),
    };
    nic.configure = {
        make_metric("RDMA trace toggle", "Tracing", "Record RDMA verbs as trace slices",
                    "rocprof-sys-run --nic-trace rdma -- ./app", true, std::string("[demo]"),
                    "bool"),
        make_metric("Packet trace toggle", "Tracing", "Record per-packet metadata as trace slices",
                    "rocprof-sys-run --nic-trace packet -- ./app", true, std::string("[demo]"),
                    "bool", "Packet-level tracing produces very large traces"),
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

std::string MockOnlineDeviceCatalog::label() const {
    return "this machine";
}

std::vector<Device> MockOnlineDeviceCatalog::devices() const {
    return cached_devices();
}

std::vector<Metric> MockOnlineMetricCatalog::measure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? d->measure : std::vector<Metric>{};
}

std::vector<Metric> MockOnlineMetricCatalog::configure_metrics(std::string_view slot) const {
    const Device* d = find_slot(slot);
    return d ? d->configure : std::vector<Metric>{};
}

std::string MockOnlineRocmVersion::version() const {
    return "6.4.0 (mock)";
}

std::vector<std::string> MockOnlineLibraryEnv::loaded_libraries() const {
    return {"librocprofiler-sdk.so.1", "libhsa-runtime64.so.1", "libamdhip64.so.6"};
}

}  // namespace avail
