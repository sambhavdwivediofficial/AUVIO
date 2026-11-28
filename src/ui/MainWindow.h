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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private slots:
    void onOpenFile();
    void onPlayPauseClicked();
    void onSeekRequested(double fraction);
    void onVolumeChanged(int percent);
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
    void showControls();
    void hideControls();
    void resetInactivityTimer();

    std::unique_ptr<PlaybackController> m_controller;

    QWidget* m_central = nullptr;
    TitleBar* m_titleBar = nullptr;
    QLabel* m_videoLabel = nullptr;
    PlayerControls* m_controls = nullptr;
    QTimer* m_positionTimer = nullptr;
    QTimer* m_inactivityTimer = nullptr;

    bool m_controlsVisible = true;
};

}
