#include "TitleBar.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QWindow>
#include <QPixmap>

namespace auvio {

namespace {

QPushButton* makeIconButton(const QString& glyph, QWidget* parent) {
    auto* button = new QPushButton(glyph, parent);
    button->setFixedSize(40, 32);
    button->setCursor(Qt::PointingHandCursor);
    button->setObjectName("titleBarButton");
    return button;
}

}

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(40);
    setObjectName("titleBar");
    setAttribute(Qt::WA_StyledBackground, true);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(QPixmap(":/icon.png").scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconLabel->setFixedSize(18, 18);

    m_titleLabel = new QLabel("AUVIO", this);
    m_titleLabel->setObjectName("titleBarLabel");

    m_fileButton = makeIconButton(QString::fromUtf8("\xF0\x9F\x93\x81"), this);
    m_fileButton->setToolTip("Open file");

    m_settingsButton = makeIconButton(QString::fromUtf8("\xE2\x9A\x99"), this);
    m_settingsButton->setToolTip("Settings");

    m_minimizeButton = makeIconButton(QString::fromUtf8("\xE2\x88\x92"), this);
    m_minimizeButton->setToolTip("Minimize");

    m_maximizeButton = makeIconButton(QString::fromUtf8("\xE2\x96\xA1"), this);
    m_maximizeButton->setToolTip("Maximize");

    m_closeButton = makeIconButton(QString::fromUtf8("\xC3\x97"), this);
    m_closeButton->setObjectName("titleBarCloseButton");
    m_closeButton->setToolTip("Close");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(m_iconLabel);
    layout->addSpacing(10);
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_fileButton);
    layout->addWidget(m_settingsButton);
    layout->addSpacing(10);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    connect(m_fileButton, &QPushButton::clicked, this, &TitleBar::openFileRequested);
    connect(m_settingsButton, &QPushButton::clicked, this, &TitleBar::openSettingsRequested);
    connect(m_minimizeButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximizeButton, &QPushButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::setTitleText(const QString& text) {
    m_titleLabel->setText(text);
}

void TitleBar::setMaximized(bool maximized) {
    m_maximizeButton->setToolTip(maximized ? "Restore" : "Maximize");
}

void TitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && windowHandle()) {
        windowHandle()->startSystemMove();
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent*) {
    emit maximizeRestoreRequested();
}

}
