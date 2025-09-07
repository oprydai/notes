#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <QString>
#include <QStringList>
#include <QMap>

class ConfigLoader
{
public:
    static ConfigLoader& instance();
    
    // Load configuration from multiple sources
    bool loadConfig();
    
    // Get configuration values
    QString getClientId() const;
    QString getClientSecret() const;
    QString getRedirectUri() const;
    QString getScope() const;
    int getSyncInterval() const;
    QString getSyncFolderName() const;
    
    // Check if configuration is valid
    bool isValid() const;
    QStringList getValidationErrors() const;

private:
    ConfigLoader() = default;
    ~ConfigLoader() = default;
    ConfigLoader(const ConfigLoader&) = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;
    
    // Load from different sources (priority order)
    bool loadFromEnvironment();
    bool loadFromConfigFile();
    void loadFromDefaultConfig();
    
    // Validation
    bool validateConfig();
    
    // Configuration values
    QString m_clientId;
    QString m_clientSecret;
    QString m_redirectUri;
    QString m_scope;
    int m_syncInterval;
    QString m_syncFolderName;
    
    // Validation state
    bool m_isValid;
    QStringList m_validationErrors;
};

#endif // CONFIGLOADER_H
