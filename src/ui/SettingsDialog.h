#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString getNotesDirectory() const;
    bool isAutoSaveEnabled() const;
    int getAutoSaveInterval() const;

private slots:
    void browseNotesDirectory();
    void accept() override;

private:
    void setupUi();
    void loadCurrentSettings();

    QLineEdit *m_notesDirectoryEdit;
    QPushButton *m_browseButton;
    QCheckBox *m_autoSaveCheckBox;
    QSpinBox *m_autoSaveIntervalSpinBox;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};
