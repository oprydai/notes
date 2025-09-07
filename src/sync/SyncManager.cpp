#include "SyncManager.h"
#include "../db/DatabaseManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

SyncManager::SyncManager(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_driveManager(new GoogleDriveManager(this))
    , m_isSyncing(false)
    , m_autoSyncEnabled(false)
    , m_autoSyncTimer(new QTimer(this))
    , m_autoSyncInterval(15)
{
    // Connect Google Drive signals
    connect(m_driveManager, &GoogleDriveManager::authenticationChanged, this, &SyncManager::onAuthenticationChanged);
    connect(m_driveManager, &GoogleDriveManager::notesListReceived, this, &SyncManager::onNotesListReceived);
    connect(m_driveManager, &GoogleDriveManager::uploadComplete, this, &SyncManager::onUploadComplete);
    connect(m_driveManager, &GoogleDriveManager::downloadComplete, this, &SyncManager::onDownloadComplete);
    connect(m_driveManager, &GoogleDriveManager::deleteComplete, this, &SyncManager::onDeleteComplete);
    connect(m_driveManager, &GoogleDriveManager::syncComplete, this, &SyncManager::onFolderCreated);
    connect(m_driveManager, &GoogleDriveManager::smartSyncComplete, this, &SyncManager::onSmartSyncComplete);
    connect(m_driveManager, &GoogleDriveManager::error, this, &SyncManager::onError);
    
    // Set up auto-sync timer
    connect(m_autoSyncTimer, &QTimer::timeout, this, &SyncManager::performAutoSync);
    
    // Load saved sync state
    loadSyncState();
}

SyncManager::~SyncManager()
{
    saveSyncState();
}

void SyncManager::startAutoSync(int intervalMinutes)
{
    m_autoSyncInterval = intervalMinutes;
    m_autoSyncEnabled = true;
    m_syncCompletedEmitted = false;  // Reset flag when starting auto-sync
    
    if (m_driveManager->isAuthenticated()) {
        m_autoSyncTimer->start(intervalMinutes * 60 * 1000);
        performAutoSync(); // Initial sync
    }
}

void SyncManager::stopAutoSync()
{
    m_autoSyncEnabled = false;
    m_autoSyncTimer->stop();
}

void SyncManager::syncNow()
{
    if (m_isSyncing) {
        return;
    }
    
    if (!m_driveManager->isAuthenticated()) {
        emit syncFailed("Not authenticated with Google Drive");
        return;
    }
    
    m_isSyncing = true;
    m_syncCompletedEmitted = false;  // Reset flag for new sync operation
    emit syncStarted();
    
    // Start by getting the list of remote notes
    m_driveManager->listNotes();
}

void SyncManager::setAutoSyncEnabled(bool enabled)
{
    m_autoSyncEnabled = enabled;
    
    if (enabled && m_driveManager->isAuthenticated()) {
        startAutoSync(m_autoSyncInterval);
    } else {
        stopAutoSync();
    }
}

bool SyncManager::isSyncing() const
{
    return m_isSyncing;
}

QString SyncManager::getLastSyncTime() const
{
    if (m_lastSyncTime.isValid()) {
        return m_lastSyncTime.toString("yyyy-MM-dd hh:mm:ss");
    }
    return "Never";
}

QString SyncManager::getSyncStatus() const
{
    if (m_isSyncing) {
        return "Syncing...";
    } else if (m_driveManager->isAuthenticated()) {
        return "Connected to Google Drive";
    } else {
        return "Not connected";
    }
}

bool SyncManager::isAuthenticated() const
{
    bool authenticated = m_driveManager->isAuthenticated();
    qDebug() << "SyncManager::isAuthenticated() called, returning:" << authenticated;
    return authenticated;
}

void SyncManager::authenticate()
{
    m_driveManager->authenticate();
}

void SyncManager::logout()
{
    m_driveManager->logout();
    
    // Clear sync state
    m_syncCompletedEmitted = false;  // Reset flag when logging out
}

void SyncManager::forceReauthenticate()
{
    qDebug() << "Force re-authentication requested...";
    
    // Stop any ongoing sync
    m_isSyncing = false;
    m_syncCompletedEmitted = false;
    
    // Force re-authentication in the drive manager
    m_driveManager->forceReauthenticate();
}

void SyncManager::clearStructureData()
{
    qDebug() << "Clearing structure data in SyncManager...";
    m_driveManager->clearStructureData();
}

void SyncManager::completeOAuth(const QString &authCode)
{
    qDebug() << "Completing OAuth flow with auth code:" << authCode.mid(0, 10) + "...";
    
    // Call the GoogleDriveManager to complete the OAuth flow
    m_driveManager->completeOAuth(authCode);
}

void SyncManager::uploadAllNotes()
{
    if (!m_driveManager->isAuthenticated()) {
        emit syncFailed("Not authenticated");
        return;
    }
    
    qDebug() << "Starting upload of all local notes to Google Drive";
    m_syncCompletedEmitted = false;  // Reset flag for new upload operation
    
    // Get the hierarchical folder structure from the database
    QList<QPair<QString, QList<QPair<QString, QString>>>> folderStructure = m_dbManager->getFolderStructure();
    
    if (folderStructure.isEmpty()) {
        qDebug() << "No folder structure found to upload";
        if (!m_syncCompletedEmitted) {
            emit syncCompleted();
            m_syncCompletedEmitted = true;
        }
        return;
    }
    
    qDebug() << "Found" << folderStructure.size() << "folders to upload";
    
    // Upload the hierarchical folder structure to Google Drive
    m_driveManager->uploadFolderStructure(folderStructure);
    
    m_syncCompletedEmitted = false;  // Reset flag when starting upload
    emit syncStarted();
}

void SyncManager::downloadAllNotes()
{
    if (!m_driveManager->isAuthenticated()) {
        emit syncFailed("Not authenticated");
        return;
    }
    
    qDebug() << "Starting download of all remote notes from Google Drive";
    m_syncCompletedEmitted = false;  // Reset flag when starting download
    
    // Get list of remote notes and download them all
    m_driveManager->listNotes();
}

void SyncManager::syncAllNotes()
{
    if (!m_driveManager->isAuthenticated()) {
        emit syncFailed("Not authenticated");
        return;
    }
    
    qDebug() << "Starting full sync: upload local notes, then download remote notes";
    m_syncCompletedEmitted = false;  // Reset flag for new sync operation
    
    // Clear any existing structure data to prevent duplication
    m_driveManager->clearStructureData();
    
    // Check if we have a sync folder, create one if needed
    if (m_syncFolderId.isEmpty()) {
        qDebug() << "No sync folder found, creating one...";
        m_isSyncing = true;  // Set sync flag before creating folder
        m_driveManager->createNotesFolder();
        return; // Wait for folder creation to complete
    }
    
    qDebug() << "Sync folder exists, proceeding with sync...";
    m_isSyncing = true;  // Set sync flag for manual sync
    
    // First upload all local notes
    uploadAllNotes();
    
    // After upload completes, download will be triggered
    // This creates a two-phase sync process
}

void SyncManager::smartSync()
{
    if (!m_driveManager->isAuthenticated()) {
        emit syncFailed("Not authenticated");
        return;
    }
    
    qDebug() << "Starting smart sync: checking existing structure and syncing only changes";
    m_syncCompletedEmitted = false;  // Reset flag for new sync operation
    
    // Clear any existing structure data to prevent duplication
    m_driveManager->clearStructureData();
    
    // Check if we have a sync folder, create one if needed
    if (m_syncFolderId.isEmpty()) {
        qDebug() << "No sync folder found, creating one...";
        m_isSyncing = true;  // Set sync flag before creating folder
        m_driveManager->createNotesFolder();
        return; // Wait for folder creation to complete
    }
    
    qDebug() << "Sync folder exists, proceeding with smart sync...";
    m_isSyncing = true;  // Set sync flag for smart sync
    
    // Use the new smart sync method
    m_driveManager->smartSync();
}

void SyncManager::syncSingleNote(const QString &noteId, const QString &content, const QString &title, const QString &folderName)
{
    if (!m_driveManager->isAuthenticated()) {
        emit syncFailed("Not authenticated");
        return;
    }
    
    qDebug() << "Syncing single note:" << title << "in folder:" << folderName;
    
    // Use the smart sync method for individual notes
    m_driveManager->syncSingleNote(noteId, content, title, folderName);
}

void SyncManager::handleNoteChanged(const QString &noteId, const QString &content, const QString &title, const QString &folderName)
{
    if (!m_driveManager->isAuthenticated()) {
        qDebug() << "Not authenticated, skipping note sync";
        return;
    }
    
    qDebug() << "Note changed, syncing:" << title << "in folder:" << folderName;
    
    // Check if we have the structure information
    if (!m_driveManager->isStructureChecked()) {
        qDebug() << "Structure not checked yet, performing smart sync first";
        smartSync();
        return;
    }
    
    // Use the smart sync method for individual notes
    m_driveManager->syncSingleNote(noteId, content, title, folderName);
}

void SyncManager::resolveConflicts()
{
    // This would show a dialog to let users resolve conflicts manually
    // For now, we'll use automatic conflict resolution
    emit syncFailed("Manual conflict resolution not yet implemented");
}

// Private slots

void SyncManager::onAuthenticationChanged(bool authenticated)
{
    if (authenticated) {
        qDebug() << "Authentication successful! Creating notes folder in Google Drive...";
        m_syncCompletedEmitted = false;  // Reset flag for new authentication session
        
        // Create the notes folder in Google Drive
        m_driveManager->createNotesFolder();
        
        if (m_autoSyncEnabled) {
            startAutoSync(m_autoSyncInterval);
        }
    } else {
        stopAutoSync();
        m_syncCompletedEmitted = false;  // Reset flag when logging out
    }
    
    // Emit the authentication changed signal
    emit authenticationChanged(authenticated);
}

void SyncManager::onNotesListReceived(const QJsonArray &notes)
{
    // Compare remote notes with local notes
    compareNotes(notes);
}

void SyncManager::onUploadComplete(const QString &noteId, bool success)
{
    if (success) {
        // Update the ID mapping
        // This would need to be implemented based on your data structure
        emit noteUploaded(noteId, true);
    } else {
        emit noteUploaded(noteId, false);
    }
    
    // Check if all pending operations are complete
    checkSyncCompletion();
}

void SyncManager::onDownloadComplete(const QString &noteId, const QString &content, bool success)
{
    if (success) {
        // Save the downloaded note to local database
        // This would need to be implemented based on your DatabaseManager interface
        emit noteDownloaded(noteId, true);
    } else {
        emit noteDownloaded(noteId, false);
    }
    
    checkSyncCompletion();
}

void SyncManager::onDeleteComplete(const QString &noteId, bool success)
{
    if (success) {
        // Remove from ID mappings
        QString localId = getLocalNoteId(noteId);
        if (!localId.isEmpty()) {
            m_localToRemoteIdMap.remove(localId);
            m_remoteToLocalIdMap.remove(noteId);
        }
        
        emit noteDownloaded(noteId, true);
    } else {
        emit noteDownloaded(noteId, false);
    }
    
    checkSyncCompletion();
}

void SyncManager::onError(const QString &errorMessage)
{
    qDebug() << "Sync error received:" << errorMessage;
    
    // Check if this is an authentication error
    if (errorMessage.contains("authentication", Qt::CaseInsensitive) || 
        errorMessage.contains("Host requires authentication", Qt::CaseInsensitive) ||
        errorMessage.contains("401", Qt::CaseInsensitive) ||
        errorMessage.contains("Unauthorized", Qt::CaseInsensitive)) {
        
        qDebug() << "Authentication error detected, suggesting re-authentication";
        emit syncFailed("Authentication failed. Please re-authenticate with Google Drive.");
        
        // Automatically trigger re-authentication
        QTimer::singleShot(1000, this, [this]() {
            qDebug() << "Automatically triggering re-authentication...";
            forceReauthenticate();
        });
    } else {
        emit syncFailed(errorMessage);
    }
    
    m_isSyncing = false;
}

void SyncManager::onFolderCreated()
{
    qDebug() << "Notes folder created successfully in Google Drive!";
    qDebug() << "Folder ID from drive manager:" << m_driveManager->getNotesFolderId();
    qDebug() << "Current sync folder ID:" << m_syncFolderId;
    
    // Store the folder ID for future use
    m_syncFolderId = m_driveManager->getNotesFolderId();
    
    qDebug() << "Updated sync folder ID to:" << m_syncFolderId;
    
    // Ensure the GoogleDriveManager also has the correct folder ID
    m_driveManager->setSyncFolder(m_syncFolderId);
    
    qDebug() << "Set GoogleDriveManager sync folder to:" << m_syncFolderId;
    
    // Emit syncCompleted to update the UI (only once per folder creation)
    if (!m_syncCompletedEmitted) {
        emit syncCompleted();
        m_syncCompletedEmitted = true;
    }
    
    // Now we can start syncing notes - check if this was triggered by a manual sync
    if (m_isSyncing) {
        // Check if this is a smart sync or regular sync
        // For now, we'll use the existing hierarchical upload
        // In the future, we could add a flag to distinguish between sync types
        qDebug() << "Manual sync in progress, continuing with hierarchical upload...";
        // Get the folder structure and upload it
        QList<QPair<QString, QList<QPair<QString, QString>>>> folderStructure = m_dbManager->getFolderStructure();
        m_driveManager->createSubfoldersAndUploadNotes(folderStructure);
    } else if (m_autoSyncEnabled) {
        qDebug() << "Starting initial auto-sync...";
        syncNow();
    }
}

void SyncManager::onSmartSyncComplete()
{
    qDebug() << "Smart sync structure check completed!";
    
    // Mark sync as complete
    m_isSyncing = false;
    updateSyncTimestamp();
    
    // Clear structure data to prevent duplication in next sync
    clearStructureData();
    
    // Emit sync completed signal
    if (!m_syncCompletedEmitted) {
        emit syncCompleted();
        m_syncCompletedEmitted = true;
    }
    
    qDebug() << "Smart sync completed successfully";
}

void SyncManager::performAutoSync()
{
    if (m_autoSyncEnabled && !m_isSyncing) {
        m_syncCompletedEmitted = false;  // Reset flag for auto-sync operation
        syncNow();
    }
}

// Private methods

void SyncManager::compareNotes(const QJsonArray &remoteNotes)
{
    // This is a simplified comparison logic
    // In production, you'd want more sophisticated conflict detection
    
    QMap<QString, QJsonObject> remoteNotesMap;
    
    // Build map of remote notes by filename
    for (const QJsonValue &value : remoteNotes) {
        QJsonObject note = value.toObject();
        QString filename = note["name"].toString();
        remoteNotesMap[filename] = note;
    }
    
    // Get local notes (this would need to be implemented based on your DatabaseManager)
    // For now, we'll assume we have a way to get local notes
    
    // Compare and determine what needs to be uploaded/downloaded
    // This is a placeholder for the actual comparison logic
    
    // For now, we'll just mark sync as complete
    m_isSyncing = false;
    updateSyncTimestamp();
    if (!m_syncCompletedEmitted) {
        emit syncCompleted();
        m_syncCompletedEmitted = true;
    }
}

void SyncManager::uploadLocalNote(const QString &noteId)
{
    // This would get the note content from your DatabaseManager
    // and upload it via GoogleDriveManager
    // For now, it's a placeholder
}

void SyncManager::downloadRemoteNote(const QString &noteId)
{
    m_driveManager->downloadNote(noteId);
}

void SyncManager::createRemoteNote(const QString &title, const QString &content)
{
    m_driveManager->createNote(title, content);
}

void SyncManager::deleteRemoteNote(const QString &noteId)
{
    m_driveManager->deleteNote(noteId);
}

void SyncManager::resolveNoteConflict(const QString &noteId, const QString &localContent, const QString &remoteContent)
{
    // Simple conflict resolution: use the longer content
    // In production, you'd want more sophisticated logic
    if (shouldUseLocalVersion(localContent, remoteContent)) {
        // Use local version
        uploadLocalNote(noteId);
    } else {
        // Use remote version
        downloadRemoteNote(noteId);
    }
}

bool SyncManager::shouldUseLocalVersion(const QString &localContent, const QString &remoteContent)
{
    // Simple heuristic: use the longer content
    // In production, you might want to compare timestamps or use other criteria
    return localContent.length() >= remoteContent.length();
}

QString SyncManager::getNoteTitleFromFilename(const QString &filename) const
{
    // Remove .md extension and return the title
    if (filename.endsWith(".md")) {
        return filename.left(filename.length() - 3);
    }
    return filename;
}

QString SyncManager::getRemoteNoteId(const QString &localNoteId) const
{
    return m_localToRemoteIdMap.value(localNoteId);
}

QString SyncManager::getLocalNoteId(const QString &remoteNoteId) const
{
    return m_remoteToLocalIdMap.value(remoteNoteId);
}

void SyncManager::updateSyncTimestamp()
{
    m_lastSyncTime = QDateTime::currentDateTime();
    saveSyncState();
}

void SyncManager::loadSyncState()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile stateFile(configPath + "/sync_state.json");
    
    if (stateFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(stateFile.readAll());
        QJsonObject state = doc.object();
        
        m_lastSyncTime = QDateTime::fromString(state["last_sync"].toString(), Qt::ISODate);
        m_autoSyncEnabled = state["auto_sync_enabled"].toBool();
        m_autoSyncInterval = state["auto_sync_interval"].toInt(15);
        
        // Load ID mappings
        QJsonObject localToRemote = state["local_to_remote"].toObject();
        for (auto it = localToRemote.begin(); it != localToRemote.end(); ++it) {
            m_localToRemoteIdMap[it.key()] = it.value().toString();
            m_remoteToLocalIdMap[it.value().toString()] = it.key();
        }
    }
}

void SyncManager::saveSyncState()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    
    QFile stateFile(configPath + "/sync_state.json");
    if (stateFile.open(QIODevice::WriteOnly)) {
        QJsonObject state;
        state["last_sync"] = m_lastSyncTime.toString(Qt::ISODate);
        state["auto_sync_enabled"] = m_autoSyncEnabled;
        state["auto_sync_interval"] = m_autoSyncInterval;
        
        // Save ID mappings
        QJsonObject localToRemote;
        for (auto it = m_localToRemoteIdMap.begin(); it != m_localToRemoteIdMap.end(); ++it) {
            localToRemote[it.key()] = it.value();
        }
        state["local_to_remote"] = localToRemote;
        
        QTextStream stream(&stateFile);
        stream << QJsonDocument(state).toJson();
    }
}

void SyncManager::checkSyncCompletion()
{
    // Check if all pending operations are complete
    if (m_pendingUploads.isEmpty() && m_pendingDownloads.isEmpty() && m_pendingDeletes.isEmpty()) {
        m_isSyncing = false;
        updateSyncTimestamp();
        
        // Clear structure data to prevent duplication in next sync
        clearStructureData();
        
        if (!m_syncCompletedEmitted) {
            emit syncCompleted();
            m_syncCompletedEmitted = true;
        }
    }
}
