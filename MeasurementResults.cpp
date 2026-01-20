#include "MeasurementResults.h"
#include "ui_MeasurementResults.h"
#include <QCalendarWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <QPen>
#include <cstdlib>
#include <ctime>

MeasurementResults::MeasurementResults(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeasurementResults)
    , currentButtelinType(Updated)
    , currentOutputFormat(String)
{
    ui->setupUi(this);

    currentDateTime = QDateTime::currentDateTime();
    int minutes = currentDateTime.time().minute();
    minutes = (minutes / 10) * 10;
    currentDateTime.setTime(QTime(currentDateTime.time().hour(), minutes, 0));

    setupMockData();

    // Настройка графиков для среднего ветра
    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setBrush(Qt::red);
    symbol->setSize(5);
    setupPlot(ui->plot_midWindSpeed, &d_curveAverageWindSpeed, &d_gridAverageWindSpeed,
              symbol, "Скорость, м/с", "Высота, м", 0, 50, 10);

    symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setBrush(Qt::red);
    symbol->setSize(5);
    setupPlot(ui->plot_midWindAzimut, &d_curveAverageWindAzimut, &d_gridAverageWindAzimut,
              symbol, "Направление, град", "Высота, м", 0, 360, 60);

    // Настройка графиков для действительного ветра
    symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setBrush(Qt::red);
    symbol->setSize(5);
    setupPlot(ui->plot_realWindSpeed, &d_curveRealWindSpeed, &d_gridRealWindSpeed,
              symbol, "Скорость, м/с", "Высота, м", 0, 50, 10);

    symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setBrush(Qt::red);
    symbol->setSize(5);
    setupPlot(ui->plot_realWindAzimut, &d_curveRealWindAzimut, &d_gridRealWindAzimut,
              symbol, "Направление, град", "Высота, м", 0, 360, 60);

    // Настройка графиков для измеренного ветра
    symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setBrush(Qt::red);
    symbol->setSize(5);
    setupPlot(ui->plot_izmWindSpeed_2, &d_curveIzmWindSpeed, &d_gridIzmWindSpeed,
              symbol, "Скорость, м/с", "Высота, м", 0, 50, 10);

    symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setBrush(Qt::red);
    symbol->setSize(5);
    setupPlot(ui->plot_izmWindAzimut_2, &d_curveIzmWindAzimut, &d_gridIzmWindAzimut,
              symbol, "Направление, град", "Высота, м", 0, 360, 60);

    loadAvailableMeasurements();
    updateDateTimeDisplay();
    updateSliderRange();

    connect(ui->btnPrevDate, &QPushButton::clicked, this, &MeasurementResults::onPrevDateClicked);
    connect(ui->btnNextDate, &QPushButton::clicked, this, &MeasurementResults::onNextDateClicked);
    connect(ui->btnSelectDate, &QPushButton::clicked, this, &MeasurementResults::onSelectDateClicked);
    connect(ui->timeSlider, &QSlider::valueChanged, this, &MeasurementResults::onTimeSliderChanged);

//    connect(ui->pushButton_updated, &QPushButton::clicked, this, &MeasurementResults::onUpdatedButtonClicked);
//    connect(ui->pushButton_approximate, &QPushButton::clicked, this, &MeasurementResults::onApproximateButtonClicked);
//    connect(ui->pushButton_fromMeteoStat, &QPushButton::clicked, this, &MeasurementResults::onFromMeteoStateButtonClicked);

//    connect(ui->pushButton_string, &QPushButton::clicked, this, &MeasurementResults::onStringFormatClicked);
//    connect(ui->pushButton_table, &QPushButton::clicked, this, &MeasurementResults::onTableFormatClicked);

//    switchMeteo11Display();
}

MeasurementResults::~MeasurementResults()
{
    delete ui;
}

//void MeasurementResults::switchMeteo11Display()
//{
//    QStackedWidget *stackedWidget = ui->meteo11StackedWidget;
//}

void MeasurementResults::setupMockData()
{
    QDate today = QDate::currentDate();

    for (int dayOffset = -7; dayOffset <= 0; dayOffset++){
        QDate date = today.addDays(dayOffset);
        QSet<QTime> times;

        for (int hour = 8; hour <= 20; hour++){
            for (int minute = 0; minute < 60; minute += 30){
                times.insert(QTime(hour, minute, 0));
            }
        }
        availableMeasurements[date] = times;
    }
}

void MeasurementResults::loadAvailableMeasurements()
{
    updateDisplay();
}

void MeasurementResults::updateDateTimeDisplay()
{
    QString dateTimeStr = currentDateTime.toString("dd.MM.yyyy hh:mm");
    ui->lblCurrentDateTime->setText(dateTimeStr);

    int totalMinutes = currentDateTime.time().hour() * 60 + currentDateTime.time().minute();
    int sliderValue = totalMinutes / 10;
    ui->timeSlider->blockSignals(true);
    ui->timeSlider->setValue(sliderValue);
    ui->timeSlider->blockSignals(false);

    loadMeasurementData(currentDateTime);
}

void MeasurementResults::updateSliderRange()
{
    QDate currentDate = currentDateTime.date();
    QList<QTime> availableTimes = getAvailableTimesForDate(currentDate);

    if (!availableTimes.isEmpty()){
        ui->lblAvailableRecords->setText(QString("Доступно записей: %1").arg(availableTimes.count()));
    } else {
        ui->lblAvailableRecords->setText("Нет данных за выбранную дату");
    }
}

QList<QTime> MeasurementResults::getAvailableTimesForDate(const QDate &date)
{
    QList<QTime> times;
    if (availableMeasurements.contains(date)) {
        times = availableMeasurements[date].values();
        std::sort(times.begin(), times.end());
    }
    return times;
}

void MeasurementResults::loadMeasurementData(const QDateTime &dateTime)
{
    QDate date = dateTime.date();
    QTime time = dateTime.time();

    if (availableMeasurements.contains(date) && availableMeasurements[date].contains(time)){
        ui->lblDataStatus->setText("Данные загружены");
        ui->lblDataStatus->setStyleSheet("color: green; font-weight: bold;");

        // TODO: Для загрузки реальных данных нужно:
        // 1. Создать механизм сохранения измерений из dataStore в БД (с датой/временем)
        // 2. Добавить SQL запросы для загрузки данных по выбранной дате/времени
        // 3. Использовать загруженные данные вместо тестовых
        //
        // Пример интеграции с dataStore (если данные доступны в реальном времени):
        // extern Store dataStore;
        // int size = dataStore.requestData.izmWind.LENGTH;
        // for (int i = 0; i < size; i++) {
        //     if (dataStore.requestData.izmWind.data[i].curr) {
        //         izmSpeed[s] = dataStore.requestData.izmWind.data[i].speed;
        //         izmAzimut[s] = dataStore.requestData.izmWind.data[i].azimut;
        //         izmHeight[s] = dataStore.requestData.izmWind.data[i].height; // ВАЖНО: высота берется из данных!
        //         s++;
        //     }
        // }

        // Генерация тестовых данных для действительного ветра (30 точек)
        const int realWindSize = 30;
        double realSpeed[realWindSize], realAzimut[realWindSize], realHeight[realWindSize];

        // Генерация высот для действительного ветра по логике из storetypes.h
        int plus = 25;
        for (int i = 0; i < realWindSize; i++) {
            if (i == 0) {
                realHeight[i] = 50;
            } else {
                if (realHeight[i - 1] == 100)
                    plus = 50;
                else if (realHeight[i - 1] == 200)
                    plus = 100;
                else if (realHeight[i - 1] == 1000)
                    plus = 200;
                realHeight[i] = realHeight[i - 1] + plus;
            }
            // Генерация случайных значений скорости и направления для демонстрации
            realSpeed[i] = 5.0 + (rand() % 100) / 10.0;
            realAzimut[i] = (rand() % 360);
        }

        // Отображение данных действительного ветра на графиках
        d_curveRealWindSpeed.setSamples(realSpeed, realHeight, realWindSize);
        d_curveRealWindAzimut.setSamples(realAzimut, realHeight, realWindSize);
        ui->plot_realWindSpeed->replot();
        ui->plot_realWindAzimut->replot();

        // Генерация тестовых данных для среднего ветра (16 точек)
        const int avgWindSize = 16;
        double avgSpeed[avgWindSize], avgAzimut[avgWindSize], avgHeight[avgWindSize];

        // Генерация высот для среднего ветра по логике из storetypes.h
        plus = 50;
        for (int i = 0; i < avgWindSize; i++) {
            if (i == 0) {
                avgHeight[i] = 50;
            } else {
                if (avgHeight[i - 1] == 100)
                    plus = 100;
                else if (avgHeight[i - 1] == 200)
                    plus = 200;
                else if (avgHeight[i - 1] == 800)
                    plus = 400;
                else if (avgHeight[i - 1] == 2400)
                    plus = 200;
                else if (avgHeight[i - 1] == 2600)
                    plus = 400;
                else if (avgHeight[i - 1] == 3000)
                    plus = 1000;
                else if (avgHeight[i - 1] == 6000)
                    plus = 2000;
                avgHeight[i] = avgHeight[i - 1] + plus;
            }
            // Генерация случайных значений
            avgSpeed[i] = 5.0 + (rand() % 100) / 10.0;
            avgAzimut[i] = (rand() % 360);
        }

        // Отображение данных среднего ветра на графиках
        d_curveAverageWindSpeed.setSamples(avgSpeed, avgHeight, avgWindSize);
        d_curveAverageWindAzimut.setSamples(avgAzimut, avgHeight, avgWindSize);
        ui->plot_midWindSpeed->replot();
        ui->plot_midWindAzimut->replot();

        // Генерация тестовых данных для измеренного ветра
        // Используем переменное количество точек с разными высотами
        const int izmWindSize = 23;
        double izmSpeed[izmWindSize], izmAzimut[izmWindSize], izmHeight[izmWindSize];

        for (int i = 0; i < izmWindSize; i++) {
            // Используем высоты из таблицы для измеренного ветра
            izmHeight[i] = (i < 3) ? (i + 1) * 50 : 100 + i * 100;
            // Генерация случайных значений
            izmSpeed[i] = 4.0 + (rand() % 120) / 10.0;
            izmAzimut[i] = (rand() % 360);
        }

        // Отображение данных измеренного ветра на графиках
        d_curveIzmWindSpeed.setSamples(izmSpeed, izmHeight, izmWindSize);
        d_curveIzmWindAzimut.setSamples(izmAzimut, izmHeight, izmWindSize);
        ui->plot_izmWindSpeed_2->replot();
        ui->plot_izmWindAzimut_2->replot();

    } else {
        ui->lblDataStatus->setText("Нет данных для выбранного времени");
        ui->lblDataStatus->setStyleSheet("color: red;");
    }
}

void MeasurementResults::updateDisplay()
{
    updateDateTimeDisplay();
    updateSliderRange();
}

void MeasurementResults::onPrevDateClicked()
{
    currentDateTime = currentDateTime.addSecs(-10 * 60);
    updateDisplay();
}

void MeasurementResults::onNextDateClicked()
{
    currentDateTime = currentDateTime.addSecs(10 * 60);
    updateDisplay();
}

void MeasurementResults::onSelectDateClicked()
{
    QDialog *dateDialog = new QDialog(this);
    dateDialog->setWindowTitle("Выбор даты и времени");
    dateDialog->resize(400, 500);

    QVBoxLayout *layout = new QVBoxLayout(dateDialog);

    QCalendarWidget *calendar = new QCalendarWidget(dateDialog);
    calendar->setSelectedDate(currentDateTime.date());
    layout->addWidget(calendar);

    QGroupBox *timeGroup = new QGroupBox("Доступные измерения", dateDialog);
    QVBoxLayout *timeLayout = new QVBoxLayout(timeGroup);

    QListWidget *timeList = new QListWidget(dateDialog);
    timeLayout->addWidget(timeList);
    layout->addWidget(timeGroup);

    auto updateTimeList = [this, timeList, calendar](){
        timeList->clear();
        QList<QTime> times = getAvailableTimesForDate(calendar->selectedDate());

        if (times.isEmpty()){
           QListWidgetItem *item = new QListWidgetItem("Нет данных");
           item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
           timeList->addItem(item);
        } else {
            for (const QTime &time : times){
                timeList->addItem(time.toString("hh.mm"));
            }
        }
    };

    connect(calendar, &QCalendarWidget::selectionChanged, updateTimeList);
    updateTimeList();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dateDialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, dateDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dateDialog, &QDialog::reject);

    if (dateDialog->exec() == QDialog::Accepted){
        QDate selectedDate = calendar->selectedDate();
        QTime selectedTime = currentDateTime.time();

        if (timeList->currentItem() && !timeList->currentItem()->text().isEmpty()
                && timeList->currentItem()->text() != "Нет данных"){
            selectedTime = QTime::fromString(timeList->currentItem()->text(), "hh::mm");
        }

        currentDateTime = QDateTime(selectedDate, selectedTime);
        updateDisplay();
    }

    delete dateDialog;
}

void MeasurementResults::onTimeSliderChanged(int value)
{
    int totalMinutes = value * 10;
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;

    QTime newTime(hours, minutes, 0);
    currentDateTime.setTime(newTime);

    updateDateTimeDisplay();
}

void MeasurementResults::setupPlot(QwtPlot *plot, QwtPlotCurve *curve, QwtPlotGrid *grid,
                                    QwtSymbol *symbol, const QString &xAxisTitle, const QString &yAxisTitle,
                                    double xMin, double xMax, double xStep)
{
    plot->setAxisTitle(QwtPlot::yLeft, yAxisTitle);
    plot->setAxisTitle(QwtPlot::xBottom, xAxisTitle);
    plot->setAxisScale(QwtPlot::yLeft, 0.0, 4500.0);
    plot->setAxisScale(QwtPlot::xBottom, xMin, xMax, xStep);

    curve->setSymbol(symbol);
    curve->attach(plot);
    curve->setStyle(QwtPlotCurve::NoCurve);

    setupPlotGrid(grid);
    grid->attach(plot);

    plot->replot();
}

void MeasurementResults::setupPlotGrid(QwtPlotGrid *grid)
{
    grid->setMajorPen(QPen(Qt::black, 0, Qt::DotLine));
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMinorPen(QPen(Qt::lightGray, 0, Qt::DotLine));
}
