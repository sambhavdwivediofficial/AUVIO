#include "HardwareDetect_Linux.h"
#include "Logger.h"

#include <thread>
#include <cstdio>
#include <memory>
#include <array>

namespace auvio {

namespace {

std::string detectGpuName() {
    std::array<char, 256> buffer{};
    std::string result = "Unknown GPU";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen("lspci 2>/dev/null | grep -i 'vga\\|3d controller'", "r"), pclose);

    if (pipe && fgets(buffer.data(), buffer.size(), pipe.get())) {
        result = buffer.data();
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }

    return result;
}

bool probeHardwareDevice(AVHWDeviceType type) {
    AVBufferRef* ctx = nullptr;
    const bool ok = av_hwdevice_ctx_create(&ctx, type, nullptr, nullptr, 0) == 0;
    if (ctx) av_buffer_unref(&ctx);
    return ok;
}

}

HardwareCapabilities HardwareDetector::detect() {
    HardwareCapabilities caps;
    caps.cpuCoreCount = std::thread::hardware_concurrency();
    caps.gpuName = detectGpuName();

    if (probeHardwareDevice(AV_HWDEVICE_TYPE_VAAPI)) {
        caps.hardwareDecodeAvailable = true;
        caps.hwDeviceType = AV_HWDEVICE_TYPE_VAAPI;
    } else if (probeHardwareDevice(AV_HWDEVICE_TYPE_VDPAU)) {
        caps.hardwareDecodeAvailable = true;
        caps.hwDeviceType = AV_HWDEVICE_TYPE_VDPAU;
    }

    AUVIO_LOG_INFO << "GPU: " << caps.gpuName << ", hardware decode: "
                   << (caps.hardwareDecodeAvailable ? "available" : "unavailable");

    return caps;
}

}
