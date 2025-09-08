#include "GoogleDriveManager.h"
#include "ConfigLoader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QCryptographicHash>

// Helper function to convert technical error messages to user-friendly ones
static QString makeUserFriendlyError(const QString& technicalError) {
    if (technicalError.contains("No refresh token available")) {
        return "Google Drive connection has expired. Please reconnect to Google Drive.";
    }
    if (technicalError.contains("Not authenticated")) {
        return "Please connect to Google Drive first to sync your notes.";
    }
    if (technicalError.contains("No sync folder set")) {
        return "Google Drive sync folder is not configured. Please check your sync settings.";
    }
    if (technicalError.contains("Note content is empty")) {
        return "Cannot sync empty notes. Please add some content to your note first.";
    }
    if (technicalError.contains("Note content is only whitespace")) {
        return "Cannot sync notes with only spaces. Please add some content to your note first.";
    }
    if (technicalError.contains("No access token available")) {
        return "Google Drive authentication has expired. Please reconnect to Google Drive.";
    }
    if (technicalError.contains("Access token expired")) {
        return "Google Drive connection has expired. Please reconnect to Google Drive.";
    }
    if (technicalError.contains("Authentication failed")) {
        return "Failed to connect to Google Drive. Please check your internet connection and try again.";
    }
    if (technicalError.contains("Token refresh failed")) {
        return "Google Drive connection has expired. Please reconnect to Google Drive.";
    }
    if (technicalError.contains("Failed to list notes")) {
        return "Unable to retrieve notes from Google Drive. Please check your connection and try again.";
    }
    if (technicalError.contains("Failed to create folder")) {
        return "Unable to create folder in Google Drive. Please check your permissions and try again.";
    }
    if (technicalError.contains("Failed to create subfolder")) {
        return "Unable to create subfolder in Google Drive. Please check your permissions and try again.";
    }
    if (technicalError.contains("Failed to search for folder")) {
        return "Unable to find folder in Google Drive. Please check your sync settings.";
    }
    if (technicalError.contains("Failed to list subfolders")) {
        return "Unable to retrieve folders from Google Drive. Please check your connection and try again.";
    }
    if (technicalError.contains("Failed to list notes in folder")) {
        return "Unable to retrieve notes from Google Drive folder. Please check your connection and try again.";
    }
    
    // For network errors, provide a generic user-friendly message
    if (technicalError.contains("errorString") || technicalError.contains("network")) {
        return "Unable to connect to Google Drive. Please check your internet connection and try again.";
    }
    
    // For other technical errors, provide a generic message
    return "A sync error occurred. Please try again or reconnect to Google Drive.";
}

// Constants
const QString GoogleDriveManager::API_BASE_URL = "https://www.googleapis.com/drive/v3";
const QString GoogleDriveManager::AUTH_BASE_URL = "https://accounts.google.com/oauth/authorize";
const QString GoogleDriveManager::TOKEN_BASE_URL = "https://oauth2.googleapis.com/token";
const QString GoogleDriveManager::SCOPE = "https://www.googleapis.com/auth/drive.file";

GoogleDriveManager::GoogleDriveManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_isAuthenticated(false)
    , m_tokenRefreshTimer(new QTimer(this))
    , m_pendingSubfolderIndex(0)
    , m_structureChecked(false)
{
    // Load credentials from ConfigLoader
    m_clientId = ConfigLoader::instance().getClientId();
    m_clientSecret = ConfigLoader::instance().getClientSecret();
    m_redirectUri = ConfigLoader::instance().getRedirectUri();
    
    // Load saved tokens
    loadTokens();
    
    // Set up token refresh timer
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, &GoogleDriveManager::refreshTokenIfNeeded);
    startTokenRefreshTimer();
    
    // Connect network replies
    connect(m_networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        // Route responses based on stored request info
        QString requestType = reply->property("requestType").toString();
        if (requestType == "auth") {
            handleAuthResponse(reply);
        } else if (requestType == "token_refresh") {
            handleTokenRefresh(reply);
        } else if (requestType == "upload") {
            handleUploadResponse(reply);
        } else if (requestType == "upload_metadata") {
            handleUploadMetadataResponse(reply);
        } else if (requestType == "upload_content") {
            handleUploadContentResponse(reply);
        } else if (requestType == "upload_session") {
            handleUploadSessionResponse(reply);
        } else if (requestType == "download") {
            handleDownloadResponse(reply);
        } else if (requestType == "delete") {
            handleDeleteResponse(reply);
        } else if (requestType == "list") {
            handleListResponse(reply);
        } else if (requestType == "create") {
            handleCreateResponse(reply);
        } else if (requestType == "create_folder") {
            handleCreateFolderResponse(reply);
        } else if (requestType == "create_subfolder") {
            handleCreateSubfolderResponse(reply);
        } else if (requestType == "find_folder") {
            handleFindFolderResponse(reply);
        } else if (requestType == "list_subfolders") {
            handleListSubfoldersResponse(reply);
        } else if (requestType == "list_notes_in_folder") {
            handleListNotesInFolderResponse(reply);
        }
    });
}

GoogleDriveManager::~GoogleDriveManager()
{
    saveTokens();
}

bool GoogleDriveManager::isAuthenticated() const
{
    // Check if token is expired and refresh if needed
    if (m_isAuthenticated && !m_accessToken.isEmpty()) {
        QDateTime now = QDateTime::currentDateTime();
        if (m_tokenExpiry.isValid() && now >= m_tokenExpiry) {
            qDebug() << "Token expired, attempting refresh...";
            // Trigger token refresh
            const_cast<GoogleDriveManager*>(this)->refreshTokenIfNeeded();
        }
    }
    
    bool authenticated = m_isAuthenticated && !m_accessToken.isEmpty();
    qDebug() << "isAuthenticated() called:";
    qDebug() << "  m_isAuthenticated:" << m_isAuthenticated;
    qDebug() << "  m_accessToken.isEmpty():" << m_accessToken.isEmpty();
    qDebug() << "  m_accessToken.length():" << m_accessToken.length();
    qDebug() << "  returning:" << authenticated;
    return authenticated;
}

void GoogleDriveManager::authenticate()
{
    if (m_isAuthenticated) {
        return;
    }
    
    // Open browser for OAuth flow
    QString authUrl = getAuthUrl();
    QDesktopServices::openUrl(QUrl(authUrl));
    
    // Note: For desktop apps, user will need to copy the authorization code
    // and paste it into a dialog. This is a simplified approach.
    // In a production app, you might want to use a local server for the redirect.
}

void GoogleDriveManager::completeOAuth(const QString &authCode)
{
    qDebug() << "Completing OAuth flow with auth code:" << authCode.mid(0, 10) + "...";
    
    // Exchange the authorization code for access tokens
    requestAccessToken(authCode);
}

QString GoogleDriveManager::getAuthUrl() const
{
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("scope", SCOPE);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("access_type", "offline");
    query.addQueryItem("prompt", "consent");
    
    return QString("%1?%2").arg(AUTH_BASE_URL, query.toString());
}

void GoogleDriveManager::requestAccessToken(const QString &authCode)
{
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("code", authCode);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("redirect_uri", m_redirectUri);
    
    QNetworkRequest request{QUrl(TOKEN_BASE_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    trackRequest(reply, "auth");
}

void GoogleDriveManager::refreshToken()
{
    if (m_refreshToken.isEmpty()) {
        emit error(makeUserFriendlyError("No refresh token available"));
        return;
    }
    
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("refresh_token", m_refreshToken);
    query.addQueryItem("grant_type", "refresh_token");
    
    QNetworkRequest request{QUrl(TOKEN_BASE_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    trackRequest(reply, "token_refresh");
}

void GoogleDriveManager::logout()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenExpiry = QDateTime();
    m_isAuthenticated = false;
    saveTokens();
    emit authenticationChanged(false);
}

void GoogleDriveManager::forceReauthenticate()
{
    qDebug() << "Forcing re-authentication...";
    
    // Clear current tokens
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenExpiry = QDateTime();
    m_isAuthenticated = false;
    m_structureChecked = false;
    
    // Clear stored sync state
    m_remoteNoteHashes.clear();
    m_remoteNoteIds.clear();
    m_remoteFolderIds.clear();
    m_subfolderIds.clear();
    
    // Save cleared state
    saveTokens();
    
    // Emit authentication changed
    emit authenticationChanged(false);
    
    // Start new authentication flow
    authenticate();
}

void GoogleDriveManager::uploadNote(const QString &noteId, const QString &content, const QString &title)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    if (m_syncFolderId.isEmpty()) {
        emit error(makeUserFriendlyError("No sync folder set for upload"));
        return;
    }
    
    qDebug() << "Uploading note:" << title << "to folder:" << m_syncFolderId;
    qDebug() << "Note content length:" << content.length();
    qDebug() << "Note content preview:" << content.mid(0, 100) + "...";
    qDebug() << "Note content (full):" << content;
    
    // Validate content before proceeding
    if (content.isEmpty()) {
        qDebug() << "ERROR: Content is empty, cannot upload note!";
        emit error(makeUserFriendlyError("Note content is empty"));
        return;
    }
    
    if (content.trimmed().isEmpty()) {
        qDebug() << "ERROR: Content is only whitespace, cannot upload note!";
        emit error(makeUserFriendlyError("Note content is only whitespace"));
        return;
    }
    
    // Check if content is just the title (which would indicate an error)
    if (content.trimmed() == title.trimmed()) {
        qDebug() << "ERROR: Content is just the title, this indicates a serious error!";
        qDebug() << "Title:" << title;
        qDebug() << "Content:" << content;
        emit error("Note content is just the title - this indicates an error in content passing");
        return;
    }
    
    // Use resumable upload instead of multipart for better reliability
    QString url = noteId.isEmpty() ? 
        QString("%1/files?uploadType=resumable").arg(API_BASE_URL) :
        QString("%1/files/%2?uploadType=resumable").arg(API_BASE_URL, noteId);
    
    QNetworkRequest request{QUrl(url)};
    addAuthHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Create metadata JSON
    QJsonObject metadata;
    metadata["name"] = title + ".md";
    metadata["parents"] = QJsonArray() << m_syncFolderId;
    metadata["mimeType"] = "text/markdown";
    
    if (!noteId.isEmpty()) {
        // Update existing file
        metadata["id"] = noteId;
    }
    
    QByteArray metadataJson = QJsonDocument(metadata).toJson();
    qDebug() << "Upload metadata:" << QString::fromUtf8(metadataJson);
    
    // First, create the file with metadata
    QNetworkReply *reply;
    if (noteId.isEmpty()) {
        // For new files, POST the metadata
        reply = m_networkManager->post(request, metadataJson);
    } else {
        // For updates, PUT the metadata
        reply = m_networkManager->put(request, metadataJson);
    }
    
    // Store the content for the second request
    reply->setProperty("content", content);
    reply->setProperty("title", title);
    reply->setProperty("folderId", m_syncFolderId);
    reply->setProperty("noteId", noteId);
    
    trackRequest(reply, "upload_metadata", noteId);
    
    qDebug() << "Upload metadata request sent for note:" << title;
}

void GoogleDriveManager::uploadNoteToFolder(const QString &noteId, const QString &content, const QString &title, const QString &folderId)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    if (folderId.isEmpty()) {
        emit error("No folder ID specified for upload");
        return;
    }
    
    qDebug() << "Uploading note:" << title << "to specific folder:" << folderId;
    qDebug() << "Note content length:" << content.length();
    qDebug() << "Note content preview:" << content.mid(0, 100) + "...";
    qDebug() << "Note content (full):" << content;
    
    // Validate content before proceeding
    if (content.isEmpty()) {
        qDebug() << "ERROR: Content is empty, cannot upload note!";
        emit error(makeUserFriendlyError("Note content is empty"));
        return;
    }
    
    if (content.trimmed().isEmpty()) {
        qDebug() << "ERROR: Content is only whitespace, cannot upload note!";
        emit error(makeUserFriendlyError("Note content is only whitespace"));
        return;
    }
    
    // Use resumable upload instead of multipart for better reliability
    QString url = noteId.isEmpty() ? 
        QString("%1/files?uploadType=resumable").arg(API_BASE_URL) :
        QString("%1/files/%2?uploadType=resumable").arg(API_BASE_URL, noteId);
    
    QNetworkRequest request{QUrl(url)};
    addAuthHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Create metadata JSON
    QJsonObject metadata;
    metadata["name"] = title + ".md";
    metadata["parents"] = QJsonArray() << folderId;
    metadata["mimeType"] = "text/markdown";
    
    if (!noteId.isEmpty()) {
        // Update existing file
        metadata["id"] = noteId;
    }
    
    QByteArray metadataJson = QJsonDocument(metadata).toJson();
    qDebug() << "Upload metadata:" << QString::fromUtf8(metadataJson);
    
    // First, create the file with metadata
    QNetworkReply *reply;
    if (noteId.isEmpty()) {
        // For new files, POST the metadata
        reply = m_networkManager->post(request, metadataJson);
    } else {
        // For updates, PUT the metadata
        reply = m_networkManager->put(request, metadataJson);
    }
    
    // Store the content for the second request
    reply->setProperty("content", content);
    reply->setProperty("title", title);
    reply->setProperty("folderId", folderId);
    reply->setProperty("noteId", noteId);
    
    trackRequest(reply, "upload_metadata", noteId);
    
    qDebug() << "Upload metadata request sent for note:" << title << "to folder:" << folderId;
}

void GoogleDriveManager::downloadNote(const QString &noteId)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    QString url = QString("%1/files/%2?alt=media").arg(API_BASE_URL, noteId);
    QNetworkRequest request{QUrl(url)};
    addAuthHeader(request);
    
    QNetworkReply *reply = m_networkManager->get(request);
    trackRequest(reply, "download", noteId);
}

void GoogleDriveManager::deleteNote(const QString &noteId)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    QString url = QString("%1/files/%2").arg(API_BASE_URL, noteId);
    QNetworkRequest request{QUrl(url)};
    addAuthHeader(request);
    
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    trackRequest(reply, "delete", noteId);
}

void GoogleDriveManager::listNotes()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // Only list notes if we have a sync folder
    if (m_syncFolderId.isEmpty()) {
        qDebug() << "No sync folder ID set, cannot list notes yet";
        return;
    }
    
    // Build the query properly
    QUrl url(API_BASE_URL + "/files");
    QUrlQuery query;
    query.addQueryItem("q", QString("'%1' in parents and trashed=false").arg(m_syncFolderId));
    query.addQueryItem("fields", "files(id,name,modifiedTime,size)");
    url.setQuery(query);
    
    QNetworkRequest request(url);
    addAuthHeader(request);
    
    qDebug() << "Listing notes from folder:" << m_syncFolderId;
    qDebug() << "URL:" << url.toString();
    
    QNetworkReply *reply = m_networkManager->get(request);
    trackRequest(reply, "list");
}

void GoogleDriveManager::createNote(const QString &title, const QString &content)
{
    uploadNote("", content, title);
}

void GoogleDriveManager::createFolder(const QString &folderName)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    if (m_syncFolderId.isEmpty()) {
        emit error("No sync folder set for folder creation");
        return;
    }
    
    qDebug() << "Creating folder:" << folderName << "in parent folder:" << m_syncFolderId;
    
    // Create the folder in Google Drive
    QUrl url(API_BASE_URL + "/files");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    addAuthHeader(request);
    
    QJsonObject folderMetadata;
    folderMetadata["name"] = folderName;
    folderMetadata["mimeType"] = "application/vnd.google-apps.folder";
    
    // Set parent folder to the Notes App folder
    folderMetadata["parents"] = QJsonArray{m_syncFolderId};
    
    QJsonDocument doc(folderMetadata);
    QByteArray data = doc.toJson();
    
    qDebug() << "Folder creation request data:" << QString::fromUtf8(data);
    qDebug() << "Parent ID being set:" << m_syncFolderId;
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    trackRequest(reply, "create_subfolder", "");
    
    qDebug() << "Folder creation request sent for:" << folderName;
}

void GoogleDriveManager::syncAll()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    if (m_syncFolderId.isEmpty()) {
        emit error("No sync folder set");
        return;
    }
    
    qDebug() << "Starting full sync with Google Drive folder:" << m_syncFolderId;
    
    // First, get the list of remote notes
    listNotes();
}

void GoogleDriveManager::smartSync()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    if (m_syncFolderId.isEmpty()) {
        emit error("No sync folder set");
        return;
    }
    
    qDebug() << "Starting smart sync with Google Drive folder:" << m_syncFolderId;
    
    // Check existing structure first
    if (!m_structureChecked) {
        checkExistingStructure();
    } else {
        // Structure already checked, proceed with sync
        emit syncComplete();
    }
}

void GoogleDriveManager::uploadAllNotes(const QList<QPair<QString, QString>> &notes)
{
    qDebug() << "uploadAllNotes called with" << notes.size() << "notes";
    qDebug() << "Authentication status:" << isAuthenticated();
    qDebug() << "Sync folder ID:" << m_syncFolderId;
    
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    if (m_syncFolderId.isEmpty()) {
        qDebug() << "ERROR: No sync folder set!";
        emit error("No sync folder set");
        return;
    }
    
    qDebug() << "Uploading" << notes.size() << "notes to Google Drive";
    
    for (const auto &note : notes) {
        QString title = note.first;
        QString content = note.second;
        
        qDebug() << "Starting upload for note:" << title << "with content length:" << content.length();
        
        // Upload each note
        uploadNote("", content, title);
    }
    
    qDebug() << "All upload requests sent for" << notes.size() << "notes";
}

void GoogleDriveManager::uploadFolderStructure(const QList<QPair<QString, QList<QPair<QString, QString>>>> &folderStructure)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    qDebug() << "Starting folder structure upload with" << folderStructure.size() << "folders";
    
    // Check if we have a Notes App folder
    if (m_syncFolderId.isEmpty()) {
        qDebug() << "No Notes App folder found, creating one first...";
        createNotesFolder();
        // The folder creation will trigger the upload process via onFolderCreated
        return;
    }
    
    qDebug() << "Notes App folder exists with ID:" << m_syncFolderId << ", proceeding with subfolder creation...";
    
    // We have a Notes App folder, now create subfolders and upload notes
    createSubfoldersAndUploadNotes(folderStructure);
}

void GoogleDriveManager::createSubfoldersAndUploadNotes(const QList<QPair<QString, QList<QPair<QString, QString>>>> &folderStructure)
{
    qDebug() << "Creating subfolders and uploading notes...";
    qDebug() << "Current sync folder ID:" << m_syncFolderId;
    qDebug() << "Number of folders to create:" << folderStructure.size();
    
    if (m_syncFolderId.isEmpty()) {
        qDebug() << "ERROR: Cannot create subfolders - no sync folder ID set!";
        emit error("No sync folder ID set for subfolder creation");
        return;
    }
    
    // Clear any existing structure data to ensure fresh start
    m_subfolderIds.clear();
    m_remoteFolderIds.clear();
    m_remoteNoteIds.clear();
    m_remoteNoteHashes.clear();
    m_structureChecked = false;
    
    // Store the folder structure for later use
    m_pendingFolderStructure = folderStructure;
    
    // First check if subfolders already exist to prevent duplication
    if (m_structureChecked && !m_subfolderIds.isEmpty()) {
        qDebug() << "Subfolders already exist, skipping creation and starting note uploads...";
        qDebug() << "Existing subfolder IDs:" << m_subfolderIds;
        startNoteUploads();
        return;
    }
    
    // Reset subfolder index and check existing structure
    m_pendingSubfolderIndex = 0;
    checkExistingStructure();
}

void GoogleDriveManager::checkExistingStructure()
{
    qDebug() << "Checking existing structure in Google Drive...";
    
    // List subfolders to see what already exists
    listSubfolders();
}

void GoogleDriveManager::createNextSubfolder()
{
    if (m_pendingSubfolderIndex >= m_pendingFolderStructure.size()) {
        // All subfolders processed, start uploading notes
        qDebug() << "All subfolders processed, starting note uploads...";
        qDebug() << "Available subfolder IDs:" << m_subfolderIds;
        startNoteUploads();
        return;
    }
    
    const auto &folderData = m_pendingFolderStructure[m_pendingSubfolderIndex];
    QString folderName = folderData.first;
    
    // Check if this subfolder already exists
    if (m_subfolderIds.contains(folderName)) {
        qDebug() << "Subfolder already exists:" << folderName << "with ID:" << m_subfolderIds[folderName] << ", skipping creation";
        m_pendingSubfolderIndex++;
        createNextSubfolder(); // Process next folder
        return;
    }
    
    qDebug() << "Creating subfolder:" << folderName << "in parent folder:" << m_syncFolderId;
    createFolder(folderName);
}

void GoogleDriveManager::startNoteUploads()
{
    for (const auto &folderData : m_pendingFolderStructure) {
        QString folderName = folderData.first;
        QList<QPair<QString, QString>> notes = folderData.second;
        
        if (m_subfolderIds.contains(folderName)) {
            QString subfolderId = m_subfolderIds[folderName];
            qDebug() << "Processing" << notes.size() << "notes in subfolder:" << folderName << "with ID:" << subfolderId;
            
            for (const auto &note : notes) {
                QString title = note.first;
                QString content = note.second;
                
                // Check if note already exists
                if (m_remoteNoteIds.contains(title)) {
                    QString existingNoteId = m_remoteNoteIds[title];
                    qDebug() << "Note already exists:" << title << "with ID:" << existingNoteId << ", checking if update needed";
                    
                    // Check if content has changed
                    QString existingHash = m_remoteNoteHashes.value(title, "");
                    QString newHash = calculateFileHash(content);
                    
                    if (existingHash != newHash) {
                        qDebug() << "Note content changed, updating:" << title;
                        uploadNoteToFolder(existingNoteId, content, title, subfolderId);
                    } else {
                        qDebug() << "Note unchanged, skipping:" << title;
                    }
                } else {
                    qDebug() << "Note doesn't exist, creating new:" << title;
                    uploadNoteToFolder("", content, title, subfolderId);
                }
            }
        } else {
            qDebug() << "Warning: Subfolder ID not found for:" << folderName;
        }
    }
}

void GoogleDriveManager::listSubfolders()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // Query for subfolders in the Notes App folder
    QUrl url(API_BASE_URL + "/files");
    QUrlQuery query;
    query.addQueryItem("q", QString("'%1' in parents and mimeType='application/vnd.google-apps.folder' and trashed=false").arg(m_syncFolderId));
    query.addQueryItem("fields", "files(id,name)");
    query.addQueryItem("spaces", "drive");
    
    url.setQuery(query);
    QNetworkRequest request(url);
    addAuthHeader(request);
    
    QNetworkReply *reply = m_networkManager->get(request);
    trackRequest(reply, "list_subfolders", "");
    
    qDebug() << "Listing subfolders in Notes App folder...";
}

void GoogleDriveManager::listNotesInFolder(const QString &folderId, const QString &folderName)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // Query for notes in the specific subfolder
    QUrl url(API_BASE_URL + "/files");
    QUrlQuery query;
    query.addQueryItem("q", QString("'%1' in parents and trashed=false").arg(folderId));
    query.addQueryItem("fields", "files(id,name)");
    query.addQueryItem("spaces", "drive");
    
    url.setQuery(query);
    QNetworkRequest request(url);
    addAuthHeader(request);
    
    QNetworkReply *reply = m_networkManager->get(request);
    trackRequest(reply, "list_notes_in_folder", "");
    
    // Store folder name in the reply for response handling
    reply->setProperty("folderName", folderName);
    
    qDebug() << "Listing notes in subfolder:" << folderName;
}

void GoogleDriveManager::setSyncFolder(const QString &folderId)
{
    qDebug() << "Setting sync folder ID to:" << folderId;
    m_syncFolderId = folderId;
    qDebug() << "Sync folder ID is now:" << m_syncFolderId;
}

// Private helper methods

QNetworkRequest GoogleDriveManager::createRequest(const QString &url)
{
    QNetworkRequest request{QUrl(url)};
    addAuthHeader(request);
    return request;
}

void GoogleDriveManager::addAuthHeader(QNetworkRequest &request)
{
    qDebug() << "Adding auth header, access token length:" << m_accessToken.length();
    qDebug() << "Access token (first 20 chars):" << m_accessToken.mid(0, 20) + "...";
    qDebug() << "Token expiry:" << m_tokenExpiry.toString();
    qDebug() << "Current time:" << QDateTime::currentDateTime().toString();
    qDebug() << "Is authenticated:" << m_isAuthenticated;
    
    if (!m_accessToken.isEmpty()) {
        // Check if token is expired
        if (m_tokenExpiry.isValid() && QDateTime::currentDateTime() >= m_tokenExpiry) {
            qDebug() << "WARNING: Token is expired, attempting refresh...";
            refreshTokenIfNeeded();
            // Continue with current token for this request, it will be refreshed for next request
        }
        
        QString authHeader = QString("Bearer %1").arg(m_accessToken);
        request.setRawHeader("Authorization", authHeader.toUtf8());
        qDebug() << "Auth header set:" << authHeader.mid(0, 30) + "...";
    } else {
        qDebug() << "ERROR: No access token available!";
        emit error(makeUserFriendlyError("No access token available. Please authenticate with Google Drive first."));
    }
}

QString GoogleDriveManager::getApiUrl(const QString &endpoint) const
{
    return QString("%1/%2").arg(API_BASE_URL, endpoint);
}

void GoogleDriveManager::trackRequest(QNetworkReply *reply, const QString &requestType, const QString &noteId)
{
    if (reply) {
        reply->setProperty("requestType", requestType);
        if (!noteId.isEmpty()) {
            reply->setProperty("noteId", noteId);
        }
    }
}

void GoogleDriveManager::startTokenRefreshTimer()
{
    // Check token every 5 minutes
    m_tokenRefreshTimer->start(5 * 60 * 1000);
}

void GoogleDriveManager::refreshTokenIfNeeded()
{
    qDebug() << "Checking if token refresh is needed...";
    qDebug() << "Token expiry:" << m_tokenExpiry.toString();
    qDebug() << "Current time:" << QDateTime::currentDateTime().toString();
    
    if (m_tokenExpiry.isValid()) {
        int secondsToExpiry = QDateTime::currentDateTime().secsTo(m_tokenExpiry);
        qDebug() << "Seconds until token expiry:" << secondsToExpiry;
        
        if (secondsToExpiry < 300) {
            qDebug() << "Token expires soon, refreshing...";
            refreshToken();
        } else {
            qDebug() << "Token is still valid";
        }
    } else {
        qDebug() << "Token expiry is not valid";
    }
    
    // Check if token is already expired
    if (m_tokenExpiry.isValid() && QDateTime::currentDateTime() >= m_tokenExpiry) {
        qDebug() << "Token is expired, attempting refresh...";
        if (!m_refreshToken.isEmpty()) {
            refreshToken();
        } else {
            qDebug() << "No refresh token available, need to re-authenticate";
            m_isAuthenticated = false;
            emit authenticationChanged(false);
            emit error(makeUserFriendlyError("Access token expired and no refresh token available. Please re-authenticate."));
        }
    }
}

void GoogleDriveManager::saveTokens()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    
    QFile tokenFile(configPath + "/google_drive_tokens.json");
    if (tokenFile.open(QIODevice::WriteOnly)) {
        QJsonObject tokens;
        tokens["access_token"] = m_accessToken;
        tokens["refresh_token"] = m_refreshToken;
        tokens["expiry"] = m_tokenExpiry.toString(Qt::ISODate);
        
        QTextStream stream(&tokenFile);
        stream << QJsonDocument(tokens).toJson();
    }
}

void GoogleDriveManager::loadTokens()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile tokenFile(configPath + "/google_drive_tokens.json");
    
    if (tokenFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(tokenFile.readAll());
        QJsonObject tokens = doc.object();
        
        m_accessToken = tokens["access_token"].toString();
        m_refreshToken = tokens["refresh_token"].toString();
        m_tokenExpiry = QDateTime::fromString(tokens["expiry"].toString(), Qt::ISODate);
        
        m_isAuthenticated = !m_accessToken.isEmpty();
    }
}

// Response handlers

void GoogleDriveManager::handleAuthResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        
        m_accessToken = response["access_token"].toString();
        m_refreshToken = response["refresh_token"].toString();
        
        int expiresIn = response["expires_in"].toInt();
        m_tokenExpiry = QDateTime::currentDateTime().addSecs(expiresIn);
        
        m_isAuthenticated = true;
        saveTokens();
        emit authenticationChanged(true);
        
        // Get or create app data folder
        // TODO: Implement folder creation logic
    } else {
        emit error(makeUserFriendlyError("Authentication failed: " + reply->errorString()));
    }
}

void GoogleDriveManager::handleTokenRefresh(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        
        m_accessToken = response["access_token"].toString();
        
        int expiresIn = response["expires_in"].toInt();
        m_tokenExpiry = QDateTime::currentDateTime().addSecs(expiresIn);
        
        saveTokens();
    } else {
        emit error(makeUserFriendlyError("Token refresh failed: " + reply->errorString()));
        m_isAuthenticated = false;
        emit authenticationChanged(false);
    }
}

void GoogleDriveManager::handleUploadResponse(QNetworkReply *reply)
{
    QString noteId = reply->property("noteId").toString();
    bool success = (reply->error() == QNetworkReply::NoError);
    
    qDebug() << "Upload response received for note ID:" << noteId;
    qDebug() << "Success:" << success;
    
    if (success) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        
        qDebug() << "Upload response:" << QJsonDocument(response).toJson();
        
        if (noteId.isEmpty()) {
            // New note created
            noteId = response["id"].toString();
            qDebug() << "New note created with ID:" << noteId;
        }
        
        qDebug() << "Note uploaded successfully with ID:" << noteId;
    } else {
        qDebug() << "Upload failed with error:" << reply->errorString();
        qDebug() << "HTTP status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    
    emit uploadComplete(noteId, success);
}

void GoogleDriveManager::handleUploadMetadataResponse(QNetworkReply *reply)
{
    QString noteId = reply->property("noteId").toString();
    QString content = reply->property("content").toString();
    QString title = reply->property("title").toString();
    QString folderId = reply->property("folderId").toString();
    
    qDebug() << "Upload metadata response received for note:" << title;
    qDebug() << "Content length from property:" << content.length();
    qDebug() << "Content preview from property:" << content.mid(0, 200) + "...";
    qDebug() << "Content (full) from property:" << content;
    
    if (reply->error() == QNetworkReply::NoError) {
        // For resumable uploads, we need to check the response headers for the upload session URL
        QByteArray responseData = reply->readAll();
        QString locationHeader = reply->rawHeader("Location");
        
            if (!locationHeader.isEmpty()) {
            qDebug() << "Got resumable upload session URL:" << locationHeader;
            // Validate content before uploading
            if (content.isEmpty()) {
                qDebug() << "ERROR: Content is empty, cannot upload!";
                emit uploadComplete(noteId, false);
                return;
            }
            
            // Check if content is just whitespace
            if (content.trimmed().isEmpty()) {
                qDebug() << "ERROR: Content is only whitespace, cannot upload!";
                emit uploadComplete(noteId, false);
                return;
            }
            
            // Check if content is just the title (which would indicate an error)
            if (content.trimmed() == title.trimmed()) {
                qDebug() << "WARNING: Content appears to be just the title, this might indicate an error";
                qDebug() << "Title:" << title;
                qDebug() << "Content:" << content;
                qDebug() << "This suggests the content parameter is not being passed correctly!";
            }
            
            // Check if content contains markdown-like content
            if (content.contains("#") || content.contains("*") || content.contains("`") || content.contains("[")) {
                qDebug() << "Content appears to contain markdown formatting - good!";
            } else {
                qDebug() << "WARNING: Content does not appear to contain markdown formatting";
                qDebug() << "This might indicate the content is not being passed correctly";
            }
            
            // Use the resumable upload session URL to upload content
            uploadFileContentToSession(locationHeader, content, title, noteId);
        } else {
            // Fallback: try to get file ID from response body
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject response = doc.object();
            
            QString fileId = response["id"].toString();
            if (!fileId.isEmpty()) {
                qDebug() << "File metadata uploaded successfully, file ID:" << fileId;
                // Validate content before uploading
                if (content.isEmpty()) {
                    qDebug() << "ERROR: Content is empty, cannot upload!";
                    emit uploadComplete(noteId, false);
                    return;
                }
                
                // Check if content is just whitespace
                if (content.trimmed().isEmpty()) {
                    qDebug() << "ERROR: Content is only whitespace, cannot upload!";
                    emit uploadComplete(noteId, false);
                    return;
                }
                
                // Check if content is just the title (which would indicate an error)
                if (content.trimmed() == title.trimmed()) {
                    qDebug() << "WARNING: Content appears to be just the title, this might indicate an error";
                    qDebug() << "Title:" << title;
                    qDebug() << "Content:" << content;
                    qDebug() << "This suggests the content parameter is not being passed correctly!";
                }
                
                // Check if content contains markdown-like content
                if (content.contains("#") || content.contains("*") || content.contains("`") || content.contains("[")) {
                    qDebug() << "Content appears to contain markdown formatting - good!";
                } else {
                    qDebug() << "WARNING: Content does not appear to contain markdown formatting";
                    qDebug() << "This might indicate the content is not being passed correctly";
                }
                
                // Add a small delay before uploading content to allow Google Drive to process
                QTimer::singleShot(1000, [this, fileId, content, title, noteId]() {
                    uploadFileContent(fileId, content, title, noteId);
                });
            } else {
                qDebug() << "No file ID found in response, upload failed";
                emit uploadComplete(noteId, false);
            }
        }
        
    } else {
        qDebug() << "Upload metadata failed with error:" << reply->errorString();
        qDebug() << "HTTP status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit uploadComplete(noteId, false);
    }
}

void GoogleDriveManager::uploadFileContent(const QString &fileId, const QString &content, const QString &title, const QString &noteId)
{
    qDebug() << "Uploading file content for:" << title << "with file ID:" << fileId;
    qDebug() << "Content length:" << content.length();
    qDebug() << "Content preview:" << content.mid(0, 200) + "...";
    
    // Validate content before uploading
    if (content.isEmpty()) {
        qDebug() << "ERROR: Content is empty, cannot upload!";
        emit uploadComplete(noteId.isEmpty() ? fileId : noteId, false);
        return;
    }
    
    // Check if content is just whitespace
    if (content.trimmed().isEmpty()) {
        qDebug() << "ERROR: Content is only whitespace, cannot upload!";
        emit uploadComplete(noteId.isEmpty() ? fileId : noteId, false);
        return;
    }
    
    // Check if content is just the title (which would indicate an error)
    if (content.trimmed() == title.trimmed()) {
        qDebug() << "WARNING: Content appears to be just the title, this might indicate an error";
        qDebug() << "Title:" << title;
        qDebug() << "Content:" << content;
        qDebug() << "This suggests the content parameter is not being passed correctly!";
    }
    
    // Check if content contains markdown-like content
    if (content.contains("#") || content.contains("*") || content.contains("`") || content.contains("[")) {
        qDebug() << "Content appears to contain markdown formatting - good!";
    } else {
        qDebug() << "WARNING: Content does not appear to contain markdown formatting";
        qDebug() << "This might indicate the content is not being passed correctly";
    }
    
    // Upload the content to the file
    QString url = QString("%1/files/%2?alt=media").arg(API_BASE_URL, fileId);
    
    QNetworkRequest request{QUrl(url)};
    addAuthHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/markdown; charset=utf-8");
    
    QByteArray contentData = content.toUtf8();
    qDebug() << "Content data size (UTF-8):" << contentData.size();
    qDebug() << "Content data preview (hex):" << contentData.left(100).toHex();
    
    QNetworkReply *reply = m_networkManager->put(request, contentData);
    
    // Store properties for response handling
    reply->setProperty("fileId", fileId);
    reply->setProperty("title", title);
    reply->setProperty("noteId", noteId);
    
    trackRequest(reply, "upload_content", fileId);
    
    qDebug() << "Content upload request sent for file:" << fileId;
}

void GoogleDriveManager::uploadFileContentToSession(const QString &sessionUrl, const QString &content, const QString &title, const QString &noteId)
{
    qDebug() << "Uploading file content to resumable session for:" << title;
    qDebug() << "Content length:" << content.length();
    qDebug() << "Content preview:" << content.mid(0, 200) + "...";
    
    // Validate content before uploading
    if (content.isEmpty()) {
        qDebug() << "ERROR: Content is empty, cannot upload!";
        emit uploadComplete(noteId, false);
        return;
    }
    
    // Check if content is just whitespace
    if (content.trimmed().isEmpty()) {
        qDebug() << "ERROR: Content is only whitespace, cannot upload!";
        emit uploadComplete(noteId, false);
        return;
    }
    
    // Check if content is just the title (which would indicate an error)
    if (content.trimmed() == title.trimmed()) {
        qDebug() << "WARNING: Content appears to be just the title, this might indicate an error";
        qDebug() << "Title:" << title;
        qDebug() << "Content:" << content;
        qDebug() << "This suggests the content parameter is not being passed correctly!";
    }
    
    // Check if content contains markdown-like content
    if (content.contains("#") || content.contains("*") || content.contains("`") || content.contains("[")) {
        qDebug() << "Content appears to contain markdown formatting - good!";
    } else {
        qDebug() << "WARNING: Content does not appear to contain markdown formatting";
        qDebug() << "This might indicate the content is not being passed correctly";
    }
    
    QNetworkRequest request{QUrl(sessionUrl)};
    // No need to add auth header for resumable upload session URLs
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/markdown; charset=utf-8");
    
    QByteArray contentData = content.toUtf8();
    qDebug() << "Content data size (UTF-8):" << contentData.size();
    qDebug() << "Content data preview (hex):" << contentData.left(100).toHex();
    
    QNetworkReply *reply = m_networkManager->put(request, contentData);
    
    // Store properties for response handling
    reply->setProperty("sessionUrl", sessionUrl);
    reply->setProperty("title", title);
    reply->setProperty("noteId", noteId);
    
    trackRequest(reply, "upload_session", noteId);
    
    qDebug() << "Content upload to session sent for:" << title;
}

void GoogleDriveManager::handleUploadContentResponse(QNetworkReply *reply)
{
    QString fileId = reply->property("fileId").toString();
    QString title = reply->property("title").toString();
    QString noteId = reply->property("noteId").toString();
    bool success = (reply->error() == QNetworkReply::NoError);
    
    qDebug() << "Upload content response received for file:" << fileId;
    qDebug() << "Success:" << success;
    qDebug() << "Response data:" << reply->readAll();
    
    if (success) {
        qDebug() << "File content uploaded successfully for:" << title;
        qDebug() << "HTTP status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit uploadComplete(noteId.isEmpty() ? fileId : noteId, true);
    } else {
        qDebug() << "File content upload failed with error:" << reply->errorString();
        qDebug() << "HTTP status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Error details:" << reply->error();
        emit uploadComplete(noteId.isEmpty() ? fileId : noteId, false);
    }
}

void GoogleDriveManager::handleUploadSessionResponse(QNetworkReply *reply)
{
    QString title = reply->property("title").toString();
    QString noteId = reply->property("noteId").toString();
    bool success = (reply->error() == QNetworkReply::NoError);
    
    qDebug() << "Upload session response received for:" << title;
    qDebug() << "Success:" << success;
    qDebug() << "Response data:" << reply->readAll();
    
    if (success) {
        qDebug() << "File content uploaded successfully via session for:" << title;
        qDebug() << "HTTP status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit uploadComplete(noteId, true);
    } else {
        qDebug() << "File content upload via session failed with error:" << reply->errorString();
        qDebug() << "HTTP status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Error details:" << reply->error();
        emit uploadComplete(noteId, false);
    }
}

void GoogleDriveManager::handleDownloadResponse(QNetworkReply *reply)
{
    QString noteId = reply->property("noteId").toString();
    bool success = (reply->error() == QNetworkReply::NoError);
    QString content;
    
    if (success) {
        content = QString::fromUtf8(reply->readAll());
    }
    
    emit downloadComplete(noteId, content, success);
}

void GoogleDriveManager::handleDeleteResponse(QNetworkReply *reply)
{
    QString noteId = reply->property("noteId").toString();
    bool success = (reply->error() == QNetworkReply::NoError);
    
    emit deleteComplete(noteId, success);
}

void GoogleDriveManager::handleListResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        QJsonArray files = response["files"].toArray();
        
        emit notesListReceived(files);
    } else {
        emit error("Failed to list notes: " + reply->errorString());
    }
}

void GoogleDriveManager::handleCreateResponse(QNetworkReply *reply)
{
    // Same as upload response for new files
    handleUploadResponse(reply);
}

void GoogleDriveManager::createNotesFolder()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // First, try to find an existing Notes App folder
    findExistingNotesFolder();
}

void GoogleDriveManager::findExistingNotesFolder()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // Search for existing "Notes App" folder
    QUrl url(API_BASE_URL + "/files");
    QUrlQuery query;
    query.addQueryItem("q", "name='Notes App' and mimeType='application/vnd.google-apps.folder' and trashed=false");
    query.addQueryItem("fields", "files(id,name)");
    query.addQueryItem("spaces", "drive");
    query.addQueryItem("pageSize", "10");
    url.setQuery(query);
    
    qDebug() << "Query string:" << query.toString();
    
    QNetworkRequest request(url);
    addAuthHeader(request);
    
    qDebug() << "Searching for existing Notes App folder...";
    qDebug() << "URL:" << url.toString();
    
    QNetworkReply *reply = m_networkManager->get(request);
    trackRequest(reply, "find_folder", "");
}

void GoogleDriveManager::createNewNotesFolder()
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // Create the notes folder in Google Drive
    QUrl url(API_BASE_URL + "/files");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    addAuthHeader(request);
    
    QJsonObject folderMetadata;
    folderMetadata["name"] = "Notes App";
    folderMetadata["mimeType"] = "application/vnd.google-apps.folder";
    
    // Set parent folder to root (optional)
    folderMetadata["parents"] = QJsonArray{"root"};
    
    QJsonDocument doc(folderMetadata);
    QByteArray data = doc.toJson();
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    trackRequest(reply, "create_folder", "");
    
    qDebug() << "Creating new Notes App folder in Google Drive...";
    qDebug() << "URL:" << url.toString();
    qDebug() << "Request data:" << QString::fromUtf8(data);
}

QString GoogleDriveManager::getNotesFolderId() const
{
    return m_syncFolderId;
}

bool GoogleDriveManager::isStructureChecked() const
{
    return m_structureChecked;
}

void GoogleDriveManager::handleCreateFolderResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        
        QString folderId = response["id"].toString();
        QString folderName = response["name"].toString();
        
        qDebug() << "Successfully created folder:" << folderName << "with ID:" << folderId;
        
        // Store the folder ID for future use
        m_syncFolderId = folderId;
        
        // Set the sync folder in the drive manager
        setSyncFolder(folderId);
        
        qDebug() << "Sync folder set to:" << m_syncFolderId;
        
        // Emit success signal
        emit syncComplete();
        
        qDebug() << "Notes folder created successfully in Google Drive!";
    } else {
        QString errorMsg = "Failed to create folder: " + reply->errorString();
        qDebug() << errorMsg;
        emit error(errorMsg);
    }
}

void GoogleDriveManager::handleCreateSubfolderResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        
        QString folderId = response["id"].toString();
        QString folderName = response["name"].toString();
        
        qDebug() << "Successfully created subfolder:" << folderName << "with ID:" << folderId;
        
        // Store the subfolder ID for future use
        m_subfolderIds[folderName] = folderId;
        qDebug() << "Stored subfolder ID:" << folderName << "->" << folderId;
        
        // Move to next subfolder or start uploading notes
        m_pendingSubfolderIndex++;
        createNextSubfolder();
        
    } else {
        QString errorMsg = "Failed to create subfolder: " + reply->errorString();
        qDebug() << errorMsg;
        emit error(errorMsg);
        
        // Even on error, try to continue with next subfolder
        m_pendingSubfolderIndex++;
        createNextSubfolder();
    }
}

void GoogleDriveManager::handleFindFolderResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        QJsonArray files = response["files"].toArray();
        
        if (!files.isEmpty()) {
            // Found existing folder
            QJsonObject folder = files.first().toObject();
            QString folderId = folder["id"].toString();
            QString folderName = folder["name"].toString();
            
            qDebug() << "Found existing Notes App folder:" << folderName << "with ID:" << folderId;
            
            // Use the existing folder
            m_syncFolderId = folderId;
            setSyncFolder(folderId);
            
            qDebug() << "Using existing folder with ID:" << m_syncFolderId;
            
            // Emit success signal
            emit syncComplete();
            
            qDebug() << "Existing Notes App folder found and set!";
        } else {
            // No existing folder found, create a new one
            qDebug() << "No existing Notes App folder found, creating new one...";
            createNewNotesFolder();
        }
    } else {
        QString errorMsg = "Failed to search for folder: " + reply->errorString();
        qDebug() << errorMsg;
        emit error(errorMsg);
    }
}

void GoogleDriveManager::handleListSubfoldersResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        QJsonArray files = response["files"].toArray();
        
        qDebug() << "Found" << files.size() << "existing subfolders";
        
        // Store existing subfolder IDs
        for (const auto &file : files) {
            QJsonObject folder = file.toObject();
            QString folderId = folder["id"].toString();
            QString folderName = folder["name"].toString();
            
            m_remoteFolderIds[folderName] = folderId;
            m_subfolderIds[folderName] = folderId;
            qDebug() << "Found existing subfolder:" << folderName << "with ID:" << folderId;
        }
        
        // Mark structure as checked
        m_structureChecked = true;
        
        // Now check notes in each existing subfolder
        for (const auto &folderName : m_remoteFolderIds.keys()) {
            QString folderId = m_remoteFolderIds[folderName];
            listNotesInFolder(folderId, folderName);
        }
        
        // After checking existing structure, continue with creating any missing subfolders
        if (!m_pendingFolderStructure.isEmpty()) {
            qDebug() << "Structure check complete, continuing with missing subfolder creation...";
            createNextSubfolder();
        }
        
    } else {
        QString errorMsg = "Failed to list subfolders: " + reply->errorString();
        qDebug() << errorMsg;
        emit error(errorMsg);
    }
}

void GoogleDriveManager::handleListNotesInFolderResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        QJsonArray files = response["files"].toArray();
        
        QString folderName = reply->property("folderName").toString();
        qDebug() << "Found" << files.size() << "notes in subfolder:" << folderName;
        
        // Store existing note IDs and hashes
        for (const auto &file : files) {
            QJsonObject note = file.toObject();
            QString noteId = note["id"].toString();
            QString noteName = note["name"].toString();
            
            // Remove .md extension for comparison
            QString title = noteName;
            if (title.endsWith(".md")) {
                title = title.left(title.length() - 3);
            }
            
            m_remoteNoteIds[title] = noteId;
            qDebug() << "Found existing note:" << title << "with ID:" << noteId;
        }
        
        // Check if this was the last folder to process
        // For now, we'll emit smart sync complete after processing notes
        // In a more sophisticated implementation, you'd track completion of all folders
        if (m_structureChecked && !m_remoteFolderIds.isEmpty()) {
            qDebug() << "Smart sync structure check completed";
            emit smartSyncComplete();
        }
        
    } else {
        QString errorMsg = "Failed to list notes in folder: " + reply->errorString();
        qDebug() << errorMsg;
        emit error(errorMsg);
    }
}

void GoogleDriveManager::syncSingleNote(const QString &noteId, const QString &content, const QString &title, const QString &folderName)
{
    if (!isAuthenticated()) {
        emit error(makeUserFriendlyError("Not authenticated"));
        return;
    }
    
    // Check if folder exists, create if needed
    if (!m_remoteFolderIds.contains(folderName)) {
        qDebug() << "Creating missing subfolder:" << folderName;
        createFolder(folderName);
        // Note: The folder creation response will handle the note upload
        return;
    }
    
    QString folderId = m_remoteFolderIds[folderName];
    
    // Check if note exists and needs update
    updateNoteIfChanged(noteId, content, title, folderName);
}

void GoogleDriveManager::updateNoteIfChanged(const QString &noteId, const QString &content, const QString &title, const QString &folderName)
{
    QString remoteNoteId = m_remoteNoteIds.value(title, "");
    QString currentHash = calculateFileHash(content);
    
    if (remoteNoteId.isEmpty()) {
        // Note doesn't exist remotely, upload it
        qDebug() << "Note doesn't exist remotely, uploading:" << title;
        QString folderId = m_remoteFolderIds[folderName];
        uploadNoteToFolder("", content, title, folderId);
    } else {
        // Note exists, check if it needs update
        QString remoteHash = m_remoteNoteHashes.value(title, "");
        if (remoteHash != currentHash) {
            qDebug() << "Note changed, updating:" << title;
            QString folderId = m_remoteFolderIds[folderName];
            uploadNoteToFolder(remoteNoteId, content, title, folderId);
        } else {
            qDebug() << "Note unchanged, skipping:" << title;
        }
    }
}

QString GoogleDriveManager::calculateFileHash(const QString &content)
{
    // Simple hash for content comparison
    // In production, you might want to use a more robust hashing algorithm
    QByteArray hash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
    return hash.toHex();
}

QString GoogleDriveManager::getRemoteNoteId(const QString &title, const QString &folderName)
{
    return m_remoteNoteIds.value(title, "");
}

void GoogleDriveManager::clearStructureData()
{
    qDebug() << "Clearing structure data for fresh sync...";
    m_subfolderIds.clear();
    m_remoteFolderIds.clear();
    m_remoteNoteIds.clear();
    m_remoteNoteHashes.clear();
    m_structureChecked = false;
    m_pendingFolderStructure.clear();
    m_pendingSubfolderIndex = 0;
}
