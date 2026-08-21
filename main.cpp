#include "ui/mainwindow.h"
#include "calculationAlgorithms/WindShearCalculator.h"
#include <QApplication>
#include "calculationAlgorithms/windprofilecalculator.h"
#include <iostream>

#ifdef DEZHURNY_SELFTEST
void runDezhurnySelfTest()
{
    std::cout << "\n========================================\n";
    std::cout << "  ТЕСТ ДЕЖУРНОГО РЕЖИМА (k=0, реальные координаты)\n";
    std::cout << "========================================\n";

    WindProfileCalculator calc("climatData/climat/");

    WindProfileCalculator::Input in;
    in.measuredWind.clear();               // ← ПУСТО, имитируем k=0 (дежурный режим)
    in.latitudeDeg        = 55.7558;
    in.longitudeDeg       = 37.6172;
    in.stationAltitudeM   = 156.0f;
    in.surfaceWindSpeedMs = 13.0f;
    in.surfaceWindDirDeg  = 311.0f;
    in.groundWindHeightM  = 10.0f;          // уточните реальную высоту датчика IWS
    in.z0                 = 0.01f;
    in.sondingTime        = QDateTime(QDate(2026, 8, 21), QTime(12, 0));  // месяц=8

    std::cout << "[TEST] Входные данные: lat=" << in.latitudeDeg
              << " lon=" << in.longitudeDeg
              << " alt=" << in.stationAltitudeM
              << " surfaceWind=" << in.surfaceWindSpeedMs << "м/с "
              << in.surfaceWindDirDeg << "°"
              << " измеренных точек=" << in.measuredWind.size() << std::endl;

    WindProfileCalculator::Output out;
    auto result = calc.calculate(in, out);

    std::cout << "\n[TEST] Результат: " << WindProfileCalculator::resultString(result).toStdString()
              << "\n[TEST] " << out.debugSummary.toStdString() << std::endl;

    std::cout << "\n[TEST] --- Средний ветер (avg) ---" << std::endl;
    for (int i = 0; i < out.avgWind.size(); ++i) {
        const auto &p = out.avgWind[i];
        std::cout << "  #" << i << "  h=" << p.height
                  << "  V=" << p.windSpeed
                  << "  dir=" << p.windDirection
                  << "  valid=" << p.isValid << std::endl;
    }
    std::cout << "========================================\n\n";
}
#endif

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

#ifdef DEZHURNY_SELFTEST
    runDezhurnySelfTest();
    return 0;   // выходим сразу после теста, GUI не поднимаем
#endif

    WindShearCalculator::runSelfTest();
    MainWindow w;
    // Открываем окно развёрнутым — содержимое гарантированно помещается
    // и масштабируется под реальный размер экрана
    w.showMaximized();
    return a.exec();
}