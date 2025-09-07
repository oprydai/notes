#ifndef GOOGLEDRIVECONFIG_H
#define GOOGLEDRIVECONFIG_H

#include <QString>

class GoogleDriveConfig
{
public:
    // Google Cloud Project credentials
    // Replace these with your actual credentials from Google Cloud Console
    static const QString CLIENT_ID;
    static const QString CLIENT_SECRET;
    
    // OAuth 2.0 settings
    static const QString REDIRECT_URI;
    static const QString SCOPE;
    
    // API endpoints
    static const QString API_BASE_URL;
    static const QString AUTH_BASE_URL;
    static const QString TOKEN_BASE_URL;
    
    // Sync settings
    static const int DEFAULT_SYNC_INTERVAL_MINUTES;
    static const QString SYNC_FOLDER_NAME;
    
    // File settings
    static const QString NOTE_FILE_EXTENSION;
    static const QString NOTE_MIME_TYPE;
};

#endif // GOOGLEDRIVECONFIG_H
