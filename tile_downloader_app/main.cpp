#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("TileDownloaderApp");
    app.setOrganizationName("MMK");

    MainWindow w;
    w.show();
    return app.exec();
}
