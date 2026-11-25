#include "VideoRenderer.h"
#include "Logger.h"

namespace auvio {

VideoRenderer::VideoRenderer() = default;
VideoRenderer::~VideoRenderer() = default;

void VideoRenderer::setFrameReadyCallback(FrameReadyCallback callback) {
    m_callback = std::move(callback);
}

void VideoRenderer::renderFrame(const AVFrame* frame) {
    if (!frame || !m_callback) return;

    const auto srcFormat = static_cast<AVPixelFormat>(frame->format);
    const bool needsNewContext = !m_swsContext
        || frame->width != m_swsSrcWidth
        || frame->height != m_swsSrcHeight
        || srcFormat != m_swsSrcFormat;

    if (needsNewContext) {
        SwsContext* raw = sws_getContext(
            frame->width, frame->height, srcFormat,
            frame->width, frame->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!raw) {
            AUVIO_LOG_ERROR << "Failed to create video scaling context";
            return;
        }

        m_swsContext.reset(raw);
        m_swsSrcWidth = frame->width;
        m_swsSrcHeight = frame->height;
        m_swsSrcFormat = srcFormat;
        m_rgbaBuffer.assign(static_cast<std::size_t>(frame->width) * frame->height * 4, 0);
    }

    uint8_t* dstData[4] = { m_rgbaBuffer.data(), nullptr, nullptr, nullptr };
    const int dstStride[4] = { frame->width * 4, 0, 0, 0 };

    sws_scale(
        m_swsContext.get(),
        frame->data, frame->linesize,
        0, frame->height,
        dstData, dstStride);

    m_callback(frame->width, frame->height, m_rgbaBuffer.data(), dstStride[0]);
}

}
