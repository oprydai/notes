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
#include "ui/MainWindow.h"

static void setApplicationIdentity() {
    QCoreApplication::setOrganizationName("Orchard");
    QCoreApplication::setOrganizationDomain("orchard.local");
    QCoreApplication::setApplicationName("Notes");
    QCoreApplication::setApplicationVersion("0.1.0");
}

int main(int argc, char *argv[]) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication app(argc, argv);

    setApplicationIdentity();
    
    // Simple single instance check using a lock file
    QString lockFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/notes-app.lock";
    QFile lock(lockFile);
    if (lock.exists()) {
        // Another instance might be running, check if it's still active
        QFileInfo lockInfo(lockFile);
        if (lockInfo.lastModified().secsTo(QDateTime::currentDateTime()) < 30) {
            // Lock file is recent, another instance is likely running
            return 0;
        }
    }
    
    // Create lock file
    lock.open(QIODevice::WriteOnly);
    lock.write(QDateTime::currentDateTime().toString().toUtf8());
    lock.close();
    
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
    app.setWindowIcon(appIcon);

    MainWindow window;
    window.resize(1200, 720);
    window.show();

    return app.exec();
}


