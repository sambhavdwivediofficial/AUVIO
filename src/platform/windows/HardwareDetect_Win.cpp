#include "HardwareDetect_Win.h"
#include "Logger.h"

#include <thread>

#include <dxgi.h>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")

namespace auvio {

namespace {

std::string detectGpuName() {
    using Microsoft::WRL::ComPtr;

    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return "Unknown GPU";
    }

    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        return "Unknown GPU";
    }

    DXGI_ADAPTER_DESC1 desc{};
    adapter->GetDesc1(&desc);

    const std::wstring wideName(desc.Description);
    return std::string(wideName.begin(), wideName.end());
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

    if (probeHardwareDevice(AV_HWDEVICE_TYPE_D3D11VA)) {
        caps.hardwareDecodeAvailable = true;
        caps.hwDeviceType = AV_HWDEVICE_TYPE_D3D11VA;
    } else if (probeHardwareDevice(AV_HWDEVICE_TYPE_DXVA2)) {
        caps.hardwareDecodeAvailable = true;
        caps.hwDeviceType = AV_HWDEVICE_TYPE_DXVA2;
    }

    AUVIO_LOG_INFO << "GPU: " << caps.gpuName << ", hardware decode: "
                   << (caps.hardwareDecodeAvailable ? "available" : "unavailable");

    return caps;
}

}
