#ifndef METEO11DATA_H
#define METEO11DATA_H

#include <QString>
#include <QDateTime>
#include <QVector>

// ─────────────────────────────────────────────────────────────────────────────
// Бюллетень Метео-11 в закодированном виде.
//
// Структура описывает ДАННЫЕ бюллетеня и не знает ничего об интерфейсе: ни о
// виджетах, ни о таблицах, ни о том, каким экраном она будет показана. Раньше
// она была вложена в приватную секцию класса MeasurementResults — то есть
// предметная модель протокола жила внутри виджета архива измерений.
//
// Кодирование полей выполняет Meteo11Codec (см. соседний заголовок).
// ─────────────────────────────────────────────────────────────────────────────
struct Meteo11Data {
    // --- Заголовок ---
    QString stationNumber;      // NNNNN  (условный номер, 5 цифр)
    int     day;                // ДД     — день месяца окончания зондирования
    int     hour;               // ЧЧ     — часы
    int     tenMinutes;         // М      — десятки минут (0-5)
    int     stationAltitude;    // BBBB   — высота станции над уровнем моря, м (+60)
    int     pressureDeviation;  // БББ    — отклонение давления, мм рт.ст. (-6 закодировано +5)
    int     tempVirtualDev;     // T0T0   — отклонение виртуальной темп., °С (-31 закод.)

    // --- Слои ---
    // Каждый слой: TTHHНСС — откл. темп.(ТТ), дирекц.угол направления(НН),
    //                        скорость ветра(СС)
    // Ниже 10 км — группы 4-значные (ППТТНН) + 6-значные (ССНН)  (реально хранится как пары)
    // Высоты стандартные: 02(200м), 04, 08, 12, 16, 24, 30, 40, 50, 60, 80,
    //                     10(1000м), 12, 14, 18, 22, 26, 30 (км)
    struct LayerData {
        int     heightCode;     // стандартная высота в коде бюллетеня
        int     windDir;        // ДД направление в больших делениях угломера (0-60, шаг 6°)
        int     windSpeed;      // СС скорость м/с
        int     tempDev;        // ТТ — отклонение температуры, закодированное (0 = нет данных)
        bool    isAbove10km;    // для высот ≥10 км высота в км (двузначная)
        bool    isUnavailable;  // true → нет данных, в строку пишем 00////
        QString pp;             // ПП — поправка за плотность ("//" если не измерялась)
        LayerData() : heightCode(0), windDir(0), windSpeed(0), tempDev(0),
                      isAbove10km(false), isUnavailable(false), pp("//") {}
    };
    QVector<LayerData> layers;

    // --- Достигнутые высоты ---
    int reachedTempHeightKm;    // BтBт — достигнутая высота темп. зондирования, км
    int reachedWindHeightKm;    // BвBв — достигнутая высота ветрового зондирования, км

    // --- Метаданные для отображения (не входят в строку) ---
    QDateTime bulletinTime;     // время составления
    bool      isValid;          // бюллетень годен
    bool      isApproximate;    // true → приближённый формат
    QString   rawString;        // сырая строка от МС (для FromMeteoStat)

    Meteo11Data() : day(0), hour(0), tenMinutes(0), stationAltitude(0),
        pressureDeviation(0), tempVirtualDev(0),
        reachedTempHeightKm(0), reachedWindHeightKm(0),
        isValid(false), isApproximate(false) {}
};

#endif // METEO11DATA_H
