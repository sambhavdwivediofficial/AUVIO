#pragma once

#include <QMainWindow>
#include <QImage>
#include <memory>

#include "PlaybackController.h"

class QLabel;
class QTimer;

namespace auvio {

class PlayerControls;
class PlaylistView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpenFile();
    void onPlayPauseClicked();
    void onSeekRequested(double fraction);
    void onVolumeChanged(int percent);
    void onPlaylistFileActivated(const QString& path);
    void onFrameReady(QImage image);
    void onPositionTimer();

signals:
    void frameReady(QImage image);

private:
    void openPath(const QString& path);
    void createMenus();

    std::unique_ptr<PlaybackController> m_controller;

    QLabel* m_videoLabel;
    PlayerControls* m_controls;
    PlaylistView* m_playlist;
    QTimer* m_positionTimer;
};

}
