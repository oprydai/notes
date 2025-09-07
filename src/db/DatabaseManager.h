#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QTimer>
#include <QSet>

class QStandardItemModel;
class QStandardItem;

struct NoteData {
    int id;
    int folderId;
    QString title;
    QString body;
    QString filepath;  // Path to the .md file
    QDateTime createdAt;
    QDateTime updatedAt;
};

struct FolderData {
    int id;
    QString name;
    int parentId;
};

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager &instance();

    bool open();
    bool initializeSchema();
    bool isOpen() const;
    QSqlDatabase database() const;

    // Note operations
    int createNote(int folderId, const QString &title, const QString &body);
    bool updateNote(int noteId, const QString &title, const QString &body);
    bool deleteNote(int noteId);
    NoteData getNote(int noteId);
    QList<NoteData> getNotesInFolder(int folderId);
    QList<QPair<QString, QString>> getAllNotes();
    QList<NoteData> getAllNotesWithPaths();
    QList<QPair<QString, QList<QPair<QString, QString>>>> getFolderStructure();
    
    // Auto-save tracking
    void markNoteAsModified(int noteId);
    
    // Markdown file operations
    QString generateMarkdownFilename(const QString &title) const;
    bool saveNoteToMarkdownFile(int noteId, const QString &title, const QString &body);
    bool loadNoteFromMarkdownFile(int noteId);
    QString getNoteFilePath(int noteId) const;
    bool ensureNoteFileExists(int noteId);
    bool validateMarkdownFile(int noteId);
    QStringList getMarkdownFileList();
    bool syncNoteWithFile(int noteId);
    
    // Folder operations
    int createFolder(const QString &name, int parentId = -1);
    bool updateFolder(int folderId, const QString &name);
    bool deleteFolder(int folderId);
    FolderData getFolder(int folderId);
    QList<FolderData> getAllFolders();
    
    // Auto-save functionality
    void enableAutoSave(bool enabled = true);
    void setAutoSaveInterval(int milliseconds = 2000);
    void setNotesDirectory(const QString &path);
    QString getNotesDirectory() const;
    
    // File system integration
    void importReadmeFiles(const QString &directory);
    void scanAndImportMarkdownFiles();
    void exportNoteToFile(int noteId, const QString &filePath);
    void importNoteFromFile(const QString &filePath, int folderId);
    
    // Auto-import control
    void setAutoImportEnabled(bool enabled);
    bool isAutoImportEnabled() const;
    void manualImportMarkdownFiles();
    
    // Settings
    void saveSettings();
    void loadSettings();
    
    // Bulk operations
    bool syncAllNotesWithFiles();
    bool recreateAllMarkdownFiles();
    
    // Model integration
    void populateFolderModel(QStandardItemModel *model);
    void populateNotesModel(QStandardItemModel *model, int folderId);
    void saveNoteFromModel(const QModelIndex &index, QStandardItemModel *model);
    
    // Helper functions
    int getOrCreateImportedFolder();

signals:
    void noteSaved(int noteId);
    void noteDeleted(int noteId);
    void folderSaved(int folderId);
    void folderDeleted(int folderId);
    void autoSaveTriggered();

private slots:
    void performAutoSave();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QString databaseFilePath() const;
    QString settingsFilePath() const;
    void createDefaultFolders();
    void ensureNotesDirectoryExists();
    void migrateDatabase();
    void convertExistingNotesToMarkdown();
    
    QSqlDatabase m_db;
    QTimer *m_autoSaveTimer;
    QString m_notesDirectory;
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
    
    // Track modified notes for auto-save
    QSet<int> m_modifiedNotes;
    
    // Auto-import settings
    bool m_autoImportEnabled;
};


