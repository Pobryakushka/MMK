#include "MeasurementResults.h"
#include "CoordHelper.h"
#include "ui_MeasurementResults.h"
#include "databasemanager.h"
#include "amsprotocol.h"
#include "MeasurementExporter.h"
#include "ExportDialog.h"
#include <qwt_plot_renderer.h>
#include <QFileDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QCalendarWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <limits>
#include <algorithm>  // Для std::sort
#include <cmath>       // Для std::pow (расчёт давления МСА в Метео-11)

MeasurementResults::MeasurementResults(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeasurementResults)
    , currentButtelinType(Updated)
    , currentOutputFormat(String)
    , m_mapCoordinatesMode(false)
    , m_zoomsContainer(nullptr)
    , m_windShearCurve(nullptr)
    , m_windShearGrid(nullptr)
    , m_currentStationAltitude(0.0)
    , m_currentPressureMmHg(750.0)
    , m_currentTempC(15.0)
    , m_currentWindDirSurface(0.0)
    , m_currentWindSpeedSurface(0.0)
    , m_currentLatitude(0.0)
    , m_currentLongitude(0.0)
//    , m_dbPort(5432)
//    , m_dbConfigured(false)
{
    ui->setupUi(this);

    currentDateTime = QDateTime::currentDateTime();
    int minutes = currentDateTime.time().minute();
    minutes = (minutes / 10) * 10;
    currentDateTime.setTime(QTime(currentDateTime.time().hour(), minutes, 0));

    connect(ui->btnPrevDate, &QPushButton::clicked, this, &MeasurementResults::onPrevDateClicked);
    connect(ui->btnNextDate, &QPushButton::clicked, this, &MeasurementResults::onNextDateClicked);
    connect(ui->btnSelectDate, &QPushButton::clicked, this, &MeasurementResults::onSelectDateClicked);

    connect(ui->pushButton_updated, &QPushButton::clicked, this, &MeasurementResults::onUpdatedButtonClicked);
    connect(ui->pushButton_approximate, &QPushButton::clicked, this, &MeasurementResults::onApproximateButtonClicked);
    connect(ui->pushButton_fromMeteoStat, &QPushButton::clicked, this, &MeasurementResults::onFromMeteoStatButtonClicked);

    connect(ui->pushButton_string, &QPushButton::clicked, this, &MeasurementResults::onStringFormatClicked);
    connect(ui->pushButton_table, &QPushButton::clicked, this, &MeasurementResults::onTableFormatClicked);

    connect(ui->btnExport, &QPushButton::clicked, this, &MeasurementResults::onExportClicked);

    switchMeteo11Display();

    // Поля координат — только для отображения данных из архива, редактирование запрещено
    ui->editLatitude->setReadOnly(true);
    ui->editLongitude->setReadOnly(true);
    ui->editAltitude->setReadOnly(true);
    ui->cmbLatitudeType->setEnabled(false);
    ui->cmbLongitudeType->setEnabled(false);

    // Все графики инициализируются ДО загрузки данных —
    // иначе displayWindProfile/clearWindShearDisplay обращаются к неготовым виджетам
    setupPlots();
    setupZoom();
    setupWindShearTab();

    loadAvailableMeasurements();

    updateDateTimeDisplay();
    updateSliderRange();
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

    // Теперь проверяем наличие профилей ветра для каждой записи через wind_profiles_references
    for (auto it = recordsByDateTime.begin(); it != recordsByDateTime.end(); ++it) {
        int rid = it.value().recordId;

        QSqlQuery refQuery(db);
        refQuery.prepare(
            "SELECT avg_wind_profile_id, actual_wind_profile_id, measured_wind_profile_id "
            "FROM wind_profiles_references WHERE record_id = :rid"
            );
        refQuery.bindValue(":rid", rid);

        if (refQuery.exec() && refQuery.next()) {
            it.value().hasAvgWind      = !refQuery.value(0).isNull();
            it.value().hasActualWind   = !refQuery.value(1).isNull();
            it.value().hasMeasuredWind = !refQuery.value(2).isNull();
        }
        // Если строки нет в wind_profiles_references — все флаги остаются false
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

QVector<WindProfileData> MeasurementResults::loadAvgWindProfile(int recordId)
{
    QVector<WindProfileData> profile;

    if (recordId <= 0 || !connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем profile_id из wind_profiles_references
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT avg_wind_profile_id FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
        qDebug() << "MeasurementResults: Нет avg_wind_profile для record_id=" << recordId;
        return profile;
    }

    int profileId = refQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction "
        "FROM avg_wind_profile "
        "WHERE profile_id = :pid "
        "ORDER BY height ASC"
        );
    query.bindValue(":pid", profileId);

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки среднего ветра:" << query.lastError().text();
        return profile;
    }

    QVector<WindProfileData> dbData;
    while (query.next()) {
        WindProfileData point;
        point.height        = query.value(0).toFloat();
        point.windSpeed     = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        point.isValid       = true;
        dbData.append(point);
    }

    profile = dbData;
    QStringList heightList;
    for (const auto &pt : profile)
        heightList << QString::number(qRound(pt.height));
    qDebug() << "MeasurementResults: Средний ветер record_id=" << recordId
             << "profile_id=" << profileId
             << "точек=" << profile.size()
             << "высоты:" << heightList.join(", ");
    return profile;
}

QVector<WindProfileData> MeasurementResults::loadActualWindProfile(int recordId)
{
    QVector<WindProfileData> profile;

    if (recordId <= 0 || !connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем profile_id из wind_profiles_references
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT actual_wind_profile_id FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
        qDebug() << "MeasurementResults: Нет actual_wind_profile для record_id=" << recordId;
        return profile;
    }

    int profileId = refQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction "
        "FROM actual_wind_profile "
        "WHERE profile_id = :pid "
        "ORDER BY height ASC"
        );
    query.bindValue(":pid", profileId);

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки действительного ветра:" << query.lastError().text();
        return profile;
    }

    QVector<WindProfileData> dbData;
    while (query.next()) {
        WindProfileData point;
        point.height        = query.value(0).toFloat();
        point.windSpeed     = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        point.isValid       = true;
        dbData.append(point);
    }

    profile = dbData;
    QStringList heightListA;
    for (const auto &pt : profile)
        heightListA << QString::number(qRound(pt.height));
    qDebug() << "MeasurementResults: Действительный ветер record_id=" << recordId
             << "profile_id=" << profileId
             << "точек=" << profile.size()
             << "высоты:" << heightListA.join(", ");
    return profile;
}

QVector<MeasuredWindData> MeasurementResults::loadMeasuredWindProfile(int recordId)
{
    QVector<MeasuredWindData> profile;

    if (recordId <= 0 || !connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем profile_id из wind_profiles_references
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT measured_wind_profile_id FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
        qDebug() << "MeasurementResults: Нет measured_wind_profile для record_id=" << recordId;
        return profile;
    }

    int profileId = refQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction "
        "FROM measured_wind_profile "
        "WHERE profile_id = :pid "
        "ORDER BY height ASC"
        );
    query.bindValue(":pid", profileId);

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки измеренного ветра:" << query.lastError().text();
        return profile;
    }

    while (query.next()) {
        MeasuredWindData point;
        point.height        = query.value(0).toFloat();
        point.windSpeed     = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        point.reliability   = 2; // Из БД только достоверные данные
        profile.append(point);
    }

    qDebug() << "MeasurementResults: Загружен профиль измеренного ветра," << profile.size()
             << "точек (record_id=" << recordId << ", profile_id=" << profileId << ")";
    return profile;
}

void MeasurementResults::loadSurfaceMeteoData(int recordId)
{
    ui->tableWidget_parm1b65->clearContents();
    if (recordId <= 0 || !connectDatabase()) return;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT temperature, humidity, pressure, wind_speed_surface, wind_direction_surface "
        "FROM surface_meteo WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: Нет данных ИВС для record_id=" << recordId;
        return;
    }

    auto setCell = [&](int row, const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget_parm1b65->setItem(row, 0, item);
    };

    setCell(0, QString::number(query.value(2).toDouble(), 'f', 1)); // давление
    setCell(1, QString::number(query.value(0).toDouble(), 'f', 1)); // температура
    setCell(2, QString::number(query.value(1).toDouble(), 'f', 1)); // влажность
    setCell(3, QString::number(query.value(4).toInt(), 10));         // направление
    setCell(4, QString::number(query.value(3).toDouble(), 'f', 1)); // скорость

    // Сохраняем значения для последующего формирования Метео-11
    m_currentPressureMmHg     = query.value(2).toDouble(); // из БД уже в мм рт.ст.
    m_currentTempC            = query.value(0).toDouble();
    m_currentWindDirSurface   = query.value(4).toDouble();
    m_currentWindSpeedSurface = query.value(3).toDouble();
}

void MeasurementResults::loadStationCoordinates(int recordId)
{
    if (recordId <= 0 || !connectDatabase()) return;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT latitude, longitude, altitude "
        "FROM station_coordinates WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: Нет координат для record_id=" << recordId;
        ui->editLatitude->clear();
        ui->editLongitude->clear();
        ui->editAltitude->clear();
        return;
    }

    double lat = query.value(0).toDouble();
    double lon = query.value(1).toDouble();
    double alt = query.value(2).toDouble();

    m_currentLatitude = lat;
    m_currentLongitude = lon;

    ui->editLatitude->setText(CoordHelper::toDisplayDMS(qAbs(lat)));
    ui->cmbLatitudeType->setCurrentIndex(lat >= 0 ? 0 : 1);  // 0=Северная, 1=Южная

    ui->editLongitude->setText(CoordHelper::toDisplayDMS(qAbs(lon)));
    ui->cmbLongitudeType->setCurrentIndex(lon >= 0 ? 0 : 1); // 0=Восточная, 1=Западная

    ui->editAltitude->setText(QString::number(alt, 'f', 1));

    // Сохраняем высоту станции для Метео-11
    m_currentStationAltitude = alt;

    qDebug() << "MeasurementResults: Координаты загружены для record_id=" << recordId
             << "lat=" << lat << "lon=" << lon << "alt=" << alt;
}

// -------------------------------------------------------
// Стандартные высоты Метео-11 и их коды (объявлены до первого использования)
// -------------------------------------------------------
struct Meteo11Height {
    int    codeValue;
    float  heightM;
    bool   above10km;
};

// Уточнённый: все 19 стандартных уровней
static const Meteo11Height kMeteo11Heights[] = {
    {  200,   200.f, false },
    {  400,   400.f, false },
    {  800,   800.f, false },
    { 1200,  1200.f, false },
    { 1600,  1600.f, false },
    { 2000,  2000.f, false },
    { 2400,  2400.f, false },
    { 3000,  3000.f, false },
    { 4000,  4000.f, false },
    { 5000,  5000.f, false },
    { 6000,  6000.f, false },
    { 8000,  8000.f, false },
    {   10, 10000.f, true  },
    {   12, 12000.f, true  },
    {   14, 14000.f, true  },
    {   18, 18000.f, true  },
    {   22, 22000.f, true  },
    {   26, 26000.f, true  },
    {   30, 30000.f, true  },
};
static const int kMeteo11HeightCount =
    static_cast<int>(sizeof(kMeteo11Heights) / sizeof(kMeteo11Heights[0]));

// Приближённый: 02 04 08 12 16 24 30 40 (без 2000 м)
static const Meteo11Height kApproxHeights[] = {
    {  200,   200.f, false },
    {  400,   400.f, false },
    {  800,   800.f, false },
    { 1200,  1200.f, false },
    { 1600,  1600.f, false },
    { 2400,  2400.f, false },
    { 3000,  3000.f, false },
    { 4000,  4000.f, false },
};
static const int kApproxHeightCount =
    static_cast<int>(sizeof(kApproxHeights) / sizeof(kApproxHeights[0]));

// ──────────────────────────────────────────────────────────────────────────────
// Коэффициенты экстраполяции ветра для приближённого бюллетеня (Приложение 4)
// Формулы: Wy = K'y × V₀;  αWy = αV₀ + Δα'Wy
// K'y получены из примера учебника (V₀=6 м/с): Wy=9,11,11,12,13,14,14,14 м/с
// Δα'Wy — приращение направления (д.у.), ветер поворачивает вправо с высотой
// Порядок: 200, 400, 800, 1200, 1600, 2400, 3000, 4000 м (совпадает с kApproxHeights)
// При V₀ < 3 м/с скорость ветра на всех высотах принимается равной нулю
// ──────────────────────────────────────────────────────────────────────────────
struct ApproxWindCoeff { float heightM; float ky; int dalpha; };
static const ApproxWindCoeff kApproxWindCoeffs[] = {
    //  Y, м   K'y    Δα'Wy (д.у.)
    {  200.f, 1.50f,  1 },
    {  400.f, 1.83f,  2 },
    {  800.f, 1.83f,  3 },
    { 1200.f, 2.00f,  3 },
    { 1600.f, 2.17f,  4 },
    { 2400.f, 2.33f,  4 },
    { 3000.f, 2.33f,  5 },
    { 4000.f, 2.33f,  5 },
};

// ──────────────────────────────────────────────────────────────────────────────
// Таблица 3: средние отклонения температуры ΔτY (°C) без бюллетеня «Метеосредний»
// Строки: стандартные высоты 200..4000 м (9 уровней)
// Столбцы: |Δτ₀МП| = 1,2,3,4,5,6,7,8,9,10,20,30,40,50
// Значения — абсолютные; знак = знак Δτ₀МП
// ──────────────────────────────────────────────────────────────────────────────
static const int kTable3Heights[9] = { 200, 400, 800, 1200, 1600, 2000, 2400, 3000, 4000 };
static const int kTable3ColKeys[14] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50 };

// Значения для ОТРИЦАТЕЛЬНОГО Δτ₀МП (числитель дроби)
static const int kTable3Neg[9][14] = {
    //  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 20, 30, 40, 50
    {   1,  2,  3,  4,  5,  6,  7,  8,  8,  9, 20, 29, 39, 49 }, // 200 м
    {   1,  2,  3,  4,  5,  6,  6,  7,  8,  9, 19, 29, 38, 48 }, // 400 м
    {   1,  2,  3,  4,  5,  6,  7,  7,  7,  8, 18, 28, 37, 46 }, // 800 м
    {   1,  2,  3,  4,  4,  5,  5,  6,  7,  8, 17, 26, 35, 44 }, // 1200 м
    {   1,  2,  3,  3,  4,  4,  5,  6,  7,  7, 17, 25, 34, 42 }, // 1600 м
    {   1,  2,  3,  3,  4,  4,  5,  6,  6,  7, 16, 24, 32, 40 }, // 2000 м
    {   1,  2,  2,  3,  4,  4,  5,  5,  6,  7, 15, 23, 31, 38 }, // 2400 м
    {   1,  2,  2,  3,  4,  4,  4,  5,  5,  6, 15, 22, 30, 37 }, // 3000 м
    {   1,  2,  2,  3,  4,  4,  4,  4,  5,  6, 14, 20, 27, 34 }, // 4000 м
};

// Значения для ПОЛОЖИТЕЛЬНОГО Δτ₀МП (знаменатель) — одинаковы для всех высот
// Столбцы 40 и 50 не используются (нет данных)
static const int kTable3Pos[14] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 0, 0 };

// Вспомогательная функция: поиск одного значения по таблице 3
static int table3LookupAbs(int heightRow, int absKey, bool negative)
{
    for (int c = 0; c < 14; ++c) {
        if (kTable3ColKeys[c] == absKey) {
            return negative ? kTable3Neg[heightRow][c] : kTable3Pos[c];
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// Таблица 4: виртуальные поправки ΔTv при относительной влажности 50%
// Источник: формула e = 0.5E, давление H = 750 мм рт.ст.
// t (°C):    -20  -10    0    5   10   15   20   25   30   35   40
// ΔTv (°C):    0  0.1  0.3  0.5  0.7  0.9  1.3  1.8  2.4  3.3  4.4
// ──────────────────────────────────────────────────────────────────────────────
static const double kTable4T[]   = { -20, -10,  0,   5,  10,  15,  20,  25,  30,  35,  40 };
static const double kTable4Dtv[] = { 0.0, 0.1, 0.3, 0.5, 0.7, 0.9, 1.3, 1.8, 2.4, 3.3, 4.4 };
static const int    kTable4N     = 11;

// Вернуть виртуальную поправку ΔTv для наземной температуры t (°C) по Таблице 4.
// Для t ≤ −20 °C → 0; для t ≥ 40 °C → 4.4; между точками — линейная интерполяция.
static double virtualTempCorrection(double tempC)
{
    if (tempC <= kTable4T[0])          return kTable4Dtv[0];
    if (tempC >= kTable4T[kTable4N-1]) return kTable4Dtv[kTable4N-1];
    for (int i = 0; i < kTable4N - 1; ++i) {
        if (tempC >= kTable4T[i] && tempC <= kTable4T[i+1]) {
            double frac = (tempC - kTable4T[i]) / (kTable4T[i+1] - kTable4T[i]);
            return kTable4Dtv[i] + frac * (kTable4Dtv[i+1] - kTable4Dtv[i]);
        }
    }
    return 0.0;
}

// Вычислить ΔτY по Таблице 3 (без Метеосредний) для одной высоты
// Возвращает готовое закодированное ТТ значение (см. encodeTempDev)
static int computeApproxTempDev(float heightM, double deltaTau0)
{
    // Находим строку таблицы
    int row = -1;
    for (int i = 0; i < 9; ++i) {
        if (qAbs(static_cast<int>(heightM) - kTable3Heights[i]) < 1) { row = i; break; }
    }
    if (row < 0 || qAbs(deltaTau0) < 0.5) return 0;

    bool negative = deltaTau0 < 0.0;
    int  abs0     = qRound(qAbs(deltaTau0));
    if (abs0 > 50) abs0 = 50; // ограничиваем диапазоном таблицы

    // Разложение: сотни десятков + единицы (аналогично тому, как описано в протоколе)
    int tens  = (abs0 / 10) * 10;
    int units = abs0 % 10;
    int absVal = 0;
    if (tens  > 0) absVal += table3LookupAbs(row, tens, negative);
    if (units > 0) absVal += table3LookupAbs(row, units, negative);

    double signedDev = negative ? -static_cast<double>(absVal) : static_cast<double>(absVal);

    // Кодируем так же как encodeTempDev: отрицательные +50 к первой цифре
    int val = qRound(qAbs(signedDev));
    val = qMin(val, 49);
    if (signedDev < 0.0) val += 50;
    return val;
}

void MeasurementResults::loadMeteo11FromStation(int recordId)
{
    // Сбрасываем предыдущие данные
    m_meteo11FromStation         = Meteo11Data();
    m_meteo11FromStation.isValid = false;

    if (recordId <= 0 || !connectDatabase()) return;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT bulletin_data, bulletin_time "
        "FROM meteo_11_bulletin WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: бюллетень МС не найден для record_id=" << recordId;
        return; // нормально — бюллетень мог не вводиться
    }

    const QString   jsonStr = query.value(0).toString();
    const QDateTime dt      = query.value(1).toDateTime();

    QJsonObject obj = QJsonDocument::fromJson(jsonStr.toUtf8()).object();

    m_meteo11FromStation.isValid       = true;
    m_meteo11FromStation.bulletinTime  = dt;
    m_meteo11FromStation.rawString     = obj["raw_string"].toString();
    m_meteo11FromStation.stationNumber = obj["station_num"].toString();

    // Достигнутые высоты BтBтBвBв — читаем из явных полей JSON (новые записи),
    // иначе парсим последнюю 4-символьную группу из rawString (старые записи)
    {
        const QString jTH = obj["achieved_temp_height"].toString();
        const QString jWH = obj["achieved_wind_height"].toString();
        if (!jTH.isEmpty() || !jWH.isEmpty()) {
            m_meteo11FromStation.reachedTempHeightKm = jTH.toInt();
            m_meteo11FromStation.reachedWindHeightKm = jWH.toInt();
        } else if (!m_meteo11FromStation.rawString.isEmpty()) {
            QString norm = m_meteo11FromStation.rawString;
            norm.replace(QRegularExpression("[—–\\-]+"), " ");
            norm.replace(QRegularExpression("\\s+"), " ").trimmed();
            const QStringList parts = norm.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                const QString last = parts.last();
                bool ok = false;
                last.toInt(&ok);
                if (ok && last.length() == 4) {
                    m_meteo11FromStation.reachedTempHeightKm = last.left(2).toInt();
                    m_meteo11FromStation.reachedWindHeightKm = last.right(2).toInt();
                }
            }
        }
    }
    m_meteo11FromStation.stationAltitude =
        obj["station_height"].toString().toInt();
    m_meteo11FromStation.pressureDeviation =
        obj["ground_pres_dev"].toString().toInt();
    m_meteo11FromStation.tempVirtualDev =
        obj["ground_virt_temp_dev"].toString().toInt();

    // Парсим дату/время из поля "datetime" (формат ДДЧЧМ)
    {
        const QString dtStr = obj["datetime"].toString();
        if (dtStr.length() == 5) {
            m_meteo11FromStation.day        = dtStr.left(2).toInt();
            m_meteo11FromStation.hour       = dtStr.mid(2, 2).toInt();
            m_meteo11FromStation.tenMinutes = dtStr.right(1).toInt();
        }
    }

    // Парсим слои из массива layers — используем позиционное сопоставление
    // с kMeteo11Heights, чтобы корректно разделить "12" (1200м) от "12" (12км)
    {
        QJsonArray layersArr = obj["layers"].toArray();
        int scanFrom = 0;
        for (const QJsonValue &v : layersArr) {
            QJsonObject lo = v.toObject();
            int hc = lo["height_code"].toString().toInt();

            // Ищем соответствие в kMeteo11Heights начиная с scanFrom
            for (int i = scanFrom; i < kMeteo11HeightCount; ++i) {
                const Meteo11Height &h = kMeteo11Heights[i];
                int codeAsInt = h.above10km ? h.codeValue : (h.codeValue / 100);
                if (codeAsInt == hc) {
                    Meteo11Data::LayerData layer;
                    layer.heightCode  = h.codeValue;
                    layer.isAbove10km = h.above10km;
                    layer.windDir     = lo["nn"].toString().toInt();
                    layer.windSpeed   = lo["ss"].toString().toInt();
                    layer.pp          = lo["pp"].toString("//");
                    m_meteo11FromStation.layers.append(layer);
                    scanFrom = i + 1;
                    break;
                }
            }
        }
    }

    qDebug() << "MeasurementResults: бюллетень МС загружен для record_id=" << recordId
             << "время:" << dt.toString("dd.MM.yyyy HH:mm")
             << "слоёв:" << m_meteo11FromStation.layers.size();
}

// ===== ОТОБРАЖЕНИЕ ДАННЫХ =====

void MeasurementResults::displayWindProfile(const QVector<WindProfileData> &avgWind,
                                            const QVector<WindProfileData> &actualWind,
                                            const QVector<MeasuredWindData> &measuredWind)
{
    // Заполняем таблицу среднего ветра: высота из БД — в заголовок строки
    ui->tableWidget_AverageWind->setRowCount(avgWind.size());
    for (int i = 0; i < avgWind.size(); i++) {
        ui->tableWidget_AverageWind->setVerticalHeaderItem(
            i, new QTableWidgetItem(QString::number(qRound(avgWind[i].height))));
        ui->tableWidget_AverageWind->setItem(i, 0,
            new QTableWidgetItem(QString::number(avgWind[i].windSpeed, 'f', 2)));
        ui->tableWidget_AverageWind->setItem(i, 1,
            new QTableWidgetItem(QString::number(avgWind[i].windDirection)));
    }

    // Заполняем таблицу действительного ветра: высота из БД — в заголовок строки
    ui->tableWidget_realWind->setRowCount(actualWind.size());
    for (int i = 0; i < actualWind.size(); i++) {
        ui->tableWidget_realWind->setVerticalHeaderItem(
            i, new QTableWidgetItem(QString::number(qRound(actualWind[i].height))));
        ui->tableWidget_realWind->setItem(i, 0,
            new QTableWidgetItem(QString::number(actualWind[i].windSpeed, 'f', 2)));
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

    // Поля координат всегда только для чтения — независимо от режима карты
    // (данные берутся из архива БД, не от пользователя)

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

    // Все три типа поддерживают оба формата
    ui->pushButton_string->setEnabled(true);
    ui->pushButton_table->setEnabled(true);

    stackedWidget->setCurrentIndex(currentOutputFormat == String ? 0 : 1);

    updateMeteo11Display();
}

void MeasurementResults::onUpdatedButtonClicked()
{
    currentButtelinType = Updated;
    switchMeteo11Display();
    updateWindShearDisplay();
}

void MeasurementResults::onApproximateButtonClicked()
{
    currentButtelinType = Approximate;
    // switchMeteo11Display сам выставит Table и заблокирует String
    switchMeteo11Display();
    updateWindShearDisplay();
}

void MeasurementResults::onFromMeteoStatButtonClicked()
{
    currentButtelinType = FromMeteoStat;
    switchMeteo11Display();
    updateWindShearDisplay();
}

void MeasurementResults::onStringFormatClicked()
{
    currentOutputFormat = String;
    switchMeteo11Display();
}

void MeasurementResults::onTableFormatClicked()
{
    currentOutputFormat = Table;
    switchMeteo11Display();
}

void MeasurementResults::navigateToRecord(int recordId)
{
    if (recordId <= 0) return;

    // Ищем запись по recordId во всех загруженных датах
    for (auto it = availableMeasurements.constBegin(); it != availableMeasurements.constEnd(); ++it) {
        for (const MeasurementRecord &record : it.value()) {
            if (record.recordId == recordId) {
                currentDateTime = record.measurementTime;
                updateDisplay();
                qDebug() << "MeasurementResults: Переход к записи" << recordId
                         << "время" << currentDateTime.toString("dd.MM.yyyy hh:mm:ss");
                return;
            }
        }
    }

    // Если запись ещё не загружена (например, только что добавлена) — перезагружаем список
    qDebug() << "MeasurementResults: Запись" << recordId << "не найдена, перезагружаем список...";
    loadMeasurementsFromDatabase();

    for (auto it = availableMeasurements.constBegin(); it != availableMeasurements.constEnd(); ++it) {
        for (const MeasurementRecord &record : it.value()) {
            if (record.recordId == recordId) {
                currentDateTime = record.measurementTime;
                updateDisplay();
                qDebug() << "MeasurementResults: Переход к записи" << recordId
                         << "после перезагрузки, время" << currentDateTime.toString("dd.MM.yyyy hh:mm:ss");
                return;
            }
        }
    }

    qWarning() << "MeasurementResults: Запись" << recordId << "не найдена даже после перезагрузки";
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

    // Сначала ищем точное совпадение по времени
    MeasurementRecord record;
    if (availableMeasurements.contains(date)) {
        for (const MeasurementRecord &r : availableMeasurements[date]) {
            if (r.measurementTime == dateTime) {
                record = r;
                break;
            }
        }
    }
    if (record.recordId <= 0)
        record = findClosestRecord(date, hour);

    qDebug() << "MeasurementResults: loadMeasurementData"
             << dateTime.toString("yyyy-MM-dd hh:mm:ss")
             << "→ record_id=" << record.recordId;

    if (record.recordId > 0) {
        m_currentSondingTime = record.measurementTime;
        ui->lblDataStatus->setText(QString("Данные загружены (ID: %1)").arg(record.recordId));
        ui->lblDataStatus->setStyleSheet("color: green; font-weight: bold;");

        QVector<WindProfileData>  avgWind      = loadAvgWindProfile(record.recordId);
        QVector<WindProfileData>  actualWind   = loadActualWindProfile(record.recordId);
        QVector<MeasuredWindData> measuredWind = loadMeasuredWindProfile(record.recordId);

        loadSurfaceMeteoData(record.recordId);
        loadStationCoordinates(record.recordId);
        loadMeteo11FromStation(record.recordId);

        displayWindProfile(avgWind, actualWind, measuredWind);
        computeMeteo11(record.recordId, avgWind, actualWind, measuredWind);

        QString info = "Доступно: ";
        QStringList available;
        if (record.hasAvgWind)      available << "Средний ветер";
        if (record.hasActualWind)   available << "Действительный ветер";
        if (record.hasMeasuredWind) available << "Измеренный ветер";
        info += available.isEmpty() ? "Нет данных профилей" : available.join(", ");
        ui->lblDataStatus->setText(info);

    } else {
        ui->lblDataStatus->setText("Нет данных для выбранного времени");
        ui->lblDataStatus->setStyleSheet("color: red; font-weight: bold;");

        ui->tableWidget_AverageWind->clearContents();
        ui->tableWidget_realWind->clearContents();
        ui->tableWidget_izmWind_2->clearContents();
        ui->tableWidget_parm1b65->clearContents();
        ui->editLatitude->clear();
        ui->editLongitude->clear();
        ui->editAltitude->clear();

        // Сбрасываем данные Метео-11
        m_meteo11Updated     = Meteo11Data();
        m_meteo11Approximate = Meteo11Data();
        m_meteo11FromStation = Meteo11Data();
        clearMeteo11Display();
    }

    updateAvailableRecordsLabel();
    updateWindShearDisplay();
}

void MeasurementResults::updateDisplay()
{
    updateDateTimeDisplay();
    updateSliderRange();
    updateWindShearDisplay();
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
    curve->setStyle(QwtPlotCurve::Lines);

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
    curve->setStyle(QwtPlotCurve::Lines);

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
    curve->setStyle(QwtPlotCurve::Lines);

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
    curve->setStyle(QwtPlotCurve::Lines);

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

    // Очищаем график сдвига ветра
    clearWindShearDisplay();

    // Очищаем Метео-11
    m_meteo11Updated     = Meteo11Data();
    m_meteo11Approximate = Meteo11Data();
    m_meteo11FromStation = Meteo11Data();
    clearMeteo11Display();
}

// ============================================================
// МЕТОДЫ ДЛЯ РАБОТЫ СО СДВИГОМ ВЕТРА
// ============================================================

/**
 * Стандартное давление МСА на заданной высоте (формула барометрии), мм рт.ст.
 */
double MeasurementResults::standardPressureAtAlt(double altM)
{
    // МСА: P = 760 * (1 - 0.0000226 * h)^5.256
    double ratio = 1.0 - 0.0000226 * altM;
    if (ratio <= 0.0) return 0.0;
    return 760.0 * std::pow(ratio, 5.256);
}

/**
 * Стандартная температура МСА на высоте altM (м), °C
 */
double MeasurementResults::standardTempAtAlt(double altM)
{
    // Тропосфера: T = 15 - 6.5 * h/1000
    if (altM <= 11000.0)
        return 15.0 - 6.5 * altM / 1000.0;
    return -56.5; // Стратосфера
}

/**
 * Кодировать направление ветра в делениях угломера (большие деления, шаг 0-60).
 * degrees — метеорологическое направление 0..360 (откуда дует).
 * Возвращает 00..60 (60 означает «штиль» или отдельно обрабатывается).
 */
int MeasurementResults::encodeWindDir(int degrees)
{
    // Большие деления угломера: 1 д.у. = 6°, диапазон 0-60
    // Округление до ближайшего целого
    int du = qRound(degrees / 6.0);
    if (du >= 60) du = 0; // 360° = 00
    return du;
}

/**
 * Кодировать отклонение давления (мм рт.ст.) в поле БББ.
 * Правило: если отклонение отрицательное — прибавляем 500 к первой цифре
 * (или по упрощённому: добавляем 500 ко всему значению при отклонении < 0).
 */
int MeasurementResults::encodePressureDev(double deltaMmHg)
{
    int val = qRound(deltaMmHg); // округление до целого мм рт.ст.
    if (val < 0) {
        val = 500 + val; // отрицательное: +500 к первой цифре (кодирование "минус")
    }
    // Ограничиваем диапазоном 000..999
    val = qBound(0, val, 999);
    return val;
}

/**
 * Кодировать отклонение температуры (°C) в поле ТТ (двузначное).
 * Правило: отрицательные — прибавить 50 к первой цифре => первая цифра 5-9.
 */
int MeasurementResults::encodeTempDev(double deltaCelsius)
{
    int val = qRound(qAbs(deltaCelsius));
    val = qMin(val, 49); // максимум 49°
    if (deltaCelsius < 0.0) {
        val += 50; // кодирование "минус"
    }
    return val;
}

/**
 * Сформировать текстовую группу одного слоя для строкового бюллетеня.
 * Ниже 10 км: ППТТННСС (4+6 цифр со знаком «-» между ними).
 * Выше 10 км: ВВ-ТТННСС (2+6 цифр).
 * В данной реализации группа = "HННСС" (с указанием высоты и параметров).
 *
 * Реальный формат строки Метео-11:
 *  ≤10 км: ХХХХ-ТТННСС  (4-значный код высоты + 6-значный ТТННСС)
 *  >10 км: ХХ-ТТННСС   (2-значный код + 6-значный)
 */
QString MeasurementResults::formatMeteo11Group(int heightCode, const QString &pp, int dir, int speed, int tempDev, bool above10km, bool includePP, bool unavailable)
{
    // Формат уточнённого (includePP=true):
    //  ≤8000 м:  ВВПП-ТТННСС  где ВВ = высота в сотнях метров (02..80), ПП — из данных
    //  ≥10 км:   ВВПП-ТТННСС  где ВВ = высота в км (10..30)
    // Формат приближённого (includePP=false):
    //  ВВ-ТТННСС  (без ПП)
    QString hPart;

    if (!above10km) {
        int hHundreds = heightCode / 100;
        if (includePP)
            hPart = QString("%1%2").arg(hHundreds, 2, 10, QChar('0')).arg(pp);
        else
            hPart = QString("%1").arg(hHundreds, 2, 10, QChar('0'));
    } else {
        if (includePP)
            hPart = QString("%1%2").arg(heightCode, 2, 10, QChar('0')).arg(pp);
        else
            hPart = QString("%1").arg(heightCode, 2, 10, QChar('0'));
    }

    // Нет данных → ТТ=00, НН=//, СС=//
    if (unavailable)
        return hPart + "-" + "00////";

    QString ssStr = (speed >= 99)
                        ? "//"
                        : QString("%1").arg(speed, 2, 10, QChar('0'));

    QString dataPart = QString("%1%2%3")
                           .arg(tempDev, 2, 10, QChar('0'))  // ТТ
                           .arg(dir,     2, 10, QChar('0'))  // НН
                           .arg(ssStr);                      // СС

    return hPart + "-" + dataPart;
}

/**
 * Построить полную строку бюллетеня Метео-11 из структуры данных.
 * Формат: «Метео 11NNNNN–ДДЧЧМ–BBBB–БББT0T0–02ПП–ТТННСС–...–BтBтBвBв»
 */
QString MeasurementResults::buildMeteo11String(const Meteo11Data &d)
{
    if (!d.isValid)
        return "Метео 11 — нет данных";

    QStringList parts;

    // Заголовок
    if (d.isApproximate) {
        parts << "Метео 11 приближенный";
    } else {
        parts << QString("Метео 11%1").arg(d.stationNumber);
    }

    // ДДЧЧМ
    parts << QString("%1%2%3")
                 .arg(d.day,        2, 10, QChar('0'))
                 .arg(d.hour,       2, 10, QChar('0'))
                 .arg(d.tenMinutes, 1, 10, QChar('0'));

    // BBBB — высота станции
    parts << QString("%1").arg(d.stationAltitude, 4, 10, QChar('0'));

    // БББТ0Т0 — отклонение давления + отклонение виртуальной температуры
    parts << QString("%1%2")
                 .arg(d.pressureDeviation, 3, 10, QChar('0'))
                 .arg(d.tempVirtualDev,    2, 10, QChar('0'));

    // Слои: приближённый — без ПП (ВВ-ТТННСС), уточнённый — с ПП (ВВПП-ТТННСС)
    const bool includePP = !d.isApproximate;
    for (const Meteo11Data::LayerData &layer : d.layers) {
        parts << formatMeteo11Group(layer.heightCode, layer.pp,
                                    layer.windDir, layer.windSpeed, layer.tempDev,
                                    layer.isAbove10km, includePP,
                                    layer.isUnavailable);
    }

    // Достигнутые высоты BтBтBвBв (только для уточнённого)
    if (!d.isApproximate) {
        parts << QString("%1%2")
                     .arg(d.reachedTempHeightKm, 2, 10, QChar('0'))
                     .arg(d.reachedWindHeightKm, 2, 10, QChar('0'));
    }

    return parts.join("–");
}


/**
 * Построить Meteo11Data из профиля ветра.
 * windProfile — может быть avgWind или actualWind.
 */
MeasurementResults::Meteo11Data MeasurementResults::buildMeteo11(
    const QVector<WindProfileData> &windProfile,
    double stationAltitudeM,
    double pressureHpa,
    double tempC,
    const QDateTime &sondingTime,
    bool useActual,
    const Meteo11Data *oldBulletin)
{
    Meteo11Data d;
    d.isApproximate = !useActual;

    if (windProfile.isEmpty()) {
        d.isValid = false;
        return d;
    }

    // --- Заголовок ---
    d.stationNumber  = "00000"; // По умолчанию; реально из настроек станции
    d.day            = sondingTime.date().day();
    d.hour           = sondingTime.time().hour();
    d.tenMinutes     = sondingTime.time().minute() / 10;
    d.bulletinTime   = sondingTime;

    // BBBB: высота станции над уровнем моря, в метрах
    int altEncoded = qRound(stationAltitudeM);
    d.stationAltitude = qBound(0, altEncoded, 9999);

    // ΔH₀: отклонение наземного давления по протоколу Метео-11
    // ΔH₀ = H₀ - 750  (мм рт.ст., табличное значение = 750)
    // Если > 750 → знак «+», если < 750 → знак «-»
    double deltaH0      = pressureHpa - 750.0; // pressureHpa теперь уже в мм рт.ст.
    d.pressureDeviation = encodePressureDev(deltaH0);

    // Δτ₀: отклонение наземной виртуальной температуры по протоколу Метео-11
    // Шаг 1: виртуальная поправка ΔTᵥ из Таблицы 4 (r = 50%, H = 750 мм рт.ст.)
    double deltaTV = virtualTempCorrection(tempC);

    // Шаг 2: наземная виртуальная температура τ₀ = t₀ + ΔTᵥ
    double tau0 = tempC + deltaTV;

    // Шаг 3: Δτ₀ = τ₀ - 15.9  (табличное значение τ = +15.9°C)
    double deltaTau0 = tau0 - 15.9;
    d.tempVirtualDev = encodeTempDev(deltaTau0);

    // --- Слои ---
    // Для каждой стандартной высоты Метео-11 ищем ближайшую точку профиля
    float maxWindHeightM = 0.f;

    const Meteo11Height *heightTable = d.isApproximate ? kApproxHeights    : kMeteo11Heights;
    const int            heightCount = d.isApproximate ? kApproxHeightCount : kMeteo11HeightCount;

    for (int i = 0; i < heightCount; ++i) {
        const Meteo11Height &lvl = heightTable[i];

        // Поиск ближайшей точки профиля по высоте
        int    bestIdx  = -1;
        float  bestDiff = 1e9f;
        for (int j = 0; j < windProfile.size(); ++j) {
            float diff = qAbs(windProfile[j].height - lvl.heightM);
            if (diff < bestDiff) {
                bestDiff = diff;
                bestIdx  = j;
            }
        }

        // Допускаем отклонение не более 400 м для слоёв ≤8000 м
        // и не более 1500 м для слоёв ≥10000 м.
        // Дополнительное условие: профильная точка должна быть не ниже 80% целевой высоты
        // (чтобы точка 8000 м не попала в слой 10000 м).
        float tolerance = lvl.above10km ? 1500.f : 400.f;
        if (bestIdx < 0 || bestDiff > tolerance) break;
        if (windProfile[bestIdx].height < lvl.heightM * 0.80f) break;

        const WindProfileData &pt = windProfile[bestIdx];
        if (!pt.isValid) break;

        maxWindHeightM = qMax(maxWindHeightM, pt.height);

        Meteo11Data::LayerData layer;
        layer.heightCode  = lvl.codeValue;
        layer.windDir     = encodeWindDir(pt.windDirection);
        layer.windSpeed   = qRound(pt.windSpeed);
        layer.isAbove10km = lvl.above10km;
        // Для приближённого — ΔτY из Таблицы 3 (без Метеосредний)
        layer.tempDev = d.isApproximate
                        ? computeApproxTempDev(lvl.heightM, deltaTau0)
                        : 0;
        d.layers.append(layer);
    }

    // --- Для уточнённого: слои выше данных АМС ---
    // Если есть актуальный входящий бюллетень — берём его данные (уточняем до 30 км).
    // Если нет — ставим "//" (недостижимые слои).
    if (!d.isApproximate) {
        int filledCount = d.layers.size();
        for (int i = filledCount; i < heightCount; ++i) {
            const Meteo11Height &lvl = heightTable[i];

            bool foundInOld = false;
            if (oldBulletin) {
                // Ищем слой с этой высотой во входящем бюллетене
                for (const Meteo11Data::LayerData &ol : oldBulletin->layers) {
                    float oldHM = ol.isAbove10km
                                  ? ol.heightCode * 1000.f
                                  : static_cast<float>(ol.heightCode);
                    if (qAbs(oldHM - lvl.heightM) < 1.f && !ol.isUnavailable) {
                        Meteo11Data::LayerData layer = ol;
                        layer.tempDev = 0; // ТТ пока не уточняем (нет темп. зондирования)
                        maxWindHeightM = qMax(maxWindHeightM, oldHM);
                        d.layers.append(layer);
                        foundInOld = true;
                        break;
                    }
                }
            }

            if (!foundInOld) {
                Meteo11Data::LayerData layer;
                layer.heightCode    = lvl.codeValue;
                layer.isAbove10km   = lvl.above10km;
                layer.isUnavailable = true;
                d.layers.append(layer);
            }
        }
    }

    // --- Достигнутые высоты ---
    d.reachedTempHeightKm = 30; // нет темп. зондирования — заглушка
    d.reachedWindHeightKm = qRound(maxWindHeightM / 1000.0);

    d.isValid = !d.layers.isEmpty();
    return d;
}

/**
 * Приближённый бюллетень — только из наземных параметров (IWS):
 *  ΔH₀, Δτ₀МП вычисляются по протоколу; НН/СС = наземный ветер для всех высот;
 *  ΔτY на каждой высоте — по Таблице 3 (без Метеосредний).
 */
MeasurementResults::Meteo11Data MeasurementResults::buildMeteo11Approximate(
    double stationAltitudeM,
    double pressureHpa,
    double tempC,
    double surfaceWindDirDeg,
    double surfaceWindSpeedMs,
    const QDateTime &sondingTime)
{
    Meteo11Data d;
    d.isApproximate = true;

    d.stationNumber   = "00000";
    d.day             = sondingTime.date().day();
    d.hour            = sondingTime.time().hour();
    d.tenMinutes      = sondingTime.time().minute() / 10;
    d.bulletinTime    = sondingTime;
    d.stationAltitude = qBound(0, qRound(stationAltitudeM), 9999);

    // ΔH₀ = H₀ - 750 (мм рт. ст.; pressureHpa уже в мм рт.ст. из БД)
    double deltaH0 = pressureHpa - 750.0;
    d.pressureDeviation = encodePressureDev(deltaH0);

    // Виртуальная поправка ΔTv по Таблице 4 (r = 50%, H = 750 мм рт.ст.)
    // τ₀ = t₀ + ΔTv  (наземная виртуальная температура)
    // Δτ₀МП = τ₀ − 15.9  (наземное отклонение виртуальной температуры, таблица: +15.9°C)
    double deltaTV   = virtualTempCorrection(tempC);
    double deltaTau0 = (tempC + deltaTV) - 15.9;
    d.tempVirtualDev = encodeTempDev(deltaTau0);

    // НН и СС: экстраполяция по Приложению 4 (Wy = K'y × V₀, αWy = αV₀ + Δα'Wy)
    // При V₀ < 3 м/с скорость принимается равной нулю на всех высотах
    bool windTooLow  = surfaceWindSpeedMs < 3.0;
    int  groundDirDU = encodeWindDir(qRound(surfaceWindDirDeg));

    // Слои: 02 04 08 12 16 24 30 40 (без 2000 м)
    for (int i = 0; i < kApproxHeightCount; ++i) {
        const Meteo11Height    &lvl   = kApproxHeights[i];
        const ApproxWindCoeff  &coeff = kApproxWindCoeffs[i];

        int speed = windTooLow ? 0 : qMin(99, qRound(coeff.ky * surfaceWindSpeedMs));
        int dir   = (groundDirDU + coeff.dalpha + 60) % 60; // вращение вправо, ограничение 0-59

        Meteo11Data::LayerData layer;
        layer.heightCode  = lvl.codeValue;
        layer.isAbove10km = false;
        layer.windSpeed   = speed;
        layer.windDir     = dir;
        layer.tempDev     = computeApproxTempDev(lvl.heightM, deltaTau0);
        d.layers.append(layer);
    }

    d.reachedWindHeightKm = 4; // фиксированный потолок приближённого
    d.reachedTempHeightKm = 0; // не используется
    d.isValid = true;           // всегда строится при наличии наземных данных
    return d;
}

/**
 * Главная точка входа: вычислить все три варианта бюллетеня.
 */
void MeasurementResults::computeMeteo11(int recordId,
                                        const QVector<WindProfileData>  &avgWind,
                                        const QVector<WindProfileData>  &actualWind,
                                        const QVector<MeasuredWindData> &/*measuredWind*/)
{
    Q_UNUSED(recordId)

    // ── ВРЕМЯ ВХОДЯЩЕГО БЮЛЛЕТЕНЯ ────────────────────────────────────────────
    // Используем bulletin_time из БД: вычислено в onApplyClicked из ДДЧЧМ
    // и хранит правильный год/месяц/день без двусмысленности границы месяца.
    QDateTime fromStationDT;
    if (m_meteo11FromStation.isValid && m_currentSondingTime.isValid()
            && m_meteo11FromStation.bulletinTime.isValid())
        fromStationDT = m_meteo11FromStation.bulletinTime;

    // ── УТОЧНЁННЫЙ бюллетень ────────────────────────────────────────────────
    // Строится по действительному ветру (actualWind → avgWind если нет).
    // Если входящий бюллетень в пределах ±12 ч от зондирования — используем
    // его данные для слоёв выше данных АМС (уточнение до 30 км).
    // Без актуального входящего — только до высоты АМС, выше "//"
    {
        const QVector<WindProfileData> &profile =
            !actualWind.isEmpty() ? actualWind : avgWind;

        const Meteo11Data *oldBulletin = nullptr;
        if (m_meteo11FromStation.isValid && fromStationDT.isValid()) {
            qint64 ageSec = fromStationDT.secsTo(m_currentSondingTime);
            if (qAbs(ageSec) <= 12 * 3600) {
                oldBulletin = &m_meteo11FromStation;
                qDebug() << "Метео-11: входящий бюллетень актуален ("
                         << qAbs(ageSec) / 3600.0 << "ч), уточняем до 30 км";
            } else {
                qDebug() << "Метео-11: входящий бюллетень устарел ("
                         << qAbs(ageSec) / 3600.0 << "ч > 12), строим до АМС";
            }
        }

        m_meteo11Updated = buildMeteo11(profile,
                                        m_currentStationAltitude,
                                        m_currentPressureMmHg,
                                        m_currentTempC,
                                        m_currentSondingTime,
                                        true /*useActual*/,
                                        oldBulletin);
        m_meteo11Updated.isValid = !profile.isEmpty();
    }

    // ── ПРИБЛИЖЁННЫЙ бюллетень ───────────────────────────────────────────────
    // Строится только из наземных параметров IWS (давление, температура, ветер).
    // Не требует профиля ветра. НН/СС = наземный ветер для всех высот.
    // ΔτY вычисляется по Таблице 3 протокола.
    {
        m_meteo11Approximate = buildMeteo11Approximate(
            m_currentStationAltitude,
            m_currentPressureMmHg,
            m_currentTempC,
            m_currentWindDirSurface,
            m_currentWindSpeedSurface,
            m_currentSondingTime);
    }

    // ── ОТ МЕТЕОСТАНЦИИ ──────────────────────────────────────────────────────
    // Данные уже загружены из meteo_11_bulletin через loadMeteo11FromStation().
    // Если бюллетень не вводился — m_meteo11FromStation.isValid = false.

    // ── АВТОВЫБОР ТИПА БЮЛЛЕТЕНЯ ────────────────────────────────────────────
    // Правило:
    //  • Бюллетень от МС есть в БД И не старше 12 ч → показываем УТОЧНЁННЫЙ
    //  • Иначе → ПРИБЛИЖЁННЫЙ
    //  • «От метеостанции» — только по явному нажатию кнопки
    {
        bool stationActual = false;
        if (m_meteo11FromStation.isValid && fromStationDT.isValid()) {
            const qint64 ageSec = fromStationDT.secsTo(m_currentSondingTime);
            stationActual = (qAbs(ageSec) <= 12 * 3600);
        }

        if (stationActual && m_meteo11Updated.isValid) {
            currentButtelinType = Updated;
        } else if (m_meteo11Approximate.isValid) {
            currentButtelinType = Approximate;
            qDebug() << "Метео-11: автовыбор → ПРИБЛИЖЁННЫЙ"
                     << (m_meteo11FromStation.isValid ? "(бюллетень МС устарел)" : "(бюллетень МС не введён)");
        }
        // Если ни один не валиден — оставляем текущий выбор пользователя
    }

    updateMeteo11Display();
}

/**
 * Обновить отображение вкладки Метео-11 в соответствии с текущим типом и форматом.
 */
void MeasurementResults::updateMeteo11Display()
{
    // Выбираем нужную структуру данных
    const Meteo11Data *d = nullptr;
    switch (currentButtelinType) {
    case Updated:      d = &m_meteo11Updated;     break;
    case Approximate:  d = &m_meteo11Approximate; break;
    case FromMeteoStat:d = &m_meteo11FromStation;  break;
    }

    if (!d) return;

    // Вычисляем давность входящего бюллетеня МС относительно времени зондирования.
    // Используется для индикации устаревшего бюллетеня и объяснения, почему уточнённый
    // не включает данные выше АМС.
    double stationAgeH  = -1.0;  // <0 = нет данных / нельзя вычислить
    bool   stationStale = false;
    if (m_meteo11FromStation.isValid && m_currentSondingTime.isValid()
            && m_meteo11FromStation.bulletinTime.isValid()) {
        qint64 ageSec   = m_meteo11FromStation.bulletinTime.secsTo(m_currentSondingTime);
        stationAgeH     = qAbs(ageSec) / 3600.0;
        stationStale    = (stationAgeH > 12.0);
    }

    // Скрываем поля, не применимые к приближённому бюллетеню
    const bool isApprox = (currentButtelinType == Approximate);
    ui->lineEdit_numStation->setVisible(!isApprox);
    ui->label_numStation->setVisible(!isApprox);
    ui->lineEdit_ht->setVisible(!isApprox);
    ui->label_tempSensingHReached->setVisible(!isApprox);

    // Обновляем поля ГОДНЫЙ / ВРЕМЯ СОСТАВЛЕНИЯ
    if (d->isValid) {
        QString text, style;

        if (currentButtelinType == FromMeteoStat && stationAgeH >= 0.0) {
            // "От МС" — показываем давность и годность входящего бюллетеня
            if (stationStale) {
                text  = QString("УСТАРЕЛ (%1 ч)").arg(stationAgeH, 0, 'f', 1);
                style = "color: #e65100; font-weight: bold; background-color: #fff8e1;";
            } else {
                text  = QString("ГОДНЫЙ (%1 ч)").arg(stationAgeH, 0, 'f', 1);
                style = "color: #2e7d32; font-weight: bold;";
            }
        } else if (currentButtelinType == Updated) {
            // "Уточнённый" — бюллетень построен, цвет всегда зелёный
            if (!m_meteo11FromStation.isValid) {
                text  = "ГОДНЫЙ";
                style = "color: #2e7d32; font-weight: bold;";
            } else if (stationStale) {
                text  = QString("ГОДНЫЙ (МС устарел %1 ч)").arg(stationAgeH, 0, 'f', 0);
                style = "color: #2e7d32; font-weight: bold;";
            } else {
                text  = "ГОДНЫЙ (с данными МС)";
                style = "color: #2e7d32; font-weight: bold;";
            }
        } else {
            text  = "ГОДНЫЙ";
            style = "color: #2e7d32; font-weight: bold;";
        }

        ui->lineEdit_bulleten->setText(text);
        ui->lineEdit_bulleten->setStyleSheet(style);
    } else {
        ui->lineEdit_bulleten->setText("НЕТ ДАННЫХ");
        ui->lineEdit_bulleten->setStyleSheet("color: red; font-weight: bold;");
    }

    // Время составления бюллетеня — янтарный фон когда устарел
    ui->lineEdit_bulletenTime->setText(
        d->bulletinTime.isValid()
            ? d->bulletinTime.toString("dd.MM.yyyy hh:mm")
            : "—"
        );
    if (currentButtelinType == FromMeteoStat && stationStale && d->isValid) {
        ui->lineEdit_bulletenTime->setStyleSheet(
            "QLineEdit { background-color: #fff3e0; color: #e65100; font-weight: bold; }");
    } else {
        ui->lineEdit_bulletenTime->setStyleSheet("");
    }

    // Кнопки: имитируем «нажатое» состояние через стиль
    auto setPressed = [](QPushButton *btn, bool pressed) {
        if (!btn) return;
        btn->setStyleSheet(pressed
                               ? "QPushButton { background-color: #3a7bd5; color: white; font-weight: bold; "
                                 "border: 2px inset #1a4fa0; border-radius: 4px; padding: 4px 8px; }"
                               : "");
    };
    setPressed(ui->pushButton_updated,       currentButtelinType == Updated);
    setPressed(ui->pushButton_approximate,   currentButtelinType == Approximate);
    setPressed(ui->pushButton_fromMeteoStat, currentButtelinType == FromMeteoStat);

    auto setFmtPressed = [](QPushButton *btn, bool pressed) {
        if (!btn) return;
        btn->setStyleSheet(pressed
                               ? "QPushButton { background-color: #3a7bd5; color: white; font-weight: bold; "
                                 "border: 2px inset #1a4fa0; border-radius: 4px; padding: 4px 8px; }"
                               : "");
    };
    setFmtPressed(ui->pushButton_string, currentOutputFormat == String);
    setFmtPressed(ui->pushButton_table,  currentOutputFormat == Table);

    // Кнопка «От МС» — янтарный/оранжевый и тултип когда бюллетень устарел
    if (stationStale && m_meteo11FromStation.isValid) {
        const bool sel = (currentButtelinType == FromMeteoStat);
        ui->pushButton_fromMeteoStat->setStyleSheet(sel
            ? "QPushButton { background-color: #e65100; color: white; font-weight: bold; "
              "border: 2px inset #bf360c; border-radius: 4px; padding: 4px 8px; }"
            : "QPushButton { color: #e65100; font-weight: bold; "
              "border: 1px solid #e65100; border-radius: 4px; padding: 4px 8px; }");
        ui->pushButton_fromMeteoStat->setToolTip(
            QString("Бюллетень МС устарел на %1 ч (>12 ч) — не используется в уточнённом")
                .arg(stationAgeH, 0, 'f', 1));
    } else {
        ui->pushButton_fromMeteoStat->setToolTip("");
    }

    if (!d->isValid) {
        // Для FromMeteoStat — особое сообщение, остальные — «нет данных»
        QString msg;
        if (currentButtelinType == FromMeteoStat) {
            msg = "<html><body style=\"font-family:'Tahoma'; font-size:12pt; color:#555;\">"
                  "<p>Бюллетень «От метеостанции» формируется по данным внешней "
                  "метеостанции (устаревший бюллетень «Метеосредний»).</p>"
                  "<p>Введите данные устаревшего бюллетеня вручную или загрузите "
                  "из внешнего источника.</p>"
                  "</body></html>";
        } else {
            msg = "<html><body style=\"font-family:'Tahoma'; font-size:14pt;\">"
                  "<p style=\"color:gray;\">— нет данных —</p></body></html>";
        }
        ui->textEdit_meteo11->setHtml(msg);

        QTableWidget *table = ui->tableWidget_meteo11Formalize;
        if (table) {
            for (int r = 0; r < table->rowCount(); ++r)
                for (int c = 0; c < table->columnCount(); ++c)
                    table->setItem(r, c, new QTableWidgetItem(""));
        }

        ui->lineEdit_dt->clear();
        ui->lineEdit_h->clear();
        ui->lineEdit_p->clear();
        ui->lineEdit_t->clear();
        ui->lineEdit_ht->clear();
        ui->lineEdit_hw->clear();
        ui->lineEdit_numStation->clear();
        return;
    }

    // Заполняем поля таблицы верхнего уровня (нечувствительны к формату)
    fillMeteo11InfoFields(*d);

    // Заполняем контентные виджеты
    if (currentOutputFormat == String) {
        fillMeteo11StringView(*d);
    } else {
        fillMeteo11TableView(*d);
    }
}

/**
 * Заполнить информационные поля на вкладке таблицы (правый верхний блок).
 */
void MeasurementResults::fillMeteo11InfoFields(const Meteo11Data &d)
{
    // lineEdit_dt — дата/время ДДЧЧММ
    ui->lineEdit_dt->setText(QString("%1%2%3")
                                 .arg(d.day,        2, 10, QChar('0'))
                                 .arg(d.hour,       2, 10, QChar('0'))
                                 .arg(d.tenMinutes, 1, 10, QChar('0')));

    // lineEdit_h — высота станции BBBB
    ui->lineEdit_h->setText(QString("%1").arg(d.stationAltitude, 4, 10, QChar('0')));

    // lineEdit_p — отклонение давления БББ
    ui->lineEdit_p->setText(QString("%1").arg(d.pressureDeviation, 3, 10, QChar('0')));

    // lineEdit_t — отклонение вирт. температуры T0T0
    ui->lineEdit_t->setText(QString("%1").arg(d.tempVirtualDev, 2, 10, QChar('0')));

    // lineEdit_ht — достигнутая высота темп. зондирования (только для уточнённого)
    if (d.isApproximate)
        ui->lineEdit_ht->clear();
    else
        ui->lineEdit_ht->setText(QString::number(d.reachedTempHeightKm));

    // lineEdit_hw — достигнутая высота ветрового зондирования
    ui->lineEdit_hw->setText(QString::number(d.reachedWindHeightKm));

    // lineEdit_numStation
    ui->lineEdit_numStation->setText(d.stationNumber);
}

/**
 * Заполнить строковое представление бюллетеня.
 * Пишем в оба textEdit (строка на стр.0 и строка-приближённый на стр.2).
 */
void MeasurementResults::fillMeteo11StringView(const Meteo11Data &d)
{
    // Для бюллетеня «От метеостанции» — показываем сырую строку из БД как есть
    QString text = (!d.rawString.isEmpty() && currentButtelinType == FromMeteoStat)
                   ? d.rawString
                   : buildMeteo11String(d);

    // Форматирование: переносим длинную строку на несколько строк блоками
    // (каждая строка ≈ 5 групп)
    const int groupsPerLine = 5;
    QStringList allGroups = text.split("–");
    QStringList lines;
    QString currentLine;
    int groupCount = 0;

    for (int i = 0; i < allGroups.size(); ++i) {
        if (groupCount == 0) {
            currentLine = allGroups[i];
        } else {
            currentLine += "–" + allGroups[i];
        }
        groupCount++;

        if (groupCount >= groupsPerLine && i < allGroups.size() - 1) {
            lines << currentLine + "–";
            currentLine.clear();
            groupCount = 0;
        }
    }
    if (!currentLine.isEmpty()) lines << currentLine;

    QString html = "<html><body style=\"font-family:'Tahoma'; font-size:14pt;\">";
    for (const QString &line : lines) {
        html += "<p align=\"justify\">" + line.toHtmlEscaped() + "</p>";
    }
    html += "</body></html>";

    ui->textEdit_meteo11->setHtml(html);
}

/**
 * Заполнить табличное представление бюллетеня (tableWidget_meteo11Formalize).
 * Строки: стандартные высоты; столбцы: ПП (код высоты), ТТННСС.
 */
void MeasurementResults::fillMeteo11TableView(const Meteo11Data &d)
{
    QTableWidget *table = ui->tableWidget_meteo11Formalize;
    if (!table) return;

    // Все высоты таблицы (19 строк)
    static const float kTableHeights[] = {
        200, 400, 800, 1200, 1600, 2000, 2400, 3000, 4000,
        5000, 6000, 8000, 10000, 12000, 14000, 18000, 22000, 26000, 30000
    };
    static const int kTableRowCount = static_cast<int>(sizeof(kTableHeights)/sizeof(kTableHeights[0]));

    // Высоты приближённого бюллетеня (02 04 08 12 16 24 30 40 — без 2000 м)
    static const float kApproxTableHeights[] = {
        200, 400, 800, 1200, 1600, 2400, 3000, 4000
    };
    static const int kApproxTableCount = static_cast<int>(sizeof(kApproxTableHeights)/sizeof(kApproxTableHeights[0]));

    // Показываем / скрываем строки в зависимости от типа бюллетеня
    for (int r = 0; r < kTableRowCount; ++r) {
        bool visible = true;
        if (d.isApproximate) {
            visible = false;
            for (int a = 0; a < kApproxTableCount; ++a) {
                if (qAbs(kApproxTableHeights[a] - kTableHeights[r]) < 1.f) { visible = true; break; }
            }
        }
        table->setRowHidden(r, !visible);
    }

    // Очищаем содержимое видимых строк
    for (int r = 0; r < kTableRowCount; ++r) {
        if (table->isRowHidden(r)) continue;
        for (int c = 0; c < table->columnCount(); ++c)
            table->setItem(r, c, new QTableWidgetItem(""));
    }

    // Заполняем данными слоёв
    for (const Meteo11Data::LayerData &layer : d.layers) {
        float heightM = layer.isAbove10km
                            ? layer.heightCode * 1000.f
                            : static_cast<float>(layer.heightCode);

        for (int r = 0; r < kTableRowCount; ++r) {
            if (qAbs(kTableHeights[r] - heightM) < 1.f) {
                auto *itemPP = new QTableWidgetItem(layer.pp);
                itemPP->setTextAlignment(Qt::AlignCenter);
                table->setItem(r, 0, itemPP);

                QString dataText;
                if (layer.isUnavailable) {
                    dataText = "//";
                } else {
                    QString ssStr = (layer.windSpeed >= 99)
                                        ? "//"
                                        : QString("%1").arg(layer.windSpeed, 2, 10, QChar('0'));
                    dataText = QString("%1%2%3")
                                   .arg(layer.tempDev, 2, 10, QChar('0'))
                                   .arg(layer.windDir, 2, 10, QChar('0'))
                                   .arg(ssStr);
                }
                auto *itemData = new QTableWidgetItem(dataText);
                itemData->setTextAlignment(Qt::AlignCenter);
                table->setItem(r, 1, itemData);
                break;
            }
        }
    }

    table->resizeColumnsToContents();
}

/**
 * Сбросить все отображения Метео-11 в пустое состояние.
 */
void MeasurementResults::clearMeteo11Display()
{
    ui->textEdit_meteo11->setHtml(
        "<html><body style=\"font-family:'Tahoma'; font-size:14pt;\">"
        "<p>— нет данных —</p></body></html>");

    QTableWidget *table = ui->tableWidget_meteo11Formalize;
    if (table) {
        for (int r = 0; r < table->rowCount(); ++r)
            for (int c = 0; c < table->columnCount(); ++c)
                table->setItem(r, c, new QTableWidgetItem(""));
    }

    ui->lineEdit_dt->clear();
    ui->lineEdit_h->clear();
    ui->lineEdit_p->clear();
    ui->lineEdit_t->clear();
    ui->lineEdit_ht->clear();
    ui->lineEdit_hw->clear();
    ui->lineEdit_numStation->clear();
    ui->lineEdit_bulleten->setText("НЕТ ДАННЫХ");
    ui->lineEdit_bulleten->setStyleSheet("color: gray;");
    ui->lineEdit_bulletenTime->clear();
}

// ============================================================
// ==================== СДВИГ ВЕТРА ===========================
// ============================================================


void MeasurementResults::setupWindShearTab()
{
    qDebug() << "setupWindShearTab: начало";

    // Проверяем наличие элементов UI
    if (!ui->plot_windShearSpeed || !ui->plot_windShearDirection || !ui->table_windShear) {
        qWarning() << "WindShear UI elements not found!";
        qWarning() << "plot_windShearSpeed:" << ui->plot_windShearSpeed;
        qWarning() << "plot_windShearDirection:" << ui->plot_windShearDirection;
        qWarning() << "table_windShear:" << ui->table_windShear;
        return;
    }

    qDebug() << "setupWindShearTab: UI элементы найдены";

    // ===== НАСТРОЙКА ГРАФИКА СКОРОСТИ =====
    ui->plot_windShearSpeed->setTitle(QString::fromUtf8("Скорость сдвига ветра"));
    ui->plot_windShearSpeed->setAxisTitle(QwtPlot::xBottom, QString::fromUtf8("Скорость, м/с/30м"));
    ui->plot_windShearSpeed->setAxisTitle(QwtPlot::yLeft, QString::fromUtf8("Высота, м"));
    ui->plot_windShearSpeed->setCanvasBackground(Qt::white);

    // Сетка для графика скорости
    m_windShearGrid = new QwtPlotGrid();
    m_windShearGrid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    m_windShearGrid->setMinorPen(QPen(Qt::lightGray, 0, Qt::DotLine));
    m_windShearGrid->attach(ui->plot_windShearSpeed);

    // Кривая для скорости
    m_windShearCurve = new QwtPlotCurve(QString::fromUtf8("Сдвиг ветра"));
    m_windShearCurve->setPen(QPen(QColor(0, 0, 255), 2));  // Синяя линия
    m_windShearCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    // Символы на точках
    QwtSymbol *symbolSpeed = new QwtSymbol(QwtSymbol::Ellipse);
    symbolSpeed->setSize(8, 8);
    symbolSpeed->setPen(QPen(QColor(0, 0, 255)));
    symbolSpeed->setBrush(QBrush(QColor(0, 0, 255)));
    m_windShearCurve->setSymbol(symbolSpeed);

    m_windShearCurve->attach(ui->plot_windShearSpeed);

    // Легенда для графика скорости
    QwtLegend *legendSpeed = new QwtLegend();
    ui->plot_windShearSpeed->insertLegend(legendSpeed, QwtPlot::BottomLegend);

    qDebug() << "setupWindShearTab: график скорости настроен";

    // ===== НАСТРОЙКА ГРАФИКА НАПРАВЛЕНИЯ =====
    ui->plot_windShearDirection->setTitle(QString::fromUtf8("Изменение направления ветра"));
    ui->plot_windShearDirection->setAxisTitle(QwtPlot::xBottom, QString::fromUtf8("Направление, °"));
    ui->plot_windShearDirection->setAxisTitle(QwtPlot::yLeft, QString::fromUtf8("Высота, м"));
    ui->plot_windShearDirection->setCanvasBackground(Qt::white);

    // Сетка для графика направления
    QwtPlotGrid *gridDirection = new QwtPlotGrid();
    gridDirection->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    gridDirection->setMinorPen(QPen(Qt::lightGray, 0, Qt::DotLine));
    gridDirection->attach(ui->plot_windShearDirection);

    // Кривая для направления
    QwtPlotCurve *curveDirection = new QwtPlotCurve(QString::fromUtf8("Изменение направления"));
    curveDirection->setPen(QPen(QColor(255, 0, 0), 2));  // Красная линия
    curveDirection->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    // Символы на точках
    QwtSymbol *symbolDirection = new QwtSymbol(QwtSymbol::Ellipse);
    symbolDirection->setSize(8, 8);
    symbolDirection->setPen(QPen(QColor(255, 0, 0)));
    symbolDirection->setBrush(QBrush(QColor(255, 0, 0)));
    curveDirection->setSymbol(symbolDirection);

    curveDirection->attach(ui->plot_windShearDirection);

    // Легенда для графика направления
    QwtLegend *legendDirection = new QwtLegend();
    ui->plot_windShearDirection->insertLegend(legendDirection, QwtPlot::BottomLegend);

    qDebug() << "setupWindShearTab: график направления настроен";

    // ===== НАСТРОЙКА ТАБЛИЦЫ =====
    ui->table_windShear->horizontalHeader()->setStretchLastSection(true);
    ui->table_windShear->verticalHeader()->setVisible(false);
    ui->table_windShear->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table_windShear->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table_windShear->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->table_windShear->setColumnWidth(0, 90);   // Высота
    ui->table_windShear->setColumnWidth(1, 140);  // Скорость
    ui->table_windShear->setColumnWidth(2, 180);  // Изменение направления

    qDebug() << "setupWindShearTab: завершено успешно";
}

/**
 * @brief Обновление отображения сдвига ветра
 */
void MeasurementResults::updateWindShearDisplay()
{
    // Проверяем что вкладка инициализирована
    if (!ui->plot_windShearSpeed || !ui->plot_windShearDirection || !ui->table_windShear || !m_windShearCurve) {
        return;
    }

    // Загружаем ТОЛЬКО измеренный ветер через record_id текущей записи
    MeasurementRecord record = findClosestRecord(currentDateTime.date(), currentDateTime.time().hour());
    // Уточняем: ищем точное совпадение
    if (availableMeasurements.contains(currentDateTime.date())) {
        for (const MeasurementRecord &r : availableMeasurements[currentDateTime.date()]) {
            if (r.measurementTime == currentDateTime) { record = r; break; }
        }
    }

    QVector<MeasuredWindData> measuredWind = loadMeasuredWindProfile(record.recordId);

    qDebug() << "updateWindShearDisplay: measuredWind size =" << measuredWind.size();

    QVector<WindShearData> shearData;

    // ВСЕГДА используем измеренный ветер (он содержит данные скорости и направления)
    if (!measuredWind.isEmpty()) {
        qDebug() << "updateWindShearDisplay: используем measuredWind (способ 1: скорость+направление)";
        shearData = WindShearCalculator::calculateShear(measuredWind);
    }

    qDebug() << "updateWindShearDisplay: shearData size =" << shearData.size();

    // Сохраняем данные
    m_currentShearData = shearData;

    // Отображаем
    if (!shearData.isEmpty()) {
        plotWindShear(shearData);
        updateWindShearTable(shearData);
    } else {
        clearWindShearDisplay();
    }
}

/**
 * @brief Построение графика сдвига ветра
 */
void MeasurementResults::plotWindShear(const QVector<WindShearData> &shearData)
{
    if (!m_windShearCurve || !ui->plot_windShearSpeed || !ui->plot_windShearDirection || shearData.isEmpty()) {
        return;
    }

    // Подготовка данных для скорости
    QVector<double> xDataSpeed;      // Скорость сдвига (м/с/30м)
    QVector<double> yDataSpeed;      // Высота (м)

    // Подготовка данных для направления
    QVector<double> xDataDirection;  // Изменение направления (°)
    QVector<double> yDataDirection;  // Высота (м)

    for (const WindShearData &shear : shearData) {
        // Данные для графика скорости
        xDataSpeed.append(shear.shearPer30m);
        yDataSpeed.append(shear.height);

        // Данные для графика направления
        xDataDirection.append(shear.shearDirection);
        yDataDirection.append(shear.height);
    }

    // Установка данных в кривую скорости
    m_windShearCurve->setSamples(xDataSpeed.data(), yDataSpeed.data(), xDataSpeed.size());

    // Получаем кривую направления
    QwtPlotItemList items = ui->plot_windShearDirection->itemList(QwtPlotItem::Rtti_PlotCurve);
    if (!items.isEmpty()) {
        QwtPlotCurve *curveDirection = static_cast<QwtPlotCurve*>(items.first());
        curveDirection->setSamples(xDataDirection.data(), yDataDirection.data(), xDataDirection.size());
    }

    // Обновление графиков
    ui->plot_windShearSpeed->replot();
    ui->plot_windShearDirection->replot();
}

/**
 * @brief Обновление таблицы сдвига ветра с цветовой индикацией
 */
void MeasurementResults::updateWindShearTable(const QVector<WindShearData> &shearData)
{
    if (!ui->table_windShear) {
        return;
    }

    ui->table_windShear->setRowCount(shearData.size());

    for (int i = 0; i < shearData.size(); ++i) {
        const WindShearData &shear = shearData[i];

        // Колонка 0: Высота
        QTableWidgetItem *heightItem = new QTableWidgetItem(
            QString::number(static_cast<int>(shear.height))
            );
        heightItem->setTextAlignment(Qt::AlignCenter);
        ui->table_windShear->setItem(i, 0, heightItem);

        // Колонка 1: Скорость сдвига с цветовой индикацией
        QTableWidgetItem *speedItem = new QTableWidgetItem(
            QString::number(shear.shearPer30m, 'f', 2)
            );
        speedItem->setTextAlignment(Qt::AlignCenter);

        // Цвет фона по критичности
        QColor bgColor = WindShearCalculator::getSeverityColor(shear.severityLevel);
        speedItem->setBackground(QBrush(bgColor));

        ui->table_windShear->setItem(i, 1, speedItem);

        // Колонка 2: Изменение направления
        QTableWidgetItem *directionItem = new QTableWidgetItem(
            QString::number(shear.shearDirection, 'f', 1)
            );
        directionItem->setTextAlignment(Qt::AlignCenter);
        ui->table_windShear->setItem(i, 2, directionItem);

        // Колонка 3: Текстовое описание уровня
        QTableWidgetItem *levelItem = new QTableWidgetItem(
            WindShearCalculator::getSeverityText(shear.severityLevel)
            );
        levelItem->setTextAlignment(Qt::AlignCenter);
        levelItem->setBackground(QBrush(bgColor));

        ui->table_windShear->setItem(i, 3, levelItem);
    }

    // Автоподгонка высоты строк
    ui->table_windShear->resizeRowsToContents();
}

/**
 * @brief Очистка отображения сдвига ветра
 */
void MeasurementResults::clearWindShearDisplay()
{
    if (m_windShearCurve) {
        m_windShearCurve->setSamples(QVector<QPointF>());
        if (ui->plot_windShearSpeed)
            ui->plot_windShearSpeed->replot();
    }

    if (ui->plot_windShearDirection) {
        const QwtPlotItemList items = ui->plot_windShearDirection->itemList(QwtPlotItem::Rtti_PlotCurve);
        for (QwtPlotItem *item : items)
            static_cast<QwtPlotCurve*>(item)->setSamples(QVector<QPointF>());
        ui->plot_windShearDirection->replot();
    }

    if (ui->table_windShear)
        ui->table_windShear->setRowCount(0);

    m_currentShearData.clear();
}

MeasurementSnapshot MeasurementResults::buildSnapshot() const
{
    MeasurementSnapshot snap;

    // Определяем текущую запись (та же логика, что в loadMeasurementData)
    MeasurementRecord record;
    QDate date = currentDateTime.date();
    if (availableMeasurements.contains(date)) {
        for (const MeasurementRecord &r : availableMeasurements[date]) {
            if (r.measurementTime == currentDateTime) { record = r; break; }
        }
    }
    if (record.recordId <= 0)
        record = const_cast<MeasurementResults*>(this)
                     ->findClosestRecord(date, currentDateTime.time().hour());

    snap.recordId        = record.recordId;
    snap.measurementTime = (record.recordId > 0)
                               ? record.measurementTime
                               : currentDateTime;
    snap.stationNumber   = ui->lineEdit_numStation->text().trimmed();

    if (snap.recordId <= 0)
        return snap;   // Нет загруженных данных — возвращаем пустой снимок

    // ── Координаты ───────────────────────────────────────────────────────────
    // Используем кешированные значения из loadStationCoordinates()
    // (без вызова CoordHelper::fromDisplayDMS — см. ШАГ 3)
    snap.coordinatesValid = !ui->editLatitude->text().trimmed().isEmpty()
                            && !ui->editLongitude->text().trimmed().isEmpty();
    if (snap.coordinatesValid) {
        snap.latitude  = m_currentLatitude;
        snap.longitude = m_currentLongitude;
        snap.altitude  = m_currentStationAltitude;
    }

    // ── Наземные метеоусловия ────────────────────────────────────────────────
    {
        QTableWidget *t = ui->tableWidget_parm1b65;
        snap.surfaceMeteoValid = (t->item(0, 0) != nullptr &&
                                  !t->item(0, 0)->text().isEmpty());
        if (snap.surfaceMeteoValid) {
            // Строки в tableWidget_parm1b65: 0=давление, 1=темп, 2=влажность,
            //                                3=направление, 4=скорость
            auto cell = [&](int row) -> double {
                return t->item(row, 0) ? t->item(row, 0)->text().toDouble() : 0.0;
            };
            snap.pressureMmHg      = cell(0);
            snap.temperatureC     = cell(1);
            snap.humidityPct      = cell(2);
            snap.surfaceWindDir   = cell(3);
            snap.surfaceWindSpeed = cell(4);
        }
    }

    // ── Профили ветра ────────────────────────────────────────────────────────
    // Загружаем из БД (лёгкий повторный запрос — данные кешируются на уровне БД)
    snap.avgWind      = const_cast<MeasurementResults*>(this)->loadAvgWindProfile(snap.recordId);
    snap.actualWind   = const_cast<MeasurementResults*>(this)->loadActualWindProfile(snap.recordId);
    snap.measuredWind = const_cast<MeasurementResults*>(this)->loadMeasuredWindProfile(snap.recordId);

    // ── Сдвиг ветра ──────────────────────────────────────────────────────────
    snap.windShear = m_currentShearData;

    // ── Рендеринг графиков в QImage для PDF ──────────────────────────────────
    auto renderPlot = [](QwtPlot *plot, int w, int h) -> QImage {
        if (!plot) return {};
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(Qt::white);
        QPainter painter(&img);
        QwtPlotRenderer renderer;
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground, false);
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasFrame,  false);
        renderer.render(plot, &painter, QRectF(img.rect()));
        return img;
    };

    const int CW = 560, CH = 250;
    snap.charts["avgSpeed"]    = renderPlot(ui->plot_midWindSpeed,       CW, CH);
    snap.charts["avgDir"]      = renderPlot(ui->plot_midWindAzimut,      CW, CH);
    snap.charts["actualSpeed"] = renderPlot(ui->plot_realWindSpeed,      CW, CH);
    snap.charts["actualDir"]   = renderPlot(ui->plot_realWindAzimut,     CW, CH);
    snap.charts["measSpeed"]   = renderPlot(ui->plot_izmWindSpeed_2,     CW, CH);
    snap.charts["measDir"]     = renderPlot(ui->plot_izmWindAzimut_2,    CW, CH);
    snap.charts["shearSpeed"]  = renderPlot(ui->plot_windShearSpeed,     CW, CH);
    snap.charts["shearDir"]    = renderPlot(ui->plot_windShearDirection, CW, CH);

    // ── Метео-11 ─────────────────────────────────────────────────────────────
    auto copyM11 = [](const MeasurementResults::Meteo11Data        &src,
                      MeasurementSnapshot::Meteo11Export            &dst) {
        dst.valid           = src.isValid;
        dst.bulletinString  = src.isValid
                                 ? MeasurementResults::buildMeteo11String(src)
                                 : QString();
        dst.stationNumber   = src.stationNumber;
        dst.day             = src.day;
        dst.hour            = src.hour;
        dst.tenMinutes      = src.tenMinutes;
        dst.stationAltitude = src.stationAltitude;
        dst.pressureDev     = src.pressureDeviation;
        dst.tempVirtDev     = src.tempVirtualDev;
        dst.reachedTempKm   = src.reachedTempHeightKm;
        dst.reachedWindKm   = src.reachedWindHeightKm;
    };

    copyM11(m_meteo11Updated,     snap.meteo11Updated);
    copyM11(m_meteo11Approximate, snap.meteo11Approximate);
    copyM11(m_meteo11FromStation, snap.meteo11FromStation);

    return snap;

}

// ─────────────────────────────────────────────────────────────────────────────
// Слот кнопки «Экспорт»
// ─────────────────────────────────────────────────────────────────────────────
void MeasurementResults::onExportClicked()
{
    // 1. Собираем снимок данных
    MeasurementSnapshot snap = buildSnapshot();

    if (snap.recordId <= 0) {
        QMessageBox::information(this, "Нет данных для экспорта",
                                 "Выберите дату и время с доступными измерениями,\n"
                                 "чтобы данные были загружены из архива.");
        return;
    }

    // 2. Показываем диалог настройки экспорта (из .ui-файла)
    ExportDialog dlg(snap, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    ExportOptions opts = dlg.getOptions();

    // 3. Диалог выбора пути
    const struct { ExportOptions::Format fmt; const char *filter; } kFilters[] = {
                    { ExportOptions::TXT,  "Текстовый файл (*.txt);;Все файлы (*)"  },
                    { ExportOptions::CSV,  "CSV файл (*.csv);;Все файлы (*)"        },
                    { ExportOptions::JSON, "JSON файл (*.json);;Все файлы (*)"      },
                    { ExportOptions::PDF,  "PDF файл (*.pdf);;Все файлы (*)"        },
                    { ExportOptions::XLSX, "Excel файл (*.xlsx);;Все файлы (*)"     },
                    };
    QString filter;
    for (const auto &kf : kFilters)
        if (kf.fmt == opts.format) { filter = kf.filter; break; }

    QString defaultName = MeasurementExporter::suggestedFileName(snap, opts.format);
    QString path = QFileDialog::getSaveFileName(
        this,
        "Сохранить результаты измерений",
        QDir::homePath() + QDir::separator() + defaultName,
        filter);

    if (path.isEmpty())
        return;

    // 4. Сохранение
    bool ok = false;
    QString errorMsg;

    if (opts.format == ExportOptions::PDF) {
        ok = MeasurementExporter::generatePdf(snap, opts, path, errorMsg);
    }
    else if (opts.format == ExportOptions::XLSX) {
        ok = MeasurementExporter::generateXlsx(snap, opts, path, errorMsg);
    }
    else {
        // TXT / CSV / JSON
        QString content = MeasurementExporter::generate(snap, opts, errorMsg);
        if (!errorMsg.isEmpty()) {
            QMessageBox::warning(this, "Ошибка экспорта", errorMsg);
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Ошибка записи",
                                  QString("Не удалось открыть файл:\n%1\n\nОшибка: %2")
                                      .arg(path, file.errorString()));
            return;
        }

        QTextStream stream(&file);
        // Совместимость Qt5 / Qt6

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
#else
        stream.setCodec("UTF-8");
#endif
        stream << content;
        file.close();
        ok = true;
    }

    // 5. Результат
    if (!ok) {
        if (!errorMsg.isEmpty())
            QMessageBox::critical(this, "Ошибка экспорта", errorMsg);
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Экспорт выполнен");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QString("Файл успешно сохранён:\n<b>%1</b>")
                       .arg(QFileInfo(path).fileName()));
    msgBox.setInformativeText(path);
    msgBox.setStandardButtons(QMessageBox::Ok);
    QPushButton *openBtn = msgBox.addButton("Открыть папку",
                                            QMessageBox::ActionRole);
    msgBox.exec();

    if (msgBox.clickedButton() == openBtn)
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QFileInfo(path).absolutePath()));

}