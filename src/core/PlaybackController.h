#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "Demuxer.h"
#include "Decoder.h"
#include "AudioEngine.h"
#include "VideoRenderer.h"

extern "C" {
#include <libavcodec/packet.h>
}

namespace auvio {

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

class PlaybackController {
public:
    PlaybackController();
    ~PlaybackController();

    PlaybackController(const PlaybackController&) = delete;
    PlaybackController& operator=(const PlaybackController&) = delete;

    bool open(const std::string& path);
    void play();
    void pause();
    void stop();
    void seek(double seconds);

    void setVolume(float volume);
    void setVideoFrameCallback(VideoRenderer::FrameReadyCallback callback);

    PlaybackState state() const;
    double positionSeconds() const;
    double durationSeconds() const;

private:
    struct PacketQueue {
        std::mutex mutex;
        std::condition_variable notEmpty;
        std::condition_variable notFull;
        std::queue<AVPacket*> packets;
        static constexpr std::size_t kMaxSize = 256; // bounds memory, paces the demuxer to playback speed

        void push(AVPacket* packet, const std::atomic<bool>& stopFlag);
        AVPacket* pop(const std::atomic<bool>& stopFlag);
        void clear();
    };

    void demuxLoop();
    void audioLoop();
    void videoLoop();

    void closeInternal();

    Demuxer m_demuxer;
    Decoder m_audioDecoder;
    Decoder m_videoDecoder;
    AudioEngine m_audioEngine;
    VideoRenderer m_videoRenderer;

    PacketQueue m_audioQueue;
    PacketQueue m_videoQueue;

    std::thread m_demuxThread;
    std::thread m_audioThread;
    std::thread m_videoThread;

    std::atomic<bool> m_stopRequested{false};
    std::atomic<PlaybackState> m_state{PlaybackState::Stopped};

    mutable std::mutex m_seekMutex;
    std::atomic<bool> m_seekRequested{false};
    double m_seekTargetSeconds = 0.0;

    std::atomic<double> m_videoClockSeconds{0.0};
    bool m_hasAudio = false;
    bool m_hasVideo = false;
};

}
