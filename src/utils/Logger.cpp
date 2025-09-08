#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <iostream>

// Define logging categories
Q_LOGGING_CATEGORY(database, "notes.database")
Q_LOGGING_CATEGORY(sync, "notes.sync")
Q_LOGGING_CATEGORY(ui, "notes.ui")
Q_LOGGING_CATEGORY(config, "notes.config")
Q_LOGGING_CATEGORY(file, "notes.file")
Q_LOGGING_CATEGORY(network, "notes.network")

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_logLevel(Info)  // Default to Info level in production
    , m_logToFile(false)
    , m_logToConsole(false)
    , m_logFile(nullptr)
    , m_logStream(nullptr)
{
    // Set up default logging based on build type
#ifdef QT_DEBUG
    m_logLevel = Debug;
    m_logToConsole = true;
#else
    m_logLevel = Warning;  // Only warnings and errors in release
    m_logToConsole = false;
#endif
}

Logger::~Logger()
{
    if (m_logStream) {
        delete m_logStream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

void Logger::setLogToFile(bool enabled, const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_logToFile == enabled) {
        return;
    }
    
    m_logToFile = enabled;
    
    if (enabled) {
        if (filePath.isEmpty()) {
            // Use default log file location
            QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir().mkpath(logDir);
            m_logFilePath = logDir + "/notes.log";
        } else {
            m_logFilePath = filePath;
        }
        
        m_logFile = new QFile(m_logFilePath);
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_logStream = new QTextStream(m_logFile);
            m_logStream->setCodec("UTF-8");
        } else {
            delete m_logFile;
            m_logFile = nullptr;
            m_logToFile = false;
        }
    } else {
        if (m_logStream) {
            delete m_logStream;
            m_logStream = nullptr;
        }
        if (m_logFile) {
            m_logFile->close();
            delete m_logFile;
            m_logFile = nullptr;
        }
    }
}

void Logger::setLogToConsole(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_logToConsole = enabled;
}

void Logger::log(LogLevel level, const QString& category, const QString& message)
{
    if (level < m_logLevel) {
        return;
    }
    
    QString formattedMessage = formatMessage(level, category, message);
    
    QMutexLocker locker(&m_mutex);
    
    if (m_logToConsole) {
        std::cerr << formattedMessage.toStdString() << std::endl;
    }
    
    if (m_logToFile && m_logStream) {
        *m_logStream << formattedMessage << Qt::endl;
        m_logStream->flush();
    }
}

void Logger::debug(const QString& category, const QString& message)
{
    log(Debug, category, message);
}

void Logger::info(const QString& category, const QString& message)
{
    log(Info, category, message);
}

void Logger::warning(const QString& category, const QString& message)
{
    log(Warning, category, message);
}

void Logger::error(const QString& category, const QString& message)
{
    log(Error, category, message);
}

void Logger::critical(const QString& category, const QString& message)
{
    log(Critical, category, message);
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case Debug: return "DEBUG";
        case Info: return "INFO";
        case Warning: return "WARN";
        case Error: return "ERROR";
        case Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

QString Logger::formatMessage(LogLevel level, const QString& category, const QString& message) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    return QString("[%1] %2 [%3] %4").arg(timestamp, levelStr, category, message);
}
