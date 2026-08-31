// ─────────────────────────────────────────────────────────────────────────
// Плашки состояния датчиков в статус-панели и агрегированный индикатор
// готовности комплекса, включая анимацию прогресса и стрелки РПВ.
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

// Больше не нужны: btnModeWorking/btnModeStandby взаимоисключаются сами
// (checkable + autoExclusive="true" в mainwindow.ui), как раньше исключались
// QRadioButton cbWorkMode/cbStandbyMode.
// void MainWindow::onWorkModeChanged(bool checked) {}
// void MainWindow::onStandbyModeChanged(bool checked) {}

// ==================== Методы обновления статуса датчиков ====================

void MainWindow::updateSensorStatusPanel()
{
    updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
    updateAmsStatusLabel(m_amsHandler && m_amsHandler->isConnected());
    updateBinsStatusLabel(m_binsHandler && m_binsHandler->isConnected());
    updateIwsStatusLabel(m_iwsDeviceActive);
}

void MainWindow::updateGnssStatusLabel(bool connected)
{
    if (connected) {
        ui->lblGnssStatus->setText("GNSS: подключен");
        ui->lblGnssStatus->setStyleSheet(
            "background-color: #E8F5E9; color: #1B5E20; border: 1px solid #A5D6A7; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblGnssStatus->setText("GNSS: отключен");
        ui->lblGnssStatus->setStyleSheet(
            "background-color: #FFEBEE; color: #B71C1C; border: 1px solid #FFCDD2; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    // Ручной ввод положения (широта/долгота/высота) перекрывает обычную
    // зелёную/красную окраску жёлто-янтарной — независимо от того, что
    // передал сюда вызывающий код (connected относится только к самому
    // ГНСС-приёмнику, а не к источнику данных, которые сейчас в полях).
    if (m_gnssManualHighlight) {
        ui->lblGnssStatus->setText("GNSS: ручной ввод");
        ui->lblGnssStatus->setStyleSheet(
            "background-color: #FFF8E1; color: #8a6100; border: 1px solid #FFE082; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    updateConnectAllButtonVisibility();
    updateOverallReadiness();
}

void MainWindow::updateAmsStatusLabel(bool connected)
{
    if (connected) {
        ui->lblAmsStatus->setText("АМС: подключен");
        ui->lblAmsStatus->setStyleSheet(
            "background-color: #E8F5E9; color: #1B5E20; border: 1px solid #A5D6A7; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblAmsStatus->setText("АМС: отключен");
        ui->lblAmsStatus->setStyleSheet(
            "background-color: #FFEBEE; color: #B71C1C; border: 1px solid #FFCDD2; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    updateConnectAllButtonVisibility();
    updateOverallReadiness();
}

void MainWindow::updateBinsStatusLabel(bool connected)
{
    if (connected) {
        ui->lblBinsStatus->setText("БИНС: подключен");
        ui->lblBinsStatus->setStyleSheet(
            "background-color: #E8F5E9; color: #1B5E20; border: 1px solid #A5D6A7; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblBinsStatus->setText("БИНС: отключен");
        ui->lblBinsStatus->setStyleSheet(
            "background-color: #FFEBEE; color: #B71C1C; border: 1px solid #FFCDD2; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    // Ручной ввод ориентации (курс/крен/тангаж) — та же жёлто-янтарная
    // подсветка, независимая от подсветки ГНСС (вариант B).
    if (m_binsManualHighlight) {
        ui->lblBinsStatus->setText("БИНС: ручной ввод");
        ui->lblBinsStatus->setStyleSheet(
            "background-color: #FFF8E1; color: #8a6100; border: 1px solid #FFE082; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    updateConnectAllButtonVisibility();
    updateOverallReadiness();
}

void MainWindow::updateIwsStatusLabel(bool connected)
{
    if (connected) {
        ui->lblIwsStatus->setText("ИВС: подключен");
        ui->lblIwsStatus->setStyleSheet(
            "background-color: #E8F5E9; color: #1B5E20; border: 1px solid #A5D6A7; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblIwsStatus->setText("ИВС: отключен");
        ui->lblIwsStatus->setStyleSheet(
            "background-color: #FFEBEE; color: #B71C1C; border: 1px solid #FFCDD2; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    // Ручной ввод приземных данных (ИВС не подключён, но GroundMeteoParams
    // применил значения вручную через "Применить") — та же подсветка.
    GroundMeteoParams *gmp = GroundMeteoParams::instance();
    if (gmp && gmp->hasLastData() && gmp->lastUpdateWasManual()) {
        ui->lblIwsStatus->setText("ИВС: ручной ввод");
        ui->lblIwsStatus->setStyleSheet(
            "background-color: #FFF8E1; color: #8a6100; border: 1px solid #FFE082; "
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
    updateConnectAllButtonVisibility();
    updateOverallReadiness();
}

void MainWindow::updateConnectAllButtonVisibility()
{
    // Кнопка "Подключить всё" отключена по требованию — теперь при
    // отсутствии подключения датчики подключаются по отдельности кликом по
    // каждому из них (шторка датчика). Логику ниже не удаляю (может
    // понадобиться вернуть в будущем) — просто держу её за флагом.
    static constexpr bool kConnectAllButtonEnabled = false;
    if (!kConnectAllButtonEnabled) {
        ui->btnConnectAll->setVisible(false);
        if (m_connectAllPopup) hideConnectAllPopup();
        return;
    }

    // До первого завершения стартового опроса (успешного, неудачного или
    // вовсе не потребовавшегося) кнопку не показываем ни при каких условиях.
    if (!m_startupSensorCheckDone) {
        ui->btnConnectAll->setVisible(false);
        if (m_connectAllPopup) hideConnectAllPopup();
        return;
    }

    // Пока идёт поиск (полный или одиночный из шторки) — кнопку не
    // показываем, за происходящим уже следит toast.
    if (m_autoConnector && m_autoConnector->isDetecting()) {
        ui->btnConnectAll->setVisible(false);
        if (m_connectAllPopup) hideConnectAllPopup();
        return;
    }

    const bool anyConnected =
        (m_gnssHandler && m_gnssHandler->isConnected()) ||
        (m_amsHandler  && m_amsHandler->isConnected())  ||
        (m_binsHandler && m_binsHandler->isConnected())  ||
        m_iwsDeviceActive;

    // Показываем только когда НИ ОДИН датчик не подключён — это же правило
    // само по себе покрывает и отмену поиска: если к моменту отмены хоть
    // один датчик успел найтись, кнопка остаётся скрытой.
    ui->btnConnectAll->setVisible(!anyConnected);
    // Если кнопка только что скрылась (что-то подключилось), пока была
    // открыта шторка — закрываем и её, чтобы не висела без иконки-якоря.
    if (anyConnected && m_connectAllPopup) hideConnectAllPopup();
}

void MainWindow::onSurfaceStateChanged(GroundMeteoParams::SurfaceState newState)
{
    // Последнее известное состояние приземных данных — используется попапом
    // индикатора даже когда lblStatus сейчас занят текстом "РАБОТА"/"ОШИБКА".
    m_lastKnownSurfaceState = newState;

    // Приземные данные изменились (от ИВС или вручную) — плашка "ИВС" могла
    // поменять источник (жёлтая подсветка "ручной ввод" вкл./выкл.).
    updateIwsStatusLabel(m_iwsDeviceActive);

    // Единая точка агрегированной готовности (ОТКАЗ/ДАННЫЕ УСТАРЕЛИ/ГОТОВ) —
    // см. updateOverallReadiness(). Она же обновляет readinessIndicatorFrame/
    // lblStatus/lblStartReadiness и, если попап открыт, его содержимое.
    updateOverallReadiness();
}

// Собирает все непройденные проверки готовности комплекса в список
// коротких причин на русском — по одной строке на пункт. Пустой список
// означает "всё готово" (не считая возможной "устарелости" приземки,
// которая уже НЕ считается отказом — см. updateOverallReadiness()).
QStringList MainWindow::collectReadinessIssues() const
{
    QStringList issues;

    const bool amsConnected = (m_amsHandler && m_amsHandler->isConnected());
    if (!amsConnected)
        issues << "Нет связи с АМС — проверьте подключение АМС.";

    if (m_lastKnownSurfaceState == GroundMeteoParams::NoData)
        issues << "Нет приземных данных — проверьте подключение ИВС.";

    if (!hasPositionData())
        issues << "Нет положения комплекса — проверьте подключение ГНСС.";

    if (!hasOrientationData())
        issues << "Нет ориентации комплекса — проверьте подключение БИНС.";

    return issues;
}

// Единая точка правды для агрегированного индикатора "ОТКАЗ"/"ГОТОВ"
// (readinessIndicatorFrame/lblStatus, левый верхний угол). Раньше плашка
// отражала ТОЛЬКО GroundMeteoParams::SurfaceState — теперь агрегирует ВСЕ
// датчики и все требуемые данные (приземка/положение/ориентация): если
// чего-то не хватает — единый текст "ОТКАЗ", а конкретная причина(ы) видна
// только в информационном попапе (см. populateReadinessPopupContent).
// "Данные устарели" остаётся отдельным, более мягким состоянием — оно
// показывается только когда ВСЁ остальное в порядке, а приземка именно
// устарела (не отсутствует).
void MainWindow::updateOverallReadiness()
{
    const bool measurementRunning =
        (m_amsHandler && m_amsHandler->getMeasurementStatus() == STATUS_RUNNING);

    const QStringList issues = collectReadinessIssues();
    const bool stale = issues.isEmpty() && (m_lastKnownSurfaceState == GroundMeteoParams::Stale);

    QString text;
    QString style;
    QString statusBarMsg;
    QString pillStyle;   // фон+рамка всей плашки readinessIndicatorFrame
    QString dotColor;    // цвет точки lblReadinessIcon

    if (!issues.isEmpty()) {
        text = "ОТКАЗ";
        style = "color: #C62828; font-weight: bold; font-size: 9pt; background: transparent;";
        statusBarMsg = issues.first();
        pillStyle = "QFrame#readinessIndicatorFrame { background-color: #FFEBEE; border: 1px solid #FFCDD2; border-radius: 16px; }";
        dotColor = "#C62828";
    } else if (stale) {
        text = "ДАННЫЕ УСТАРЕЛИ";
        style = "color: #e65100; font-weight: bold; font-size: 9pt; background: transparent;";
        statusBarMsg = "Приземные данные старше 30 минут — рекомендуется обновить";
        pillStyle = "QFrame#readinessIndicatorFrame { background-color: #FFF3E0; border: 1px solid #FFE0B2; border-radius: 16px; }";
        dotColor = "#E65100";
    } else {
        text = "ГОТОВ";
        style = "color: #2E7D32; font-weight: bold; font-size: 9pt; background: transparent;";
        statusBarMsg = "Система готова";
        pillStyle = "QFrame#readinessIndicatorFrame { background-color: #E8F5E9; border: 1px solid #A5D6A7; border-radius: 16px; }";
        dotColor = "#43A047";
    }

    // Сама плашка (фон+рамка+точка) отражает состояние ВСЕГДА, независимо
    // от того, идёт ли сейчас измерение — в отличие от lblStatus, у
    // которого текст на время измерения занят под "РАБОТА".
    ui->readinessIndicatorFrame->setStyleSheet(pillStyle);
    ui->lblReadinessIcon->setStyleSheet(
        QString("color: %1; font-size: 10pt; background: transparent;").arg(dotColor));

    if (measurementRunning) {
        // Во время измерения lblStatus занят надписью "РАБОТА" — туда не пишем.
        // Только уведомление через статус-бар.
        statusBar()->showMessage(statusBarMsg, 8000);
    } else {
        ui->lblStatus->setText(text);
        ui->lblStatus->setStyleSheet(style);
    }

    // Если попап сейчас открыт — освежаем его содержимое под новое состояние
    // (чтобы не показывать устаревший текст, если что-то поменялось прямо
    // во время просмотра уведомления).
    if (m_readinessPopup && m_readinessPopup->isVisible())
        populateReadinessPopupContent();

    // Плитка "Готов к запуску" на экране "Пуск измерения".
    updateMeasureReadinessLabel();
}

// Единая точка правды и для текста плитки "Готов к запуску" над кнопкой
// "Пуск", и для доступности самой кнопки btnStart. Раньше это были два
// независимых места (btnStart->setEnabled(...) в нескольких обработчиках +
// отдельный текст) — из-за этого подпись и реальная кликабельность кнопки
// могли разойтись (например, кнопка ещё активна, а подпись уже говорит
// "не готов"). Теперь оба решения принимаются здесь и всегда согласованы.
//
// Условия ровно те же, что проверяет onStartClicked() при клике —
// используются как страховка на случай прямого клика в узком окне между
// событиями, а не как единственная защита.
void MainWindow::updateMeasureReadinessLabel()
{
    if (!ui->lblStartReadiness)
        return;

    const bool measurementRunning =
        (m_amsHandler && m_amsHandler->getMeasurementStatus() == STATUS_RUNNING);
    const bool amsConnected = (m_amsHandler && m_amsHandler->isConnected());
    const bool hasGroundData = (m_lastKnownSurfaceState != GroundMeteoParams::NoData);
    const bool hasPosition = hasPositionData();
    const bool hasOrientation = hasOrientationData();

    QString text;
    QString color;
    bool startEnabled;

    if (measurementRunning) {
        text  = "Идёт измерение";
        color = "#1565C0";
        startEnabled = false;
    } else if (!amsConnected) {
        text  = "Не готов к запуску: нет подключения к АМС.\nПроверьте статус АМС вверху экрана.";
        color = "#C62828";
        startEnabled = false;
    } else if (!hasGroundData) {
        text  = "Не готов к запуску: нет приземных данных.\nПроверьте ИВС вверху экрана.";
        color = "#C62828";
        startEnabled = false;
    } else if (!hasPosition) {
        text  = "Не готов к запуску: нет положения комплекса.\nПроверьте ГНСС вверху экрана.";
        color = "#C62828";
        startEnabled = false;
    } else if (!hasOrientation) {
        text  = "Не готов к запуску: нет ориентации комплекса.\nПроверьте БИНС вверху экрана.";
        color = "#C62828";
        startEnabled = false;
    } else if (m_lastKnownSurfaceState == GroundMeteoParams::Stale) {
        text  = "Готов к запуску\n(приземные данные устарели)";
        color = "#E65100";
        startEnabled = true;
    } else {
        text  = "Готов к запуску";
        color = "#2E7D32";
        startEnabled = true;
    }

    ui->lblStartReadiness->setText(text);
    ui->lblStartReadiness->setStyleSheet(
        QString("font-size: 8pt; font-weight: bold; color: %1; border: none; background: transparent;")
            .arg(color));

    if (ui->btnStart)
        ui->btnStart->setEnabled(startEnabled);
}

// Плавно анимирует прогресс-бар измерения к новому значению вместо
// мгновенной установки. Переиспользует один и тот же QPropertyAnimation —
// если предыдущая анимация ещё не закончилась, она просто перенацеливается
// на новое значение с текущей позиции (не дёргается назад к 0).
void MainWindow::animateProgressBarTo(int value)
{
    if (!ui->progressBarMeasurement)
        return;

    if (!m_progressBarAnimation) {
        m_progressBarAnimation = new QPropertyAnimation(ui->progressBarMeasurement, "value", this);
        m_progressBarAnimation->setEasingCurve(QEasingCurve::OutCubic);
    }

    m_progressBarAnimation->stop();
    m_progressBarAnimation->setDuration(350);
    m_progressBarAnimation->setStartValue(ui->progressBarMeasurement->value());
    m_progressBarAnimation->setEndValue(value);
    m_progressBarAnimation->start();
}

// Плавно поворачивает стрелку компаса РПВ к новому углу — кратчайшим путём
// (а не всегда "по часовой": иначе переход, например, 350° → 10° выглядел
// бы как почти полный оборот назад, хотя реально угол изменился всего на
// 20°). RpvIndicator::setAngle не нормализует значение внутри, поэтому
// целевой угол может уйти за пределы 0..360 — это нормально, цифровая
// подпись всё равно нормализуется при отрисовке (см. RpvIndicator.cpp).
void MainWindow::animateRpvAngleTo(double targetAngleDeg)
{
    if (!ui->rpvIndicator)
        return;

    // Кратчайшая разница в диапазоне (-180; 180]
    double delta = std::fmod(targetAngleDeg - m_rpvDisplayedAngle, 360.0);
    if (delta > 180.0)  delta -= 360.0;
    if (delta < -180.0) delta += 360.0;

    const double target = m_rpvDisplayedAngle + delta;

    if (!m_rpvAngleAnimation) {
        m_rpvAngleAnimation = new QPropertyAnimation(ui->rpvIndicator, "angle", this);
        m_rpvAngleAnimation->setEasingCurve(QEasingCurve::OutCubic);
    }

    m_rpvAngleAnimation->stop();
    m_rpvAngleAnimation->setDuration(450);
    m_rpvAngleAnimation->setStartValue(m_rpvDisplayedAngle);
    m_rpvAngleAnimation->setEndValue(target);
    m_rpvAngleAnimation->start();

    m_rpvDisplayedAngle = target;
}

// ─────────────────────────────────────────────────────────────────────────
// Шторка состояния/управления датчиком
// ─────────────────────────────────────────────────────────────────────────

QString MainWindow::sensorDisplayName(AutoConnector::DeviceType type) const
{
    switch (type) {
    case AutoConnector::DEVICE_GNSS: return "GNSS";
    case AutoConnector::DEVICE_AMS:  return "АМС";
    case AutoConnector::DEVICE_BINS: return "БИНС";
    case AutoConnector::DEVICE_IWS:  return "ИВС";
    default: return QString();
    }
}

bool MainWindow::isSensorConnected(AutoConnector::DeviceType type) const
{
    switch (type) {
    case AutoConnector::DEVICE_GNSS: return m_gnssHandler && m_gnssHandler->isConnected();
    case AutoConnector::DEVICE_AMS:  return m_amsHandler && m_amsHandler->isConnected();
    case AutoConnector::DEVICE_BINS: return m_binsHandler && m_binsHandler->isConnected();
    case AutoConnector::DEVICE_IWS:  return m_iwsDeviceActive;
    default: return false;
    }
}

QWidget* MainWindow::sensorIndicatorWidget(AutoConnector::DeviceType type) const
{
    switch (type) {
    case AutoConnector::DEVICE_GNSS: return ui->lblGnssStatus;
    case AutoConnector::DEVICE_AMS:  return ui->lblAmsStatus;
    case AutoConnector::DEVICE_BINS: return ui->lblBinsStatus;
    case AutoConnector::DEVICE_IWS:  return ui->lblIwsStatus;
    default: return nullptr;
    }
}

// Дополнительная информация о датчике, если протокол это позволяет.
// Это "снимок" на момент открытия шторки (или на момент завершения
// поиска, если она открыта в это время) — не обновляется в реальном
// времени, пока шторка просто открыта.
// Форматирует "сколько времени прошло" для отметок последних полученных
// данных в шторке (GNSS/БИНС/ИВС — "вещательные" протоколы).
static QString formatElapsedSince(const QDateTime &at)
{
    if (!at.isValid())
        return QString();
    const qint64 secs = at.secsTo(QDateTime::currentDateTime());
    if (secs < 2)  return "только что";
    if (secs < 60) return QString("%1 сек назад").arg(secs);
    return QString("%1 мин назад").arg(secs / 60);
}

QString MainWindow::sensorExtraInfo(AutoConnector::DeviceType type) const
{
    switch (type) {
    case AutoConnector::DEVICE_GNSS: {
        if (!m_gnssHandler || !m_gnssHandler->hasValidFix())
            return "Фикс ещё не получен";
        GNSSData d = m_gnssHandler->getCurrentData();
        return QString("Тип фикса: %1\nСпутников: %2\nHDOP: %3")
            .arg(d.fixType.isEmpty() ? QString("—") : d.fixType)
            .arg(d.satellites)
            .arg(d.hdop, 0, 'f', 1);
    }
    case AutoConnector::DEVICE_AMS: {
        if (!m_amsHandler) return QString();
        switch (m_amsHandler->getMeasurementStatus()) {
        case STATUS_IDLE:    return "Состояние: ожидание (проверка связи каждые 5 сек)";
        case STATUS_RUNNING: return "Состояние: идёт измерение";
        case STATUS_READY:   return "Состояние: измерение завершено";
        case STATUS_FAILURE: return "Состояние: ошибка измерения";
        }
        return QString();
    }
    case AutoConnector::DEVICE_BINS: {
        if (!m_binsHandler) return QString();
        BINSData d = m_binsHandler->getCurrentData();
        if (!d.valid) return "Данные ещё не получены";
        return QString("Курс: %1°\nКрен: %2°\nТангаж: %3°")
            .arg(d.heading, 0, 'f', 1).arg(d.roll, 0, 'f', 1).arg(d.pitch, 0, 'f', 1);
    }
    case AutoConnector::DEVICE_IWS: {
        GroundMeteoParams *gmp = GroundMeteoParams::instance();
        if (!gmp || !gmp->hasLastData()) return "Данные ещё не получены";
        return QString("Ветер: %1 м/с, %2°")
            .arg(gmp->lastWindSpeed(), 0, 'f', 1)
            .arg(gmp->lastWindDirection(), 0, 'f', 0);
    }
    default: return QString();
    }
}

// "Последние данные: N сек назад" — только для "вещательных" протоколов
// (GNSS/БИНС/ИВС), где это и есть механизм health-check. У АМС health-check
// активный (LINE_TEST), отдельная метка времени ему не так показательна —
// см. sensorExtraInfo для АМС.
QString MainWindow::sensorLastSeenText(AutoConnector::DeviceType type) const
{
    QDateTime at;
    switch (type) {
    case AutoConnector::DEVICE_GNSS: at = m_gnssLastDataAt; break;
    case AutoConnector::DEVICE_BINS: at = m_binsLastDataAt; break;
    case AutoConnector::DEVICE_IWS:  at = m_iwsLastDataAt;  break;
    default: return QString();
    }
    if (!at.isValid()) return QString();
    return "Последние данные: " + formatElapsedSince(at);
}

// Причина проблемы с подключением: реальная последняя ошибка от хендлера,
// если она есть, иначе общий чек-лист.
QString MainWindow::sensorProblemReason(AutoConnector::DeviceType type) const
{
    QString lastError;
    switch (type) {
    case AutoConnector::DEVICE_GNSS: lastError = m_gnssLastError; break;
    case AutoConnector::DEVICE_AMS:  lastError = m_amsLastError;  break;
    case AutoConnector::DEVICE_BINS: lastError = m_binsLastError; break;
    case AutoConnector::DEVICE_IWS:  lastError = m_iwsLastError;  break;
    default: break;
    }

    if (!lastError.isEmpty())
        return "Причина: " + lastError;

    return "Проверьте: кабель подключён, порт и скорость (бод) верны в настройках датчиков.";
}

// Есть ли уже сейчас данные, за которые отвечает этот датчик — неважно,
// пришли они с самого датчика или введены вручную ранее. Используется,
// чтобы решить, показывать ли в шторке кнопку "перейти к заполнению".
bool MainWindow::sensorHasRequiredData(AutoConnector::DeviceType type) const
{
    switch (type) {
    case AutoConnector::DEVICE_IWS: {
        GroundMeteoParams *gmp = GroundMeteoParams::instance();
        return gmp && gmp->surfaceState() != GroundMeteoParams::NoData;
    }
    case AutoConnector::DEVICE_GNSS: return hasPositionData();
    case AutoConnector::DEVICE_BINS: return hasOrientationData();
    default: return true; // АМС и прочее — у них нет понятия "данные"
    }
}

QString MainWindow::sensorManualEntryButtonText(AutoConnector::DeviceType type) const
{
    switch (type) {
    case AutoConnector::DEVICE_IWS:  return "Заполнить вручную";
    case AutoConnector::DEVICE_GNSS: return "Указать положение";
    case AutoConnector::DEVICE_BINS: return "Указать ориентацию";
    default: return QString();
    }
}
