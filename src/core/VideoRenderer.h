#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

namespace auvio {

class VideoRenderer {
public:
    using FrameReadyCallback = std::function<void(int width, int height, const uint8_t* rgba, int stride)>;

    VideoRenderer();
    ~VideoRenderer();

    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;

    void setFrameReadyCallback(FrameReadyCallback callback);
    void renderFrame(const AVFrame* frame);

private:
    struct SwsContextDeleter {
        void operator()(SwsContext* ctx) const {
            if (ctx) sws_freeContext(ctx);
        }
    };

    std::unique_ptr<SwsContext, SwsContextDeleter> m_swsContext;
    int m_swsSrcWidth = 0;
    int m_swsSrcHeight = 0;
    AVPixelFormat m_swsSrcFormat = AV_PIX_FMT_NONE;

    std::vector<uint8_t> m_rgbaBuffer;
    FrameReadyCallback m_callback;
};

}
