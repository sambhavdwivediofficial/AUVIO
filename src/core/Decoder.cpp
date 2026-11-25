#include "Decoder.h"
#include "Logger.h"

namespace auvio {

Decoder::Decoder() = default;

Decoder::~Decoder() {
    close();
}

bool Decoder::open(const AVCodecParameters* params) {
    close();

    if (!params) {
        AUVIO_LOG_ERROR << "No codec parameters supplied to Decoder::open";
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (!codec) {
        AUVIO_LOG_ERROR << "No decoder found for codec id " << params->codec_id;
        return false;
    }

    AVCodecContext* rawContext = avcodec_alloc_context3(codec);
    if (!rawContext) {
        AUVIO_LOG_ERROR << "Failed to allocate codec context";
        return false;
    }
    m_context.reset(rawContext);

    if (avcodec_parameters_to_context(m_context.get(), params) < 0) {
        AUVIO_LOG_ERROR << "Failed to copy codec parameters into context";
        close();
        return false;
    }

    if (avcodec_open2(m_context.get(), codec, nullptr) < 0) {
        AUVIO_LOG_ERROR << "Failed to open codec";
        close();
        return false;
    }

    m_frame.reset(av_frame_alloc());
    return true;
}

void Decoder::close() {
    m_context.reset();
    m_frame.reset();
}

bool Decoder::sendPacket(const AVPacket* packet) {
    if (!m_context) return false;
    return avcodec_send_packet(m_context.get(), packet) == 0;
}

AVFrame* Decoder::receiveFrame() {
    if (!m_context || !m_frame) return nullptr;

    const int ret = avcodec_receive_frame(m_context.get(), m_frame.get());
    if (ret != 0) {
        return nullptr; // EAGAIN or EOF
    }

    AVFrame* owned = av_frame_alloc();
    av_frame_move_ref(owned, m_frame.get());
    return owned;
}

void Decoder::flush() {
    if (m_context) {
        avcodec_flush_buffers(m_context.get());
    }
}

}
