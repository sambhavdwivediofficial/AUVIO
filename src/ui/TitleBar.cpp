#include "TitleBar.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace auvio {

namespace {

enum class Glyph { Folder, Settings, Minimize, Maximize, Close };

class IconButton : public QPushButton {
public:
    IconButton(Glyph glyph, QWidget* parent) : QPushButton(parent), m_glyph(glyph) {
        setFixedSize(46, 40);
        setFlat(true);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
    }

protected:
    void enterEvent(QEnterEvent* event) override {
        update();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        update();
        QPushButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (underMouse()) {
            const QColor hoverColor = (m_glyph == Glyph::Close)
                ? QColor(232, 17, 35)
                : QColor(255, 255, 255, 28);
            painter.fillRect(rect(), hoverColor);
        }

        QPen pen(underMouse() && m_glyph == Glyph::Close ? Qt::white : QColor(210, 210, 210));
        pen.setWidthF(1.3);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        const QPointF c = rect().center();
        constexpr qreal s = 5.0;

        switch (m_glyph) {
        case Glyph::Minimize:
            painter.drawLine(QPointF(c.x() - s, c.y()), QPointF(c.x() + s, c.y()));
            break;
        case Glyph::Maximize:
            painter.drawRect(QRectF(c.x() - s, c.y() - s, s * 2, s * 2));
            break;
        case Glyph::Close:
            painter.drawLine(QPointF(c.x() - s, c.y() - s), QPointF(c.x() + s, c.y() + s));
            painter.drawLine(QPointF(c.x() - s, c.y() + s), QPointF(c.x() + s, c.y() - s));
            break;
        case Glyph::Folder: {
            QPainterPath path;
            path.moveTo(c.x() - 7, c.y() - 3);
            path.lineTo(c.x() - 3, c.y() - 3);
            path.lineTo(c.x() - 1, c.y() - 5);
            path.lineTo(c.x() + 7, c.y() - 5);
            path.lineTo(c.x() + 7, c.y() + 5);
            path.lineTo(c.x() - 7, c.y() + 5);
            path.closeSubpath();
            painter.drawPath(path);
            break;
        }
        case Glyph::Settings:
            painter.drawEllipse(c, 5.5, 5.5);
            painter.drawEllipse(c, 1.8, 1.8);
            break;
        }
    }

private:
    Glyph m_glyph;
};

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

    m_fileButton = new IconButton(Glyph::Folder, this);
    m_fileButton->setToolTip("Open file");

    m_settingsButton = new IconButton(Glyph::Settings, this);
    m_settingsButton->setToolTip("Settings");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_iconLabel);
    layout->addSpacing(10);
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_fileButton);
    layout->addWidget(m_settingsButton);

    connect(m_fileButton, &QPushButton::clicked, this, &TitleBar::openFileRequested);
    connect(m_settingsButton, &QPushButton::clicked, this, &TitleBar::openSettingsRequested);

#ifdef _WIN32
    m_minimizeButton = new IconButton(Glyph::Minimize, this);
    m_minimizeButton->setToolTip("Minimize");

    m_maximizeButton = new IconButton(Glyph::Maximize, this);
    m_maximizeButton->setToolTip("Maximize");

    m_closeButton = new IconButton(Glyph::Close, this);
    m_closeButton->setObjectName("titleBarCloseButton");
    m_closeButton->setToolTip("Close");

    layout->addSpacing(14);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    connect(m_minimizeButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximizeButton, &QPushButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
#endif
}

void TitleBar::setTitleText(const QString& text) {
    m_titleLabel->setText(text);
}

void TitleBar::setMaximized(bool maximized) {
    if (m_maximizeButton) {
        m_maximizeButton->setToolTip(maximized ? "Restore" : "Maximize");
    }
}

std::vector<QRect> TitleBar::buttonRects() const {
    std::vector<QRect> rects{m_fileButton->geometry(), m_settingsButton->geometry()};
    if (m_minimizeButton) rects.push_back(m_minimizeButton->geometry());
    if (m_maximizeButton) rects.push_back(m_maximizeButton->geometry());
    if (m_closeButton) rects.push_back(m_closeButton->geometry());
    return rects;
}

}
