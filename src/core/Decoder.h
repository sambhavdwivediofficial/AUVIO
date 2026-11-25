#pragma once

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace auvio {

class Decoder {
public:
    Decoder();
    ~Decoder();

    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    bool open(const AVCodecParameters* params);
    void close();

    bool sendPacket(const AVPacket* packet);
    AVFrame* receiveFrame(); // caller takes ownership, free with av_frame_free

    void flush();

    AVCodecContext* context() const { return m_context.get(); }

private:
    struct CodecContextDeleter {
        void operator()(AVCodecContext* ctx) const {
            if (ctx) avcodec_free_context(&ctx);
        }
    };
    struct FrameDeleter {
        void operator()(AVFrame* frame) const {
            if (frame) av_frame_free(&frame);
        }
    };

    std::unique_ptr<AVCodecContext, CodecContextDeleter> m_context;
    std::unique_ptr<AVFrame, FrameDeleter> m_frame;
};

}
