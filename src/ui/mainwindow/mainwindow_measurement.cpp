// ─────────────────────────────────────────────────────────────────────────
// Цикл измерения: пуск и останов, ход измерения АМС, добор приземных
// данных от ИВС, расчёт профиля ветра и запись результата.
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

void MainWindow::requestIwsDataForRecord(int recordId)
{
    qDebug() << "MainWindow: Финальный запрос к ИВС для record_id=" << recordId;

    // Запоминаем record_id — ответ придёт в onSerialDataReceived → GroundMeteoParams::dataUpdated
    m_pendingIwsRecordId = recordId;

    // Инициализируем таймаут (5 секунд)
    if (!m_iwsFinalRequestTimer) {
        m_iwsFinalRequestTimer = new QTimer(this);
        m_iwsFinalRequestTimer->setSingleShot(true);
        connect(m_iwsFinalRequestTimer, &QTimer::timeout, this, [this]() {
            if (m_pendingIwsRecordId > 0) {
                qWarning() << "MainWindow: Таймаут ответа ИВС для record_id=" << m_pendingIwsRecordId
                           << "— surface_meteo не заполнена";
                statusBar()->showMessage("Предупреждение: ИВС не ответил, приземные данные не сохранены", 8000);
                m_pendingIwsRecordId = -1;
            }
        });
    }
    m_iwsFinalRequestTimer->start(5000);

    // Подключаем одноразовый обработчик — сработает на следующий dataUpdated
    // (используем лямбду с Qt::SingleShotConnection чтобы не дублировать)
    GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
    if (meteoParams) {
        // Уже существует — подключаем одноразово
        QMetaObject::Connection *conn = new QMetaObject::Connection();
        *conn = connect(meteoParams, &GroundMeteoParams::dataUpdated,
                        this, [this, conn](const QMap<QString, double> &values) {
                            disconnect(*conn);
                            delete conn;
                            onIwsFinalDataReceived(values);
                        });
    } else {
        // GroundMeteoParams ещё не создан — создаём временный только для парсинга ответа
        // Данные придут через onSerialDataReceived который создаст instance при первом вызове
        // Подключаемся через pollMeteoStation — он уже умеет посылать запрос без instance
        qDebug() << "MainWindow: GroundMeteoParams не создан, используем прямой запрос";
    }

    // Формируем и отправляем запрос напрямую (не через pollMeteoStation чтобы не зависеть от таймера)
    QList<quint16> params;
    if (IWS_PROTOCOL == 0) { // UMB
        params << 0x0064 << 0x00C8 << 0x012C << 0x0190 << 0x01F4;
    } else { // Modbus RTU — средние значения
        params << 13 << 21 << 34 << 45 << 82;
    }

    QByteArray request;
    if (meteoParams) {
        request = (IWS_PROTOCOL == 0)
        ? meteoParams->createUmbReadRequest(params)
        : meteoParams->createModbusReadRequest(params);
    }

    if (!request.isEmpty()) {
        qint64 written = serialPort->write(request);
        qDebug() << "MainWindow: Финальный запрос ИВС отправлен," << written << "байт:" << request.toHex(' ');
    } else {
        qWarning() << "MainWindow: Не удалось сформировать запрос к ИВС";
        m_pendingIwsRecordId = -1;
        m_iwsFinalRequestTimer->stop();
    }
}

void MainWindow::onIwsFinalDataReceived(const QMap<QString, double> &values)
{
    if (m_pendingIwsRecordId <= 0) return;

    m_iwsFinalRequestTimer->stop();
    int recordId = m_pendingIwsRecordId;
    m_pendingIwsRecordId = -1;

    qDebug() << "MainWindow: Получены данные ИВС для финальной записи, record_id=" << recordId;

    m_surfaceMeteoSaver->updateLastValues(values);
    tryFinalizeMeasurement(recordId);
}

// Логика:
//   • Определяем наземный ветер. Приоритет:
//       1) данные железного ИВС (уже в m_surfaceMeteoSaver, если IWS отвечал);
//       2) ручной ввод (GroundMeteoParams::hasLastData()).
//   • Если surface_meteo есть полностью (5 параметров) — сохраняем её в БД.
//   • Если есть хотя бы наземный ВЕТЕР — запускаем расчёт профилей.
//   • Если наземного ветра нет вовсе — расчёт пропускаем (запись измерения
//     при этом уже сохранена: main_archive, координаты, измеренный ветер).

void MainWindow::tryFinalizeMeasurement(int recordId)
{
    // ── 1. Сохраняем surface_meteo, если набор данных полный ────────────────
    // (m_surfaceMeteoSaver мог быть наполнен либо из onIwsFinalDataReceived,
    //  либо — для ручного ввода — наполним его прямо сейчас, см. ниже).
    GroundMeteoParams *gmp = GroundMeteoParams::instance();

    // Если железного ИВС не было, но есть ручной ввод — переносим ручные
    // значения ветра в m_surfaceMeteoSaver, чтобы они тоже могли уйти в БД.
    if (!m_iwsDeviceActive && gmp && gmp->hasLastData()) {
        QMap<QString, double> manualValues;
        manualValues["Wind Speed Avg"]     = gmp->lastWindSpeed();
        manualValues["Wind Direction Avg"] = gmp->lastWindDirection();
        // Температуру/давление/влажность ручной ввод может не содержать —
        // updateLastValues выставит has-флаги только для переданных полей.
        m_surfaceMeteoSaver->updateLastValues(manualValues);
        qDebug() << "MainWindow: применены ручные приземные данные:"
                 << "V=" << gmp->lastWindSpeed()
                 << "dir=" << gmp->lastWindDirection();
    }

    if (m_surfaceMeteoSaver->hasData()) {
        // hasData() == true означает, что заполнены все 5 параметров.
        if (m_surfaceMeteoSaver->saveToDatabase(recordId)) {
            statusBar()->showMessage(
                QString("Приземные данные сохранены (ID: %1)").arg(recordId), 5000);
        }
    } else {
        qDebug() << "MainWindow: surface_meteo неполная — таблица surface_meteo "
                    "пропущена (для расчёта ветров достаточно наземного ветра)";
    }

    // ── 2. Определяем наземный ВЕТЕР для расчёта ─────────────────────────────
    double surfWindSpeed = 0.0;
    double surfWindDir   = 0.0;
    bool   haveSurfWind  = false;

    if (m_surfaceMeteoSaver->windSpeed() > 0.0 || m_surfaceMeteoSaver->windDirection() != 0) {
        // m_surfaceMeteoSaver уже содержит ветер (от IWS либо перенесённый
        // ручной выше). Берём оттуда.
        surfWindSpeed = m_surfaceMeteoSaver->windSpeed();
        surfWindDir   = m_surfaceMeteoSaver->windDirection();
        haveSurfWind  = true;
    } else if (gmp && gmp->hasLastData()) {
        // Резервный путь — напрямую из GroundMeteoParams.
        surfWindSpeed = gmp->lastWindSpeed();
        surfWindDir   = gmp->lastWindDirection();
        haveSurfWind  = true;
    }

    if (!haveSurfWind) {
        qWarning() << "MainWindow: наземный ветер недоступен (ни ИВС, ни ручной ввод)"
                   << "— расчёт профилей ветра пропущен для record_id=" << recordId;
        statusBar()->showMessage(
            "Расчёт ветра пропущен: нет наземного ветра (ИВС/ручной ввод)", 8000);
        return;
    }

    // ── 3. Запускаем расчёт ──────────────────────────────────────────────────
    runWindProfileCalculation(recordId, surfWindSpeed, surfWindDir);
}

void MainWindow::onAmsMeasurementStageChanged(MeasurementStage stage, const QString &description)
{
    Q_UNUSED(stage)
    qDebug() << "MainWindow: Этап измерения:" << description;
    statusBar()->showMessage(description);

    // Можно добавить визуализацию этапов в UI
    // Например, подсветить текущий этап в списке
}

void MainWindow::onAmsMeasurementProgress(int percent, float angle)
{
    qDebug() << "MainWindow: Прогресс измерения:" << percent << "%, угол:" << angle << "°";

    // Показываем виджет прогресса (если ещё не виден)
    if (!ui->measurementProgressWidget->isVisible()) {
        ui->measurementProgressWidget->setVisible(true);
    }

    int displayPercent = qBound(0, percent, 100);
    animateProgressBarTo(displayPercent);
    ui->lblProgressPercent->setText(QString("%1%").arg(displayPercent));
    ui->lblRpvAngle->setText(QString("%1°").arg(angle, 0, 'f', 1));
    // Обновляем индикатор положения РПВ — плавный поворот кратчайшим путём
    animateRpvAngleTo(angle);

    statusBar()->showMessage(
        QString("Измерение: %1%, Угол РПВ: %2°").arg(displayPercent).arg(angle, 0, 'f', 1)
        );
}

void MainWindow::onAmsMeasurementCompleted(int recordId)
{
    qInfo() << "MainWindow: Измерение завершено успешно, ID записи:" << recordId;

    // Сохраняем бюллетень Метео-11 от МС (если оператор ввёл его в «Исходных данных»).
    // Делаем это здесь, а не при нажатии «Пуск», потому что m_currentRecordId в AMSHandler
    // гарантированно валиден в слоте сигнала measurementCompleted (сброс — после emit).
    if (sourceDataInstance && sourceDataInstance->hasMeteo11Bulletin()) {
        qInfo() << "MainWindow: сохраняем бюллетень Метео-11 в БД, record_id=" << recordId;
        const bool saved = m_amsHandler->saveMeteo11Bulletin(
            sourceDataInstance->meteo11BulletinJson(),
            sourceDataInstance->meteo11BulletinTime(),
            sourceDataInstance->meteo11ValidityPeriod()
            );
        if (saved) {
            sourceDataInstance->resetMeteo11Applied();
            statusBar()->showMessage("Метео-11: бюллетень от МС сохранён", 5000);
        } else {
            qWarning() << "MainWindow: не удалось сохранить Метео-11 в БД";
        }
    } else {
        qDebug() << "MainWindow: бюллетень Метео-11 не введён — пропускаем сохранение";
    }

    showNotice(QString("Измерение завершено успешно! ID записи в БД: %1. "
                        "Результаты доступны в разделе «Результаты измерений».")
                   .arg(recordId),
               NotificationToast::Success);

    // Обновляем UI
    ui->lblStatus->setText("ГОТОВ");
    ui->lblStatus->setStyleSheet("color: #2E7D32; font-weight: bold; font-size: 9pt;");

    ui->btnStop->setEnabled(false);

    // Скрываем прогрессбар
    ui->measurementProgressWidget->setVisible(false);
    ui->progressBarMeasurement->setValue(0);

    statusBar()->showMessage("Измерение завершено успешно", 10000);

    // Обновляем плитку готовности к запуску (и доступность btnStart —
    // теперь это её ответственность, см. updateMeasureReadinessLabel)
    updateMeasureReadinessLabel();

    if (m_gnssHandler && m_gnssHandler->isConnected() && m_gnssHandler->hasValidFix()) {
        m_gnssHandler->updateCoordinatesInDb(recordId);
    } else {
        qDebug() << "MainWindow: GNSS недоступен - в БД записываются координаты из UI-полей";
    }
}

void MainWindow::onAmsMeasurementFailed(const QString &reason)
{
    qWarning() << "MainWindow: Измерение не удалось:" << reason;

    showNotice(QString("Измерение не было завершено: %1. Данные о неисправностях сохранены — "
                        "откройте «Функциональный контроль» для просмотра.")
                   .arg(reason),
               NotificationToast::Error);

    // Обновляем UI
    ui->lblStatus->setText("ОШИБКА");
    ui->lblStatus->setStyleSheet("color: #C62828; font-weight: bold; font-size: 9pt;");

    ui->btnStop->setEnabled(false);

    // Перерисовываем по актуальному состоянию приземных данных. Это также
    // вызовет updateMeasureReadinessLabel() (см. конец onSurfaceStateChanged),
    // которая сама решит, включать ли btnStart — с учётом связи с АМС.
    // ЗАМЕЧАНИЕ: это перепишет "ОШИБКА" обратно на "ГОТОВ"/"УСТАРЕЛИ"/"НЕТ ДАННЫХ"
    // — то есть индикация ошибки исчезнет с lblStatus. Сообщение об ошибке
    // оператор уже видел в тосте-уведомлении (showNotice выше в этом же методе,
    // тост держится несколько секунд и снимается тапом), а в
    // statusBar остаётся "Ошибка измерения АМС: ..." на 10 секунд. Если такое
    // поведение нежелательно — можно эту строку НЕ добавлять, тогда "ОШИБКА"
    // на lblStatus останется до следующего события surfaceStateChanged.
    if (GroundMeteoParams *gmp = GroundMeteoParams::instance())
        onSurfaceStateChanged(gmp->surfaceState());

    // Скрываем прогрессбар
    ui->measurementProgressWidget->setVisible(false);
    ui->progressBarMeasurement->setValue(0);

    statusBar()->showMessage("Ошибка измерения АМС: " + reason, 10000);
}

void MainWindow::onAmsNeedIntermediateData(int progress)
{
    qDebug() << "MainWindow: Требуются промежуточные данные на" << progress << "%";

    // --- Дата и время — берём из поля интерфейса (там актуальное значение:
    //     GNSS, ручной ввод или системные часы — в зависимости от режима) ---
    QDateTime currentDateTime = QDateTime::fromString(
        ui->editDateTime->text(), "dd.MM.yyyy hh:mm:ss");
    if (!currentDateTime.isValid()) {
        // Резервный вариант на случай нестандартного формата в поле
        currentDateTime = QDateTime::currentDateTime();
        qWarning() << "MainWindow: Не удалось распарсить время из editDateTime, используем системное";
    }

    int day        = currentDateTime.date().day();
    int hour       = currentDateTime.time().hour();
    int tenMinutes = currentDateTime.time().minute() / 10;

    // --- Высота над уровнем моря из поля положения метеокомплекса ---
    float stationAltitude = ui->editAltitude->text().toFloat();

    // --- Достигнутая высота и профили среднего ветра заполняются нулями ---
    float reachedHeight = 0.0f;
    QVector<float> avgWindDir(23, 0.0f);
    QVector<float> avgWindSpeed(23, 0.0f);

    // --- Приземный ветер из GroundMeteoParams ---
    float surfaceWindDir   = 0.0f;
    float surfaceWindSpeed = 0.0f;

    GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
    if (meteoParams && meteoParams->hasLastData()) {
        surfaceWindDir   = static_cast<float>(meteoParams->lastWindDirection());
        surfaceWindSpeed = static_cast<float>(meteoParams->lastWindSpeed());
        qDebug() << "MainWindow: Приземный ветер из ИВС:"
                 << "направление" << surfaceWindDir << "°,"
                 << "скорость" << surfaceWindSpeed << "м/с";
    } else {
        qWarning() << "MainWindow: Данные ИВС недоступны, приземный ветер = 0";
    }

    // --- Отправляем данные в АМС ---
    bool success = m_amsHandler->sendSourceDataDuringMeasurement(
        day, hour, tenMinutes,
        stationAltitude,
        avgWindDir, avgWindSpeed,
        reachedHeight,
        surfaceWindDir, surfaceWindSpeed,
        currentDateTime
        );

    if (success) {
        qDebug() << "MainWindow: Промежуточные данные отправлены";
        statusBar()->showMessage(
            QString("Промежуточные данные отправлены (прогресс %1%)").arg(progress), 3000);
    } else {
        qWarning() << "MainWindow: Не удалось отправить промежуточные данные";
        showNotice("Не удалось отправить промежуточные данные в АМС.", NotificationToast::Error);
    }
}

void MainWindow::onAmsAvgWindReceived(const QVector<WindProfileData> &data)
{
    qDebug() << "MainWindow: Получен профиль среднего ветра:" << data.size() << "точек";

    // Здесь можно отобразить данные в UI
    // Например, построить график или показать в таблице

    statusBar()->showMessage(
        QString("Получен профиль среднего ветра (%1 уровней)").arg(data.size()),
        3000
        );
}

void MainWindow::onAmsActualWindReceived(const QVector<WindProfileData> &data)
{
    qDebug() << "MainWindow: Получен профиль действительного ветра:" << data.size() << "точек";

    statusBar()->showMessage(
        QString("Получен профиль действительного ветра (%1 уровней)").arg(data.size()),
        3000
        );
}

void MainWindow::onAmsMeasuredWindReceived(const QVector<MeasuredWindData> &data)
{
    qDebug() << "MainWindow: Получен профиль измеренного ветра:" << data.size() << "точек";

    statusBar()->showMessage(
        QString("Получен профиль измеренного ветра (%1 измерений)").arg(data.size()),
        3000
        );
}

void MainWindow::onStartClicked()
{
    // В норме btnStart уже ЗАБЛОКИРОВАНА для всех трёх случаев ниже —
    // см. updateMeasureReadinessLabel(), которая держит кнопку и подпись
    // над ней синхронными. Проверки здесь — страховка на случай гонки
    // состояний, а не основной механизм защиты. Модальных окон больше не
    // показываем: причина и так постоянно видна в подписи над кнопкой.

    // Проверяем подключение к АМС
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        updateMeasureReadinessLabel();
        return;
    }

    // Проверяем, не выполняется ли уже измерение
    if (m_amsHandler->getMeasurementStatus() == STATUS_RUNNING) {
        updateMeasureReadinessLabel();
        return;
    }

    // Страховочная проверка: приземные данные должны быть применены целиком
    // (все 5 строк). По нормальной логике кнопка при NoData отключена
    // (см. updateMeasureReadinessLabel), но мало ли — на всякий случай.
    if (GroundMeteoParams *gmp = GroundMeteoParams::instance()) {
        if (gmp->surfaceState() == GroundMeteoParams::NoData) {
            updateMeasureReadinessLabel();
            return;
        }
    }

    // Страховочная проверка положения/ориентации — по той же логике, что
    // и приземка выше (кнопка и так должна быть заблокирована при их
    // отсутствии, см. updateMeasureReadinessLabel).
    if (!hasPositionData() || !hasOrientationData()) {
        updateMeasureReadinessLabel();
        return;
    }

    // Обновляем UI
    ui->lblStatus->setText("РАБОТА");
    ui->lblStatus->setStyleSheet("color: #1565C0; font-weight: bold; font-size: 9pt;");

    // Получаем параметры для запуска измерения.
    // comboAvgTime/comboLitera/cbWorkMode заменены сегментированными кнопками,
    // но сама логика выбора (индекс/режим) не изменилась — только источник чтения.
    WorkMode mode = ui->btnModeWorking->isChecked() ? MODE_WORKING : MODE_STANDBY;

    // Литера (индекс 0→LITERA_1, 1→LITERA_2, 2→LITERA_3)
    int literaIndex = 0;
    if (ui->btnLitera2->isChecked())      literaIndex = 1;
    else if (ui->btnLitera3->isChecked()) literaIndex = 2;
    Litera litera = static_cast<Litera>(literaIndex);

    // Время усреднения (0→3 мин, 1→6 мин, 2→9 мин)
    AveragingTime avgTime = AVERAGING_3_MIN;
    if (ui->btnAvg6->isChecked())      avgTime = AVERAGING_6_MIN;
    else if (ui->btnAvg9->isChecked()) avgTime = AVERAGING_9_MIN;
    else                                avgTime = AVERAGING_3_MIN;

    // Собираем координаты станции
    StationCoordinates coords;
    bool ok;

    // Широта
    double lat = getCoordField(ui->editLatitude, ok);
    if (!ok) lat = 0.0;
    coords.latitude = static_cast<int>(lat * 3600.0);

    // Долгота
    double lon = getCoordField(ui->editLongitude, ok);
    if (!ok) lon = 0.0;
    coords.longitude = static_cast<int>(lon * 3600.0);

    // Высота
    coords.altitude = ui->editAltitude->text().toFloat();

    // Углы ориентации
    coords.azimuth = ui->editDirectionAngle->text().toFloat();
    coords.pitch = ui->editPitchAngle->text().toFloat();
    coords.roll = ui->editRollAngle->text().toFloat();

    // Дата и время
    QDateTime dateTime;
    if (m_useManualDateTime && m_manualDateTimeSet) {
        dateTime = m_manualDateTime;
    } else {
        dateTime = QDateTime::currentDateTime();
    }

    // Запускаем полную последовательность измерения
    qDebug() << "MainWindow: Запуск измерения АМС";
    qDebug() << "  Режим:" << (mode == MODE_WORKING ? "РАБОЧИЙ" : "ДЕЖУРНЫЙ");
    qDebug() << "  Координаты:" << lat << lon << coords.altitude;
    qDebug() << "  Углы:" << coords.azimuth << coords.pitch << coords.roll;

    bool success = m_amsHandler->startMeasurementSequence(mode, avgTime, litera, coords, dateTime);

    if (!success) {
        showNotice("Не удалось запустить измерение АМС. Проверьте подключение.", NotificationToast::Error);

        // Возвращаем статус в ГОТОВ
        ui->lblStatus->setText("ГОТОВ");
        ui->lblStatus->setStyleSheet("color: #2E7D32; font-weight: bold; font-size: 9pt;");
        updateMeasureReadinessLabel();
        return;
    }

    // Блокируем кнопку старта, разблокируем стоп
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);

    // Сбрасываем и показываем прогрессбар
    if (m_progressBarAnimation) m_progressBarAnimation->stop();
    ui->progressBarMeasurement->setValue(0);
    ui->lblProgressPercent->setText("0%");
    ui->lblRpvAngle->setText("0.0°");
    // Мгновенный (без анимации) сброс стрелки — это НОВОЕ измерение, а не
    // продолжение предыдущего: анимировать долгий поворот от угла, на
    // котором закончилось прошлое измерение, сюда не нужно.
    if (m_rpvAngleAnimation) m_rpvAngleAnimation->stop();
    m_rpvDisplayedAngle = 0.0;
    ui->rpvIndicator->setAngle(0.0);
    ui->measurementProgressWidget->setVisible(true);

    statusBar()->showMessage("Измерение АМС запущено...", 5000);

    // Плитка готовности теперь должна отражать, что измерение выполняется
    updateMeasureReadinessLabel();
}

void MainWindow::onStopClicked()
{
    // Если измерение фактически не идёт — кнопка и так должна быть
    // заблокирована (см. onStartClicked/onSurfaceStateChanged), но на всякий
    // случай подтверждение не показываем и ничего не делаем.
    if (!m_amsHandler || m_amsHandler->getMeasurementStatus() != STATUS_RUNNING)
        return;

    // Подтверждение остановки — та же карточка и тот же паттерн, что и при
    // остановке поиска датчиков (onToastCloseClicked), чтобы не было ложных
    // срабатываний от случайного нажатия.
    showConfirmOverlay(
        "Остановить измерение?",
        "Текущие данные измерения будут потеряны,\nизмерение придётся запускать заново.",
        [this]() {
            // Останавливаем измерение АМС
            bool stopped = m_amsHandler->stopMeasurement();

            if (stopped) {
                statusBar()->showMessage("Измерение АМС остановлено", 3000);
            } else {
                showNotice("Не удалось корректно остановить измерение АМС.", NotificationToast::Error);
            }

            // Обновляем UI
            ui->lblStatus->setText("ГОТОВ");
            ui->lblStatus->setStyleSheet("color: #2E7D32; font-weight: bold; font-size: 9pt;");

            // Разблокируем кнопку старта, блокируем стоп
            ui->btnStart->setEnabled(true);
            ui->btnStop->setEnabled(false);

            // Измерение завершено — перерисовываем lblStatus и доступность btnStart
            // через агрегированную готовность (updateOverallReadiness, вызывается
            // изнутри onSurfaceStateChanged). Если за время измерения приземка
            // успела устареть — увидим "ДАННЫЕ УСТАРЕЛИ" (пуск следующего
            // измерения при этом остаётся разрешённым). Если оператор тем временем
            // нажал "Очистить", или пропало положение/ориентация — увидим "ОТКАЗ"
            // и кнопка снова заблокируется.
            if (GroundMeteoParams *gmp = GroundMeteoParams::instance())
                onSurfaceStateChanged(gmp->surfaceState());

            // Скрываем прогрессбар
            ui->measurementProgressWidget->setVisible(false);
            ui->progressBarMeasurement->setValue(0);

            // Обновляем плитку готовности к запуску на экране "Пуск измерения"
            updateMeasureReadinessLabel();
        },
        "Да, остановить", "Нет, продолжить");
}

void MainWindow::runWindProfileCalculation(int recordId,
                                           double surfaceWindSpeed,
                                           double surfaceWindDirection)
{
    qInfo() << "MainWindow: Запуск расчёта профилей ветра для record_id=" << recordId
            << "| наземный ветер: V=" << surfaceWindSpeed
            << "dir=" << surfaceWindDirection;

    if (!m_windProfileCalculator) {
        qCritical() << "MainWindow: m_windProfileCalculator == nullptr";
        return;
    }
    if (!m_amsHandler) {
        qCritical() << "MainWindow: m_amsHandler == nullptr";
        return;
    }
    if (!DatabaseManager::instance()->connect()) {
        qCritical() << "MainWindow: нет подключения к БД для расчёта профиля";
        return;
    }

    QSqlDatabase db = DatabaseManager::instance()->database();

    // ── 1. Измеренный ветер из БД (сохранён обработчиком 0xAC) ───────────────
    QVector<MeasuredWindData> measuredWind;
    {
        QSqlQuery refQuery(db);
        refQuery.prepare(
            "SELECT measured_wind_profile_id FROM wind_profiles_references "
            "WHERE record_id = :rid"
            );
        refQuery.bindValue(":rid", recordId);

        if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
            qWarning() << "MainWindow: нет measured_wind_profile_id для record_id="
                       << recordId << "— расчёт невозможен";
            statusBar()->showMessage(
                "Расчёт ветра пропущен: нет данных измеренного ветра", 8000);
            return;
        }

        int measuredProfileId = refQuery.value(0).toInt();

        QSqlQuery q(db);
        q.prepare(
            "SELECT height, wind_speed, wind_direction, reliability "
            "FROM measured_wind_profile WHERE profile_id = :pid ORDER BY profile_id"
            );
        q.bindValue(":pid", measuredProfileId);
        if (!q.exec()) {
            qWarning() << "MainWindow: ошибка чтения measured_wind_profile:"
                       << q.lastError().text();
            return;
        }
        while (q.next()) {
            MeasuredWindData pt;
            pt.height        = q.value(0).toFloat();
            pt.windSpeed     = q.value(1).toFloat();
            pt.windDirection = q.value(2).toFloat();
            // Достоверность от АМС (1 - достоверная, 0 - недостоверная).
            // Калькулятор сам отфильтрует недостоверные точки.
            pt.reliability   = q.value(3).toInt();
            measuredWind.append(pt);
        }
    }

    if (measuredWind.isEmpty()) {
        qWarning() << "MainWindow: measured_wind_profile пуст для record_id=" << recordId;
        statusBar()->showMessage("Расчёт ветра пропущен: измеренный ветер пуст", 8000);
        return;
    }

    qDebug() << "MainWindow: для расчёта прочитано" << measuredWind.size()
             << "точек измеренного ветра";

    // ── 2. Координаты и высота станции ───────────────────────────────────────
    double latitudeDeg  = 0.0;
    double longitudeDeg = 0.0;
    float  altitudeM    = 0.0f;
    {
        QSqlQuery q(db);
        q.prepare(
            "SELECT latitude, longitude, altitude FROM station_coordinates "
            "WHERE record_id = :rid"
            );
        q.bindValue(":rid", recordId);
        if (q.exec() && q.next()) {
            latitudeDeg  = q.value(0).toDouble();
            longitudeDeg = q.value(1).toDouble();
            altitudeM    = q.value(2).toFloat();
        } else {
            qWarning() << "MainWindow: нет station_coordinates для record_id="
                       << recordId << "— берём текущие из UI";
            bool ok1, ok2;
            latitudeDeg  = getCoordField(ui->editLatitude,  ok1);
            longitudeDeg = getCoordField(ui->editLongitude, ok2);
            altitudeM    = ui->editAltitude->text().toFloat();
        }
    }

    // ── 3. Время зондирования ────────────────────────────────────────────────
    QDateTime sondingTime = QDateTime::fromString(
        ui->editDateTime->text(), QStringLiteral("dd.MM.yyyy hh:mm:ss"));
    if (!sondingTime.isValid())
        sondingTime = QDateTime::currentDateTime();

    // ── 4. Заполняем Input и запускаем расчёт ────────────────────────────────
    WindProfileCalculator::Input in;
    in.measuredWind       = measuredWind;
    in.latitudeDeg        = latitudeDeg;
    in.longitudeDeg       = longitudeDeg;
    in.stationAltitudeM   = altitudeM;
    in.surfaceWindSpeedMs = static_cast<float>(surfaceWindSpeed);
    in.surfaceWindDirDeg  = static_cast<float>(surfaceWindDirection);
    in.groundWindHeightM  = 8.0f;
    in.z0                 = 0.0f;
    in.sondingTime        = sondingTime;

    qDebug() << "MainWindow: Параметры расчёта: lat=" << latitudeDeg
             << "lon=" << longitudeDeg << "alt=" << altitudeM
             << "surfaceWind=" << surfaceWindSpeed << "м/с" << surfaceWindDirection << "°";

    WindProfileCalculator::Output out;
    auto t0 = QDateTime::currentDateTime();
    WindProfileCalculator::Result res = m_windProfileCalculator->calculate(in, out);
    qDebug() << "MainWindow: Расчёт занял"
             << t0.msecsTo(QDateTime::currentDateTime()) << "мс";

    if (res != WindProfileCalculator::OK) {
        qWarning() << "MainWindow: Расчёт профилей не выполнен —"
                   << WindProfileCalculator::resultString(res);
        statusBar()->showMessage(
            QString("Расчёт профилей не выполнен: %1")
                .arg(WindProfileCalculator::resultString(res)), 10000);
        return;
    }

    // ── 5. Убираем старые рассчитанные профили (на случай повторного расчёта) ─
    m_amsHandler->deleteCalculatedWindProfiles(recordId);

    // ── 6. Сохраняем рассчитанные профили в БД ───────────────────────────────
    const bool okAvg    = m_amsHandler->saveAvgWindProfile(recordId, out.avgWind);
    const bool okActual = m_amsHandler->saveActualWindProfile(recordId, out.actualWind);

    if (okAvg && okActual) {
        qInfo() << "MainWindow: Рассчитанные профили сохранены в БД для record_id="
                << recordId;
        statusBar()->showMessage(
            QString("Расчёт профилей ветра завершён (ID: %1)").arg(recordId), 5000);
    } else {
        qWarning() << "MainWindow: Ошибка сохранения профилей: avg=" << okAvg
                   << "actual=" << okActual;
        statusBar()->showMessage("Ошибка сохранения рассчитанных профилей", 8000);
    }
}

void MainWindow::runPlowSelfTest()
{
    qInfo() << "════════════════════════════════════════════════════════════";
    qInfo() << "  САМОТЕСТ РАСЧЁТА ВЕТРА (ФЕЙКОВЫЙ измеренный профиль, 320 точек)";
    qInfo() << "════════════════════════════════════════════════════════════";

    if (!m_windProfileCalculator) {
        qWarning() << "[SelfTest] m_windProfileCalculator == nullptr — пропуск";
        return;
    }

    // ── Координаты станции (можно заменить на реальные) ──────────────────────
    const double kLat = 55.75;     // широта, град
    const double kLon = 37.62;     // долгота, град
    const float  kAlt = 150.0f;    // высота над уровнем моря, м

    // ── Приземный ветер — ЖЁСТКО ─────────────────────────────────────────────
    const float surfaceWindSpeed = 13.0f;
    const float surfaceWindDir   = 351.0f;

    // ── Генерация фейкового измеренного профиля (320 точек) ──────────────────
    // Физичная модель:
    //   • высота h: 25, 50, 75, ... 8000 (шаг 25 м, 320 уровней);
    //   • скорость: логарифмический рост у земли + линейный выше
    //     V(h) = 5 + 0.003*h  (на 8000 м ≈ 29 м/с) — типичный сдвиг;
    //   • направление: старт 351°, плавный правый поворот с высотой
    //     dir(h) = 351 + 0.004*h градусов (через 360 заворачиваем).
    QVector<MeasuredWindData> measured;
    const int N = 320;
    measured.reserve(N);
    for (int i = 0; i < N; ++i) {
        const float h = 25.0f * (i + 1);            // 25 … 8000 м

        MeasuredWindData m;
        m.height        = h;
        m.windSpeed     = 5.0f + 0.003f * h;        // 5 … ~29 м/с
        float dir       = 351.0f + 0.004f * h;      // плавный поворот
        while (dir >= 360.0f) dir -= 360.0f;
        m.windDirection = dir;
        m.reliability   = 1;                         // достоверная (под фильтр ==1)
        measured.append(m);
    }

    qInfo() << "[SelfTest] сгенерировано фейковых точек:" << measured.size()
            << "| диапазон высот: 25 .. " << (25 * N) << "м";
    qInfo() << "[SelfTest] пример точек:";
    for (int i : {0, 39, 79, 159, 319}) {
        if (i < measured.size())
            qInfo("    h=%7.1f  V=%6.2f  dir=%6.2f  rel=%d",
                  measured[i].height, measured[i].windSpeed,
                  measured[i].windDirection, measured[i].reliability);
    }

    // ── Собираем Input ───────────────────────────────────────────────────────
    WindProfileCalculator::Input in;
    in.measuredWind       = measured;
    in.latitudeDeg        = kLat;
    in.longitudeDeg       = kLon;
    in.stationAltitudeM   = kAlt;
    in.surfaceWindSpeedMs = surfaceWindSpeed;
    in.surfaceWindDirDeg  = surfaceWindDir;
    in.groundWindHeightM  = 8.0f;
    in.z0                 = 0.01f;
    in.sondingTime        = QDateTime::currentDateTime();

    qInfo() << "[SelfTest] координаты: lat=" << kLat << "lon=" << kLon << "alt=" << kAlt;
    qInfo() << "[SelfTest] приземный ветер: V=" << surfaceWindSpeed << "dir=" << surfaceWindDir;
    qInfo() << "[SelfTest] ─── запуск calculate() ─── (ниже лог библиотеки plow)";

    WindProfileCalculator::Output out;
    WindProfileCalculator::Result r = m_windProfileCalculator->calculate(in, out);

    qInfo() << "[SelfTest] результат:" << WindProfileCalculator::resultString(r);
    if (r != WindProfileCalculator::OK) {
        qWarning() << "[SelfTest] расчёт не выполнен";
        return;
    }

    // ── Печать результата с полной дробной частью ────────────────────────────
    qInfo() << "[SelfTest] ─── ДЕЙСТВИТЕЛЬНЫЙ ветер ───";
    for (int i = 0; i < out.actualWind.size(); ++i) {
        const WindProfileData &p = out.actualWind[i];
        qInfo("  #%2d  h=%8.2f  V=%15.6f  dir=%15.6f  valid=%d",
              i, p.height, p.windSpeed, p.windDirection, int(p.isValid));
    }

    qInfo() << "[SelfTest] ─── СРЕДНИЙ ветер ───";
    for (int i = 0; i < out.avgWind.size(); ++i) {
        const WindProfileData &p = out.avgWind[i];
        qInfo("  #%2d  h=%8.2f  V=%15.6f  dir=%15.6f  valid=%d",
              i, p.height, p.windSpeed, p.windDirection, int(p.isValid));
    }

    qInfo() << "════════════════════════════════════════════════════════════";
    qInfo() << "  САМОТЕСТ ЗАВЕРШЁН (фейковые данные)";
    qInfo() << "════════════════════════════════════════════════════════════";
}
