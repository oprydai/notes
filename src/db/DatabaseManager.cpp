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
      m_autoSaveInterval(2000),
      m_autoImportEnabled(false) {
    
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
        QString errorMsg = QString("Unable to open the notes database. This may be due to file permissions or disk space issues.\n\nError details: %1").arg(m_db.lastError().text());
        emit databaseError(errorMsg);
        qWarning() << "Failed to open database:" << m_db.lastError();
        return false;
    }
    return true;
}

bool DatabaseManager::initializeSchema() {
    if (!isOpen() && !open()) return false;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        QString errorMsg = QString("Database initialization failed. The application may not function correctly.\n\nError details: %1").arg(q.lastError().text());
        emit databaseError(errorMsg);
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
  filepath TEXT,
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
            QString errorMsg = QString("Failed to initialize database structure. The application may not function correctly.\n\nError details: %1").arg(q.lastError().text());
            emit databaseError(errorMsg);
            qWarning() << "Failed to run schema statement:" << stmt << "error:" << q.lastError();
            return false;
        }
    }
    
    // Create default folders if none exist
    createDefaultFolders();
    
    // Migrate existing database if needed
    migrateDatabase();
    
    // Load settings
    loadSettings();
    
    // Ensure notes directory exists
    ensureNotesDirectoryExists();
    
    // Scan for existing markdown files and import them (only if auto-import is enabled)
    if (m_autoImportEnabled) {
        scanAndImportMarkdownFiles();
    }
    
    return true;
}

void DatabaseManager::migrateDatabase() {
    QSqlQuery q(m_db);
    
    // Check if filepath column exists
    if (!q.exec("PRAGMA table_info(notes)")) {
        qWarning() << "Failed to check table schema:" << q.lastError();
        return;
    }
    
    bool hasFilepathColumn = false;
    while (q.next()) {
        if (q.value(1).toString() == "filepath") {
            hasFilepathColumn = true;
            break;
        }
    }
    
    // Add filepath column if it doesn't exist
    if (!hasFilepathColumn) {
        if (!q.exec("ALTER TABLE notes ADD COLUMN filepath TEXT")) {
            qWarning() << "Failed to add filepath column:" << q.lastError();
            return;
        }
        qDebug() << "Added filepath column to notes table";
        
        // Convert existing notes to markdown files
        convertExistingNotesToMarkdown();
    }
}

void DatabaseManager::convertExistingNotesToMarkdown() {
    QSqlQuery q(m_db);
    q.exec("SELECT id, title, body FROM notes WHERE filepath IS NULL OR filepath = ''");
    
    while (q.next()) {
        int noteId = q.value(0).toInt();
        QString title = q.value(1).toString();
        QString body = q.value(2).toString();
        
        // Save existing note to markdown file
        saveNoteToMarkdownFile(noteId, title, body);
    }
    
    qDebug() << "Converted existing notes to markdown format";
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
    q.prepare("INSERT INTO notes (folder_id, title, body, filepath, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(folderId);
    q.addBindValue(title);
    q.addBindValue(body);
    q.addBindValue(QString()); // filepath will be set when saving to markdown
    q.addBindValue(QDateTime::currentDateTime());
    q.addBindValue(QDateTime::currentDateTime());
    
    if (!q.exec()) {
        QString errorMsg = QString("Unable to create the note. Please check if you have sufficient disk space and try again.\n\nError details: %1").arg(q.lastError().text());
        emit operationFailed("Create Note", errorMsg);
        qWarning() << "Failed to create note:" << q.lastError();
        return -1;
    }
    
    int noteId = q.lastInsertId().toInt();
    
    // Automatically save to markdown file
    if (noteId > 0) {
        saveNoteToMarkdownFile(noteId, title, body);
    }
    
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
        QString errorMsg = QString("Unable to save changes to the note. Please try again.\n\nError details: %1").arg(q.lastError().text());
        emit operationFailed("Update Note", errorMsg);
        qWarning() << "Failed to update note:" << q.lastError();
        return false;
    }
    
    // Automatically save to markdown file
    saveNoteToMarkdownFile(noteId, title, body);
    
    emit noteSaved(noteId);
    return true;
}



bool DatabaseManager::deleteNote(int noteId) {
    // Get note info before deletion to remove markdown file
    NoteData note = getNote(noteId);
    
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM notes WHERE id = ?");
    q.addBindValue(noteId);
    
    if (!q.exec()) {
        QString errorMsg = QString("Unable to delete the note. Please try again.\n\nError details: %1").arg(q.lastError().text());
        emit operationFailed("Delete Note", errorMsg);
        qWarning() << "Failed to delete note:" << q.lastError();
        return false;
    }
    
    // Remove markdown file if it exists
    if (!note.filepath.isEmpty()) {
        QString filePath = m_notesDirectory + QDir::separator() + note.filepath;
        QFile file(filePath);
        if (file.exists()) {
            if (!file.remove()) {
                qWarning() << "Failed to remove markdown file:" << filePath;
            }
        }
    }
    
    emit noteDeleted(noteId);
    return true;
}

NoteData DatabaseManager::getNote(int noteId) {
    QSqlQuery q(m_db);
    q.prepare("SELECT id, folder_id, title, body, filepath, created_at, updated_at FROM notes WHERE id = ?");
    q.addBindValue(noteId);
    
    NoteData note;
    note.id = -1;
    
    if (q.exec() && q.next()) {
        note.id = q.value(0).toInt();
        note.folderId = q.value(1).toInt();
        note.title = q.value(2).toString();
        note.body = q.value(3).toString();
        note.filepath = q.value(4).toString();
        note.createdAt = q.value(5).toDateTime();
        note.updatedAt = q.value(6).toDateTime();
    }
    
    return note;
}

QList<NoteData> DatabaseManager::getNotesInFolder(int folderId) {
    QList<NoteData> notes;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, folder_id, title, body, filepath, created_at, updated_at FROM notes WHERE folder_id = ? ORDER BY updated_at DESC");
    q.addBindValue(folderId);
    
    if (q.exec()) {
        while (q.next()) {
            NoteData note;
            note.id = q.value(0).toInt();
            note.folderId = q.value(1).toInt();
            note.title = q.value(2).toString();
            note.body = q.value(3).toString();
            note.filepath = q.value(4).toString();
            note.createdAt = q.value(5).toDateTime();
            note.updatedAt = q.value(6).toDateTime();
            notes.append(note);
        }
    }
    
    return notes;
}

QList<QPair<QString, QString>> DatabaseManager::getAllNotes() {
    QList<QPair<QString, QString>> notes;
    QSqlQuery q(m_db);
    q.exec("SELECT title, body FROM notes ORDER BY updated_at DESC");
    
    while (q.next()) {
        QString title = q.value(0).toString();
        QString body = q.value(1).toString();
        notes.append(qMakePair(title, body));
    }
    
    return notes;
}

QList<NoteData> DatabaseManager::getAllNotesWithPaths() {
    QList<NoteData> notes;
    QSqlQuery q(m_db);
    q.exec("SELECT id, folder_id, title, body, filepath, created_at, updated_at FROM notes ORDER BY updated_at DESC");
    
    while (q.next()) {
        NoteData note;
        note.id = q.value(0).toInt();
        note.folderId = q.value(1).toInt();
        note.title = q.value(2).toString();
        note.body = q.value(3).toString();
        note.filepath = q.value(4).toString();
        note.createdAt = q.value(5).toDateTime();
        note.updatedAt = q.value(6).toDateTime();
        notes.append(note);
    }
    
    return notes;
}

QList<QPair<QString, QList<QPair<QString, QString>>>> DatabaseManager::getFolderStructure() {
    QList<QPair<QString, QList<QPair<QString, QString>>>> folderStructure;
    
    // Get all folders
    QList<FolderData> folders = getAllFolders();
    
    for (const FolderData &folder : folders) {
        QString folderName = folder.name;
        QList<QPair<QString, QString>> notesInFolder;
        
        // Get notes in this folder
        QList<NoteData> notes = getNotesInFolder(folder.id);
        for (const NoteData &note : notes) {
            notesInFolder.append(qMakePair(note.title, note.body));
        }
        
        folderStructure.append(qMakePair(folderName, notesInFolder));
    }
    
    return folderStructure;
}

// Folder operations
int DatabaseManager::createFolder(const QString &name, int parentId) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO folders (name, parent_id) VALUES (?, ?)");
    q.addBindValue(name);
    q.addBindValue(parentId > 0 ? parentId : QVariant());
    
    if (!q.exec()) {
        QString errorMsg = QString("Unable to create the folder. Please try again.\n\nError details: %1").arg(q.lastError().text());
        emit operationFailed("Create Folder", errorMsg);
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
        QString errorMsg = QString("Unable to rename the folder. Please try again.\n\nError details: %1").arg(q.lastError().text());
        emit operationFailed("Update Folder", errorMsg);
        qWarning() << "Failed to update folder:" << q.lastError();
        return false;
    }
    
    emit folderSaved(folderId);
    return true;
}

bool DatabaseManager::deleteFolder(int folderId) {
    // Get all notes in this folder before deletion to remove markdown files
    QList<NoteData> notes = getNotesInFolder(folderId);
    
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM folders WHERE id = ?");
    q.addBindValue(folderId);
    
    if (!q.exec()) {
        QString errorMsg = QString("Unable to delete the folder. Please try again.\n\nError details: %1").arg(q.lastError().text());
        emit operationFailed("Delete Folder", errorMsg);
        qWarning() << "Failed to delete folder:" << q.lastError();
        return false;
    }
    
    // Remove all markdown files for notes in this folder
    for (const NoteData &note : notes) {
        if (!note.filepath.isEmpty()) {
            QString filePath = m_notesDirectory + QDir::separator() + note.filepath;
            QFile file(filePath);
            if (file.exists()) {
                if (!file.remove()) {
                    qWarning() << "Failed to remove markdown file:" << filePath;
                }
            }
        }
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
    
    // Save all modified notes to markdown files
    for (int noteId : m_modifiedNotes) {
        NoteData note = getNote(noteId);
        if (note.id != -1) {
            saveNoteToMarkdownFile(noteId, note.title, note.body);
        }
        emit autoSaveTriggered();
    }
    
    m_modifiedNotes.clear();
    
    // Restart timer for next auto-save
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

bool DatabaseManager::syncAllNotesWithFiles() {
    QList<NoteData> notes = getAllNotesWithPaths();
    bool allSynced = true;
    
    for (const NoteData &note : notes) {
        if (!syncNoteWithFile(note.id)) {
            allSynced = false;
            qWarning() << "Failed to sync note:" << note.id << note.title;
        }
    }
    
    return allSynced;
}

bool DatabaseManager::recreateAllMarkdownFiles() {
    QList<NoteData> notes = getAllNotesWithPaths();
    bool allRecreated = true;
    
    for (const NoteData &note : notes) {
        if (!saveNoteToMarkdownFile(note.id, note.title, note.body)) {
            allRecreated = false;
            qWarning() << "Failed to recreate markdown file for note:" << note.id << note.title;
        }
    }
    
    return allRecreated;
}

void DatabaseManager::markNoteAsModified(int noteId) {
    if (m_autoSaveEnabled) {
        m_modifiedNotes.insert(noteId);
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
            
                // Create note in "Imported" folder (create only if doesn't exist)
    int folderId = getOrCreateImportedFolder();
    if (folderId > 0) {
        createNote(folderId, title, content);
    }
        }
    }
}

void DatabaseManager::scanAndImportMarkdownFiles() {
    QDir dir(m_notesDirectory);
    QStringList filters;
    filters << "*.md";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    for (const QFileInfo &fileInfo : files) {
        // Check if this file is already imported
        QString filename = fileInfo.fileName();
        QSqlQuery q(m_db);
        q.prepare("SELECT id FROM notes WHERE filepath = ?");
        q.addBindValue(filename);
        
        if (q.exec() && q.next()) {
            // File already imported, skip
            continue;
        }
        
        // Import new markdown file
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            
            // Parse markdown content
            QString title = fileInfo.baseName();
            QString body = content;
            
            // Try to extract title from first heading
            QStringList lines = content.split('\n');
            for (const QString &line : lines) {
                QString trimmedLine = line.trimmed();
                if (trimmedLine.startsWith("# ")) {
                    title = trimmedLine.mid(2).trimmed();
                    // Remove the title line from body
                    body = content.replace(line, "").trimmed();
                    break;
                }
            }
            
            // Create note in "Imported" folder (create only if doesn't exist)
            int folderId = getOrCreateImportedFolder();
            if (folderId > 0) {
                createNote(folderId, title, body);
            }
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
    settings.setValue("auto_import_enabled", m_autoImportEnabled);
}

void DatabaseManager::loadSettings() {
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    m_notesDirectory = settings.value("notes_directory", m_notesDirectory).toString();
    m_autoSaveEnabled = settings.value("auto_save_enabled", m_autoSaveEnabled).toBool();
    m_autoSaveInterval = settings.value("auto_save_interval", m_autoSaveInterval).toInt();
    m_autoImportEnabled = settings.value("auto_import_enabled", m_autoImportEnabled).toBool();
    
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

// Markdown file operations
QString DatabaseManager::generateMarkdownFilename(const QString &title) const {
    // Generate a safe filename from the title
    QString filename = title;
    
    // Remove or replace invalid characters
    filename = filename.replace(QRegularExpression("[<>:\"/\\|?*]"), "_");
    filename = filename.replace(QRegularExpression("\\s+"), "_");
    filename = filename.trimmed();
    
    // Ensure it's not empty
    if (filename.isEmpty()) {
        filename = "untitled_note";
    }
    
    // Limit length to avoid filesystem issues
    if (filename.length() > 50) {
        filename = filename.left(50);
    }
    
    // Add timestamp to ensure uniqueness
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    filename = QString("%1_%2.md").arg(filename).arg(timestamp);
    
    return filename;
}

bool DatabaseManager::saveNoteToMarkdownFile(int noteId, const QString &title, const QString &body) {
    NoteData note = getNote(noteId);
    if (note.id == -1) return false;
    
    // Generate filename if not exists
    QString filename;
    if (note.filepath.isEmpty()) {
        filename = generateMarkdownFilename(title);
        note.filepath = filename;
        
        // Update database with filepath
        QSqlQuery q(m_db);
        q.prepare("UPDATE notes SET filepath = ? WHERE id = ?");
        q.addBindValue(filename);
        q.addBindValue(noteId);
        if (!q.exec()) {
            qWarning() << "Failed to update note filepath:" << q.lastError();
            return false;
        }
    } else {
        filename = note.filepath;
    }
    
    // Create full file path
    QString filePath = m_notesDirectory + QDir::separator() + filename;
    
    // Write markdown file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // Write frontmatter
    out << "---\n";
    out << "title: \"" << title << "\"\n";
    out << "created: " << note.createdAt.toString(Qt::ISODate) << "\n";
    out << "modified: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "folder_id: " << note.folderId << "\n";
    out << "---\n\n";
    
    // Write note body
    out << body;
    
    file.close();
    return true;
}

bool DatabaseManager::loadNoteFromMarkdownFile(int noteId) {
    NoteData note = getNote(noteId);
    if (note.id == -1 || note.filepath.isEmpty()) return false;
    
    QString filePath = m_notesDirectory + QDir::separator() + note.filepath;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open markdown file:" << filePath;
        return false;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString content = in.readAll();
    file.close();
    
    // Parse frontmatter and content
    QStringList lines = content.split('\n');
    QString body;
    bool inFrontmatter = false;
    bool frontmatterEnded = false;
    
    for (const QString &line : lines) {
        if (line.trimmed() == "---") {
            if (!inFrontmatter) {
                inFrontmatter = true;
            } else {
                frontmatterEnded = true;
                continue;
            }
        } else if (inFrontmatter && !frontmatterEnded) {
            // Parse frontmatter (could be extended for more metadata)
            continue;
        } else if (frontmatterEnded) {
            body += line + "\n";
        }
    }
    
    // Update note body in database
    return updateNote(noteId, note.title, body.trimmed());
}

QString DatabaseManager::getNoteFilePath(int noteId) const {
    // Query database directly to avoid calling non-const method
    QSqlQuery q(m_db);
    q.prepare("SELECT filepath FROM notes WHERE id = ?");
    q.addBindValue(noteId);
    
    if (q.exec() && q.next()) {
        QString filepath = q.value(0).toString();
        if (!filepath.isEmpty()) {
            return m_notesDirectory + QDir::separator() + filepath;
        }
    }
    
    return QString();
}

bool DatabaseManager::ensureNoteFileExists(int noteId) {
    NoteData note = getNote(noteId);
    if (note.id == -1) return false;
    
    if (note.filepath.isEmpty()) {
        // Create markdown file if it doesn't exist
        return saveNoteToMarkdownFile(noteId, note.title, note.body);
    }
    
    QString filePath = m_notesDirectory + QDir::separator() + note.filepath;
    QFile file(filePath);
    
    if (!file.exists()) {
        // Recreate file if it was deleted
        return saveNoteToMarkdownFile(noteId, note.title, note.body);
    }
    
    return true;
}

bool DatabaseManager::validateMarkdownFile(int noteId) {
    NoteData note = getNote(noteId);
    if (note.id == -1 || note.filepath.isEmpty()) return false;
    
    QString filePath = m_notesDirectory + QDir::separator() + note.filepath;
    QFile file(filePath);
    
    if (!file.exists()) return false;
    
    // Check if file is readable and contains valid content
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    // Basic validation - check if content is not empty
    return !content.trimmed().isEmpty();
}

QStringList DatabaseManager::getMarkdownFileList() {
    QStringList fileList;
    QDir dir(m_notesDirectory);
    
    if (!dir.exists()) return fileList;
    
    QStringList filters;
    filters << "*.md";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    for (const QFileInfo &fileInfo : files) {
        fileList.append(fileInfo.fileName());
    }
    
    return fileList;
}

bool DatabaseManager::syncNoteWithFile(int noteId) {
    NoteData note = getNote(noteId);
    if (note.id == -1) return false;
    
    // Check if markdown file exists and is newer than database
    QString filePath = m_notesDirectory + QDir::separator() + note.filepath;
    QFile file(filePath);
    
    if (!file.exists()) {
        // File doesn't exist, recreate it
        return saveNoteToMarkdownFile(noteId, note.title, note.body);
    }
    
    QFileInfo fileInfo(filePath);
    if (fileInfo.lastModified() > note.updatedAt) {
        // File is newer, load from file
        return loadNoteFromMarkdownFile(noteId);
    }
    
    return true;
}

int DatabaseManager::getOrCreateImportedFolder() {
    // First try to find existing "Imported" folder
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM folders WHERE name = ?");
    q.addBindValue("Imported");
    
    if (q.exec() && q.next()) {
        // Folder already exists, return its ID
        return q.value(0).toInt();
    }
    
    // Folder doesn't exist, create it
    return createFolder("Imported");
}

void DatabaseManager::setAutoImportEnabled(bool enabled) {
    m_autoImportEnabled = enabled;
}

bool DatabaseManager::isAutoImportEnabled() const {
    return m_autoImportEnabled;
}

void DatabaseManager::manualImportMarkdownFiles() {
    // Force import even if auto-import is disabled
    scanAndImportMarkdownFiles();
}


