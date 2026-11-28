#pragma once

#include <QWidget>
#include <QString>
#include <vector>

class QLabel;
class QPushButton;

namespace auvio {

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

    void setTitleText(const QString& text);
    void setMaximized(bool maximized);

    std::vector<QRect> buttonRects() const;

signals:
    void openFileRequested();
    void openSettingsRequested();
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

private:
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QPushButton* m_fileButton;
    QPushButton* m_settingsButton;
    QPushButton* m_minimizeButton = nullptr;
    QPushButton* m_maximizeButton = nullptr;
    QPushButton* m_closeButton = nullptr;
};

}
