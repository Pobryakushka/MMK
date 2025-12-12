#ifndef MEASUREMENTRESULTS_H
#define MEASUREMENTRESULTS_H

#include <QDialog>
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QListWidget>
#include <QDialogButtonBox>

namespace Ui {
class MeasurementResults;
}

class MeasurementResults : public QDialog
{
    Q_OBJECT

public:
    explicit MeasurementResults(QWidget *parent = nullptr);
    ~MeasurementResults();

public slots:
    void updateCoordinatesFromMainwindow(double latitude, double longitude);
    void setMapCoordinatesMode(bool enabled);

private slots:
    void onPrevDateClicked();
    void onNextDateClicked();
    void onSelectDateClicked();
    void onTimeSliderChanged(int value);
    void updateDisplay();
    void loadAvailableMeasurements();

    void onUpdatedButtonClicked();
    void onApproximateButtonClicked();
    void onFromMeteoStatButtonClicked();

    void onStringFormatClicked();
    void onTableFormatClicked();

private:
    Ui::MeasurementResults *ui;
    QDateTime currentDateTime;

    QMap<QDate, QSet<QTime>> availableMeasurements;

    enum BulletinType { Updated, Approximate, FromMeteoStat };
    enum OutputFormat { String, Table };

    BulletinType currentButtelinType;
    OutputFormat currentOutputFormat;

    bool m_mapCoordinatesMode;

    void updateDateTimeDisplay();
    void updateSliderRange();
    void loadMeasurementData(const QDateTime &dateTime);
    QList<QTime> getAvailableTimesForDate(const QDate &date);
    void setupMockData();

    void switchMeteo11Display();
};

#endif // MEASUREMENTRESULTS_H
