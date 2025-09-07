#include "GoogleDriveConfig.h"
#include "ConfigLoader.h"

// Google Cloud Project credentials
// Load from ConfigLoader for security
const QString GoogleDriveConfig::CLIENT_ID = ConfigLoader::instance().getClientId();
const QString GoogleDriveConfig::CLIENT_SECRET = ConfigLoader::instance().getClientSecret();

// OAuth 2.0 settings
const QString GoogleDriveConfig::REDIRECT_URI = "urn:ietf:wg:oauth:2.0:oob";
const QString GoogleDriveConfig::SCOPE = "https://www.googleapis.com/auth/drive.file";

// API endpoints
const QString GoogleDriveConfig::API_BASE_URL = "https://www.googleapis.com/drive/v3";
const QString GoogleDriveConfig::AUTH_BASE_URL = "https://accounts.google.com/oauth/authorize";
const QString GoogleDriveConfig::TOKEN_BASE_URL = "https://oauth2.googleapis.com/token";

// Sync settings
const int GoogleDriveConfig::DEFAULT_SYNC_INTERVAL_MINUTES = 15;
const QString GoogleDriveConfig::SYNC_FOLDER_NAME = "Notes App";

// File settings
const QString GoogleDriveConfig::NOTE_FILE_EXTENSION = ".md";
const QString GoogleDriveConfig::NOTE_MIME_TYPE = "text/markdown";
