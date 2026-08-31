#ifndef MEASUREMENTRESULTS_H
#define MEASUREMENTRESULTS_H

#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QListWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "sensors/amsprotocol.h"
#include "zoom/zoomscontainer.h"
#include "ui/MeasurementExporter.h"
#include "ui/ExportDialog.h"
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_symbol.h>
#include <qwt_plot_canvas.h>
#include "calculationAlgorithms/WindShearCalculator.h"
#include <QTableWidget>
#include "Meteo11Grib/GribMeteo11Pipeline.h"
#include "ui/notificationtoast.h"

namespace Ui {
class MeasurementResults;
}

class ArchiveDatePopup;
class ArchiveExportView;
class QPushButton;
class QLayout;
class QResizeEvent;
class QShowEvent;

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

// Встроенная страница общего стека MainWindow (как SourceData/GroundMeteoParams/
// регламентные работы) — раньше была отдельным QDialog, открывавшимся поверх
// главного окна отдельным всплывающим окном.
class MeasurementResults : public QWidget
{
    Q_OBJECT

public:
    explicit MeasurementResults(QWidget *parent = nullptr);
    ~MeasurementResults();

    //    void setDatabase(const QString &host, int port, const QString &dbName,
    //                    const QString &user, const QString &password);

signals:
    // Пользователь нажал "Закрыть" — MainWindow переключает stackedWidget
    // обратно на главный экран (как у остальных встроенных страниц).
    void backRequested();

private slots:
    void onPrevDateClicked();
    void onNextDateClicked();
    void onSelectDateClicked();
    void updateDisplay();
    void loadAvailableMeasurements();

    void onUpdatedButtonClicked();
    void onApproximateButtonClicked();
    void onFromMeteoStatButtonClicked();
    void onFromGribButtonClicked();
    void onGribPipelineFinished(bool success, const QVector<WindProfileData> &profile, const QString &error);

    void onStringFormatClicked();
    void onTableFormatClicked();

    void onExportClicked();
    void onExportBackRequested();
    void onExportSubmitted(const MeasurementSnapshot &snap, const ExportOptions &opts);
    void onDatePopupDateTimeSelected(const QDateTime &dt);

public slots:
    void updateCoordinatesFromMainWindow(double latitude, double longitude);
    void setMapCoordinatesMode(bool enabled);
    void navigateToRecord(int recordId); // Перейти к записи по record_id

private:
    Ui::MeasurementResults *ui;
    QDateTime currentDateTime;
    QDateTime m_lockedDateTime;

    // Карта доступных измерений: дата -> (час -> список записей)
    QMap<QDate, QVector<MeasurementRecord>> availableMeasurements;

    enum BulletinType { Updated, Approximate, FromMeteoStat, FromGrib };
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

    // Хранимые данные трёх типов бюллетеня
    Meteo11Data m_meteo11Updated;     // Уточнённый (после измерения АМС)
    Meteo11Data m_meteo11Approximate; // Приближённый (без данных метеостанции)
    Meteo11Data m_meteo11FromStation; // От метеостанции (исходный)
    Meteo11Data m_meteo11FromGrib;    // Из GRIB (прогностическое поле)

    GribMeteo11Pipeline *m_gribPipeline;

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
                             bool useActual,
                             const Meteo11Data *oldBulletin = nullptr);

    Meteo11Data buildMeteo11Approximate(double stationAltitudeM,
                                        double pressureHpa,
                                        double tempC,
                                        double surfaceWindDirDeg,
                                        double surfaceWindSpeedMs,
                                        const QDateTime &sondingTime);

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
    static QString formatMeteo11Group(int heightCode, const QString &pp, int dir, int speed, int tempDev, bool above10km, bool includePP = true, bool unavailable = false);
    static QString buildMeteo11String(const Meteo11Data &d);

    // Параметры атмосферы для кодирования
    static double standardPressureAtAlt(double altM);   // стандартное давление на высоте
    static double standardTempAtAlt(double altM);       // стандартная темп. на высоте

    // Исходные данные для текущей записи (сохраняются при loadMeasurementData)
    double m_currentStationAltitude;
    double m_currentPressureMmHg;
    double m_currentTempC;
    double m_currentWindDirSurface;
    double m_currentWindSpeedSurface;
    double m_currentLatitude;
    double m_currentLongitude;
    QDateTime m_currentSondingTime;

    // Кеш профилей ветра текущей записи
    QVector<WindProfileData> m_currentAvgWind;
    QVector<WindProfileData> m_currentActualWind;
    QVector<MeasuredWindData> m_currentMeasuredWind;

    NotificationToast *m_toast;
    void showStatus(const QString &text, NotificationToast::Kind kind);

    // ============ НОВЫЙ ВИЗУАЛЬНЫЙ СЛОЙ (боковая панель / попап даты / встроенный экспорт) ============
    ArchiveDatePopup  *m_datePopup;
    ArchiveExportView *m_exportView;
    bool m_amsProbeFieldsVisible;
    QList<QWidget *> m_amsProbeWidgets; // доп. поля АМС/зонда на странице "приближённый" — скрыты по умолчанию

    void applyArchiveStyle();          // общий QSS-стиль архива под макет
    static void setWidgetState(QWidget *w, const QString &state); // состояние через свойство [state]
    void clearStationCoordinates();    // сброс строк координат в прочерки
    void setupArchiveTables();         // общий вид таблиц архива под макет
    void setBulletinBadge(const QString &text, const QString &state); // пилюля годности Метео-11
    void fitMeteo11TextHeight();       // высота блока бюллетеня по содержимому
    void setupMeteo11TableLayout();    // компактная сетка табличного вида Метео-11

    // ─── Адаптивная (планшетная) компоновка ───────────────────────────────
    // Целевой планшет: 1200x1920 при масштабе 150% — окну достаётся 800
    // логических точек по ширине. Переключение идёт по фактической ширине
    // окна, а не по устройству, поэтому одинаково работает в обеих
    // ориентациях и на обычном мониторе.
    static constexpr int kNarrowWidthThreshold = 1000;
    bool m_narrowLayout = false;
    bool m_responsiveApplied = false;
    void applyResponsiveLayout(int width);
    void setMeteo11TableStacked(bool stacked);
    // Нижний предел высоты таблицы ПП/ТТДДСС по метрикам самой таблицы (шапка
    // + две строки + рамка), чтобы он следовал за системным шрифтом и при этом
    // помещался в самое низкое окно, которое допускает приложение.
    void updateMeteo11TableHeight();
    static void replaceWithFlowLayout(QLayout *source, int spacing);

protected:
    void resizeEvent(QResizeEvent *event) override;
    // Страница теперь постоянно живёт в стеке и не пересоздаётся при каждом
    // открытии — при каждом появлении на экране подтягиваем свежий список
    // измерений из БД (аналог того, что раньше происходило один раз в
    // конструкторе диалога, создававшегося заново на каждый клик).
    void showEvent(QShowEvent *event) override;

private:
    void setupAmsProbeCollapse();      // сворачиваемый блок доп. полей АМС/зонда

    // Оформление графиков Qwt под макет. QSS на QwtPlot почти не действует
    // (Qwt рисует холст, оси и кривые сам), поэтому цвета/шрифты/сетка
    // задаются через его собственный API.
    static void styleArchivePlot(QwtPlot *plot);
    static QwtPlotGrid *makeArchiveGrid();
    static void styleArchiveCurve(QwtPlotCurve *curve, const QColor &color);

    // Цвета кривых по макету: скорость — зелёная, направление — янтарное.
    static QColor archiveSpeedColor()     { return QColor("#0F6B4F"); }
    static QColor archiveDirectionColor() { return QColor("#F9A825"); }

    // Координаты станции показываются плоскими строками "подпись/значение"
    // (QLabel), а не полями ввода — в архиве они всегда только для чтения.
    // Флаг заменяет прежнюю проверку "поле координат не пустое".
    bool m_stationCoordsValid = false;

    // Нативная QTabBar рисуется системным QStyle и не даёт гарантированно
    // повторить плоские скруглённые вкладки макета ни на одной платформе —
    // поэтому сама QTabBar скрывается (tabBar()->hide()), а вместо неё
    // строится полностью самодельная строка кнопок, переключающая страницы
    // того же QTabWidget через setCurrentIndex().
    QWidget *m_customTabBar;
    QList<QPushButton *> m_tabButtons;
    void setupCustomTabBar();
    void updateCustomTabBarHighlight(int currentIndex);
};

#endif // MEASUREMENTRESULTS_H