#include "MainWindow.h"
#include "TitleBar.h"
#include "PlayerControls.h"
#include "SettingsDialog.h"
#include "Logger.h"

#include <QLabel>
#include <QTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QEvent>

#ifdef _WIN32
#include "NativeWindowFrame_Win.h"
#endif

namespace auvio {

namespace {
constexpr int kResizeMargin = 6;
constexpr int kInactivityMs = 2000;
constexpr int kAnimationMs = 200;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setMouseTracking(true);
    setMinimumSize(480, 320);

    m_controller = std::make_unique<PlaybackController>();

    m_central = new QWidget(this);
    m_central->setMouseTracking(true);
    setCentralWidget(m_central);

    m_videoLabel = new QLabel(m_central);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("background-color: black;");
    m_videoLabel->setMouseTracking(true);
    m_videoLabel->installEventFilter(this);

    m_titleBar = new TitleBar(m_central);
    m_titleBar->setMouseTracking(true);
    m_titleBar->installEventFilter(this);

    m_controls = new PlayerControls(m_central);
    m_controls->setObjectName("playerControlsPanel");
    m_controls->setMouseTracking(true);
    m_controls->installEventFilter(this);

    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(200);

    m_inactivityTimer = new QTimer(this);
    m_inactivityTimer->setInterval(kInactivityMs);
    m_inactivityTimer->setSingleShot(true);

    connect(m_titleBar, &TitleBar::openFileRequested, this, &MainWindow::onOpenFile);
    connect(m_titleBar, &TitleBar::openSettingsRequested, this, &MainWindow::onOpenSettings);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &MainWindow::onMinimize);
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested, this, &MainWindow::onMaximizeRestore);
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);

    connect(m_controls, &PlayerControls::playPauseClicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_controls, &PlayerControls::seekRequested, this, &MainWindow::onSeekRequested);
    connect(m_controls, &PlayerControls::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(m_positionTimer, &QTimer::timeout, this, &MainWindow::onPositionTimer);
    connect(m_inactivityTimer, &QTimer::timeout, this, &MainWindow::onInactivityTimeout);
    connect(this, &MainWindow::frameReady, this, &MainWindow::onFrameReady);

    // Emitted from the video decode thread; Qt auto-queues it since the receiver lives on the GUI thread.
    m_controller->setVideoFrameCallback([this](int width, int height, const uint8_t* rgba, int stride) {
        QImage image(rgba, width, height, stride, QImage::Format_RGBA8888);
        emit frameReady(image.copy());
    });

    resize(1100, 680);
    setWindowTitle("AUVIO");

#ifdef _WIN32
    winId(); // forces the native window to exist before we touch it
    NativeWindowFrame::enable(windowHandle());
#endif

    m_titleBar->raise();
    m_controls->raise();
    resetInactivityTimer();
}

MainWindow::~MainWindow() = default;

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    m_videoLabel->setGeometry(m_central->rect());
    repositionOverlays();
}

void MainWindow::repositionOverlays() {
    const QRect area = m_central->rect();
    m_titleBar->setGeometry(area.x(), area.y(), area.width(), m_titleBar->height());

    const int controlsHeight = m_controls->sizeHint().height();
    const int controlsY = m_controlsVisible ? (area.bottom() - controlsHeight + 1) : (area.bottom() + 1);
    m_controls->setGeometry(area.x(), controlsY, area.width(), controlsHeight);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        resetInactivityTimer();
    }
    return QMainWindow::eventFilter(watched, event);
}

#ifdef _WIN32
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (eventType == "windows_generic_MSG" && windowHandle() && m_titleBar) {
        if (NativeWindowFrame::handleNativeEvent(
                windowHandle(), message, result,
                m_titleBar->geometry(), m_titleBar->buttonRects(), kResizeMargin)) {
            return true;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::resetInactivityTimer() {
    if (!m_controlsVisible) {
        showControls();
    }
    m_inactivityTimer->start();
}

void MainWindow::showControls() {
    m_controlsVisible = true;
    m_central->unsetCursor();

    const int controlsHeight = m_controls->sizeHint().height();
    auto* anim = new QPropertyAnimation(m_controls, "geometry", this);
    anim->setDuration(kAnimationMs);
    anim->setEndValue(QRect(0, m_central->height() - controlsHeight, m_central->width(), controlsHeight));
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::hideControls() {
    m_controlsVisible = false;

    const int controlsHeight = m_controls->sizeHint().height();
    auto* anim = new QPropertyAnimation(m_controls, "geometry", this);
    anim->setDuration(kAnimationMs);
    anim->setEndValue(QRect(0, m_central->height(), m_central->width(), controlsHeight));
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    m_central->setCursor(Qt::BlankCursor);
}

void MainWindow::onInactivityTimeout() {
    hideControls();
}

void MainWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open Media File");
    if (!path.isEmpty()) {
        openPath(path);
    }
}

void MainWindow::onOpenSettings() {
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::openPath(const QString& path) {
    if (m_controller->open(path.toStdString())) {
        m_controller->play();
        m_controls->setPlaying(true);
        m_positionTimer->start();

        const QString title = QFileInfo(path).completeBaseName();
        m_titleBar->setTitleText(title + "   —   AUVIO");
        setWindowTitle(title + " - AUVIO");
    } else {
        AUVIO_LOG_ERROR << "Failed to open: " << path.toStdString();
    }
}

void MainWindow::onPlayPauseClicked() {
    if (m_controller->state() == PlaybackState::Playing) {
        m_controller->pause();
        m_controls->setPlaying(false);
    } else {
        m_controller->play();
        m_controls->setPlaying(true);
    }
}

void MainWindow::onSeekRequested(double fraction) {
    m_controller->seek(fraction * m_controller->durationSeconds());
}

void MainWindow::onVolumeChanged(int percent) {
    m_controller->setVolume(static_cast<float>(percent) / 100.0f);
}

void MainWindow::onFrameReady(QImage image) {
    m_videoLabel->setPixmap(QPixmap::fromImage(image).scaled(
        m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onPositionTimer() {
    m_controls->setPosition(m_controller->positionSeconds(), m_controller->durationSeconds());
}

void MainWindow::onMinimize() {
    showMinimized();
}

void MainWindow::onMaximizeRestore() {
    if (isMaximized()) {
        showNormal();
        m_titleBar->setMaximized(false);
    } else {
        showMaximized();
        m_titleBar->setMaximized(true);
    }
}

}
