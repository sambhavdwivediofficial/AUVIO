#include "Demuxer.h"
#include "Logger.h"

namespace auvio {

Demuxer::Demuxer() = default;

Demuxer::~Demuxer() {
    close();
}

bool Demuxer::open(const std::string& path) {
    close();

    AVFormatContext* rawContext = nullptr;
    if (avformat_open_input(&rawContext, path.c_str(), nullptr, nullptr) != 0) {
        AUVIO_LOG_ERROR << "Could not open input: " << path;
        return false;
    }
    m_formatContext.reset(rawContext);

    if (avformat_find_stream_info(m_formatContext.get(), nullptr) < 0) {
        AUVIO_LOG_ERROR << "Could not find stream info for: " << path;
        close();
        return false;
    }

    m_videoStreamIndex = av_find_best_stream(m_formatContext.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    m_audioStreamIndex = av_find_best_stream(m_formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (m_videoStreamIndex < 0 && m_audioStreamIndex < 0) {
        AUVIO_LOG_ERROR << "No playable audio or video stream in: " << path;
        close();
        return false;
    }

    return true;
}

void Demuxer::close() {
    m_formatContext.reset();
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
}

bool Demuxer::readPacket(AVPacket* packet) {
    if (!m_formatContext) return false;
    return av_read_frame(m_formatContext.get(), packet) >= 0;
}

bool Demuxer::seek(double seconds) {
    if (!m_formatContext) return false;
    const int64_t target = static_cast<int64_t>(seconds * AV_TIME_BASE);
    return av_seek_frame(m_formatContext.get(), -1, target, AVSEEK_FLAG_BACKWARD) >= 0;
}

AVCodecParameters* Demuxer::videoCodecParameters() const {
    if (!m_formatContext || m_videoStreamIndex < 0) return nullptr;
    return m_formatContext->streams[m_videoStreamIndex]->codecpar;
}

AVCodecParameters* Demuxer::audioCodecParameters() const {
    if (!m_formatContext || m_audioStreamIndex < 0) return nullptr;
    return m_formatContext->streams[m_audioStreamIndex]->codecpar;
}

AVRational Demuxer::videoTimeBase() const {
    if (!m_formatContext || m_videoStreamIndex < 0) return AVRational{1, 1};
    return m_formatContext->streams[m_videoStreamIndex]->time_base;
}

AVRational Demuxer::audioTimeBase() const {
    if (!m_formatContext || m_audioStreamIndex < 0) return AVRational{1, 1};
    return m_formatContext->streams[m_audioStreamIndex]->time_base;
}

double Demuxer::durationSeconds() const {
    if (!m_formatContext || m_formatContext->duration <= 0) return 0.0;
    return static_cast<double>(m_formatContext->duration) / AV_TIME_BASE;
}

}
