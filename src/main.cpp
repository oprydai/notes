#include <QApplication>
#include <QGuiApplication>
#include <QFontDatabase>
#include <QPalette>
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

    MainWindow window;
    window.resize(1200, 720);
    window.show();

    return app.exec();
}


