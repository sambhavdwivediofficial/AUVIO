#include "MainWindow.h"
#include "TitleBar.h"
#include "PlayerControls.h"
#include "PlaylistView.h"
#include "SettingsDialog.h"
#include "Logger.h"

#include <QLabel>
#include <QTimer>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QMouseEvent>
#include <QWindow>
#include <QPropertyAnimation>
#include <QEvent>

namespace auvio {

namespace {

constexpr int kResizeMargin = 6;
constexpr int kInactivityMs = 3000;
constexpr int kAnimationMs = 220;

Qt::Edges edgesAt(const QPoint& pos, const QSize& size) {
    Qt::Edges edges;
    if (pos.x() <= kResizeMargin) edges |= Qt::LeftEdge;
    if (pos.x() >= size.width() - kResizeMargin) edges |= Qt::RightEdge;
    if (pos.y() <= kResizeMargin) edges |= Qt::TopEdge;
    if (pos.y() >= size.height() - kResizeMargin) edges |= Qt::BottomEdge;
    return edges;
}

}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowFlag(Qt::FramelessWindowHint, true);
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

    m_playlist = new PlaylistView(this);
    auto* playlistDock = new QDockWidget("Playlist", this);
    playlistDock->setObjectName("playlistDock");
    playlistDock->setWidget(m_playlist);
    addDockWidget(Qt::RightDockWidgetArea, playlistDock);

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
    connect(m_playlist, &PlaylistView::fileActivated, this, &MainWindow::onPlaylistFileActivated);
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
    const int controlsY = m_overlaysVisible ? (area.bottom() - controlsHeight + 1) : (area.bottom() + 1);
    m_controls->setGeometry(area.x(), controlsY, area.width(), controlsHeight);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    const Qt::Edges edges = edgesAt(event->pos(), size());
    if (edges == (Qt::TopEdge | Qt::LeftEdge) || edges == (Qt::BottomEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges == (Qt::TopEdge | Qt::RightEdge) || edges == (Qt::BottomEdge | Qt::LeftEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);
    } else if (edges & (Qt::TopEdge | Qt::BottomEdge)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        unsetCursor();
    }

    resetInactivityTimer();
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && windowHandle()) {
        const Qt::Edges edges = edgesAt(event->pos(), size());
        if (edges) {
            windowHandle()->startSystemResize(edges);
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        resetInactivityTimer();
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resetInactivityTimer() {
    if (!m_overlaysVisible) {
        showOverlayControls();
    }
    m_inactivityTimer->start();
}

void MainWindow::showOverlayControls() {
    m_overlaysVisible = true;
    m_central->unsetCursor();

    auto* titleAnim = new QPropertyAnimation(m_titleBar, "geometry", this);
    titleAnim->setDuration(kAnimationMs);
    titleAnim->setEndValue(QRect(0, 0, m_central->width(), m_titleBar->height()));
    titleAnim->start(QAbstractAnimation::DeleteWhenStopped);

    const int controlsHeight = m_controls->sizeHint().height();
    auto* controlsAnim = new QPropertyAnimation(m_controls, "geometry", this);
    controlsAnim->setDuration(kAnimationMs);
    controlsAnim->setEndValue(QRect(0, m_central->height() - controlsHeight, m_central->width(), controlsHeight));
    controlsAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::hideOverlayControls() {
    m_overlaysVisible = false;

    auto* titleAnim = new QPropertyAnimation(m_titleBar, "geometry", this);
    titleAnim->setDuration(kAnimationMs);
    titleAnim->setEndValue(QRect(0, -m_titleBar->height(), m_central->width(), m_titleBar->height()));
    titleAnim->start(QAbstractAnimation::DeleteWhenStopped);

    const int controlsHeight = m_controls->sizeHint().height();
    auto* controlsAnim = new QPropertyAnimation(m_controls, "geometry", this);
    controlsAnim->setDuration(kAnimationMs);
    controlsAnim->setEndValue(QRect(0, m_central->height(), m_central->width(), controlsHeight));
    controlsAnim->start(QAbstractAnimation::DeleteWhenStopped);

    m_central->setCursor(Qt::BlankCursor);
}

void MainWindow::onInactivityTimeout() {
    hideOverlayControls();
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
        m_playlist->addFile(path);
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

void MainWindow::onPlaylistFileActivated(const QString& path) {
    openPath(path);
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
