#include "core/meteo11/Meteo11Codec.h"

#include <QStringList>
#include <QtMath>
#include <cmath>

namespace Meteo11Codec {

double standardPressureAtAlt(double altM)
{
    // МСА: P = 760 * (1 - 0.0000226 * h)^5.256
    double ratio = 1.0 - 0.0000226 * altM;
    if (ratio <= 0.0) return 0.0;
    return 760.0 * std::pow(ratio, 5.256);
}

double standardTempAtAlt(double altM)
{
    // Тропосфера: T = 15 - 6.5 * h/1000
    if (altM <= 11000.0)
        return 15.0 - 6.5 * altM / 1000.0;
    return -56.5; // Стратосфера
}

int encodeWindDir(int degrees)
{
    // Большие деления угломера: 1 д.у. = 6°, диапазон 0-60
    // Округление до ближайшего целого
    int du = qRound(degrees / 6.0);
    if (du >= 60) du = 0; // 360° = 00
    return du;
}

int encodePressureDev(double deltaMmHg)
{
    int val = qRound(deltaMmHg); // округление до целого мм рт.ст.
    if (val < 0) {
        val = 500 + val; // отрицательное: +500 к первой цифре (кодирование "минус")
    }
    // Ограничиваем диапазоном 000..999
    val = qBound(0, val, 999);
    return val;
}

int encodeTempDev(double deltaCelsius)
{
    int val = qRound(qAbs(deltaCelsius));
    val = qMin(val, 49); // максимум 49°
    if (deltaCelsius < 0.0) {
        val += 50; // кодирование "минус"
    }
    return val;
}

QString formatMeteo11Group(int heightCode, const QString &pp, int dir, int speed,
                           int tempDev, bool above10km, bool includePP,
                           bool unavailable)
{
    // Формат уточнённого (includePP=true):
    //  ≤8000 м:  ВВПП-ТТННСС  где ВВ = высота в сотнях метров (02..80), ПП — из данных
    //  ≥10 км:   ВВПП-ТТННСС  где ВВ = высота в км (10..30)
    // Формат приближённого (includePP=false):
    //  ВВ-ТТННСС  (без ПП)
    QString hPart;

    if (!above10km) {
        int hHundreds = heightCode / 100;
        if (includePP)
            hPart = QString("%1%2").arg(hHundreds, 2, 10, QChar('0')).arg(pp);
        else
            hPart = QString("%1").arg(hHundreds, 2, 10, QChar('0'));
    } else {
        if (includePP)
            hPart = QString("%1%2").arg(heightCode, 2, 10, QChar('0')).arg(pp);
        else
            hPart = QString("%1").arg(heightCode, 2, 10, QChar('0'));
    }

    // Нет данных → ТТ=00, НН=//, СС=//
    if (unavailable)
        return hPart + "-" + "00////";

    QString ssStr = (speed >= 99)
                        ? "//"
                        : QString("%1").arg(speed, 2, 10, QChar('0'));

    QString dataPart = QString("%1%2%3")
                           .arg(tempDev, 2, 10, QChar('0'))  // ТТ
                           .arg(dir,     2, 10, QChar('0'))  // НН
                           .arg(ssStr);                      // СС

    return hPart + "-" + dataPart;
}

QString buildMeteo11String(const Meteo11Data &d)
{
    if (!d.isValid)
        return "Метео 11 — нет данных";

    QStringList parts;

    // Заголовок
    if (d.isApproximate) {
        parts << "Метео 11 приближенный";
    } else {
        parts << QString("Метео 11%1").arg(d.stationNumber);
    }

    // ДДЧЧМ
    parts << QString("%1%2%3")
                 .arg(d.day,        2, 10, QChar('0'))
                 .arg(d.hour,       2, 10, QChar('0'))
                 .arg(d.tenMinutes, 1, 10, QChar('0'));

    // BBBB — высота станции
    parts << QString("%1").arg(d.stationAltitude, 4, 10, QChar('0'));

    // БББТ0Т0 — отклонение давления + отклонение виртуальной температуры
    parts << QString("%1%2")
                 .arg(d.pressureDeviation, 3, 10, QChar('0'))
                 .arg(d.tempVirtualDev,    2, 10, QChar('0'));

    // Слои: приближённый — без ПП (ВВ-ТТННСС), уточнённый — с ПП (ВВПП-ТТННСС)
    const bool includePP = !d.isApproximate;
    for (const Meteo11Data::LayerData &layer : d.layers) {
        parts << formatMeteo11Group(layer.heightCode, layer.pp,
                                    layer.windDir, layer.windSpeed, layer.tempDev,
                                    layer.isAbove10km, includePP,
                                    layer.isUnavailable);
    }

    // Достигнутые высоты BтBтBвBв (только для уточнённого)
    if (!d.isApproximate) {
        parts << QString("%1%2")
                     .arg(d.reachedTempHeightKm, 2, 10, QChar('0'))
                     .arg(d.reachedWindHeightKm, 2, 10, QChar('0'));
    }

    return parts.join("–");
}

} // namespace Meteo11Codec
