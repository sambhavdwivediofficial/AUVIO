#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;

namespace auvio {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onAccepted();

private:
    void loadFromConfig();

    QCheckBox* m_hardwareAccelCheckbox;
    QComboBox* m_themeComboBox;
};

}
