#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Invoice Scanner");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Aether Solutions");

    // Splash screen (optional — remove if you have no splash image)
    QSplashScreen *splash = new QSplashScreen;
    splash->showMessage("Loading Invoice Scanner...",
                        Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    splash->show();
    app.processEvents();

    MainWindow w;

    QTimer::singleShot(1200, [&](){
        splash->finish(&w);
        w.show();
        delete splash;
    });

    return app.exec();
}
