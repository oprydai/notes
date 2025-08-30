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
    QDateTime createdAt;
    QDateTime updatedAt;
    bool pinned;
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
    void exportNoteToFile(int noteId, const QString &filePath);
    void importNoteFromFile(const QString &filePath, int folderId);
    
    // Settings
    void saveSettings();
    void loadSettings();
    
    // Model integration
    void populateFolderModel(QStandardItemModel *model);
    void populateNotesModel(QStandardItemModel *model, int folderId);
    void saveNoteFromModel(const QModelIndex &index, QStandardItemModel *model);

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
    
    QSqlDatabase m_db;
    QTimer *m_autoSaveTimer;
    QString m_notesDirectory;
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
    
    // Track modified notes for auto-save
    QSet<int> m_modifiedNotes;
};


