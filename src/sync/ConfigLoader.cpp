#include "ConfigLoader.h"
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>

ConfigLoader& ConfigLoader::instance()
{
    static ConfigLoader instance;
    return instance;
}

bool ConfigLoader::loadConfig()
{
    // Clear previous state
    m_isValid = false;
    m_validationErrors.clear();
    
    // Try to load from different sources in priority order
    if (loadFromEnvironment()) {
        qDebug() << "Configuration loaded from environment variables";
    } else if (loadFromConfigFile()) {
        qDebug() << "Configuration loaded from config file";
    } else {
        loadFromDefaultConfig();
        qDebug() << "Configuration loaded from defaults (not functional)";
    }
    
    // Validate the configuration
    return validateConfig();
}

bool ConfigLoader::loadFromEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    m_clientId = env.value("GOOGLE_DRIVE_CLIENT_ID");
    m_clientSecret = env.value("GOOGLE_DRIVE_CLIENT_SECRET");
    m_redirectUri = env.value("GOOGLE_DRIVE_REDIRECT_URI", "urn:ietf:wg:oauth:2.0:oob");
    m_scope = env.value("GOOGLE_DRIVE_SCOPE", "https://www.googleapis.com/auth/drive.file");
    m_syncInterval = env.value("GOOGLE_DRIVE_SYNC_INTERVAL", "15").toInt();
    m_syncFolderName = env.value("GOOGLE_DRIVE_SYNC_FOLDER", "Notes App");
    
    // Check if we have the essential credentials
    return !m_clientId.isEmpty() && !m_clientSecret.isEmpty();
}

bool ConfigLoader::loadFromConfigFile()
{
    // Try multiple locations for the config file
    QStringList configPaths = {
        QDir::currentPath() + "/config/google_drive_config.ini",  // Current directory
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/google_drive_config.ini",  // App data
        QDir::homePath() + "/.notes_app/google_drive_config.ini",  // Home directory
        QDir::currentPath() + "/google_drive_config.ini",  // Root of current directory
        QCoreApplication::applicationDirPath() + "/../config/google_drive_config.ini",  // Relative to executable
        QCoreApplication::applicationDirPath() + "/../../config/google_drive_config.ini"  // Two levels up from executable
    };
    
    qDebug() << "Current working directory:" << QDir::currentPath();
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    
    QFile configFile;
    for (const QString& path : configPaths) {
        qDebug() << "Trying config path:" << path;
        configFile.setFileName(path);
        if (configFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Successfully opened config file:" << path;
            break;
        } else {
            qDebug() << "Failed to open:" << path << "Error:" << configFile.errorString();
        }
    }
    
    if (!configFile.isOpen()) {
        qDebug() << "No config file found in any of these locations:" << configPaths;
        return false;
    }
    
    QTextStream stream(&configFile);
    QString line;
    
    while (stream.readLineInto(&line)) {
        line = line.trimmed();
        
        // Skip comments and empty lines
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        
        // Parse key=value pairs
        int equalPos = line.indexOf('=');
        if (equalPos > 0) {
            QString key = line.left(equalPos).trimmed();
            QString value = line.mid(equalPos + 1).trimmed();
            
            if (key == "client_id") {
                m_clientId = value;
            } else if (key == "client_secret") {
                m_clientSecret = value;
            } else if (key == "redirect_uri") {
                m_redirectUri = value;
            } else if (key == "scope") {
                m_scope = value;
            } else if (key == "sync_interval") {
                m_syncInterval = value.toInt();
            } else if (key == "sync_folder") {
                m_syncFolderName = value;
            }
        }
    }
    
    configFile.close();
    
    // Check if we have the essential credentials
    return !m_clientId.isEmpty() && !m_clientSecret.isEmpty();
}

void ConfigLoader::loadFromDefaultConfig()
{
    // Set default values (these won't work for actual API calls)
    m_clientId = "";
    m_clientSecret = "";
    m_redirectUri = "urn:ietf:wg:oauth:2.0:oob";
    m_scope = "https://www.googleapis.com/auth/drive.file";
    m_syncInterval = 15;
    m_syncFolderName = "Notes App";
}

bool ConfigLoader::validateConfig()
{
    m_validationErrors.clear();
    
    if (m_clientId.isEmpty()) {
        m_validationErrors << "Client ID is missing";
    }
    
    if (m_clientSecret.isEmpty()) {
        m_validationErrors << "Client Secret is missing";
    }
    
    if (m_redirectUri.isEmpty()) {
        m_validationErrors << "Redirect URI is missing";
    }
    
    if (m_scope.isEmpty()) {
        m_validationErrors << "Scope is missing";
    }
    
    if (m_syncInterval <= 0) {
        m_validationErrors << "Sync interval must be positive";
    }
    
    if (m_syncFolderName.isEmpty()) {
        m_validationErrors << "Sync folder name is missing";
    }
    
    m_isValid = m_validationErrors.isEmpty();
    return m_isValid;
}

QString ConfigLoader::getClientId() const
{
    return m_clientId;
}

QString ConfigLoader::getClientSecret() const
{
    return m_clientSecret;
}

QString ConfigLoader::getRedirectUri() const
{
    return m_redirectUri;
}

QString ConfigLoader::getScope() const
{
    return m_scope;
}

int ConfigLoader::getSyncInterval() const
{
    return m_syncInterval;
}

QString ConfigLoader::getSyncFolderName() const
{
    return m_syncFolderName;
}

bool ConfigLoader::isValid() const
{
    return m_isValid;
}

QStringList ConfigLoader::getValidationErrors() const
{
    return m_validationErrors;
}
