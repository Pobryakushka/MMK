#ifndef MEASUREMENTRESULTS_H
#define MEASUREMENTRESULTS_H

#include <QDialog>
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "amsprotocol.h"
#include "zoom/zoomscontainer.h"
#include "MeasurementExporter.h"
#include "ExportDialog.h"
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_symbol.h>
#include <qwt_plot_canvas.h>
#include "WindShearCalculator.h"
#include <QTableWidget>

namespace Ui {
class MeasurementResults;
}

struct MeasurementRecord {
    int recordId;
    QDateTime measurementTime;
    bool hasAvgWind;
    bool hasActualWind;
    bool hasMeasuredWind;
    QString notes;

    MeasurementRecord() : recordId(-1), hasAvgWind(false),
        hasActualWind(false), hasMeasuredWind(false) {}
};

class MeasurementResults : public QDialog
{
    Q_OBJECT

public:
    explicit MeasurementResults(QWidget *parent = nullptr);
    ~MeasurementResults();

    //    void setDatabase(const QString &host, int port, const QString &dbName,
    //                    const QString &user, const QString &password);

private slots:
    void onPrevDateClicked();
    void onNextDateClicked();
    void onSelectDateClicked();
    void updateDisplay();
    void loadAvailableMeasurements();

    void onUpdatedButtonClicked();
    void onApproximateButtonClicked();
    void onFromMeteoStatButtonClicked();

    void onStringFormatClicked();
    void onTableFormatClicked();

public slots:
    void updateCoordinatesFromMainWindow(double latitude, double longitude);
    void setMapCoordinatesMode(bool enabled);
    void navigateToRecord(int recordId); // Перейти к записи по record_id
    void onExportClicked();

private:
    Ui::MeasurementResults *ui;
    QDateTime currentDateTime;
    QDateTime m_lockedDateTime;

    // Карта доступных измерений: дата -> (час -> список записей)
    QMap<QDate, QVector<MeasurementRecord>> availableMeasurements;

    enum BulletinType { Updated, Approximate, FromMeteoStat };
    enum OutputFormat { String, Table };

    BulletinType currentButtelinType;
    OutputFormat currentOutputFormat;

    bool m_mapCoordinatesMode;

    // Контейнер для управления масштабированием графиков
    ZoomsContainer *m_zoomsContainer;

    // База данных
    //    QSqlDatabase m_database;
    //    QString m_dbHost;
    //    int m_dbPort;
    //    QString m_dbName;
    //    QString m_dbUser;
    //    QString m_dbPassword;
    //    bool m_dbConfigured;

    void updateDateTimeDisplay();
    void updateSliderRange();
    void loadMeasurementData(const QDateTime &dateTime);
    QVector<MeasurementRecord> getRecordsForDate(const QDate &date);
    MeasurementRecord findClosestRecord(const QDate &date, int hour);
    void setupMockData();

    void switchMeteo11Display();

    // Методы работы с БД
    bool connectDatabase();
    void disconnectDatabase();
    void loadMeasurementsFromDatabase();

    // Загрузка данных профилей ветра (по record_id через wind_profiles_references)
    QVector<WindProfileData> loadAvgWindProfile(int recordId);
    QVector<WindProfileData> loadActualWindProfile(int recordId);
    QVector<MeasuredWindData> loadMeasuredWindProfile(int recordId);
    void loadSurfaceMeteoData(int recordId);
    void loadStationCoordinates(int recordId);
    void loadMeteo11FromStation(int recordId); // читает meteo_11_bulletin из БД

    // Отображение данных
    void displayWindProfile(const QVector<WindProfileData> &avgWind,
                            const QVector<WindProfileData> &actualWind,
                            const QVector<MeasuredWindData> &measuredWind);
    void updateAvailableRecordsLabel();

    void setupPlots();
    void setupZoom();  // Метод для настройки масштабирования графиков
    void plotWindSpeed(QwtPlot *plot, const QVector<WindProfileData> &data,
                       const QString &title, const QColor &color);
    void plotWindDirection(QwtPlot *plot, const QVector<WindProfileData> &data,
                           const QString &title, const QColor &color);
    void plotMeasuredWindSpeed(QwtPlot *plot, const QVector<MeasuredWindData> &data,
                               const QString &title, const QColor &color);
    void plotMeasuredWindDirection(QwtPlot *plot, const QVector<MeasuredWindData> &data,
                                   const QString &title, const QColor &color);
    void clearDisplayedData();

    // ============ СДВИГ ВЕТРА ============
    // UI элементы для вкладки сдвига ветра
    QwtPlot *m_windShearPlot;           // График сдвига
    QTableWidget *m_windShearTable;     // Таблица сдвига
    QwtPlotCurve *m_windShearCurve;     // Кривая для графика
    QwtPlotGrid *m_windShearGrid;       // Сетка для графика

    // Данные сдвига ветра
    QVector<WindShearData> m_currentShearData;

    // Методы для работы со сдвигом ветра
    void setupWindShearTab();           // Настройка вкладки сдвига
    void updateWindShearDisplay();      // Обновление отображения сдвига
    void clearWindShearDisplay();       // Очистка отображения

    void plotWindShear(const QVector<WindShearData> &shearData);  // Построение графика
    void updateWindShearTable(const QVector<WindShearData> &shearData); // Обновление таблицы

    // ============ МЕТЕО-11 ============

    // Структура, хранящая все закодированные поля бюллетеня
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
            int  heightCode;        // стандартная высота в коде бюллетеня
            int  windDir;           // ДД направление в больших делениях угломера (0-60, шаг 6°)
            int  windSpeed;         // СС скорость м/с
            bool isAbove10km;       // для высот ≥10 км высота в км (двузначная)
        };
        QVector<LayerData> layers;

        // --- Достигнутые высоты ---
        int reachedTempHeightKm;    // BтBт — достигнутая высота темп. зондирования, км
        int reachedWindHeightKm;    // BвBв — достигнутая высота ветрового зондирования, км

        // --- Метаданные для отображения (не входят в строку) ---
        QDateTime bulletinTime;     // время составления
        bool      isValid;          // бюллетень годен

        Meteo11Data() : day(0), hour(0), tenMinutes(0), stationAltitude(0),
            pressureDeviation(0), tempVirtualDev(0),
            reachedTempHeightKm(0), reachedWindHeightKm(0), isValid(false) {}
    };

    // Хранимые данные трёх типов бюллетеня
    Meteo11Data m_meteo11Updated;     // Уточнённый (после измерения АМС)
    Meteo11Data m_meteo11Approximate; // Приближённый (без данных метеостанции)
    Meteo11Data m_meteo11FromStation; // От метеостанции (исходный)

    // Вычисление и отображение
    void computeMeteo11(int recordId,
                        const QVector<WindProfileData>  &avgWind,
                        const QVector<WindProfileData>  &actualWind,
                        const QVector<MeasuredWindData> &measuredWind);

    Meteo11Data buildMeteo11(const QVector<WindProfileData> &windProfile,
                             double stationAltitudeM,
                             double pressureHpa,
                             double tempC,
                             const QDateTime &sondingTime,
                             bool useActual);

    void updateMeteo11Display();                        // Перерисовать вкладку
    void fillMeteo11StringView(const Meteo11Data &d);   // Заполнить textEdit_meteo11 / textEdit_meteo11_updated
    void fillMeteo11TableView(const Meteo11Data &d);    // Заполнить tableWidget_meteo11Formalize
    void fillMeteo11InfoFields(const Meteo11Data &d);   // Заполнить lineEdit_dt, _h, _p, _t, _ht, _hw
    void clearMeteo11Display();

    MeasurementSnapshot buildSnapshot() const;

    // Кодирование по протоколу
    static int  encodeWindDir(int degrees);             // градусы → делители угломера (0-60)
    static int  encodePressureDev(double deltaMmHg);    // отклонение давления → БББ
    static int  encodeTempDev(double deltaCelsius);     // отклонение темп. → ТТ
    static QString formatMeteo11Group(int heightCode, int dir, int speed, bool above10km);
    static QString buildMeteo11String(const Meteo11Data &d);

    // Параметры атмосферы для кодирования
    static double standardPressureAtAlt(double altM);   // стандартное давление на высоте
    static double standardTempAtAlt(double altM);       // стандартная темп. на высоте

    // Исходные данные для текущей записи (сохраняются при loadMeasurementData)
    double m_currentStationAltitude;
    double m_currentPressureHpa;
    double m_currentTempC;
    double m_currentWindDirSurface;
    double m_currentWindSpeedSurface;
    double m_currentLatitude;
    double m_currentLongitude;
    QDateTime m_currentSondingTime;
};

#endif // MEASUREMENTRESULTS_H