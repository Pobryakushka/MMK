#include "MeasurementResults.h"
#include "ui_MeasurementResults.h"
#include <QCalendarWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QStackedWidget>

MeasurementResults::MeasurementResults(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeasurementResults)
    , currentButtelinType(Updated)
    , currentOutputFormat(String)
    , m_mapCoordinatesMode(false)
{
    ui->setupUi(this);

    currentDateTime = QDateTime::currentDateTime();
    int minutes = currentDateTime.time().minute();
    minutes = (minutes / 10) * 10;
    currentDateTime.setTime(QTime(currentDateTime.time().hour(), minutes, 0));

    setupMockData();
    loadAvailableMeasurements();
    updateDateTimeDisplay();
    updateSliderRange();

    connect(ui->btnPrevDate, &QPushButton::clicked, this, &MeasurementResults::onPrevDateClicked);
    connect(ui->btnNextDate, &QPushButton::clicked, this, &MeasurementResults::onNextDateClicked);
    connect(ui->btnSelectDate, &QPushButton::clicked, this, &MeasurementResults::onSelectDateClicked);
    connect(ui->timeSlider, &QSlider::valueChanged, this, &MeasurementResults::onTimeSliderChanged);

    connect(ui->pushButton_updated, &QPushButton::clicked, this, &MeasurementResults::onUpdatedButtonClicked);
    connect(ui->pushButton_approximate, &QPushButton::clicked, this, &MeasurementResults::onApproximateButtonClicked);
    connect(ui->pushButton_fromMeteoStat, &QPushButton::clicked, this, &MeasurementResults::onFromMeteoStatButtonClicked);

    connect(ui->pushButton_string, &QPushButton::clicked, this, &MeasurementResults::onStringFormatClicked);
    connect(ui->pushButton_table, &QPushButton::clicked, this, &MeasurementResults::onTableFormatClicked);

    switchMeteo11Display();
}

MeasurementResults::~MeasurementResults()
{
    delete ui;
}

void MeasurementResults::updateCoordinatesFromMainWindow(double latitude, double longitude)
{
    if (!m_mapCoordinatesMode){
        return;
    }

    QLineEdit *latEdit = ui->editLatitude;
    QLineEdit *lonEdit = ui->editLongitude;

    if (latEdit){
        latEdit->setText(QString::number(latitude, 'f', 6));
        qDebug() << "MeasurementResults: Широта обновлена: " << latitude;
    } else {
        qDebug() << "MeasurementResults: editLatitude не найден";
    }

    if (lonEdit) {
        lonEdit->setText(QString::number(longitude, 'f', 6));
        qDebug() << "MeasurementResults: Долгота обновлена: " << longitude;
    } else {
        qDebug() << "MeasurementResults: editLongitude не найден";
    }
}

void MeasurementResults::setMapCoordinatesMode(bool enabled)
{
    m_mapCoordinatesMode = enabled;

    if (enabled) {
        m_lockedDateTime = currentDateTime;
    }

    QLineEdit *latEdit = ui->editLatitude;
    QLineEdit *lonEdit = ui->editLongitude;

    if (latEdit && lonEdit){
        QString style = enabled ?
                    "background-color: #E8F5E9; border: 2px solid #4CAF50;" :
                    "";
        latEdit->setStyleSheet(style);
        lonEdit->setStyleSheet(style);
        latEdit->setReadOnly(enabled);
        lonEdit->setReadOnly(enabled);

        if (ui->editAltitude) {
            ui->editAltitude->setStyleSheet(style);
            ui->editAltitude->setReadOnly(enabled);
        }
    }

    if (enabled) {
        ui->btnPrevDate->setEnabled(false);
        ui->btnNextDate->setEnabled(false);
        ui->btnSelectDate->setEnabled(false);
        ui->timeSlider->setEnabled(false);
    } else {
        ui->btnPrevDate->setEnabled(true);
        ui->btnNextDate->setEnabled(true);
        ui->btnSelectDate->setEnabled(true);
        ui->timeSlider->setEnabled(true);
    }
}

void MeasurementResults::switchMeteo11Display()
{
    QStackedWidget *stackedWidget = ui->meteo11StackedWidget;
    if (!stackedWidget) return;

    if (currentButtelinType == Approximate){
        stackedWidget->setCurrentIndex(2);
        ui->pushButton_string->setEnabled(false);
        ui->pushButton_table->setEnabled(true);
    } else {
        ui->pushButton_string->setEnabled(true);
        ui->pushButton_table->setEnabled(true);

        if (currentOutputFormat == String){
            stackedWidget->setCurrentIndex(0);
        } else {
            stackedWidget->setCurrentIndex(1);
        }
    }
}

void MeasurementResults::onUpdatedButtonClicked()
{
    currentButtelinType = Updated;
    switchMeteo11Display();
}

void MeasurementResults::onApproximateButtonClicked()
{
    currentButtelinType = Approximate;
    switchMeteo11Display();
}

void MeasurementResults::onFromMeteoStatButtonClicked()
{
    currentButtelinType = FromMeteoStat;
    switchMeteo11Display();
}

void MeasurementResults::onStringFormatClicked()
{
    if (currentButtelinType != Approximate){
        currentOutputFormat = String;
        switchMeteo11Display();
    }
}

void MeasurementResults::onTableFormatClicked()
{
    currentOutputFormat = Table;
    switchMeteo11Display();
}

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

        //
        //
        // Логика заполнения данных
        //
        //

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
