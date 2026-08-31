// ─────────────────────────────────────────────────────────────────────────
// Бюллетень Метео-11: стандартные высоты и таблицы кодирования, расчёт
// уточнённого и приближённого бюллетеня, вывод строкой и таблицей.
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

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

void MeasurementResults::onFromGribButtonClicked()
{
    if (!ui->pushButton_fromGrib->isEnabled())
        return; // защита от повторного нажатия, пока идёт предыдущий расчёт

    currentButtelinType = FromGrib;
    m_meteo11FromGrib = Meteo11Data(); // сбрасываем, чтобы не показать устаревший результат, пока считается новый
    switchMeteo11Display();
    updateWindShearDisplay();

    // Координаты и время берём из уже загруженной записи (m_currentLatitude/
    // m_currentLongitude/m_currentSondingTime — те же, что использует
    // остальная часть вкладки). Приземный ветер — с реального датчика
    // (m_currentWindDirSurface/m_currentWindSpeedSurface), без ручного ввода.
    if (!m_currentSondingTime.isValid()) {
        m_meteo11FromGrib = Meteo11Data();
        updateMeteo11Display();
        return;
    }

    ui->pushButton_fromGrib->setEnabled(false);
    ui->pushButton_fromGrib->setText("Идёт расчёт...");
    ui->labelGribStatus->setVisible(false);

    m_gribPipeline->run(m_currentLatitude, m_currentLongitude, m_currentSondingTime,
                        m_currentWindSpeedSurface, m_currentWindDirSurface);
}

void MeasurementResults::onGribPipelineFinished(bool success, const QVector<WindProfileData> &profile,
                                                const QString &error)
{
    if (!success) {
        qWarning() << "GRIB Метео-11: ошибка —" << error;
        m_meteo11FromGrib = Meteo11Data();
        if (currentButtelinType == FromGrib)
            updateMeteo11Display();
        return;
    }

    const Meteo11Data *oldBulletin = m_meteo11FromStation.isValid ? &m_meteo11FromStation : nullptr;

    m_meteo11FromGrib = buildMeteo11(profile, m_currentStationAltitude, m_currentPressureMmHg,
                                     m_currentTempC, m_currentSondingTime,
                                     /*useActual=*/true, oldBulletin);

    if (currentButtelinType == FromGrib)
        updateMeteo11Display();
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
    case FromGrib:     d = &m_meteo11FromGrib;     break;
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

    // TTL самого бюллетеня «Из GRIB»: он рассчитывается для конкретного
    // m_currentSondingTime (см. onFromGribButtonClicked), но остаётся в
    // памяти и дальше показывается как есть, если пользователь просто
    // переключится на другую запись (navigateToRecord), не нажимая кнопку
    // "Из GRIB" повторно. Через 12 ч после времени, для которого он
    // реально был посчитан, считаем его устаревшим — как и бюллетень "От МС".
    double gribAgeH  = -1.0;
    bool   gribStale = false;
    if (m_meteo11FromGrib.isValid && m_currentSondingTime.isValid()
            && m_meteo11FromGrib.bulletinTime.isValid()) {
        qint64 ageSec = m_meteo11FromGrib.bulletinTime.secsTo(m_currentSondingTime);
        gribAgeH      = qAbs(ageSec) / 3600.0;
        gribStale     = (gribAgeH > 12.0);
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

        // В style хранится не строка QSS, а имя состояния для селектора
        // QLineEdit#lineEdit_bulleten[state="..."] из applyArchiveStyle():
        // так пилюля не теряет общие правила (радиус, отступы, шрифт).
        const QString kBadgeOk   = "ok";
        const QString kBadgeWarn = "warn";

        if (currentButtelinType == FromMeteoStat && stationAgeH >= 0.0) {
            // "От МС" — показываем давность и годность входящего бюллетеня
            if (stationStale) {
                text  = QString("УСТАРЕЛ (%1 ч)").arg(stationAgeH, 0, 'f', 1);
                style = kBadgeWarn;
            } else {
                text  = QString("ГОДНЫЙ (%1 ч)").arg(stationAgeH, 0, 'f', 1);
                style = kBadgeOk;
            }
        } else if (currentButtelinType == Updated) {
            // "Уточнённый" — бюллетень построен, цвет всегда зелёный
            if (!m_meteo11FromStation.isValid) {
                text  = "ГОДНЫЙ";
                style = kBadgeOk;
            } else if (stationStale) {
                text  = QString("ГОДНЫЙ (МС устарел %1 ч)").arg(stationAgeH, 0, 'f', 0);
                style = kBadgeOk;
            } else {
                text  = "ГОДНЫЙ (с данными МС)";
                style = kBadgeOk;
            }
        } else if (currentButtelinType == FromGrib && gribAgeH >= 0.0) {
            // "Из GRIB" — годен 12 ч с момента расчёта (относительно текущей
            // отображаемой записи); дальше нужен пересчёт по свежим данным.
            if (gribStale) {
                text  = QString("УСТАРЕЛ (%1 ч)").arg(gribAgeH, 0, 'f', 1);
                style = kBadgeWarn;
            } else {
                text  = QString("ГОДНЫЙ (%1 ч)").arg(gribAgeH, 0, 'f', 1);
                style = kBadgeOk;
            }
        } else {
            text  = "ГОДНЫЙ";
            style = kBadgeOk;
        }

        setBulletinBadge(text, style);
    } else {
        setBulletinBadge("НЕТ ДАННЫХ", "bad");
    }

    // Время составления бюллетеня — янтарный фон когда устарел
    ui->lineEdit_bulletenTime->setText(
        d->bulletinTime.isValid()
            ? d->bulletinTime.toString("dd.MM.yyyy hh:mm")
            : "—"
        );
    setWidgetState(ui->lineEdit_bulletenTime,
                   (((currentButtelinType == FromMeteoStat && stationStale) ||
                     (currentButtelinType == FromGrib && gribStale)) && d->isValid)
                       ? "stale" : "");

    // Кнопки: имитируем «нажатое» состояние через стиль
    // Выбранная пилюля отмечается свойством [pressed] — правила для обоих
    // состояний живут в applyArchiveStyle(). Раньше здесь подставлялась
    // полная строка стиля, из-за чего для этих кнопок терялись общие правила
    // (наведение, заблокированное состояние).
    auto setPressed = [](QPushButton *btn, bool pressed) {
        if (!btn) return;
        btn->setProperty("pressed", pressed);
        setWidgetState(btn, btn->property("state").toString()); // перерисовка
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    };
    setPressed(ui->pushButton_updated,       currentButtelinType == Updated);
    setPressed(ui->pushButton_approximate,   currentButtelinType == Approximate);
    setPressed(ui->pushButton_fromMeteoStat, currentButtelinType == FromMeteoStat);
    setPressed(ui->pushButton_fromGrib,      currentButtelinType == FromGrib);

    setPressed(ui->pushButton_string, currentOutputFormat == String);
    setPressed(ui->pushButton_table,  currentOutputFormat == Table);

    // Кнопка «От МС» — янтарный/оранжевый и тултип когда бюллетень устарел
    if (stationStale && m_meteo11FromStation.isValid) {
        setWidgetState(ui->pushButton_fromMeteoStat, "stale");
        ui->pushButton_fromMeteoStat->setToolTip(
            QString("Бюллетень МС устарел на %1 ч (>12 ч) — не используется в уточнённом")
                .arg(stationAgeH, 0, 'f', 1));
    } else {
        setWidgetState(ui->pushButton_fromMeteoStat, "");
        ui->pushButton_fromMeteoStat->setToolTip("");
    }

    // Кнопка «Из GRIB» — тот же янтарный индикатор, когда посчитанный
    // бюллетень старше 12 ч относительно текущей отображаемой записи.
    if (gribStale && m_meteo11FromGrib.isValid) {
        setWidgetState(ui->pushButton_fromGrib, "stale");
        ui->pushButton_fromGrib->setToolTip(
            QString("Бюллетень «Из GRIB» устарел на %1 ч (>12 ч) — пересчитайте для текущей записи")
                .arg(gribAgeH, 0, 'f', 1));
    } else {
        setWidgetState(ui->pushButton_fromGrib, "");
        ui->pushButton_fromGrib->setToolTip("");
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
        fitMeteo11TextHeight();

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
    fitMeteo11TextHeight();
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

    // Ширины колонок задаёт setupArchiveTables() (режим Stretch — таблица
            // занимает всю ширину панели, как в макете); подгонка по содержимому
            // сбрасывала бы этот режим обратно на ручные ширины.

}

/**
 * Сбросить все отображения Метео-11 в пустое состояние.
 */
void MeasurementResults::clearMeteo11Display()
{
    ui->textEdit_meteo11->setHtml(
        "<html><body style=\"font-family:'Tahoma'; font-size:14pt;\">"
        "<p>— нет данных —</p></body></html>");
    fitMeteo11TextHeight();

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
    setBulletinBadge("НЕТ ДАННЫХ", "none");
    setWidgetState(ui->lineEdit_bulletenTime, "");
    ui->lineEdit_bulletenTime->clear();
}
