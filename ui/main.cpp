/**
 * @file main.cpp
 * @brief Entry point for the UR Dining Dollars desktop dashboard.
 *
 * Boots a QApplication, applies the bundled stylesheet (compiled into the
 * binary via Qt resources, see resources.qrc → styles.qss), and shows the
 * main window. The window owns the data-loading, charts, and tray notifier.
 */

#include "MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("UR Dining Dollars");
    app.setOrganizationName("UR Dining Dollars");

    // Force the cross-platform "Fusion" style so chrome looks the same on
    // every OS — otherwise stylesheet rules tuned for one platform's native
    // widgets (rounded corners, padding) misalign on the other.
    app.setStyle(QStyleFactory::create("Fusion"));

    // Load the stylesheet from the Qt resource system (compiled-in, not a
    // runtime file lookup). See resources.qrc.
    QFile qss(":/styles.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    ui::MainWindow w;
    w.show();
    return app.exec();
}
