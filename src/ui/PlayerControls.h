#pragma once

#include <QWidget>
#include <QString>

class QPushButton;
class QSlider;
class QLabel;

namespace auvio {

class PlayerControls : public QWidget {
    Q_OBJECT

public:
    explicit PlayerControls(QWidget* parent = nullptr);

    void setPlaying(bool playing);
    void setPosition(double currentSeconds, double durationSeconds);
    void setVolume(int percent);

signals:
    void playPauseClicked();
    void seekRequested(double fraction);
    void volumeChanged(int percent);

private:
    QString formatTime(double seconds) const;

    QPushButton* m_playPauseButton;
    QSlider* m_seekSlider;
    QSlider* m_volumeSlider;
    QLabel* m_currentTimeLabel;
    QLabel* m_durationLabel;

    bool m_userIsSeeking = false;
};

}
