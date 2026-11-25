#include "AudioEngine.h"
#include "Logger.h"

#include <algorithm>
#include <cstring>

namespace auvio {

namespace {
constexpr AVSampleFormat kOutputSampleFormat = AV_SAMPLE_FMT_S16;
constexpr std::size_t kRingBufferSeconds = 2;
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    close();
}

bool AudioEngine::open(int sampleRate, int channels) {
    close();

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        AUVIO_LOG_ERROR << "SDL audio init failed: " << SDL_GetError();
        return false;
    }

    SDL_AudioSpec desired{};
    desired.freq = sampleRate;
    desired.format = AUDIO_S16SYS;
    desired.channels = static_cast<Uint8>(channels);
    desired.samples = 1024;
    desired.callback = &AudioEngine::audioCallback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    m_deviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (m_deviceId == 0) {
        AUVIO_LOG_ERROR << "SDL_OpenAudioDevice failed: " << SDL_GetError();
        return false;
    }

    m_outputSampleRate = obtained.freq;
    m_outputChannels = obtained.channels;

    const std::size_t bytesPerSample = sizeof(int16_t) * static_cast<std::size_t>(m_outputChannels);
    m_ringBuffer.assign(bytesPerSample * static_cast<std::size_t>(m_outputSampleRate) * kRingBufferSeconds, 0);
    m_readPos = m_writePos = m_bufferedBytes = 0;
    m_bytesPlayed = 0;

    SDL_PauseAudioDevice(m_deviceId, 0);
    return true;
}

void AudioEngine::close() {
    if (m_deviceId != 0) {
        SDL_CloseAudioDevice(m_deviceId);
        m_deviceId = 0;
    }
    m_swr.reset();
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_ringBuffer.clear();
    m_readPos = m_writePos = m_bufferedBytes = 0;
}

void AudioEngine::queueFrame(const AVFrame* frame) {
    if (!frame || m_deviceId == 0) return;

    const AVSampleFormat inputFormat = static_cast<AVSampleFormat>(frame->format);
    const int inputChannels = frame->ch_layout.nb_channels;
    const int inputSampleRate = frame->sample_rate;

    const bool needsNewSwr = !m_swr
        || inputFormat != m_swrInputFormat
        || inputChannels != m_swrInputChannels
        || inputSampleRate != m_swrInputSampleRate;

    if (needsNewSwr) {
        // Targets the AVChannelLayout resampler API -- requires FFmpeg 5.1+.
        AVChannelLayout outLayout;
        av_channel_layout_default(&outLayout, m_outputChannels);

        SwrContext* raw = nullptr;
        const int ret = swr_alloc_set_opts2(
            &raw,
            &outLayout, kOutputSampleFormat, m_outputSampleRate,
            &frame->ch_layout, inputFormat, inputSampleRate,
            0, nullptr);
        av_channel_layout_uninit(&outLayout);

        if (ret < 0 || !raw || swr_init(raw) < 0) {
            AUVIO_LOG_ERROR << "Failed to set up audio resampler";
            if (raw) swr_free(&raw);
            return;
        }

        m_swr.reset(raw);
        m_swrInputFormat = inputFormat;
        m_swrInputChannels = inputChannels;
        m_swrInputSampleRate = inputSampleRate;
    }

    const int maxOutSamples = swr_get_out_samples(m_swr.get(), frame->nb_samples);
    if (maxOutSamples <= 0) return;

    std::vector<uint8_t> outBuffer(static_cast<std::size_t>(maxOutSamples) * m_outputChannels * sizeof(int16_t));
    uint8_t* outPtr = outBuffer.data();

    const int converted = swr_convert(
        m_swr.get(), &outPtr, maxOutSamples,
        const_cast<const uint8_t**>(frame->data), frame->nb_samples);

    if (converted <= 0) return;

    const std::size_t bytesToWrite = static_cast<std::size_t>(converted) * m_outputChannels * sizeof(int16_t);

    std::unique_lock<std::mutex> lock(m_bufferMutex);
    std::size_t written = 0;
    while (written < bytesToWrite) {
        const std::size_t freeSpace = m_ringBuffer.size() - m_bufferedBytes;
        if (freeSpace == 0) {
            m_bufferNotFull.wait_for(lock, std::chrono::milliseconds(50));
            continue;
        }

        const std::size_t chunk = std::min({freeSpace, bytesToWrite - written, m_ringBuffer.size() - m_writePos});
        std::memcpy(m_ringBuffer.data() + m_writePos, outBuffer.data() + written, chunk);

        m_writePos = (m_writePos + chunk) % m_ringBuffer.size();
        m_bufferedBytes += chunk;
        written += chunk;
    }
}

void AudioEngine::play() {
    if (m_deviceId != 0) SDL_PauseAudioDevice(m_deviceId, 0);
}

void AudioEngine::pause() {
    if (m_deviceId != 0) SDL_PauseAudioDevice(m_deviceId, 1);
}

bool AudioEngine::isPaused() const {
    return m_deviceId != 0 && SDL_GetAudioDeviceStatus(m_deviceId) == SDL_AUDIO_PAUSED;
}

void AudioEngine::setVolume(float volume) {
    m_volume.store(std::clamp(volume, 0.0f, 1.0f));
}

float AudioEngine::volume() const {
    return m_volume.load();
}

double AudioEngine::playedSeconds() const {
    const std::size_t bytesPerSecond = static_cast<std::size_t>(m_outputSampleRate) * m_outputChannels * sizeof(int16_t);
    if (bytesPerSecond == 0) return 0.0;
    return static_cast<double>(m_bytesPlayed.load()) / static_cast<double>(bytesPerSecond);
}

void AudioEngine::reset() {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_readPos = m_writePos = m_bufferedBytes = 0;
    m_bytesPlayed = 0;
}

void AudioEngine::audioCallback(void* userData, Uint8* stream, int len) {
    static_cast<AudioEngine*>(userData)->fillBuffer(stream, len);
}

void AudioEngine::fillBuffer(uint8_t* stream, int len) {
    std::unique_lock<std::mutex> lock(m_bufferMutex);

    const std::size_t available = std::min(static_cast<std::size_t>(len), m_bufferedBytes);
    std::size_t copied = 0;

    while (copied < available) {
        const std::size_t chunk = std::min(available - copied, m_ringBuffer.size() - m_readPos);
        std::memcpy(stream + copied, m_ringBuffer.data() + m_readPos, chunk);
        m_readPos = (m_readPos + chunk) % m_ringBuffer.size();
        copied += chunk;
    }

    m_bufferedBytes -= copied;
    m_bufferNotFull.notify_all();
    lock.unlock();

    if (copied < static_cast<std::size_t>(len)) {
        std::memset(stream + copied, 0, static_cast<std::size_t>(len) - copied);
    }

    const float vol = m_volume.load();
    if (vol < 0.999f) {
        int16_t* samples = reinterpret_cast<int16_t*>(stream);
        const std::size_t sampleCount = static_cast<std::size_t>(len) / sizeof(int16_t);
        for (std::size_t i = 0; i < sampleCount; ++i) {
            samples[i] = static_cast<int16_t>(samples[i] * vol);
        }
    }

    m_bytesPlayed += copied;
}

}
