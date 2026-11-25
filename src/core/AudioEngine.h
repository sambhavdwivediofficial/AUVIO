#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <SDL.h>
}

namespace auvio {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool open(int sampleRate, int channels);
    void close();

    void queueFrame(const AVFrame* frame);

    void play();
    void pause();
    bool isPaused() const;

    void setVolume(float volume);
    float volume() const;

    double playedSeconds() const;
    void reset();

private:
    static void audioCallback(void* userData, Uint8* stream, int len);
    void fillBuffer(uint8_t* stream, int len);

    SDL_AudioDeviceID m_deviceId = 0;
    int m_outputSampleRate = 0;
    int m_outputChannels = 0;

    struct SwrContextDeleter {
        void operator()(SwrContext* ctx) const {
            if (ctx) swr_free(&ctx);
        }
    };
    std::unique_ptr<SwrContext, SwrContextDeleter> m_swr;
    int m_swrInputSampleRate = 0;
    int m_swrInputChannels = 0;
    AVSampleFormat m_swrInputFormat = AV_SAMPLE_FMT_NONE;

    mutable std::mutex m_bufferMutex;
    std::condition_variable m_bufferNotFull;
    std::vector<uint8_t> m_ringBuffer;
    std::size_t m_readPos = 0;
    std::size_t m_writePos = 0;
    std::size_t m_bufferedBytes = 0;

    std::atomic<float> m_volume{1.0f};
    std::atomic<uint64_t> m_bytesPlayed{0};
};

}
