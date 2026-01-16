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

private:
    Ui::MeasurementResults *ui;
    QDateTime currentDateTime;
    QDateTime m_lockedDateTime;

    // Карта доступных измерений: дата -> (час -> список записей)
    QMap<QDate, QMap<int, QVector<MeasurementRecord>>> availableMeasurements;

    enum BulletinType { Updated, Approximate, FromMeteoStat };
    enum OutputFormat { String, Table };

    BulletinType currentButtelinType;
    OutputFormat currentOutputFormat;

    bool m_mapCoordinatesMode;

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
    QList<int> getAvailableHoursForDate(const QDate &date);
    MeasurementRecord findClosestRecord(const QDate &date, int hour);
    void setupMockData();

    void switchMeteo11Display();

    // Методы работы с БД
    bool connectDatabase();
    void disconnectDatabase();
    void loadMeasurementsFromDatabase();

    // Загрузка данных профилей ветра
    QVector<WindProfileData> loadAvgWindProfile(const QDateTime &time);
    QVector<WindProfileData> loadActualWindProfile(const QDateTime &time);
    QVector<MeasuredWindData> loadMeasuredWindProfile(const QDateTime &time);

    // Отображение данных
    void displayWindProfile(const QVector<WindProfileData> &avgWind,
                          const QVector<WindProfileData> &actualWind,
                          const QVector<MeasuredWindData> &measuredWind);
    void updateAvailableRecordsLabel();
};

#endif // MEASUREMENTRESULTS_H
