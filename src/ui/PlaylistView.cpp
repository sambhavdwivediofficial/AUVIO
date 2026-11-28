#include "PlaylistView.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QFileInfo>

namespace auvio {

PlaylistView::PlaylistView(QWidget* parent) : QWidget(parent) {
    m_listWidget = new QListWidget(this);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setSpacing(2);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_listWidget);
    layout->setContentsMargins(8, 8, 8, 8);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        emit fileActivated(item->data(Qt::UserRole).toString());
    });
}

void PlaylistView::addFile(const QString& path) {
    auto* item = new QListWidgetItem(QFileInfo(path).fileName());
    item->setData(Qt::UserRole, path);
    m_listWidget->addItem(item);
}

void PlaylistView::clearAll() {
    m_listWidget->clear();
}

}
