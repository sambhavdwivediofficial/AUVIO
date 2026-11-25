#include "PlaybackController.h"
#include "Logger.h"

#include <chrono>

namespace auvio {

void PlaybackController::PacketQueue::push(AVPacket* packet, const std::atomic<bool>& stopFlag) {
    std::unique_lock<std::mutex> lock(mutex);
    notFull.wait(lock, [&] { return packets.size() < kMaxSize || stopFlag.load(); });
    if (stopFlag.load()) {
        av_packet_free(&packet);
        return;
    }
    packets.push(packet);
    lock.unlock();
    notEmpty.notify_one();
}

AVPacket* PlaybackController::PacketQueue::pop(const std::atomic<bool>& stopFlag) {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [&] { return !packets.empty() || stopFlag.load(); });
    if (packets.empty()) return nullptr;
    AVPacket* packet = packets.front();
    packets.pop();
    lock.unlock();
    notFull.notify_one();
    return packet;
}

void PlaybackController::PacketQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    while (!packets.empty()) {
        AVPacket* packet = packets.front();
        packets.pop();
        av_packet_free(&packet);
    }
}

PlaybackController::PlaybackController() = default;

PlaybackController::~PlaybackController() {
    closeInternal();
}

bool PlaybackController::open(const std::string& path) {
    stop();

    if (!m_demuxer.open(path)) {
        return false;
    }

    m_hasVideo = m_demuxer.videoStreamIndex() >= 0;
    m_hasAudio = m_demuxer.audioStreamIndex() >= 0;

    if (m_hasVideo && !m_videoDecoder.open(m_demuxer.videoCodecParameters())) {
        AUVIO_LOG_ERROR << "Failed to open video decoder";
        m_hasVideo = false;
    }

    if (m_hasAudio && !m_audioDecoder.open(m_demuxer.audioCodecParameters())) {
        AUVIO_LOG_ERROR << "Failed to open audio decoder";
        m_hasAudio = false;
    }

    if (m_hasAudio) {
        const AVCodecParameters* params = m_demuxer.audioCodecParameters();
        if (!m_audioEngine.open(params->sample_rate, params->ch_layout.nb_channels)) {
            AUVIO_LOG_ERROR << "Failed to open audio output";
            m_hasAudio = false;
        }
    }

    if (!m_hasAudio && !m_hasVideo) {
        return false;
    }

    m_stopRequested = false;
    m_state = PlaybackState::Paused;

    m_demuxThread = std::thread(&PlaybackController::demuxLoop, this);
    if (m_hasAudio) m_audioThread = std::thread(&PlaybackController::audioLoop, this);
    if (m_hasVideo) m_videoThread = std::thread(&PlaybackController::videoLoop, this);

    return true;
}

void PlaybackController::play() {
    if (m_state == PlaybackState::Paused) {
        m_audioEngine.play();
        m_state = PlaybackState::Playing;
    }
}

void PlaybackController::pause() {
    if (m_state == PlaybackState::Playing) {
        m_audioEngine.pause();
        m_state = PlaybackState::Paused;
    }
}

void PlaybackController::stop() {
    closeInternal();
}

void PlaybackController::closeInternal() {
    m_stopRequested = true;
    m_audioQueue.notEmpty.notify_all();
    m_audioQueue.notFull.notify_all();
    m_videoQueue.notEmpty.notify_all();
    m_videoQueue.notFull.notify_all();

    if (m_demuxThread.joinable()) m_demuxThread.join();
    if (m_audioThread.joinable()) m_audioThread.join();
    if (m_videoThread.joinable()) m_videoThread.join();

    m_audioQueue.clear();
    m_videoQueue.clear();

    m_audioEngine.close();
    m_audioDecoder.close();
    m_videoDecoder.close();
    m_demuxer.close();

    m_state = PlaybackState::Stopped;
    m_stopRequested = false;
}

void PlaybackController::seek(double seconds) {
    std::lock_guard<std::mutex> lock(m_seekMutex);
    m_seekTargetSeconds = seconds;
    m_seekRequested = true;
}

void PlaybackController::setVolume(float volume) {
    m_audioEngine.setVolume(volume);
}

void PlaybackController::setVideoFrameCallback(VideoRenderer::FrameReadyCallback callback) {
    m_videoRenderer.setFrameReadyCallback(std::move(callback));
}

PlaybackState PlaybackController::state() const {
    return m_state.load();
}

double PlaybackController::positionSeconds() const {
    return m_hasAudio ? m_audioEngine.playedSeconds() : m_videoClockSeconds.load();
}

double PlaybackController::durationSeconds() const {
    return m_demuxer.durationSeconds();
}

void PlaybackController::demuxLoop() {
    AVPacket* packet = av_packet_alloc();

    while (!m_stopRequested.load()) {
        {
            std::lock_guard<std::mutex> lock(m_seekMutex);
            if (m_seekRequested) {
                m_demuxer.seek(m_seekTargetSeconds);
                m_audioDecoder.flush();
                m_videoDecoder.flush();
                m_audioQueue.clear();
                m_videoQueue.clear();
                m_seekRequested = false;
            }
        }

        if (!m_demuxer.readPacket(packet)) {
            break;
        }

        if (m_hasVideo && packet->stream_index == m_demuxer.videoStreamIndex()) {
            m_videoQueue.push(av_packet_clone(packet), m_stopRequested);
        } else if (m_hasAudio && packet->stream_index == m_demuxer.audioStreamIndex()) {
            m_audioQueue.push(av_packet_clone(packet), m_stopRequested);
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
}

void PlaybackController::audioLoop() {
    while (!m_stopRequested.load()) {
        AVPacket* packet = m_audioQueue.pop(m_stopRequested);
        if (!packet) continue;

        if (m_audioDecoder.sendPacket(packet)) {
            while (AVFrame* frame = m_audioDecoder.receiveFrame()) {
                m_audioEngine.queueFrame(frame);
                av_frame_free(&frame);
            }
        }

        av_packet_free(&packet);
    }
}

void PlaybackController::videoLoop() {
    using Clock = std::chrono::steady_clock;
    const auto startTime = Clock::now();

    while (!m_stopRequested.load()) {
        AVPacket* packet = m_videoQueue.pop(m_stopRequested);
        if (!packet) continue;

        if (m_videoDecoder.sendPacket(packet)) {
            while (AVFrame* frame = m_videoDecoder.receiveFrame()) {
                const AVRational timeBase = m_demuxer.videoTimeBase();
                const double pts = frame->pts == AV_NOPTS_VALUE
                    ? 0.0
                    : frame->pts * av_q2d(timeBase);

                m_videoClockSeconds.store(pts);

                const double masterClock = m_hasAudio
                    ? m_audioEngine.playedSeconds()
                    : std::chrono::duration<double>(Clock::now() - startTime).count();

                const double delay = pts - masterClock;
                if (delay > 0.0 && delay < 1.0) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(delay));
                } else if (delay < -0.1) {
                    av_frame_free(&frame);
                    continue; // too far behind, drop frame to catch up
                }

                m_videoRenderer.renderFrame(frame);
                av_frame_free(&frame);
            }
        }

        av_packet_free(&packet);
    }
}

}
