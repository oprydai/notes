#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSet>
#include <QMap>

DatabaseManager &DatabaseManager::instance() {
    static DatabaseManager mgr;
    return mgr;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent),
      m_autoSaveTimer(new QTimer(this)),
      m_notesDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Notes"),
      m_autoSaveEnabled(true),
      m_autoSaveInterval(2000) {
    
    // Setup auto-save timer
    connect(m_autoSaveTimer, &QTimer::timeout, this, &DatabaseManager::performAutoSave);
    m_autoSaveTimer->setSingleShot(true);
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isValid()) {
        m_db.close();
    }
}

QString DatabaseManager::databaseFilePath() const {
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return appData + QDir::separator() + QStringLiteral("notes.db");
}

QString DatabaseManager::settingsFilePath() const {
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return appData + QDir::separator() + QStringLiteral("settings.ini");
}

bool DatabaseManager::open() {
    if (m_db.isValid() && m_db.isOpen()) return true;

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(databaseFilePath());
    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError();
        return false;
    }
    return true;
}

bool DatabaseManager::initializeSchema() {
    if (!isOpen() && !open()) return false;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        qWarning() << "Failed to enable foreign_keys pragma:" << q.lastError();
        return false;
    }

    const QString schemaSql = QString::fromUtf8(R"SQL(
CREATE TABLE IF NOT EXISTS folders (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  parent_id INTEGER NULL,
  FOREIGN KEY(parent_id) REFERENCES folders(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS notes (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  folder_id INTEGER NOT NULL,
  title TEXT NOT NULL,
  body TEXT NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY(folder_id) REFERENCES folders(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS tags (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS note_tags (
  note_id INTEGER NOT NULL,
  tag_id INTEGER NOT NULL,
  PRIMARY KEY(note_id, tag_id),
  FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE,
  FOREIGN KEY(tag_id) REFERENCES tags(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS attachments (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  note_id INTEGER NOT NULL,
  filepath TEXT NOT NULL,
  type TEXT NOT NULL,
  FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS settings (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
)SQL");

    const QStringList statements = schemaSql.split(';', Qt::SkipEmptyParts);
    for (QString stmt : statements) {
        stmt = stmt.trimmed();
        if (stmt.isEmpty()) continue;
        if (!q.exec(stmt + ';')) {
            qWarning() << "Failed to run schema statement:" << stmt << "error:" << q.lastError();
            return false;
        }
    }
    
    // Create default folders if none exist
    createDefaultFolders();
    
    // Load settings
    loadSettings();
    
    // Ensure notes directory exists
    ensureNotesDirectoryExists();
    
    return true;
}

void DatabaseManager::createDefaultFolders() {
    QSqlQuery q(m_db);
    q.exec("SELECT COUNT(*) FROM folders");
    if (q.next() && q.value(0).toInt() == 0) {
        // Create default folders
        createFolder("Personal");
        createFolder("Work");
        createFolder("Ideas");
        createFolder("Meetings");
        createFolder("Projects");
    }
}

void DatabaseManager::ensureNotesDirectoryExists() {
    QDir dir(m_notesDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool DatabaseManager::isOpen() const {
    return m_db.isValid() && m_db.isOpen();
}

QSqlDatabase DatabaseManager::database() const {
    return m_db;
}

// Note operations
int DatabaseManager::createNote(int folderId, const QString &title, const QString &body) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO notes (folder_id, title, body, created_at, updated_at) VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(folderId);
    q.addBindValue(title);
    q.addBindValue(body);
    q.addBindValue(QDateTime::currentDateTime());
    q.addBindValue(QDateTime::currentDateTime());
    
    if (!q.exec()) {
        qWarning() << "Failed to create note:" << q.lastError();
        return -1;
    }
    
    int noteId = q.lastInsertId().toInt();
    emit noteSaved(noteId);
    return noteId;
}

bool DatabaseManager::updateNote(int noteId, const QString &title, const QString &body) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE notes SET title = ?, body = ?, updated_at = ? WHERE id = ?");
    q.addBindValue(title);
    q.addBindValue(body);
    q.addBindValue(QDateTime::currentDateTime());
    q.addBindValue(noteId);
    
    if (!q.exec()) {
        qWarning() << "Failed to update note:" << q.lastError();
        return false;
    }
    
    emit noteSaved(noteId);
    return true;
}



bool DatabaseManager::deleteNote(int noteId) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM notes WHERE id = ?");
    q.addBindValue(noteId);
    
    if (!q.exec()) {
        qWarning() << "Failed to delete note:" << q.lastError();
        return false;
    }
    
    emit noteDeleted(noteId);
    return true;
}

NoteData DatabaseManager::getNote(int noteId) {
    QSqlQuery q(m_db);
    q.prepare("SELECT id, folder_id, title, body, created_at, updated_at FROM notes WHERE id = ?");
    q.addBindValue(noteId);
    
    NoteData note;
    note.id = -1;
    
    if (q.exec() && q.next()) {
        note.id = q.value(0).toInt();
        note.folderId = q.value(1).toInt();
        note.title = q.value(2).toString();
        note.body = q.value(3).toString();
        note.createdAt = q.value(4).toDateTime();
        note.updatedAt = q.value(5).toDateTime();
    }
    
    return note;
}

QList<NoteData> DatabaseManager::getNotesInFolder(int folderId) {
    QList<NoteData> notes;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, folder_id, title, body, created_at, updated_at FROM notes WHERE folder_id = ? ORDER BY updated_at DESC");
    q.addBindValue(folderId);
    
    if (q.exec()) {
        while (q.next()) {
            NoteData note;
            note.id = q.value(0).toInt();
            note.folderId = q.value(1).toInt();
            note.title = q.value(2).toString();
            note.body = q.value(3).toString();
            note.createdAt = q.value(4).toDateTime();
            note.updatedAt = q.value(5).toDateTime();
            notes.append(note);
        }
    }
    
    return notes;
}

// Folder operations
int DatabaseManager::createFolder(const QString &name, int parentId) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO folders (name, parent_id) VALUES (?, ?)");
    q.addBindValue(name);
    q.addBindValue(parentId > 0 ? parentId : QVariant());
    
    if (!q.exec()) {
        qWarning() << "Failed to create folder:" << q.lastError();
        return -1;
    }
    
    int folderId = q.lastInsertId().toInt();
    emit folderSaved(folderId);
    return folderId;
}

bool DatabaseManager::updateFolder(int folderId, const QString &name) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE folders SET name = ? WHERE id = ?");
    q.addBindValue(name);
    q.addBindValue(folderId);
    
    if (!q.exec()) {
        qWarning() << "Failed to update folder:" << q.lastError();
        return false;
    }
    
    emit folderSaved(folderId);
    return true;
}

bool DatabaseManager::deleteFolder(int folderId) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM folders WHERE id = ?");
    q.addBindValue(folderId);
    
    if (!q.exec()) {
        qWarning() << "Failed to delete folder:" << q.lastError();
        return false;
    }
    
    emit folderDeleted(folderId);
    return true;
}

FolderData DatabaseManager::getFolder(int folderId) {
    QSqlQuery q(m_db);
    q.prepare("SELECT id, name, parent_id FROM folders WHERE id = ?");
    q.addBindValue(folderId);
    
    FolderData folder;
    folder.id = -1;
    
    if (q.exec() && q.next()) {
        folder.id = q.value(0).toInt();
        folder.name = q.value(1).toString();
        folder.parentId = q.value(2).toInt();
    }
    
    return folder;
}

QList<FolderData> DatabaseManager::getAllFolders() {
    QList<FolderData> folders;
    QSqlQuery q(m_db);
    q.exec("SELECT id, name, parent_id FROM folders ORDER BY name");
    
    while (q.next()) {
        FolderData folder;
        folder.id = q.value(0).toInt();
        folder.name = q.value(1).toString();
        folder.parentId = q.value(2).toInt();
        folders.append(folder);
    }
    
    return folders;
}

// Auto-save functionality
void DatabaseManager::enableAutoSave(bool enabled) {
    m_autoSaveEnabled = enabled;
    if (enabled) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    } else {
        m_autoSaveTimer->stop();
    }
    saveSettings();
}

void DatabaseManager::setAutoSaveInterval(int milliseconds) {
    m_autoSaveInterval = milliseconds;
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
    saveSettings();
}

void DatabaseManager::setNotesDirectory(const QString &path) {
    m_notesDirectory = path;
    ensureNotesDirectoryExists();
    saveSettings();
}

QString DatabaseManager::getNotesDirectory() const {
    return m_notesDirectory;
}

void DatabaseManager::performAutoSave() {
    if (!m_autoSaveEnabled || m_modifiedNotes.isEmpty()) {
        return;
    }
    
    // Save all modified notes
    for (int noteId : m_modifiedNotes) {
        // This would be called from the UI when a note is modified
        // For now, we just emit the signal
        emit autoSaveTriggered();
    }
    
    m_modifiedNotes.clear();
    
    // Restart timer for next auto-save
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

// File system integration
void DatabaseManager::importReadmeFiles(const QString &directory) {
    QDir dir(directory);
    QStringList filters;
    filters << "README.md" << "readme.md" << "*.md";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            
            // Extract title from first line or filename
            QString title = fileInfo.baseName();
            QStringList lines = content.split('\n');
            if (!lines.isEmpty()) {
                QString firstLine = lines[0].trimmed();
                if (firstLine.startsWith("# ")) {
                    title = firstLine.mid(2).trimmed();
                }
            }
            
            // Create note in "Imported" folder
            int folderId = createFolder("Imported");
            createNote(folderId, title, content);
        }
    }
}

void DatabaseManager::exportNoteToFile(int noteId, const QString &filePath) {
    NoteData note = getNote(noteId);
    if (note.id == -1) return;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << note.body;
        file.close();
    }
}

void DatabaseManager::importNoteFromFile(const QString &filePath, int folderId) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        
        QFileInfo fileInfo(filePath);
        QString title = fileInfo.baseName();
        
        createNote(folderId, title, content);
    }
}

// Settings
void DatabaseManager::saveSettings() {
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("notes_directory", m_notesDirectory);
    settings.setValue("auto_save_enabled", m_autoSaveEnabled);
    settings.setValue("auto_save_interval", m_autoSaveInterval);
}

void DatabaseManager::loadSettings() {
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    m_notesDirectory = settings.value("notes_directory", m_notesDirectory).toString();
    m_autoSaveEnabled = settings.value("auto_save_enabled", m_autoSaveEnabled).toBool();
    m_autoSaveInterval = settings.value("auto_save_interval", m_autoSaveInterval).toInt();
    
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

// Model integration
void DatabaseManager::populateFolderModel(QStandardItemModel *model) {
    if (!model) return;
    
    model->clear();
    model->setHorizontalHeaderLabels({"Folders"});
    
    QList<FolderData> folders = getAllFolders();
    QMap<int, QStandardItem*> folderItems;
    
    // Create all folder items
    for (const FolderData &folder : folders) {
        QStandardItem *item = new QStandardItem(folder.name);
        item->setData(folder.id, Qt::UserRole);
        folderItems[folder.id] = item;
    }
    
    // Build hierarchy
    for (const FolderData &folder : folders) {
        QStandardItem *item = folderItems[folder.id];
        if (folder.parentId > 0 && folderItems.contains(folder.parentId)) {
            folderItems[folder.parentId]->appendRow(item);
        } else {
            model->invisibleRootItem()->appendRow(item);
        }
    }
}

void DatabaseManager::populateNotesModel(QStandardItemModel *model, int folderId) {
    if (!model) return;
    
    model->clear();
    model->setColumnCount(1);
    
    QList<NoteData> notes = getNotesInFolder(folderId);
    
    for (const NoteData &note : notes) {
        QStandardItem *item = new QStandardItem(note.title);
        item->setData(note.id, Qt::UserRole);
        item->setData(note.body, Qt::UserRole + 1); // Note content
        item->setData(note.updatedAt, Qt::UserRole + 2); // Date

        
        // Create snippet from body
        QString snippet = note.body;
        QStringList lines = snippet.split('\n');
        for (const QString &line : lines) {
            QString cleanLine = line.trimmed();
            if (cleanLine.startsWith("#")) continue;
            if (cleanLine.isEmpty()) continue;
            if (cleanLine.length() > 100) {
                snippet = cleanLine.left(100) + "...";
            } else {
                snippet = cleanLine;
            }
            break;
        }
        item->setData(snippet, Qt::UserRole + 4); // Snippet
        
        model->appendRow(item);
    }
}

void DatabaseManager::saveNoteFromModel(const QModelIndex &index, QStandardItemModel *model) {
    if (!index.isValid() || !model) return;
    
    QStandardItem *item = model->itemFromIndex(index);
    if (!item) return;
    
    int noteId = item->data(Qt::UserRole).toInt();
    QString title = item->text();
    QString body = item->data(Qt::UserRole + 1).toString();
    
    if (noteId > 0) {
        updateNote(noteId, title, body);
    }
}


