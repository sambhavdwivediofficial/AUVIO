#include "SettingsDialog.h"
#include "Config.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

namespace auvio {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Settings");

    m_hardwareAccelCheckbox = new QCheckBox("Enable hardware-accelerated decoding", this);
    m_themeComboBox = new QComboBox(this);
    m_themeComboBox->addItems({"Dark", "Light"});

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_hardwareAccelCheckbox);
    layout->addWidget(new QLabel("Theme", this));
    layout->addWidget(m_themeComboBox);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

    loadFromConfig();
}

void SettingsDialog::loadFromConfig() {
    auto& config = Config::instance();
    m_hardwareAccelCheckbox->setChecked(config.get<bool>("hardware_accel", true));
    m_themeComboBox->setCurrentText(QString::fromStdString(config.get<std::string>("theme", "Dark")));
}

void SettingsDialog::onAccepted() {
    auto& config = Config::instance();
    config.set("hardware_accel", m_hardwareAccelCheckbox->isChecked());
    config.set("theme", m_themeComboBox->currentText().toStdString());
    config.save();
    accept();
}

}
