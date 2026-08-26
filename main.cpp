#include "ui/mainwindow.h"
#include "calculationAlgorithms/WindShearCalculator.h"
#include <QApplication>
#include <QStyleFactory>
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

    // Явно фиксируем стиль Fusion — без этого Qt5 берёт стиль из платформенной
    // темы окружения (см. QT_QPA_PLATFORMTHEME), а на Astra Linux/Fly это
    // оказался "cleanlooks" (нативные объёмные кнопки в духе старых GTK-тем)
    // вместо плоского Fusion, под который написаны все QSS в .ui/.cpp.
    // Практически важно то, что cleanlooks игнорирует значительную часть
    // указаний стилшита (фон/рамку/скругление кнопок, вкладок, шапок таблиц)
    // и дорисовывает поверх свой объёмный chrome — интерфейс выглядит "чужим"
    // независимо от того, что написано в QSS. Fusion же ничего не рисует там,
    // где QSS задал оформление, поэтому вид программы перестаёт зависеть от
    // темы конкретного рабочего окружения.
    a.setStyle(QStyleFactory::create("Fusion"));

    // ВАЖНО: QApplication подхватывает системную локаль (setlocale(LC_ALL, "")),
    // что на русской локали меняет десятичный разделитель на запятую и ломает
    // locale-зависимый парсинг чисел (atof/strtod/std::stod) внутри climatData,
    // читающей числа с точкой как разделителем. Возвращаем числовую локаль в "C".
    std::setlocale(LC_NUMERIC, "C");

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