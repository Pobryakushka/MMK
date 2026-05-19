#ifndef WINDPROFILECALCULATOR_H
#define WINDPROFILECALCULATOR_H

// ─────────────────────────────────────────────────────────────────────────────
//  WindProfileCalculator
//
//  Обёртка над библиотеками ProfileReal / ClimatData.
//  Получает на вход:
//    • профиль измеренного ветра (от АМС, до 100 точек, поле reliability)
//    • наземный ветер (IWS)
//    • координаты + высоту станции
//    • дату/время зондирования
//
//  На выходе формирует:
//    • действительный ветер  (actualWind)  — QVector<WindProfileData>
//    • средний ветер         (avgWind)     — QVector<WindProfileData>
//
//  Использование:
//      WindProfileCalculator calc("climatData/climat/");
//      WindProfileCalculator::Input in;
//      in.measuredWind   = measuredFromAMS;
//      in.latitudeDeg    = 55.7;
//      in.longitudeDeg   = 37.6;
//      in.stationAltitudeM = 150.0f;
//      in.surfaceWindSpeedMs = 3.5f;
//      in.surfaceWindDirDeg  = 270.0f;
//      in.groundWindHeightM  = 10.0f;
//      in.sondingTime    = QDateTime::currentDateTime();
//      WindProfileCalculator::Output out;
//      auto err = calc.calculate(in, out);
//      // out.actualWind, out.avgWind — заполнены
// ─────────────────────────────────────────────────────────────────────────────

#include <QVector>
#include <QDateTime>
#include <QString>

#include "amsprotocol.h"   // WindProfileData, MeasuredWindData

class WindProfileCalculator
{
public:
    // Соответствует ProfileReal::MessErrors, плюс ошибки уровня обёртки.
    enum Result {
        OK = 0,
        ERR_NO_MEASURED_DATA      = 1,  // Нет ни одной валидной точки измерения
        ERR_PROFILE_NOT_CONFIG    = 2,  // ProfileReal вернул PROFILE_NOT_CONFIG_PROF_RPV
        ERR_CLIMAT_NOT_READY      = 3,  // Климатическая база не загрузилась для координат
        ERR_HEIGHT_NOT_CONFIG     = 4,  // ProfileReal вернул PROFILE_NOT_CONFIG_HEIGHT
        ERR_EXCEPTION             = 5,  // Любое необработанное исключение
    };

    struct Input {
        QVector<MeasuredWindData> measuredWind;      // от АМС (0xAC), 100 точек
        double  latitudeDeg     = 0.0;               // широта станции, град
        double  longitudeDeg    = 0.0;               // долгота станции, град
        float   stationAltitudeM = 0.0f;             // высота над уровнем моря, м
        float   surfaceWindSpeedMs = 0.0f;           // скорость наземного ветра (IWS), м/с
        float   surfaceWindDirDeg  = 0.0f;           // направление наземного ветра (IWS), град
        float   groundWindHeightM  = 10.0f;          // высота установки датчика IWS, м
        float   z0                 = 0.01f;          // параметр шероховатости подстилающей поверхности
        QDateTime sondingTime;                       // время зондирования
    };

    struct Output {
        QVector<WindProfileData> actualWind;         // действительный ветер по стандартным высотам
        QVector<WindProfileData> avgWind;            // средний ветер по стандартным высотам
        QString debugSummary;                        // короткая сводка для qDebug/логов
    };

    // climatDataPath — путь к директории климатических данных (как в примере).
    // По умолчанию "climatData/climat/" — совпадает с примером calculateProfile.txt.
    explicit WindProfileCalculator(const QString &climatDataPath = QStringLiteral("climatData/climat/"));
    ~WindProfileCalculator();

    Result calculate(const Input &in, Output &out);

    // Человекочитаемое описание кода результата.
    static QString resultString(Result r);

private:
    QString m_climatDataPath;
};

#endif // WINDPROFILECALCULATOR_H
