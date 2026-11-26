#pragma once

#include <string>

extern "C" {
#include <libavutil/hwcontext.h>
}

namespace auvio {

struct HardwareCapabilities {
    bool hardwareDecodeAvailable = false;
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    std::string gpuName;
    unsigned int cpuCoreCount = 0;
};

class HardwareDetector {
public:
    static HardwareCapabilities detect();
};

}
