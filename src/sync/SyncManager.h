#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QMap>
#include "GoogleDriveManager.h"

class DatabaseManager; // Forward declaration

class SyncManager : public QObject
{
    Q_OBJECT

public:
    explicit SyncManager(DatabaseManager *dbManager, QObject *parent = nullptr);
    ~SyncManager();

    // Sync control
    void startAutoSync(int intervalMinutes = 15);
    void stopAutoSync();
    void syncNow();
    void setAutoSyncEnabled(bool enabled);

    // Status
    bool isSyncing() const;
    QString getLastSyncTime() const;
    QString getSyncStatus() const;
    
    // Authentication
    bool isAuthenticated() const;
    void authenticate();
    void completeOAuth(const QString &authCode);
    void forceReauthenticate();
    void logout();

    // Manual operations
    void uploadAllNotes();
    void downloadAllNotes();
    void syncAllNotes();
    void smartSync(); // New smart sync method
    void syncSingleNote(const QString &noteId, const QString &content, const QString &title, const QString &folderName);
    void handleNoteChanged(const QString &noteId, const QString &content, const QString &title, const QString &folderName);
    void resolveConflicts();
    void clearStructureData();

signals:
    void syncStarted();
    void syncProgress(int current, int total);
    void syncCompleted();
    void syncFailed(const QString &error);
    void noteUploaded(const QString &noteId, bool success);
    void noteDownloaded(const QString &noteId, bool success);
    void conflictDetected(const QString &noteId, const QString &localContent, const QString &remoteContent);
    void authenticationChanged(bool authenticated);

private slots:
    void onAuthenticationChanged(bool authenticated);
    void onNotesListReceived(const QJsonArray &notes);
    void onUploadComplete(const QString &noteId, bool success);
    void onDownloadComplete(const QString &noteId, const QString &content, bool success);
    void onDeleteComplete(const QString &noteId, bool success);
    void onFolderCreated();
    void onSmartSyncComplete();
    void onError(const QString &errorMessage);
    void performAutoSync();

private:
    // Sync logic
    void compareNotes(const QJsonArray &remoteNotes);
    void uploadLocalNote(const QString &noteId);
    void downloadRemoteNote(const QString &noteId);
    void createRemoteNote(const QString &title, const QString &content);
    void deleteRemoteNote(const QString &noteId);
    
    // Conflict resolution
    void resolveNoteConflict(const QString &noteId, const QString &localContent, const QString &remoteContent);
    bool shouldUseLocalVersion(const QString &localContent, const QString &remoteContent);
    
    // Helper methods
    QString getNoteTitleFromFilename(const QString &filename) const;
    QString getRemoteNoteId(const QString &localNoteId) const;
    QString getLocalNoteId(const QString &remoteNoteId) const;
    void updateSyncTimestamp();
    void loadSyncState();
    void saveSyncState();
    void checkSyncCompletion();

    DatabaseManager *m_dbManager;
    GoogleDriveManager *m_driveManager;
    
    // Sync state
    bool m_isSyncing;
    bool m_autoSyncEnabled;
    QDateTime m_lastSyncTime;
    QTimer *m_autoSyncTimer;
    
    // Mapping between local and remote note IDs
    QMap<QString, QString> m_localToRemoteIdMap;
    QMap<QString, QString> m_remoteToLocalIdMap;
    
    // Pending operations
    QList<QString> m_pendingUploads;
    QList<QString> m_pendingDownloads;
    QList<QString> m_pendingDeletes;
    
    // Sync configuration
    QString m_syncFolderId;
    int m_autoSyncInterval;
    bool m_syncCompletedEmitted = false;  // Prevent duplicate syncCompleted emissions
};

#endif // SYNCMANAGER_H
