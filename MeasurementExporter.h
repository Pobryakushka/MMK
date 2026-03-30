#ifndef MEASUREMENTEXPORTER_H
#define MEASUREMENTEXPORTER_H

#include <QString>
#include <QVector>
#include <QDateTime>

struct WindProfileData;
struct MeasuredWindData;
struct WindShearData;

struct MeasurementSnapshot
{
    int recordId = -1;
    QDateTime measurementTime;
    QString stationNumber;

    // Координаты станции
    double latitude = 0.0; // градусы, + = N
    double longitude = 0.0; // градусы, + = E
    double altitude = 0.0; // м над уровнем моря
    bool coordinatesValid = false;

    // Наземные метеоусловия
    double pressureHpa = 0.0;
    double temperatureC = 0.0;
    double humidityPct = 0.0;
    double surfaceWindDir = 0.0;  // градусы
    double surfaceWindSpeed = 0.0; // м/с
    bool surfaceMeteoValid = false;

    QVector<WindProfileData> avgWind;
    QVector<WindProfileData> actualWind;
    QVector<MeasuredWindData> measuredWind;

    QVector<WindShearData> windShear;

    struct Meteo11Export {
        bool valid = false;
        QString bulletinString;
        QString stationNumber;
        int day = 0;
        int hour = 0;
        int tenMinutes = 0;
        int stationAltitude = 0;
        int pressureDev = 0;
        int tempVirtDev = 0;
        int reachedTempKm = 0;
        int reachedWindKm = 0;
    };

    Meteo11Export meteo11Updated;
    Meteo11Export meteo11Approximate;
    Meteo11Export meteo11FromStation;
};

struct ExportOptions
{
    enum Format { TXT, CSV, JSON };

    Format format = TXT;

    bool includeCoordinates = true;
    bool includeSurfaceMeteo = true;
    bool includeAvgWind = true;
    bool includeActualWind = true;
    bool includeMeasuredWind = true;
    bool includeWindShear = true;
    bool includeMeteo11Updated = true;
    bool includeMeteo11Approx = false;
    bool includeMeteo11Station = false;

    QChar csvSeparator = ';';
};

class MeasurementExporter
{
public:
    static QString generate(const MeasurementSnapshot &snap, const ExportOptions &opts, QString &errorMsg);
    static QString suggestedFileName(const MeasurementSnapshot &snap, ExportOptions::Format format);

private:
    static QString generateTxt(const MeasurementSnapshot &snap, const ExportOptions &opts);
    static QString generateCsv(const MeasurementSnapshot &snap, const ExportOptions &opts);
    static QString generateJson(const MeasurementSnapshot &snap, const ExportOptions &opts);

    static QString latToString(double lat);
    static QString lonToString(double lon);
    static QString formatExt(ExportOptions::Format fmt);
};

#endif // MEASUREMENTEXPORTER_H
