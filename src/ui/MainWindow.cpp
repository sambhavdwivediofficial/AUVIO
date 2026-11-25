#include "MainWindow.h"
#include "PlayerControls.h"
#include "PlaylistView.h"
#include "SettingsDialog.h"
#include "Logger.h"

#include <QLabel>
#include <QTimer>
#include <QDockWidget>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QPixmap>
#include <QWidget>

namespace auvio {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_controller = std::make_unique<PlaybackController>();

    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("background-color: black;");
    setCentralWidget(m_videoLabel);

    m_controls = new PlayerControls(this);
    auto* controlsDock = new QDockWidget(this);
    controlsDock->setWidget(m_controls);
    controlsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    controlsDock->setTitleBarWidget(new QWidget(this));
    addDockWidget(Qt::BottomDockWidgetArea, controlsDock);

    m_playlist = new PlaylistView(this);
    auto* playlistDock = new QDockWidget("Playlist", this);
    playlistDock->setWidget(m_playlist);
    addDockWidget(Qt::RightDockWidgetArea, playlistDock);

    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(200);

    createMenus();

    connect(m_controls, &PlayerControls::playPauseClicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_controls, &PlayerControls::seekRequested, this, &MainWindow::onSeekRequested);
    connect(m_controls, &PlayerControls::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(m_playlist, &PlaylistView::fileActivated, this, &MainWindow::onPlaylistFileActivated);
    connect(m_positionTimer, &QTimer::timeout, this, &MainWindow::onPositionTimer);
    connect(this, &MainWindow::frameReady, this, &MainWindow::onFrameReady);

    // Emitted from the video decode thread; Qt auto-queues it since the receiver lives on the GUI thread.
    m_controller->setVideoFrameCallback([this](int width, int height, const uint8_t* rgba, int stride) {
        QImage image(rgba, width, height, stride, QImage::Format_RGBA8888);
        emit frameReady(image.copy());
    });

    resize(1024, 640);
    setWindowTitle("AUVIO");
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Open...", this, &MainWindow::onOpenFile);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);

    auto* editMenu = menuBar()->addMenu("Edit");
    QAction* settingsAction = editMenu->addAction("Settings...");
    connect(settingsAction, &QAction::triggered, this, [this] {
        SettingsDialog dialog(this);
        dialog.exec();
    });
}

void MainWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open Media File");
    if (!path.isEmpty()) {
        openPath(path);
    }
}

void MainWindow::openPath(const QString& path) {
    if (m_controller->open(path.toStdString())) {
        m_playlist->addFile(path);
        m_controller->play();
        m_controls->setPlaying(true);
        m_positionTimer->start();
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

}
