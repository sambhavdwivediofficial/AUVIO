#pragma once

#include <QMainWindow>
#include <QImage>
#include <memory>

#include "PlaybackController.h"

class QLabel;
class QTimer;

namespace auvio {

class TitleBar;
class PlayerControls;
class PlaylistView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onOpenFile();
    void onPlayPauseClicked();
    void onSeekRequested(double fraction);
    void onVolumeChanged(int percent);
    void onPlaylistFileActivated(const QString& path);
    void onFrameReady(QImage image);
    void onPositionTimer();
    void onInactivityTimeout();
    void onMinimize();
    void onMaximizeRestore();
    void onOpenSettings();

signals:
    void frameReady(QImage image);

private:
    void openPath(const QString& path);
    void repositionOverlays();
    void showOverlayControls();
    void hideOverlayControls();
    void resetInactivityTimer();

    std::unique_ptr<PlaybackController> m_controller;

    QWidget* m_central;
    TitleBar* m_titleBar;
    QLabel* m_videoLabel;
    PlayerControls* m_controls;
    PlaylistView* m_playlist;
    QTimer* m_positionTimer;
    QTimer* m_inactivityTimer;

    bool m_overlaysVisible = true;
};

}
