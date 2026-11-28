#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QPushButton;

namespace auvio {

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

    void setTitleText(const QString& text);
    void setMaximized(bool maximized);

signals:
    void openFileRequested();
    void openSettingsRequested();
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QPushButton* m_fileButton;
    QPushButton* m_settingsButton;
    QPushButton* m_minimizeButton;
    QPushButton* m_maximizeButton;
    QPushButton* m_closeButton;
};

}
