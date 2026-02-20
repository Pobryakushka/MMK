#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Необходимо для QSettings — настройки сохраняются в
    // ~/.config/MMK/MMK.conf
    a.setOrganizationName("412");
    a.setOrganizationDomain("mmk.local");
    a.setApplicationName("MMK");

    MainWindow w;
    w.show();

    return a.exec();
}
