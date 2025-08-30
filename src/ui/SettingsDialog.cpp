#include "SettingsDialog.h"
#include "../db/DatabaseManager.h"
#include <QApplication>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Settings");
    setModal(true);
    setMinimumSize(500, 300);
    setupUi();
    loadCurrentSettings();
}

void SettingsDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);
    
    // Notes Directory Group
    auto *notesGroup = new QGroupBox("Notes Storage", this);
    notesGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #404040; border-radius: 8px; margin-top: 10px; padding-top: 10px; } "
                             "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; }");
    
    auto *notesLayout = new QVBoxLayout(notesGroup);
    
    auto *notesLabel = new QLabel("Notes Directory:", notesGroup);
    notesLabel->setStyleSheet("color: #e0e0e0; margin-bottom: 5px;");
    
    auto *notesDirLayout = new QHBoxLayout();
    m_notesDirectoryEdit = new QLineEdit(notesGroup);
    m_notesDirectoryEdit->setStyleSheet("QLineEdit { background: #2d2d2d; border: 1px solid #404040; border-radius: 4px; padding: 8px; color: #e0e0e0; } "
                                       "QLineEdit:focus { border-color: #007aff; }");
    m_notesDirectoryEdit->setPlaceholderText("Select directory for storing notes...");
    
    m_browseButton = new QPushButton("Browse", notesGroup);
    m_browseButton->setStyleSheet("QPushButton { background: #007aff; border: none; border-radius: 4px; padding: 8px 16px; color: white; font-weight: bold; } "
                                 "QPushButton:hover { background: #0056cc; } "
                                 "QPushButton:pressed { background: #004499; }");
    
    notesDirLayout->addWidget(m_notesDirectoryEdit);
    notesDirLayout->addWidget(m_browseButton);
    
    auto *notesInfoLabel = new QLabel("Notes will be automatically saved to this directory. You can also import existing README.md files from this location.", notesGroup);
    notesInfoLabel->setStyleSheet("color: #999999; font-size: 11px; margin-top: 5px;");
    notesInfoLabel->setWordWrap(true);
    
    notesLayout->addWidget(notesLabel);
    notesLayout->addLayout(notesDirLayout);
    notesLayout->addWidget(notesInfoLabel);
    
    // Auto-save Group
    auto *autoSaveGroup = new QGroupBox("Auto-save Settings", this);
    autoSaveGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #404040; border-radius: 8px; margin-top: 10px; padding-top: 10px; } "
                                "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; }");
    
    auto *autoSaveLayout = new QVBoxLayout(autoSaveGroup);
    
    m_autoSaveCheckBox = new QCheckBox("Enable auto-save", autoSaveGroup);
    m_autoSaveCheckBox->setStyleSheet("QCheckBox { color: #e0e0e0; spacing: 8px; } "
                                     "QCheckBox::indicator { width: 18px; height: 18px; } "
                                     "QCheckBox::indicator:unchecked { border: 2px solid #404040; border-radius: 3px; background: #2d2d2d; } "
                                     "QCheckBox::indicator:checked { border: 2px solid #007aff; border-radius: 3px; background: #007aff; } "
                                     "QCheckBox::indicator:checked::after { content: '✓'; color: white; font-weight: bold; }");
    
    auto *intervalLayout = new QHBoxLayout();
    auto *intervalLabel = new QLabel("Auto-save interval:", autoSaveGroup);
    intervalLabel->setStyleSheet("color: #e0e0e0;");
    
    m_autoSaveIntervalSpinBox = new QSpinBox(autoSaveGroup);
    m_autoSaveIntervalSpinBox->setStyleSheet("QSpinBox { background: #2d2d2d; border: 1px solid #404040; border-radius: 4px; padding: 6px; color: #e0e0e0; } "
                                            "QSpinBox:focus { border-color: #007aff; }");
    m_autoSaveIntervalSpinBox->setRange(1, 60);
    m_autoSaveIntervalSpinBox->setSuffix(" seconds");
    m_autoSaveIntervalSpinBox->setValue(2);
    
    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(m_autoSaveIntervalSpinBox);
    intervalLayout->addStretch();
    
    auto *autoSaveInfoLabel = new QLabel("Notes will be automatically saved when you stop typing for the specified interval.", autoSaveGroup);
    autoSaveInfoLabel->setStyleSheet("color: #999999; font-size: 11px; margin-top: 5px;");
    autoSaveInfoLabel->setWordWrap(true);
    
    autoSaveLayout->addWidget(m_autoSaveCheckBox);
    autoSaveLayout->addLayout(intervalLayout);
    autoSaveLayout->addWidget(autoSaveInfoLabel);
    
    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setStyleSheet("QPushButton { background: #404040; border: none; border-radius: 4px; padding: 8px 16px; color: #e0e0e0; } "
                                 "QPushButton:hover { background: #505050; } "
                                 "QPushButton:pressed { background: #303030; }");
    
    m_okButton = new QPushButton("OK", this);
    m_okButton->setStyleSheet("QPushButton { background: #007aff; border: none; border-radius: 4px; padding: 8px 16px; color: white; font-weight: bold; } "
                             "QPushButton:hover { background: #0056cc; } "
                             "QPushButton:pressed { background: #004499; }");
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    // Add all to main layout
    layout->addWidget(notesGroup);
    layout->addWidget(autoSaveGroup);
    layout->addStretch();
    layout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_browseButton, &QPushButton::clicked, this, &SettingsDialog::browseNotesDirectory);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    
    // Set dialog style
    setStyleSheet("QDialog { background: #1e1e1e; color: #e0e0e0; }");
}

void SettingsDialog::loadCurrentSettings() {
    DatabaseManager &db = DatabaseManager::instance();
    m_notesDirectoryEdit->setText(db.getNotesDirectory());
    m_autoSaveCheckBox->setChecked(true); // Default to enabled
    m_autoSaveIntervalSpinBox->setValue(2); // Default to 2 seconds
}

void SettingsDialog::browseNotesDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Notes Directory", 
                                                   m_notesDirectoryEdit->text(),
                                                   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_notesDirectoryEdit->setText(dir);
    }
}

void SettingsDialog::accept() {
    // Validate notes directory
    QString notesDir = m_notesDirectoryEdit->text().trimmed();
    if (notesDir.isEmpty()) {
        notesDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Notes";
        m_notesDirectoryEdit->setText(notesDir);
    }
    
    // Create directory if it doesn't exist
    QDir dir(notesDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QDialog::accept();
}

QString SettingsDialog::getNotesDirectory() const {
    return m_notesDirectoryEdit->text().trimmed();
}

bool SettingsDialog::isAutoSaveEnabled() const {
    return m_autoSaveCheckBox->isChecked();
}

int SettingsDialog::getAutoSaveInterval() const {
    return m_autoSaveIntervalSpinBox->value() * 1000; // Convert to milliseconds
}
