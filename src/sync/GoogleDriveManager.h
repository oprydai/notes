#ifndef GOOGLEDRIVEMANAGER_H
#define GOOGLEDRIVEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QFile>
#include <QDir>

class GoogleDriveManager : public QObject
{
    Q_OBJECT

public:
    explicit GoogleDriveManager(QObject *parent = nullptr);
    ~GoogleDriveManager();

    // Authentication
    bool isAuthenticated() const;
    void authenticate();
    void completeOAuth(const QString &authCode);
    void refreshToken();
    void forceReauthenticate();
    void logout();

    // File operations
    void uploadNote(const QString &noteId, const QString &content, const QString &title);
    void uploadNoteToFolder(const QString &noteId, const QString &content, const QString &title, const QString &folderId);
    void downloadNote(const QString &noteId);
    void deleteNote(const QString &noteId);
    void listNotes();
    void createNote(const QString &title, const QString &content);
    void createFolder(const QString &folderName);

    // Sync operations
    void syncAll();
    void smartSync(); // New smart sync method
    void uploadAllNotes(const QList<QPair<QString, QString>> &notes);
    void uploadFolderStructure(const QList<QPair<QString, QList<QPair<QString, QString>>>> &folderStructure);
    void createSubfoldersAndUploadNotes(const QList<QPair<QString, QList<QPair<QString, QString>>>> &folderStructure);
    void setSyncFolder(const QString &folderId);
    void createNotesFolder();
    void findExistingNotesFolder();
    void createNewNotesFolder();
    QString getNotesFolderId() const;
    bool isStructureChecked() const;
    
    // Smart sync methods
    void checkExistingStructure();
    void syncSingleNote(const QString &noteId, const QString &content, const QString &title, const QString &folderName);
    void updateNoteIfChanged(const QString &noteId, const QString &content, const QString &title, const QString &folderName);
    void clearStructureData();
    
    // Private helper methods for sequential subfolder creation
    void createNextSubfolder();
    void startNoteUploads();
    
    // Utility methods
    QString calculateFileHash(const QString &content);
    QString getRemoteNoteId(const QString &title, const QString &folderName);
    void listSubfolders();
    void listNotesInFolder(const QString &folderId, const QString &folderName);
    void uploadFileContent(const QString &fileId, const QString &content, const QString &title, const QString &noteId);
    void uploadFileContentToSession(const QString &sessionUrl, const QString &content, const QString &title, const QString &noteId);

signals:
    void authenticationChanged(bool authenticated);
    void uploadComplete(const QString &noteId, bool success);
    void downloadComplete(const QString &noteId, const QString &content, bool success);
    void deleteComplete(const QString &noteId, bool success);
    void notesListReceived(const QJsonArray &notes);
    void syncProgress(int current, int total);
    void syncComplete();
    void smartSyncComplete(); // New signal for smart sync completion
    void error(const QString &errorMessage);

private slots:
    void handleAuthResponse(QNetworkReply *reply);
    void handleTokenRefresh(QNetworkReply *reply);
    void handleUploadResponse(QNetworkReply *reply);
    void handleDownloadResponse(QNetworkReply *reply);
    void handleDeleteResponse(QNetworkReply *reply);
    void handleListResponse(QNetworkReply *reply);
    void handleCreateResponse(QNetworkReply *reply);
    void handleCreateFolderResponse(QNetworkReply *reply);
    void handleCreateSubfolderResponse(QNetworkReply *reply);
    void handleFindFolderResponse(QNetworkReply *reply);
    void handleListSubfoldersResponse(QNetworkReply *reply);
    void handleListNotesInFolderResponse(QNetworkReply *reply);
    void handleUploadMetadataResponse(QNetworkReply *reply);
    void handleUploadContentResponse(QNetworkReply *reply);
    void handleUploadSessionResponse(QNetworkReply *reply);

private:
    // OAuth 2.0
    void requestAccessToken(const QString &authCode);
    void saveTokens();
    void loadTokens();
    QString getAuthUrl() const;
    QString getTokenUrl() const;

    // API helpers
    QNetworkRequest createRequest(const QString &url);
    void addAuthHeader(QNetworkRequest &request);
    QString getApiUrl(const QString &endpoint) const;
    
    // Request tracking
    void trackRequest(QNetworkReply *reply, const QString &requestType, const QString &noteId = "");

    // Token management
    void startTokenRefreshTimer();
    void refreshTokenIfNeeded();

    QNetworkAccessManager *m_networkManager;
    
    // OAuth 2.0 credentials
    QString m_clientId;
    QString m_clientSecret;
    QString m_redirectUri;
    
    // Tokens
    QString m_accessToken;
    QString m_refreshToken;
    QDateTime m_tokenExpiry;
    
    // Sync configuration
    QString m_syncFolderId;
    QString m_appDataFolderId;
    QMap<QString, QString> m_subfolderIds;  // Map folder names to their IDs
    
    // Sequential subfolder creation tracking
    QList<QPair<QString, QList<QPair<QString, QString>>>> m_pendingFolderStructure;
    int m_pendingSubfolderIndex;
    
    // Smart sync state tracking
    QMap<QString, QString> m_remoteNoteHashes; // Map note title to hash
    QMap<QString, QString> m_remoteNoteIds;    // Map note title to remote ID
    QMap<QString, QString> m_remoteFolderIds;  // Map folder name to remote ID
    bool m_structureChecked;
    
    // State
    bool m_isAuthenticated;
    QTimer *m_tokenRefreshTimer;
    
    // Constants
    static const QString API_BASE_URL;
    static const QString AUTH_BASE_URL;
    static const QString TOKEN_BASE_URL;
    static const QString SCOPE;
};

#endif // GOOGLEDRIVEMANAGER_H
