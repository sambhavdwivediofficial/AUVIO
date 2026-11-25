#include "PlayerControls.h"

#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>

namespace auvio {

PlayerControls::PlayerControls(QWidget* parent) : QWidget(parent) {
    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playPauseButton->setFixedSize(40, 40);

    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 1000);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(100);

    m_currentTimeLabel = new QLabel("00:00", this);
    m_durationLabel = new QLabel("00:00", this);

    auto* seekRow = new QHBoxLayout();
    seekRow->addWidget(m_currentTimeLabel);
    seekRow->addWidget(m_seekSlider);
    seekRow->addWidget(m_durationLabel);

    auto* controlsRow = new QHBoxLayout();
    controlsRow->addWidget(m_playPauseButton);
    controlsRow->addStretch();
    controlsRow->addWidget(new QLabel("Volume", this));
    controlsRow->addWidget(m_volumeSlider);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(seekRow);
    mainLayout->addLayout(controlsRow);

    connect(m_playPauseButton, &QPushButton::clicked, this, &PlayerControls::playPauseClicked);

    connect(m_seekSlider, &QSlider::sliderPressed, this, [this] { m_userIsSeeking = true; });
    connect(m_seekSlider, &QSlider::sliderReleased, this, [this] {
        m_userIsSeeking = false;
        emit seekRequested(static_cast<double>(m_seekSlider->value()) / m_seekSlider->maximum());
    });

    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlayerControls::volumeChanged);
}

void PlayerControls::setPlaying(bool playing) {
    m_playPauseButton->setIcon(style()->standardIcon(playing ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
}

void PlayerControls::setPosition(double currentSeconds, double durationSeconds) {
    if (m_userIsSeeking) return;

    if (durationSeconds > 0.0) {
        m_seekSlider->setValue(static_cast<int>((currentSeconds / durationSeconds) * m_seekSlider->maximum()));
    }

    m_currentTimeLabel->setText(formatTime(currentSeconds));
    m_durationLabel->setText(formatTime(durationSeconds));
}

void PlayerControls::setVolume(int percent) {
    m_volumeSlider->setValue(percent);
}

QString PlayerControls::formatTime(double seconds) const {
    const int total = static_cast<int>(seconds);
    return QString("%1:%2")
        .arg(total / 60, 2, 10, QChar('0'))
        .arg(total % 60, 2, 10, QChar('0'));
}

}
