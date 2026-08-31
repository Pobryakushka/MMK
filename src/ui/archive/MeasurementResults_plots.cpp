// ─────────────────────────────────────────────────────────────────────────
// Графики Qwt: профили ветра, измеренный ветер, сдвиг ветра и таблица
// сдвига, а также настройка масштабирования.
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

// ===== ОТОБРАЖЕНИЕ ДАННЫХ =====

void MeasurementResults::displayWindProfile(const QVector<WindProfileData> &avgWind,
                                            const QVector<WindProfileData> &actualWind,
                                            const QVector<MeasuredWindData> &measuredWind)
{
    // Высота теперь обычная первая колонка (а не заголовок строки) — таблица
    // читается как в макете: Высота | Скорость | Направление.
    auto fillRow = [](QTableWidget *t, int row, const QString &h,
                      const QString &speed, const QString &dir) {
        t->setItem(row, 0, new QTableWidgetItem(h));
        t->setItem(row, 1, new QTableWidgetItem(speed));
        t->setItem(row, 2, new QTableWidgetItem(dir));
    };

    ui->tableWidget_AverageWind->setRowCount(avgWind.size());
    for (int i = 0; i < avgWind.size(); i++) {
        fillRow(ui->tableWidget_AverageWind, i,
                QString::number(qRound(avgWind[i].height)),
                QString::number(avgWind[i].windSpeed, 'f', 2),
                QString::number(avgWind[i].windDirection));
    }

    ui->tableWidget_realWind->setRowCount(actualWind.size());
    for (int i = 0; i < actualWind.size(); i++) {
        fillRow(ui->tableWidget_realWind, i,
                QString::number(qRound(actualWind[i].height)),
                QString::number(actualWind[i].windSpeed, 'f', 2),
                QString::number(actualWind[i].windDirection));
    }

    // Высота измеренного ветра приходит от АМС и хранится в БД
    ui->tableWidget_izmWind_2->setRowCount(measuredWind.size());
    for (int i = 0; i < measuredWind.size(); i++) {
        fillRow(ui->tableWidget_izmWind_2, i,
                QString::number(measuredWind[i].height, 'f', 0),
                QString::number(measuredWind[i].windSpeed, 'f', 2),
                QString::number(measuredWind[i].windDirection));
    }

    // Строим графики (используют height из структур данных)
    // Цвет кодирует величину, а не вкладку (как в макете): скорость всегда
    // зелёная, направление — янтарное. Какой это ветер, видно по вкладке.
    plotWindSpeed(ui->plot_midWindSpeed, avgWind, "Средний ветер", archiveSpeedColor());
    plotWindDirection(ui->plot_midWindAzimut, avgWind, "Средний ветер", archiveDirectionColor());

    plotWindSpeed(ui->plot_realWindSpeed, actualWind, "Действительный ветер", archiveSpeedColor());
    plotWindDirection(ui->plot_realWindAzimut, actualWind, "Действительный ветер", archiveDirectionColor());

    plotMeasuredWindSpeed(ui->plot_izmWindSpeed_2, measuredWind, "Измеренный ветер", archiveSpeedColor());
    plotMeasuredWindDirection(ui->plot_izmWindAzimut_2, measuredWind, "Измеренный ветер", archiveDirectionColor());
}

void MeasurementResults::setupPlots()
{
    // Подписи осей не задаются: название графика вынесено в зелёную шапку
    // карточки, как в макете, а слева/снизу остаются только цифры шкал.
    auto setupPlot = [](QwtPlot *plot, double xMin, double xMax, double xStep) {
        if (!plot) return;
        styleArchivePlot(plot);
        plot->setAxisScale(QwtPlot::yLeft, 0.0, 4000.0);
        plot->setAxisScale(QwtPlot::xBottom, xMin, xMax, xStep);
        makeArchiveGrid()->attach(plot);
    };

    // Настройка графиков скорости
    setupPlot(ui->plot_midWindSpeed,   0, 50, 10);
    setupPlot(ui->plot_realWindSpeed,  0, 50, 10);
    setupPlot(ui->plot_izmWindSpeed_2, 0, 50, 10);

    // Настройка графиков направления
    setupPlot(ui->plot_midWindAzimut,   0, 360, 60);
    setupPlot(ui->plot_realWindAzimut,  0, 360, 60);
    setupPlot(ui->plot_izmWindAzimut_2, 0, 360, 60);
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
    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    QVector<double> heights, speeds;
    double maxSpeed = 0, maxHeight = 0;

    for (const WindProfileData &point : data) {
        if (point.isValid && point.windSpeed < 900.f) { // 999 = нет данных (sentinel)
            heights.append(point.height);
            speeds.append(point.windSpeed);
            maxSpeed  = qMax(maxSpeed,  (double)point.windSpeed);
            maxHeight = qMax(maxHeight, (double)point.height);
        }
    }

    if (heights.isEmpty()) { plot->replot(); return; }

    QwtPlotCurve *curve = new QwtPlotCurve(title);
    curve->setSamples(speeds, heights);
    styleArchiveCurve(curve, color);
    curve->attach(plot);

    // Динамические оси по данным
    double xMax = (maxSpeed < 1.0) ? 10.0 : maxSpeed * 1.15;
    double yMax = (maxHeight < 100.0) ? 1000.0 : maxHeight * 1.05;
    plot->setAxisScale(QwtPlot::xBottom, 0.0, xMax);
    plot->setAxisScale(QwtPlot::yLeft,   0.0, yMax);
    plot->replot();

}

void MeasurementResults::plotWindDirection(QwtPlot *plot, const QVector<WindProfileData> &data,
                                           const QString &title, const QColor &color)
{
    if (!plot || data.isEmpty()) return;

    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    QVector<double> heights, directions;

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
    styleArchiveCurve(curve, color);
    curve->attach(plot);

    plot->replot();
}

void MeasurementResults::plotMeasuredWindSpeed(QwtPlot *plot, const QVector<MeasuredWindData> &data,
                                               const QString &title, const QColor &color)
{
    if (!plot || data.isEmpty()) return;
    plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

    QVector<double> heights, speeds;
    double maxSpeed = 0, maxHeight = 0;

    for (const MeasuredWindData &point : data) {
        heights.append(point.height);
        speeds.append(point.windSpeed);
        maxSpeed  = qMax(maxSpeed,  (double)point.windSpeed);
        maxHeight = qMax(maxHeight, (double)point.height);
    }

    QwtPlotCurve *curve = new QwtPlotCurve(title);
    curve->setSamples(speeds, heights);
    styleArchiveCurve(curve, color);
    curve->attach(plot);

    double xMax = (maxSpeed < 0.01) ? 1.0 : maxSpeed * 1.15;
    double yMax = (maxHeight < 100.0) ? 1000.0 : maxHeight * 1.05;
    plot->setAxisScale(QwtPlot::xBottom, 0.0, xMax);
    plot->setAxisScale(QwtPlot::yLeft,   0.0, yMax);
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
    styleArchiveCurve(curve, color);

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
    // Заголовок и легенда убраны: название графика теперь в зелёной шапке
    // карточки (label_shearSpeed), а кривая на графике одна — легенда только
    // отнимала бы место и рисовалась системным стилем.
    styleArchivePlot(ui->plot_windShearSpeed);

    m_windShearGrid = makeArchiveGrid();
    m_windShearGrid->attach(ui->plot_windShearSpeed);

    m_windShearCurve = new QwtPlotCurve(QString::fromUtf8("Сдвиг ветра"));
    styleArchiveCurve(m_windShearCurve, archiveSpeedColor());
    m_windShearCurve->attach(ui->plot_windShearSpeed);

    qDebug() << "setupWindShearTab: график скорости настроен";

    // ===== НАСТРОЙКА ГРАФИКА НАПРАВЛЕНИЯ =====
    styleArchivePlot(ui->plot_windShearDirection);

    makeArchiveGrid()->attach(ui->plot_windShearDirection);

    QwtPlotCurve *curveDirection = new QwtPlotCurve(QString::fromUtf8("Изменение направления"));
    styleArchiveCurve(curveDirection, archiveDirectionColor());
    curveDirection->attach(ui->plot_windShearDirection);

    qDebug() << "setupWindShearTab: график направления настроен";

    // ===== НАСТРОЙКА ТАБЛИЦЫ =====
    // Остальные свойства таблицы (растяжение колонок, чередование строк,
    // скрытый вертикальный заголовок) задаёт setupArchiveTables().

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
        heightItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->table_windShear->setItem(i, 0, heightItem);

        // Колонка 1: Скорость сдвига с цветовой индикацией
        QTableWidgetItem *speedItem = new QTableWidgetItem(
            QString::number(shear.shearPer30m, 'f', 2)
            );
        speedItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Цвет фона по критичности
        QColor bgColor = WindShearCalculator::getSeverityColor(shear.severityLevel);
        speedItem->setBackground(QBrush(bgColor));

        ui->table_windShear->setItem(i, 1, speedItem);

        // Колонка 2: Изменение направления
        QTableWidgetItem *directionItem = new QTableWidgetItem(
            QString::number(shear.shearDirection, 'f', 1)
            );
        directionItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->table_windShear->setItem(i, 2, directionItem);

        // Колонка 3: Текстовое описание уровня
        QTableWidgetItem *levelItem = new QTableWidgetItem(
            WindShearCalculator::getSeverityText(shear.severityLevel)
            );
        levelItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
