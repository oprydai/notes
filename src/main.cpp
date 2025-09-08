#include <QApplication>
#include <QGuiApplication>
#include <QFontDatabase>
#include <QPalette>
#include <QIcon>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QLoggingCategory>
#include <QDebug>
#include "ui/MainWindow.h"
#include "utils/Logger.h"

static void setApplicationIdentity() {
    QCoreApplication::setOrganizationName("Orchard");
    QCoreApplication::setOrganizationDomain("orchard.local");
    QCoreApplication::setApplicationName("Notes");
    QCoreApplication::setApplicationVersion("0.1.0");
}

static void setupLogging() {
    // Set up the custom logger
    Logger& logger = Logger::instance();
    
    #ifdef QT_DEBUG
    // In debug builds, log everything to console
    logger.setLogLevel(Logger::Debug);
    logger.setLogToConsole(true);
    #else
    // In release builds, only log warnings and errors to file
    logger.setLogLevel(Logger::Warning);
    logger.setLogToConsole(false);
    logger.setLogToFile(true);  // Log to file in release
    #endif
    
    // Disable Qt's default debug output in production builds
    #ifndef QT_DEBUG
    QLoggingCategory::setFilterRules("*.debug=false\n*.info=false\n*.warning=false");
    #endif
}

static void showErrorMessage(const QString& title, const QString& message) {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

int main(int argc, char *argv[]) {
    try {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
        QApplication app(argc, argv);

        setApplicationIdentity();
        setupLogging();
        
        // Simple single instance check using a lock file
        QString lockFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/notes-app.lock";
        QFile lock(lockFile);
        if (lock.exists()) {
            // Another instance might be running, check if it's still active
            QFileInfo lockInfo(lockFile);
            if (lockInfo.lastModified().secsTo(QDateTime::currentDateTime()) < 30) {
                // Lock file is recent, another instance is likely running
                LOG_INFO("app", "Another instance of Notes is already running");
                return 0;
            }
        }
        
        // Create lock file
        if (!lock.open(QIODevice::WriteOnly)) {
            LOG_WARNING("app", QString("Could not create lock file: %1").arg(lock.errorString()));
            // Continue anyway, this is not critical
        } else {
            lock.write(QDateTime::currentDateTime().toString().toUtf8());
            lock.close();
        }
        
        // Set WM_CLASS for proper desktop integration
        app.setProperty("WM_CLASS", "Notes");
        
        // For X11 systems, set additional properties for proper desktop integration
        #ifdef Q_OS_LINUX
        app.setProperty("X11_WM_CLASS", "Notes");
        // Set the application name explicitly for WM_CLASS
        app.setApplicationName("Notes");
        #endif
        
        // Set application icon from embedded resources
        QIcon appIcon(":/icons/notes.svg");
        if (appIcon.isNull()) {
            LOG_WARNING("app", "Could not load application icon");
        } else {
            app.setWindowIcon(appIcon);
        }

        MainWindow window;
        window.resize(1200, 720);
        window.show();

        return app.exec();
        
    } catch (const std::exception& e) {
        LOG_CRITICAL("app", QString("Unhandled exception in main: %1").arg(e.what()));
        showErrorMessage("Application Error", 
            "An unexpected error occurred while starting the application.\n\n"
            "Please try restarting the application. If the problem persists, "
            "contact support.");
        return 1;
    } catch (...) {
        LOG_CRITICAL("app", "Unknown exception in main");
        showErrorMessage("Application Error", 
            "An unexpected error occurred while starting the application.\n\n"
            "Please try restarting the application. If the problem persists, "
            "contact support.");
        return 1;
    }
}


