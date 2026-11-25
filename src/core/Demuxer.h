#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
}

namespace auvio {

class Demuxer {
public:
    Demuxer();
    ~Demuxer();

    Demuxer(const Demuxer&) = delete;
    Demuxer& operator=(const Demuxer&) = delete;

    bool open(const std::string& path);
    void close();

    bool readPacket(AVPacket* packet);
    bool seek(double seconds);

    int videoStreamIndex() const { return m_videoStreamIndex; }
    int audioStreamIndex() const { return m_audioStreamIndex; }

    AVCodecParameters* videoCodecParameters() const;
    AVCodecParameters* audioCodecParameters() const;

    AVRational videoTimeBase() const;
    AVRational audioTimeBase() const;

    double durationSeconds() const;

private:
    struct FormatContextDeleter {
        void operator()(AVFormatContext* ctx) const {
            if (ctx) avformat_close_input(&ctx);
        }
    };

    std::unique_ptr<AVFormatContext, FormatContextDeleter> m_formatContext;
    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
};

}
