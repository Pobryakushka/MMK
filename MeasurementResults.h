#ifndef MEASUREMENTRESULTS_H
#define MEASUREMENTRESULTS_H

#include <QDialog>
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QListWidget>
#include <QDialogButtonBox>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>

namespace Ui {
class MeasurementResults;
}

class MeasurementResults : public QDialog
{
    Q_OBJECT

public:
    explicit MeasurementResults(QWidget *parent = nullptr);
    ~MeasurementResults();

private slots:
    void onPrevDateClicked();
    void onNextDateClicked();
    void onSelectDateClicked();
    void onTimeSliderChanged(int value);
    void updateDisplay();
    void loadAvailableMeasurements();

//    void onUpdatedButtonClicked();
//    void onApproximateButtonClicked();
//    void onFromMeteoStateButtonClicked();

//    void onStringFormatClicked();
//    void onTableFormatClicked();

private:
    Ui::MeasurementResults *ui;
    QDateTime currentDateTime;

    QMap<QDate, QSet<QTime>> availableMeasurements;

    enum BulletinType { Updated, Approximate, FromMeteoStat };
    enum OutputFormat { String, Table };

    BulletinType currentButtelinType;
    OutputFormat currentOutputFormat;

    // Графики и кривые для среднего ветра
    QwtPlotCurve d_curveAverageWindSpeed;
    QwtPlotCurve d_curveAverageWindAzimut;
    QwtPlotGrid d_gridAverageWindSpeed;
    QwtPlotGrid d_gridAverageWindAzimut;

    // Графики и кривые для действительного ветра
    QwtPlotCurve d_curveRealWindSpeed;
    QwtPlotCurve d_curveRealWindAzimut;
    QwtPlotGrid d_gridRealWindSpeed;
    QwtPlotGrid d_gridRealWindAzimut;

    // Графики и кривые для измеренного ветра
    QwtPlotCurve d_curveIzmWindSpeed;
    QwtPlotCurve d_curveIzmWindAzimut;
    QwtPlotGrid d_gridIzmWindSpeed;
    QwtPlotGrid d_gridIzmWindAzimut;

    void updateDateTimeDisplay();
    void updateSliderRange();
    void loadMeasurementData(const QDateTime &dateTime);
    QList<QTime> getAvailableTimesForDate(const QDate &date);
    void setupMockData();
    void switchMeteo11Display();

    // Методы для настройки графиков
    void setupPlot(QwtPlot *plot, QwtPlotCurve *curve, QwtPlotGrid *grid,
                   QwtSymbol *symbol, const QString &xAxisTitle, const QString &yAxisTitle,
                   double xMin, double xMax, double xStep);
    void setupPlotGrid(QwtPlotGrid *grid);
};

#endif // MEASUREMENTRESULTS_H
