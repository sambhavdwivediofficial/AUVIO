#pragma once

#include <QWidget>
#include <QString>

class QListWidget;

namespace auvio {

class PlaylistView : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistView(QWidget* parent = nullptr);

    void addFile(const QString& path);
    void clearAll();

signals:
    void fileActivated(const QString& path);

private:
    QListWidget* m_listWidget;
};

}
