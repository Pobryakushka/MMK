#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Включаем поддержку HiDPI-экранов (должно быть ДО создания QApplication)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    // Qt 5.14+: плавное масштабирование (не кратное), убирает размытость на дробных DPI
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication a(argc, argv);

    // Необходимо для QSettings — настройки сохраняются в
    // ~/.config/MMK/MMK.conf
    a.setOrganizationName("412");
    a.setOrganizationDomain("mmk.local");
    a.setApplicationName("MMK");

    MainWindow w;
    // Открываем окно развёрнутым — содержимое гарантированно помещается
    // и масштабируется под реальный размер экрана
    w.showMaximized();

    return a.exec();
}
