#pragma once

#include <QObject>
#include <QString>
#include <QLoggingCategory>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    static Logger& instance();
    
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled, const QString& filePath = QString());
    void setLogToConsole(bool enabled);
    
    void log(LogLevel level, const QString& category, const QString& message);
    void debug(const QString& category, const QString& message);
    void info(const QString& category, const QString& message);
    void warning(const QString& category, const QString& message);
    void error(const QString& category, const QString& message);
    void critical(const QString& category, const QString& message);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger() override;
    
    QString levelToString(LogLevel level) const;
    QString formatMessage(LogLevel level, const QString& category, const QString& message) const;
    
    LogLevel m_logLevel;
    bool m_logToFile;
    bool m_logToConsole;
    QString m_logFilePath;
    QFile* m_logFile;
    QTextStream* m_logStream;
    QMutex m_mutex;
};

// Convenience macros for easy logging
#define LOG_DEBUG(category, message) Logger::instance().debug(category, message)
#define LOG_INFO(category, message) Logger::instance().info(category, message)
#define LOG_WARNING(category, message) Logger::instance().warning(category, message)
#define LOG_ERROR(category, message) Logger::instance().error(category, message)
#define LOG_CRITICAL(category, message) Logger::instance().critical(category, message)

// Category definitions
Q_DECLARE_LOGGING_CATEGORY(database)
Q_DECLARE_LOGGING_CATEGORY(sync)
Q_DECLARE_LOGGING_CATEGORY(ui)
Q_DECLARE_LOGGING_CATEGORY(config)
Q_DECLARE_LOGGING_CATEGORY(file)
Q_DECLARE_LOGGING_CATEGORY(network)
