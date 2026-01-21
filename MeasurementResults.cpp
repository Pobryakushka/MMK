#include "MeasurementResults.h"
#include "ui_MeasurementResults.h"
#include "databasemanager.h"
#include "amsprotocol.h"
#include <QCalendarWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <limits>

MeasurementResults::MeasurementResults(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeasurementResults)
    , currentButtelinType(Updated)
    , currentOutputFormat(String)
    , m_mapCoordinatesMode(false)
    , m_zoomsContainer(nullptr)
//    , m_dbPort(5432)
//    , m_dbConfigured(false)
{
    ui->setupUi(this);

    currentDateTime = QDateTime::currentDateTime();
    int minutes = currentDateTime.time().minute();
    minutes = (minutes / 10) * 10;
    currentDateTime.setTime(QTime(currentDateTime.time().hour(), minutes, 0));

    loadAvailableMeasurements();

    updateDateTimeDisplay();
    updateSliderRange();

    connect(ui->btnPrevDate, &QPushButton::clicked, this, &MeasurementResults::onPrevDateClicked);
    connect(ui->btnNextDate, &QPushButton::clicked, this, &MeasurementResults::onNextDateClicked);
    connect(ui->btnSelectDate, &QPushButton::clicked, this, &MeasurementResults::onSelectDateClicked);

    connect(ui->pushButton_updated, &QPushButton::clicked, this, &MeasurementResults::onUpdatedButtonClicked);
    connect(ui->pushButton_approximate, &QPushButton::clicked, this, &MeasurementResults::onApproximateButtonClicked);
    connect(ui->pushButton_fromMeteoStat, &QPushButton::clicked, this, &MeasurementResults::onFromMeteoStatButtonClicked);

    connect(ui->pushButton_string, &QPushButton::clicked, this, &MeasurementResults::onStringFormatClicked);
    connect(ui->pushButton_table, &QPushButton::clicked, this, &MeasurementResults::onTableFormatClicked);

    switchMeteo11Display();

    setupPlots();
    setupZoom();  // Инициализация масштабирования
}

MeasurementResults::~MeasurementResults()
{
    disconnectDatabase();
    if (m_zoomsContainer) {
        delete m_zoomsContainer;
        m_zoomsContainer = nullptr;
    }
    delete ui;
}

// ===== НАСТРОЙКА БД =====

//void MeasurementResults::setDatabase(const QString &host, int port, const QString &dbName,
//                                     const QString &user, const QString &password)
//{
//    DatabaseManager::instance()->configure(host, port, dbName, user, password);
//    DatabaseManager::instance()->connect();

//    qInfo() << "MeasurementResults: Использую подключение к БД";

//    // Загружаем доступные измерения
//    loadAvailableMeasurements();
//}

bool MeasurementResults::connectDatabase()
{
    if(!DatabaseManager::instance()->isConnected()){
        return DatabaseManager::instance()->connect();
    }
    return true;
}

void MeasurementResults::disconnectDatabase()
{

}

// ===== ЗАГРУЗКА ДАННЫХ ИЗ БД =====

void MeasurementResults::loadMeasurementsFromDatabase()
{
    if (!DatabaseManager::instance()->isConnected()) {
        qWarning() << "MeasurementResults: БД не подключена";
        if (!DatabaseManager::instance()->connect()) {
            qCritical() << "MeasurementResults: Не удалось подключиться к БД";
            return;
        }
    }

    availableMeasurements.clear();

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);

    // Загружаем ВСЕ записи из main_archive за последние 30 дней
    QString sql =
        "SELECT "
        "   ma.record_id, "
        "   ma.completion_time, "
        "   ma.notes "
        "FROM main_archive ma "
        "WHERE ma.completion_time >= CURRENT_DATE - INTERVAL '30 days' "
        "ORDER BY ma.completion_time DESC";

    qDebug() << "MeasurementResults: Выполняем запрос к main_archive...";

    if (!query.exec(sql)) {
        qCritical() << "MeasurementResults: Ошибка SQL:" << query.lastError().text();
        qDebug() << "SQL запрос:" << sql;
        return;
    }

    int totalRecords = 0;
    QMap<QDateTime, MeasurementRecord> recordsByDateTime;

    while (query.next()) {
        MeasurementRecord record;
        record.recordId = query.value(0).toInt();
        record.measurementTime = query.value(1).toDateTime();
        record.notes = query.value(2).toString();

        // Флаги наличия данных заполним позже
        record.hasAvgWind = false;
        record.hasActualWind = false;
        record.hasMeasuredWind = false;

        recordsByDateTime[record.measurementTime] = record;
        totalRecords++;

//        QDate date = record.measurementTime.date();
//        int hour = record.measurementTime.time().hour();
//        QString key = QString("%1_%2").arg(date.toString("yyyy-MM-dd")).arg(hour);

//        recordsByDateTime[key] = record;
//        totalRecords++;

        qDebug() << "  Запись" << record.recordId
                 << "от" << record.measurementTime.toString("yyyy-MM-dd hh:mm:ss");
    }

    qInfo() << "MeasurementResults: Загружено" << totalRecords << "записей из main_archive";

    // Теперь проверяем наличие профилей ветра для каждой записи

    for (auto it = recordsByDateTime.begin(); it != recordsByDateTime.end(); ++it) {
        QDateTime measurementTime = it.key();

        QDateTime timeFrom = measurementTime.addSecs(-1800);
        QDateTime timeTo = measurementTime.addSecs(1800);

        QSqlQuery avgQuery(db);
        avgQuery.prepare("SELECT COUNT(*) FROM avg_wind_profile "
                         "WHERE measurement_time BETWEEN :from AND :to");
        avgQuery.bindValue(":from", timeFrom);
        avgQuery.bindValue(":to", timeTo);
        if (avgQuery.exec() && avgQuery.next()) {
            it.value().hasAvgWind = (avgQuery.value(0).toInt() > 0);
        }

        QSqlQuery actualQuery(db);
        actualQuery.prepare("SELECT COUNT(*) FROM actual_wind_profile "
                         "WHERE measurement_time BETWEEN :from AND :to");
        actualQuery.bindValue(":from", timeFrom);
        actualQuery.bindValue(":to", timeTo);
        if (actualQuery.exec() && actualQuery.next()) {
            it.value().hasActualWind = (actualQuery.value(0).toInt() > 0);
        }

        QSqlQuery measuredQuery(db);
        measuredQuery.prepare("SELECT COUNT(*) FROM measured_wind_profile "
                         "WHERE measurement_time BETWEEN :from AND :to");
        measuredQuery.bindValue(":from", timeFrom);
        measuredQuery.bindValue(":to", timeTo);
        if (measuredQuery.exec() && measuredQuery.next()) {
            it.value().hasMeasuredWind = (measuredQuery.value(0).toInt() > 0);
        }
    }

    // Переносим в основную структуру availableMeasurements
    for (auto it = recordsByDateTime.begin(); it != recordsByDateTime.end(); ++it) {
        MeasurementRecord record = it.value();
        QDate date = record.measurementTime.date();
        int hour = record.measurementTime.time().hour();

        availableMeasurements[date].append(record);
    }

    for (auto it = availableMeasurements.begin(); it != availableMeasurements.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                [](const MeasurementRecord &a, const MeasurementRecord &b) {
            return a.measurementTime > b.measurementTime;
        });
    }

    qInfo() << "MeasurementResults: Данные распределены по" << availableMeasurements.size() << "датам";

    // Выводим список дат для отладки
    QList<QDate> dates = availableMeasurements.keys();
    std::sort(dates.begin(), dates.end());
    qDebug() << "Доступные даты в архиве:" << dates;
}

QVector<WindProfileData> MeasurementResults::loadAvgWindProfile(const QDateTime &time)
{
    QVector<WindProfileData> profile;

    if (!connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT wind_speed, wind_direction "
        "FROM avg_wind_profile "
        "WHERE measurement_time >= :time_from AND measurement_time <= :time_to "
        "ORDER BY profile_id ASC"
    );

    query.bindValue(":time_from", time.addSecs(-1800)); // -30 минут
    query.bindValue(":time_to", time.addSecs(1800));    // +30 минут

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки среднего ветра:" << query.lastError().text();
        return profile;
    }

    // Загружаем данные из БД (только скорость и направление)
    QVector<WindProfileData> dbData;
    while (query.next()) {
        WindProfileData point;
        point.windSpeed = query.value(0).toFloat();
        point.windDirection = query.value(1).toInt();
        point.isValid = true;
        dbData.append(point);
    }

    // Применяем стандартные высоты АМС (фиксированные согласно протоколу)
    if (!dbData.isEmpty()) {
        QVector<float> standardHeights = AMSProtocol::getAverageWindHeights(dbData.size());
        for (int i = 0; i < dbData.size() && i < standardHeights.size(); i++) {
            dbData[i].height = standardHeights[i];
        }
    }

    profile = dbData;
    qDebug() << "MeasurementResults: Загружен профиль среднего ветра," << profile.size() << "точек";
    return profile;
}

QVector<WindProfileData> MeasurementResults::loadActualWindProfile(const QDateTime &time)
{
    QVector<WindProfileData> profile;

    if (!connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT wind_speed, wind_direction "
        "FROM actual_wind_profile "
        "WHERE measurement_time >= :time_from AND measurement_time <= :time_to "
        "ORDER BY profile_id ASC"
    );

    query.bindValue(":time_from", time.addSecs(-1800));
    query.bindValue(":time_to", time.addSecs(1800));

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки действительного ветра:" << query.lastError().text();
        return profile;
    }

    // Загружаем данные из БД (только скорость и направление)
    QVector<WindProfileData> dbData;
    while (query.next()) {
        WindProfileData point;
        point.windSpeed = query.value(0).toFloat();
        point.windDirection = query.value(1).toInt();
        point.isValid = true;
        dbData.append(point);
    }

    // Применяем стандартные высоты АМС (фиксированные согласно протоколу)
    if (!dbData.isEmpty()) {
        QVector<float> standardHeights = AMSProtocol::getActualWindHeights(dbData.size());
        for (int i = 0; i < dbData.size() && i < standardHeights.size(); i++) {
            dbData[i].height = standardHeights[i];
        }
    }

    profile = dbData;
    qDebug() << "MeasurementResults: Загружен профиль действительного ветра," << profile.size() << "точек";
    return profile;
}

QVector<MeasuredWindData> MeasurementResults::loadMeasuredWindProfile(const QDateTime &time)
{
    QVector<MeasuredWindData> profile;

    if (!connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction "
        "FROM measured_wind_profile "
        "WHERE measurement_time >= :time_from AND measurement_time <= :time_to "
        "ORDER BY height ASC"
    );

    query.bindValue(":time_from", time.addSecs(-1800));
    query.bindValue(":time_to", time.addSecs(1800));

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки измеренного ветра:" << query.lastError().text();
        return profile;
    }

    while (query.next()) {
        MeasuredWindData point;
        point.height = query.value(0).toFloat();
        point.windSpeed = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        point.reliability = 2; // Из БД только достоверные данные
        profile.append(point);
    }

    qDebug() << "MeasurementResults: Загружен профиль измеренного ветра," << profile.size() << "точек";
    return profile;
}

// ===== ОТОБРАЖЕНИЕ ДАННЫХ =====

void MeasurementResults::displayWindProfile(const QVector<WindProfileData> &avgWind,
                                           const QVector<WindProfileData> &actualWind,
                                           const QVector<MeasuredWindData> &measuredWind)
{
    // Заполняем таблицу среднего ветра
    // Заголовки строк остаются как в UI (диапазоны высот типа "0-50", "0-100")
    // Высоты уже присвоены в loadAvgWindProfile для построения графиков
    ui->tableWidget_AverageWind->setRowCount(avgWind.size());
    for (int i = 0; i < avgWind.size(); i++) {
        // Колонка 0: Скорость ветра
        ui->tableWidget_AverageWind->setItem(i, 0,
            new QTableWidgetItem(QString::number(avgWind[i].windSpeed, 'f', 2)));
        // Колонка 1: Направление ветра
        ui->tableWidget_AverageWind->setItem(i, 1,
            new QTableWidgetItem(QString::number(avgWind[i].windDirection)));
    }

    // Заполняем таблицу действительного ветра
    // Заголовки строк остаются как в UI
    // Высоты уже присвоены в loadActualWindProfile для построения графиков
    ui->tableWidget_realWind->setRowCount(actualWind.size());
    for (int i = 0; i < actualWind.size(); i++) {
        // Колонка 0: Скорость ветра
        ui->tableWidget_realWind->setItem(i, 0,
            new QTableWidgetItem(QString::number(actualWind[i].windSpeed, 'f', 2)));
        // Колонка 1: Направление ветра
        ui->tableWidget_realWind->setItem(i, 1,
            new QTableWidgetItem(QString::number(actualWind[i].windDirection)));
    }

    // Заполняем таблицу измеренного ветра
    // Высота приходит от АМС и хранится в БД - используем её для заголовков строк
    ui->tableWidget_izmWind_2->setRowCount(measuredWind.size());
    for (int i = 0; i < measuredWind.size(); i++) {
        // Устанавливаем заголовок строки с высотой из БД
        ui->tableWidget_izmWind_2->setVerticalHeaderItem(i,
            new QTableWidgetItem(QString::number(measuredWind[i].height, 'f', 0)));
        // Колонка 0: Скорость ветра
        ui->tableWidget_izmWind_2->setItem(i, 0,
            new QTableWidgetItem(QString::number(measuredWind[i].windSpeed, 'f', 2)));
        // Колонка 1: Направление ветра
        ui->tableWidget_izmWind_2->setItem(i, 1,
            new QTableWidgetItem(QString::number(measuredWind[i].windDirection)));
    }

    // Строим графики (используют height из структур данных)
    plotWindSpeed(ui->plot_midWindSpeed, avgWind, "Средний ветер", QColor(0, 120, 215));
    plotWindDirection(ui->plot_midWindAzimut, avgWind, "Средний ветер", QColor(0, 120, 215));

    plotWindSpeed(ui->plot_realWindSpeed, actualWind, "Действительный ветер", QColor(76, 175, 80));
    plotWindDirection(ui->plot_realWindAzimut, actualWind, "Действительный ветер", QColor(76, 175, 80));

    plotMeasuredWindSpeed(ui->plot_izmWindSpeed_2, measuredWind, "Измеренный ветер", QColor(255, 87, 34));
    plotMeasuredWindDirection(ui->plot_izmWindAzimut_2, measuredWind, "Измеренный ветер", QColor(255, 87, 34));
}

void MeasurementResults::updateAvailableRecordsLabel()
{
    QDate date = currentDateTime.date();

    int recordCount = 0;
    if (availableMeasurements.contains(date)) {
        recordCount = availableMeasurements[date].size();
    }

    if (recordCount > 0) {
        ui->lblAvailableRecords->setText(QString("Доступно записей за %1: %2").arg(date.toString("dd.MM.yyyy"))
                                         .arg(recordCount));
        ui->lblAvailableRecords->setStyleSheet("color: green; font-style: italic;");
    } else {
        ui->lblAvailableRecords->setText("Нет данных за выбранную дату");
        ui->lblAvailableRecords->setStyleSheet("color: red; font-style: italic;");
    }
}

// ===== ОБНОВЛЕНИЕ ИНТЕРФЕЙСА =====

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

void MeasurementResults::loadAvailableMeasurements()
{
    loadMeasurementsFromDatabase();
    updateDisplay();
}

void MeasurementResults::updateDateTimeDisplay()
{
    QString dateTimeStr = currentDateTime.toString("dd.MM.yyyy hh:mm");
    ui->lblCurrentDateTime->setText(dateTimeStr);
    ui->btnSelectDate->setText(currentDateTime.toString("dd.MM.yyyy"));

    loadMeasurementData(currentDateTime);
}

void MeasurementResults::updateSliderRange()
{
    if (ui->timeSlider){
        ui->timeSlider->setVisible(false);
    }
    if (ui->lblTimeSliderLabel){
        ui->lblTimeSliderLabel->setVisible(false);
    }
    if (ui->lblTimeStart){
        ui->lblTimeStart->setVisible(false);
    }
    if (ui->lblTimeMiddle){
        ui->lblTimeMiddle->setVisible(false);
    }
    if (ui->lblTimeEnd){
        ui->lblTimeEnd->setVisible(false);
    }

    updateAvailableRecordsLabel();
}

QVector<MeasurementRecord> MeasurementResults::getRecordsForDate(const QDate &date)
{
    if (availableMeasurements.contains(date)) {
        return availableMeasurements[date];
    }
    return QVector<MeasurementRecord>();
}

MeasurementRecord MeasurementResults::findClosestRecord(const QDate &date, int hour)
{
    MeasurementRecord result;

    if (!availableMeasurements.contains(date)) {
        return result;
    }

    QVector<MeasurementRecord> records = availableMeasurements[date];

    QTime targetTime(hour, 0, 0);
    int minDiff = std::numeric_limits<int>::max();

    for (const MeasurementRecord &record : records){
        int diff = qAbs(targetTime.secsTo(record.measurementTime.time()));
        if (diff < minDiff) {
            minDiff = diff;
            result = record;
        }
    }
    return result;
}

void MeasurementResults::loadMeasurementData(const QDateTime &dateTime)
{
    QDate date = dateTime.date();
    int hour = dateTime.time().hour();

    MeasurementRecord record = findClosestRecord(date, hour);

    if (record.recordId > 0) {
        ui->lblDataStatus->setText(QString("Данные загружены (ID: %1)").arg(record.recordId));
        ui->lblDataStatus->setStyleSheet("color: green; font-weight: bold;");

        // Загружаем профили ветра
        QVector<WindProfileData> avgWind = loadAvgWindProfile(record.measurementTime);
        QVector<WindProfileData> actualWind = loadActualWindProfile(record.measurementTime);
        QVector<MeasuredWindData> measuredWind = loadMeasuredWindProfile(record.measurementTime);

        // Отображаем данные
        displayWindProfile(avgWind, actualWind, measuredWind);

        // Показываем информацию о доступных данных
        QString info = "Доступно: ";
        QStringList available;
        if (record.hasAvgWind) available << "Средний ветер";
        if (record.hasActualWind) available << "Действительный ветер";
        if (record.hasMeasuredWind) available << "Измеренный ветер";

        if (available.isEmpty()) {
            info += "Нет данных профилей";
        } else {
            info += available.join(", ");
        }

        ui->lblDataStatus->setText(info);

    } else {
        ui->lblDataStatus->setText("Нет данных для выбранного времени");
        ui->lblDataStatus->setStyleSheet("color: red; font-weight: bold;");

        // Очищаем таблицы
        ui->tableWidget_AverageWind->clearContents();
        ui->tableWidget_realWind->clearContents();
        ui->tableWidget_izmWind_2->clearContents();
    }

    updateAvailableRecordsLabel();
}

void MeasurementResults::updateDisplay()
{
    updateDateTimeDisplay();
    updateSliderRange();
}

void MeasurementResults::onPrevDateClicked()
{
    QList<QDate> dates = availableMeasurements.keys();
    std::sort(dates.begin(), dates.end(), std::greater<QDate>());

    bool foundCurrent = false;
    for (const QDate &date : dates){
        QVector<MeasurementRecord> records = availableMeasurements[date];

        for (const MeasurementRecord &record : records) {
            if (foundCurrent) {
                currentDateTime = record.measurementTime;
                updateDisplay();
                return;
            }

            if (record.measurementTime == currentDateTime) {
                foundCurrent = true;
            }
        }
    }
}


void MeasurementResults::onNextDateClicked()
{
    QList<QDate> dates = availableMeasurements.keys();
    std::sort(dates.begin(), dates.end());

    bool foundCurrent = false;
    for (const QDate &date : dates){
        QVector<MeasurementRecord> records = availableMeasurements[date];
        std::sort(records.begin(), records.end(),
                  [](const MeasurementRecord &a, const MeasurementRecord &b){
            return a.measurementTime < b.measurementTime;
        });

        for (const MeasurementRecord &record : records) {
            if (foundCurrent) {
                currentDateTime = record.measurementTime;
                updateDisplay();
                return;
            }

            if (record.measurementTime == currentDateTime) {
                foundCurrent = true;
            }
        }
    }
}

void MeasurementResults::onSelectDateClicked()
{
    QDialog *dateDialog = new QDialog(this);
    dateDialog->setWindowTitle("Выбор даты и времени");
    dateDialog->resize(500, 600);

    QVBoxLayout *layout = new QVBoxLayout(dateDialog);

    // Календарь
    QCalendarWidget *calendar = new QCalendarWidget(dateDialog);
    calendar->setSelectedDate(currentDateTime.date());

    // Подсвечиваем даты с доступными измерениями
    QTextCharFormat availableFormat;
    availableFormat.setBackground(QColor(144, 238, 144)); // Светло-зеленый
    availableFormat.setFontWeight(QFont::Bold);

    QTextCharFormat todayFormat;
    todayFormat.setBackground(QColor(173, 216, 230)); // Голубой
    todayFormat.setFontWeight(QFont::Bold);

    for (auto it = availableMeasurements.constBegin(); it != availableMeasurements.constEnd(); ++it) {
        if (it.key() == QDate::currentDate()) {
            calendar->setDateTextFormat(it.key(), todayFormat);
        } else {
            calendar->setDateTextFormat(it.key(), availableFormat);
        }
    }

    layout->addWidget(calendar);

    // Группа с доступными измерениями
    QGroupBox *timeGroup = new QGroupBox("Доступные измерения", dateDialog);
    QVBoxLayout *timeLayout = new QVBoxLayout(timeGroup);

    QListWidget *timeList = new QListWidget(dateDialog);
    timeList->setStyleSheet(
        "QListWidget::item { padding: 5px; }"
        "QListWidget::item:selected { background-color: #4CAF50; color: white; }"
    );
    timeLayout->addWidget(timeList);

    QLabel *infoLabel = new QLabel(dateDialog);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666; font-style: italic; padding: 5px;");
    timeLayout->addWidget(infoLabel);

    layout->addWidget(timeGroup);

    // Функция обновления списка времен
    auto updateTimeList = [this, timeList, calendar, infoLabel](){
        timeList->clear();
        QDate selectedDate = calendar->selectedDate();
        QVector<MeasurementRecord> records = getRecordsForDate(selectedDate);

        if (records.isEmpty()){
           QListWidgetItem *item = new QListWidgetItem("📭 Нет данных за выбранную дату");
           item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
           item->setForeground(Qt::red);
           timeList->addItem(item);
           infoLabel->setText("Выберите другую дату");
        } else {
            infoLabel->setText(QString("Найдено измерений: %1").arg(records.size()));

            for (const MeasurementRecord &record : records){

                // Формируем строку с информацией о типах данных
                QStringList dataTypes;
                if (record.hasAvgWind) dataTypes << "Ср";
                if (record.hasActualWind) dataTypes << "Действ";
                if (record.hasMeasuredWind) dataTypes << "Изм";

                QString timeStr = QString("🕐 %1:00 — %2")
                    .arg(record.measurementTime.toString("hh:mm:ss"))
                    .arg(dataTypes.isEmpty() ? "Нет данных" : dataTypes.join(", "));

                if (!record.notes.isEmpty()) {
                    timeStr += " - " + record.notes;
                }

                QListWidgetItem *item = new QListWidgetItem(timeStr);
                item->setData(Qt::UserRole, record.measurementTime);

                // Цветовое кодирование по количеству типов данных
                if (dataTypes.size() == 3) {
                    item->setBackground(QColor(144, 238, 144)); // Зеленый - все данные
                } else if (dataTypes.size() == 2) {
                    item->setBackground(QColor(255, 255, 200)); // Желтый - частично
                } else {
                    item->setBackground(QColor(255, 228, 196)); // Бежевый - минимум
                }

                timeList->addItem(item);
            }
        }
    };

    connect(calendar, &QCalendarWidget::selectionChanged, updateTimeList);
    updateTimeList();

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        dateDialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, dateDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dateDialog, &QDialog::reject);

    if (dateDialog->exec() == QDialog::Accepted){
        QDate selectedDate = calendar->selectedDate();

        if (timeList->currentItem() && timeList->currentItem()->data(Qt::UserRole).isValid()){
            currentDateTime = timeList->currentItem()->data(Qt::UserRole).toDateTime();
        } else {
            QVector<MeasurementRecord> records = getRecordsForDate(selectedDate);
            if (!records.isEmpty()) {
                currentDateTime = records.first().measurementTime;
            }
        }
        updateDisplay();
    }

    delete dateDialog;
}

void MeasurementResults::setupPlots()
{
    auto setupPlot = [](QwtPlot *plot, const QString &xTitle, const QString &yTitle,
                        double xMin, double xMax, double xStep) {
        if (!plot) return;

        plot->setAxisTitle(QwtPlot::yLeft, yTitle);
        plot->setAxisTitle(QwtPlot::xBottom, xTitle);
        plot->setAxisScale(QwtPlot::yLeft, 0.0, 4000.0);
        plot->setAxisScale(QwtPlot::xBottom, xMin, xMax, xStep);

        QwtPlotGrid *grid = new QwtPlotGrid();
        grid->setMajorPen(QPen(Qt::black, 0, Qt::DotLine));
        grid->enableXMin(true);
        grid->enableYMin(true);
        grid->setMinorPen(QPen(Qt::lightGray, 0, Qt::DotLine));
        grid->attach(plot);
    };

    // Настройка графиков скорости
    setupPlot(ui->plot_midWindSpeed, "Скорость", "Высота", 0, 50, 10);
    setupPlot(ui->plot_realWindSpeed, "Скорость", "Высота", 0, 50, 10);
    setupPlot(ui->plot_izmWindSpeed_2, "Скорость", "Высота", 0, 50, 10);

    // Настройка графиков направления
    setupPlot(ui->plot_midWindAzimut, "Направление", "Высота", 0, 360, 60);
    setupPlot(ui->plot_realWindAzimut, "Направление", "Высота", 0, 360, 60);
    setupPlot(ui->plot_izmWindAzimut_2, "Направление", "Высота", 0, 360, 60);
}

void MeasurementResults::setupZoom()
{
    // Создаем контейнер для управления масштабированием
    m_zoomsContainer = new ZoomsContainer();
    
    // Прикрепляем масштабирование ко всем графикам
    // Используем белый цвет для рамки выделения (можно изменить на любой другой)
    
    if (ui->plot_midWindSpeed) {
        m_zoomsContainer->attachZoom(ui->plot_midWindSpeed, Qt::blue);
    }
    if (ui->plot_realWindSpeed) {
        m_zoomsContainer->attachZoom(ui->plot_realWindSpeed, Qt::green);
    }
    if (ui->plot_izmWindSpeed_2) {
        m_zoomsContainer->attachZoom(ui->plot_izmWindSpeed_2, Qt::red);
    }
    if (ui->plot_midWindAzimut) {
        m_zoomsContainer->attachZoom(ui->plot_midWindAzimut, Qt::blue);
    }
    if (ui->plot_realWindAzimut) {
        m_zoomsContainer->attachZoom(ui->plot_realWindAzimut, Qt::green);
    }
    if (ui->plot_izmWindAzimut_2) {
        m_zoomsContainer->attachZoom(ui->plot_izmWindAzimut_2, Qt::red);
    }
    
    // Синхронизируем масштабирование по оси X для всех графиков
    // (при масштабировании одного графика по горизонтали, остальные тоже изменятся)
    m_zoomsContainer->connectXZooms();
    
    qDebug() << "MeasurementResults: Масштабирование графиков настроено";
}

void MeasurementResults::plotWindSpeed(QwtPlot *plot, const QVector<WindProfileData> &data,
                                       const QString &title, const QColor &color)
{
    if (!plot || data.isEmpty()) return;

    // Очищаем старые кривые
    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    // Подготавливаем данные
    QVector<double> heights;
    QVector<double> speeds;

    for (const WindProfileData &point : data) {
        if (point.isValid) {
            heights.append(point.height);
            speeds.append(point.windSpeed);
        }
    }

    if (heights.isEmpty()) {
        plot->replot();
        return;
    }

    // Создаем кривую
    QwtPlotCurve *curve = new QwtPlotCurve(title);
    // X-ось: скорость, Y-ось: высота
    curve->setSamples(speeds, heights);
    curve->setPen(QPen(color, 2));

    // Добавляем символы на точках
    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
                                      QBrush(color),
                                      QPen(color, 1),
                                      QSize(5, 5));
    curve->setSymbol(symbol);
    curve->setStyle(QwtPlotCurve::NoCurve);

    curve->attach(plot);
    plot->replot();
}

void MeasurementResults::plotWindDirection(QwtPlot *plot, const QVector<WindProfileData> &data,
                                          const QString &title, const QColor &color)
{
    if (!plot || data.isEmpty()) return;

    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    QVector<double> heights;
    QVector<double> directions;

    for (const WindProfileData &point : data) {
        if (point.isValid) {
            heights.append(point.height);
            directions.append(point.windDirection);
        }
    }

    if (heights.isEmpty()) {
        plot->replot();
        return;
    }

    QwtPlotCurve *curve = new QwtPlotCurve(title);
    // X-ось: направление, Y-ось: высота
    curve->setSamples(directions, heights);
    curve->setPen(QPen(color, 2));

    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
                                      QBrush(color),
                                      QPen(color, 1),
                                      QSize(5, 5));
    curve->setSymbol(symbol);
    curve->setStyle(QwtPlotCurve::NoCurve);

    curve->attach(plot);
    plot->replot();
}

void MeasurementResults::plotMeasuredWindSpeed(QwtPlot *plot, const QVector<MeasuredWindData> &data,
                                              const QString &title, const QColor &color)
{
    if (!plot || data.isEmpty()) return;

    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    QVector<double> heights;
    QVector<double> speeds;

    for (const MeasuredWindData &point : data) {
        heights.append(point.height);
        speeds.append(point.windSpeed);
    }

    QwtPlotCurve *curve = new QwtPlotCurve(title);
    // X-ось: скорость, Y-ось: высота
    curve->setSamples(speeds, heights);
    curve->setPen(QPen(color, 2));

    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
                                      QBrush(color),
                                      QPen(color, 1),
                                      QSize(5, 5));
    curve->setSymbol(symbol);
    curve->setStyle(QwtPlotCurve::NoCurve);

    curve->attach(plot);
    plot->replot();
}

void MeasurementResults::plotMeasuredWindDirection(QwtPlot *plot, const QVector<MeasuredWindData> &data,
                                                  const QString &title, const QColor &color)
{
    if (!plot || data.isEmpty()) return;

    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    QVector<double> heights;
    QVector<double> directions;

    for (const MeasuredWindData &point : data) {
        heights.append(point.height);
        directions.append(point.windDirection);
    }

    QwtPlotCurve *curve = new QwtPlotCurve(title);
    // X-ось: направление, Y-ось: высота
    curve->setSamples(directions, heights);
    curve->setPen(QPen(color, 2));

    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
                                      QBrush(color),
                                      QPen(color, 1),
                                      QSize(5, 5));
    curve->setSymbol(symbol);
    curve->setStyle(QwtPlotCurve::NoCurve);

    curve->attach(plot);
    plot->replot();
}

void MeasurementResults::clearDisplayedData()
{
    ui->tableWidget_AverageWind->clearContents();
    ui->tableWidget_realWind->clearContents();
    ui->tableWidget_izmWind_2->clearContents();

    // Очищаем графики
    if (ui->plot_midWindSpeed) {
        ui->plot_midWindSpeed->detachItems(QwtPlotItem::Rtti_PlotCurve);
        ui->plot_midWindSpeed->replot();
    }
    if (ui->plot_midWindAzimut) {
        ui->plot_midWindAzimut->detachItems(QwtPlotItem::Rtti_PlotCurve);
        ui->plot_midWindAzimut->replot();
    }
    if (ui->plot_realWindSpeed) {
        ui->plot_realWindSpeed->detachItems(QwtPlotItem::Rtti_PlotCurve);
        ui->plot_realWindSpeed->replot();
    }
    if (ui->plot_realWindAzimut) {
        ui->plot_realWindAzimut->detachItems(QwtPlotItem::Rtti_PlotCurve);
        ui->plot_realWindAzimut->replot();
    }
    if (ui->plot_izmWindSpeed_2) {
        ui->plot_izmWindSpeed_2->detachItems(QwtPlotItem::Rtti_PlotCurve);
        ui->plot_izmWindSpeed_2->replot();
    }
    if (ui->plot_izmWindAzimut_2) {
        ui->plot_izmWindAzimut_2->detachItems(QwtPlotItem::Rtti_PlotCurve);
        ui->plot_izmWindAzimut_2->replot();
    }
}
