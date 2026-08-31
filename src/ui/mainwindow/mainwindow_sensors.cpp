// ─────────────────────────────────────────────────────────────────────────
// Подключение и обслуживание датчиков: ГНСС, АМС, БИНС, ИВС, автопоиск
// портов и health-check уже подключённых.
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

void MainWindow::setupGnssCheckbox()
{
    // Чекбокс теперь в UI файле, просто подключаем сигнал
    connect(ui->checkboxGnss, &QCheckBox::toggled, this, &MainWindow::onGnssCheckboxToggled);
    // Дубликат-чекбокс на странице "Положение" — не подключаем к реальному
    // обработчику напрямую (двойной вызов connectToGnss/disconnectFromGnss),
    // а просто ПЕРЕСЫЛАЕМ клик на настоящий checkboxGnss; настоящая логика
    // отработает через его собственный toggled, а состояние обоих потом
    // выравнивает syncGnssPosCheckbox() в конце onGnssCheckboxToggled.
    connect(ui->checkboxGnssPos, &QCheckBox::toggled, this, [this](bool checked) {
        if (ui->checkboxGnss->isChecked() != checked)
            ui->checkboxGnss->setChecked(checked);
    });
}

//void MainWindow::setupGnssSettingsButton()
//{
//    m_btnGnssSettings = new QPushButton("⚙", this);
//    m_btnGnssSettings->setFixedSize(30, 30);
//    m_btnGnssSettings->setToolTip("Настройки GNSS");
//    m_btnGnssSettings->setStyleSheet(
//                "QPushButton {"
//                "   background-color: white;"
//                "   border: 2px solid gray;"
//                "   border-radius: 15px;"
//                "   font-size: 16px;"
//                "}"
//                "QPushButton:hover {"
//                "   background-color: #f0f0f0;"
//                "}");

//    m_btnGnssSettings->move(width() - 210, 23);
//    m_btnGnssSettings->raise();

//    connect(m_btnGnssSettings, &QPushButton::clicked, this,
//            &MainWindow::onGnssSettingsClicked);
//}

//void MainWindow::onGnssSettingsClicked()
//{
//    if (!sensorSettingsDialog) return;

//    sensorSettingsDialog->findChild<QTabWidget*>("tabWidget")->setCurrentIndex(1);

//    if (m_gnssReceiver->isConnected()) {
//        sensorSettingsDialog->setConnectionStatus("Подключено", true);
//        sensorSettingsDialog->setConnectionEnabled(false);
//    } else {
//        sensorSettingsDialog->setConnectionStatus("Отключено", false);
//        sensorSettingsDialog->setConnectionEnabled(true);
//    }

//    sensorSettingsDialog->show();
//    sensorSettingsDialog->raise();
//    sensorSettingsDialog->activateWindow();
////    SensorSettings dialog(this);

////    connect(&dialog, &SensorSettings::connectRequested, this, [this, &dialog]() {
////        qDebug() << "MainWindow: Запрос на подключение GNSS из настроек";
////        m_gnssComPort = dialog.getIwsComPort();
////        m_gnssBaudRate = dialog.getIwsBaudRate();

////        if (m_gnssReceiver->connectToReceiver(m_gnssComPort, m_gnssBaudRate)) {
////                dialog.setConnectionStatus("Подключено", true);
////                dialog.setConnectionEnabled(false);
////                m_gnssEnabled = true;
////                m_checkboxGnss->setChecked(true);
////            } else {
////                dialog.setConnectionStatus("Ошибка подключения", false);
////            }
////    });

////    connect(&dialog, &SensorSettings::disconnectRequested, this, [this, &dialog]() {
////         qDebug() << "MainWindow: Запрос на отключение GNSS из настроек";
////         disconnectFromGnss();
////         dialog.setConnectionStatus("Отключено", false);
////         dialog.setConnectionEnabled(true);
////    });

////    if (m_gnssReceiver->isConnected()) {
////        dialog.setConnectionStatus("Подключено", true);
////        dialog.setConnectionEnabled(false);
////    }

////    dialog.exec();
//}

void MainWindow::onGnssConnectFromSettings()
{
    if (!sensorSettingsDialog) return;

    m_gnssComPort = sensorSettingsDialog->getGnssComPort();
    m_gnssBaudRate = sensorSettingsDialog->getGnssBaudRate();

    if (m_gnssHandler->connectToGnss(m_gnssComPort, m_gnssBaudRate)){
        sensorSettingsDialog->setGnssConnectionStatus("Подключено", true);
        sensorSettingsDialog->setGnssConnectionEnabled(false);
        m_gnssEnabled = true;
        ui->checkboxGnss->setChecked(true);
    } else {
        sensorSettingsDialog->setGnssConnectionStatus("Ошибка подключения", false);
    }
}

void MainWindow::onGnssDisconnectFromSettings()
{
    disconnectFromGnss();
    if (sensorSettingsDialog){
        sensorSettingsDialog->setGnssConnectionStatus("Отключено", false);
        sensorSettingsDialog->setGnssConnectionEnabled(true);
    }
}

void MainWindow::onGnssCheckboxToggled(bool checked)
{
    if (checked) {
        if (m_gnssComPort.isEmpty()) {
            qDebug() << "MainWindow: COM-порт не настроен, подключение через индикатор GNSS...";
            ui->checkboxGnss->setChecked(false);
            syncGnssPosCheckbox();
            showNotice("GNSS не подключён. Нажмите на индикатор GNSS в панели статуса, "
                       "чтобы найти и подключить приёмник.",
                       NotificationToast::Error);
            return;
        }

        connectToGnss();
    } else {
        disconnectFromGnss();
    }

    syncGnssPosCheckbox();
}

void MainWindow::connectToGnss()
{
    checkAndDisableConflictingSources("gnss");

    if (m_gnssHandler->connectToGnss(m_gnssComPort, m_gnssBaudRate)) {
        qDebug() << "MainWindow: GNSS подключение инициировано успешно";
        m_gnssEnabled = true;
        updateCoordinateSource("GNSS");
        statusBar()->showMessage("Подключение к GNSS...", 3000);
    } else {
        qDebug() << "MainWindow: Ошибка подключения к GNSS";
        m_gnssEnabled = false;
        ui->checkboxGnss->setChecked(false);
        showNotice("Не удалось подключиться к GNSS приемнику", NotificationToast::Error);
    }

    updateFieldsEditability();
    emit gnssDataSourceChanged(m_gnssEnabled);
}

void MainWindow::disconnectFromGnss()
{
    qDebug() << "MainWindow: Отключение от GNSS";
    m_gnssHandler->disconnectFromGnss();
    m_gnssEnabled = false;
    ui->checkboxGnss->setChecked(false);

    updateCoordinateSource("Нет");
    updateFieldsEditability();
    updateGnssMarkerOnMap(0, 0);

    statusBar()->showMessage("GNSS приемник отключен", 3000);
    emit gnssDataSourceChanged(m_gnssEnabled);

}

void MainWindow::onGnssDataReceived(const GNSSData &data)
{
    // Метка "последние данные получены" — для health-check сторожа,
    // обновляем ДО проверки m_gnssEnabled, чтобы отражать реальную
    // активность железа независимо от UI-переключателя.
    m_gnssLastDataAt = QDateTime::currentDateTime();

    qDebug() << "=== MainWindow: Получены GNSS данные ===";
    qDebug() << "  Широта:" << data.latitude;
    qDebug() << "  Долгота:" << data.longitude;
    qDebug() << "  Высота:" << data.altitude;
    qDebug() << "  Спутники:" << data.satellites;
    qDebug() << "  Тип фикса:" << data.fixType;
    qDebug() << "  Качество:" << data.fixQuality;
    qDebug() << "  HDOP:" << data.hdop;
    qDebug() << "  Точность Г:" << data.accuracyH;
    qDebug() << "  Точность В:" << data.accuracyV;

    if (!m_gnssEnabled) {
        return; // Игнорируем данные, если GNSS выключен
    }

    if (data.latitude == 0.0 && data.longitude == 0.0) {
        statusBar()->showMessage("GNSS: Поиск спутников...", 2000);
        return;
    }

    if (data.fixQuality == 0) {
        statusBar()->showMessage("GNSS: Cпутники: " + QString::number(data.satellites) + ")", 2000);
        return;
    }

    // Обновляем поля координат
    setCoordField(ui->editLatitude, data.latitude);
    setCoordField(ui->editLongitude, data.longitude);
    ui->editAltitude->setText(QString::number(data.altitude, 'f', 2));

    // Реальные данные получены с датчика.
    m_hasGnssPosition = true;
    updateMapCoordDisplay("GNSS");

    // Свежие данные с датчика перекрыли то, что могло быть введено
    // вручную ранее — жёлтая подсветка ГНСС больше не актуальна.
    if (m_gnssManualHighlight) {
        m_gnssManualHighlight = false;
        updateGnssStatusLabel(true);
    }
    updateOverallReadiness();

    // Передаем сигнал другим окнам
    emit coordinatesUpdatedFromMap(data.latitude, data.longitude);

    updateGnssMarkerOnMap(data.latitude, data.longitude);

    // Обновляем строку состояния
    statusBar()->showMessage(QString("GNSS: %1 | Спутники: %2 | HDOP: %3")
                                 .arg(data.fixType)
                                 .arg(data.satellites)
                                 .arg(data.hdop, 0, 'f', 1), 5000);
}

void MainWindow::onNmeaReceived(const QString &nmea)
{
    if (nmea.contains("GGA") || nmea.contains("RMC")) {
        qDebug() << "GNSS NMEA: " << nmea;
    }
}

void MainWindow::onGnssConnected()
{
    qDebug() << "GNSS приемник подключен";
    m_gnssLastError.clear();
    m_gnssLastDataAt = QDateTime::currentDateTime();
    // Внешний вид checkboxGnss теперь полностью определяется QSS страницы
    // page_position (селектор QCheckBox#checkboxGnss:checked) — точечный
    // setStyleSheet здесь раньше перебивал эту стилизацию.
    statusBar()->showMessage("GNSS приемник подключен успешно", 5000);
    updateGnssStatusLabel(true);
}

void MainWindow::onGnssDisconnected()
{
    qDebug() << "GNSS приемник отключен";

    m_gnssEnabled = false;

    if (ui->checkboxGnss->isChecked()) {
        ui->checkboxGnss->setChecked(false);
    }
    syncGnssPosCheckbox();

    updateFieldsEditability();
    updateGnssStatusLabel(false);
}

void MainWindow::onGnssError(const QString &error)
{
    qDebug() << "Ошибка GNSS:" << error;
    m_gnssLastError = error;

    if (m_gnssHandler->isConnected()) {
        statusBar()->showMessage("Ошибка GNSS: " + error, 5000);
    }
}

void MainWindow::setupAmsHandler()
{
    m_amsHandler = new AMSHandler(this);

    // Существующие сигналы
    connect(m_amsHandler, &AMSHandler::connected,
            this, &MainWindow::onAmsConnected);
    connect(m_amsHandler, &AMSHandler::disconnected,
            this, &MainWindow::onAmsDisconnected);
    connect(m_amsHandler, &AMSHandler::errorOccurred,
            this, &MainWindow::onAmsError);
    connect(m_amsHandler, &AMSHandler::statusMessage,
            this, &MainWindow::onAmsStatusMessage);

    // НОВЫЕ сигналы для процесса измерения
    connect(m_amsHandler, &AMSHandler::measurementStageChanged,
            this, &MainWindow::onAmsMeasurementStageChanged);
    connect(m_amsHandler, &AMSHandler::measurementProgressUpdated,
            this, &MainWindow::onAmsMeasurementProgress);
    connect(m_amsHandler, &AMSHandler::measurementCompleted,
            this, &MainWindow::onAmsMeasurementCompleted);
    connect(m_amsHandler, &AMSHandler::measurementFailed,
            this, &MainWindow::onAmsMeasurementFailed);
    connect(m_amsHandler, &AMSHandler::needIntermediateData,
            this, &MainWindow::onAmsNeedIntermediateData);

    // Подключаем результаты
    connect(m_amsHandler, &AMSHandler::avgWindDataReceived,
            this, &MainWindow::onAmsAvgWindReceived);
    connect(m_amsHandler, &AMSHandler::actualWindDataReceived,
            this, &MainWindow::onAmsActualWindReceived);
    connect(m_amsHandler, &AMSHandler::measuredWindDataReceived,
            this, &MainWindow::onAmsMeasuredWindReceived);
    connect(m_amsHandler, &AMSHandler::functionalControlDataReceived,
            this, [this](quint32 bitMask, quint32 powerOnCount) {
                m_functionalControlDialog->setAmsData(bitMask, powerOnCount);

                FuncControlResult fc = AMSProtocol::funcControlDetails(bitMask);
                if (!fc.allOk()) {
                    int total = fc.faults.size() + fc.errors.size();
                    statusBar()->showMessage(
                        QString("Функциональный контроль АМС: обнаружено проблем - %1. "
                                "Зайдите в 'Функциональный контроль' для просмотра.").arg(total),
                        30000);
                }
            });

    connect(m_functionalControlDialog, &FunctionalControlDialog::refreshRequested,
            this, [this]() {
                if (m_amsHandler && m_amsHandler->isConnected()
                    && m_amsHandler->getMeasurementStatus() != STATUS_RUNNING) {
                    m_functionalControlDialog->setWaitingState();
                    m_amsHandler->requestFunctionalControl();
                } else if (!m_amsHandler || !m_amsHandler->isConnected()) {
                    m_functionalControlDialog->setDisconnectedState();
                }
            });

    // Когда АМС записал данные в БД — делаем финальный запрос к ИВС
    connect(m_amsHandler, &AMSHandler::dataWrittenToDatabase,
            this, &MainWindow::onAmsDataWritten);

    // m_amsHandler теперь существует — освежаем плитку готовности на экране
    // "Пуск измерения" (до этого момента она могла быть выставлена с
    // m_amsHandler == nullptr).
    updateMeasureReadinessLabel();
}

void MainWindow::configureAmsDatabase()
{
    // Настройка подключения к БД
    // Загрузить параметры из конфигурационного файла или настроек
    QString dbHost = "localhost";
    int dbPort = 5432;
    QString dbName = "MMK";
    QString dbUser = "postgres";
    QString dbPassword = "otdel412";

    qDebug() << "MainWindow: Настройка БД:" << dbName << "на" << dbHost;

    DatabaseManager::instance()->configure(dbHost, dbPort, dbName, dbUser, dbPassword);

    connect(DatabaseManager::instance(), &DatabaseManager::connected,
            this, [this]() {
                statusBar()->showMessage("База данных подключена", 5000);
                qInfo() << "MainWindow: Сигнал connected от DatabaseManager";
            });

    if(DatabaseManager::instance()->connect()) {
        qInfo() << "MainWindow: Успешное подключение к БД";
    } else {
        qCritical() << "MainWindow: Ошибка подключения к БД";
        showNotice("Не удалось подключиться к базе данных. Проверьте параметры подключения.", NotificationToast::Error);
    }

    if (m_amsHandler){
        m_amsHandler->setDatabase(dbHost, dbPort, dbName, dbUser, dbPassword);
    }
}

// ===== СЛОТЫ ДЛЯ АМС =====

void MainWindow::onAmsConnectFromSettings()
{
    if (!sensorSettingsDialog || !m_amsHandler) return;

    m_amsComPort = sensorSettingsDialog->getAmsComPort();
    m_amsBaudRate = sensorSettingsDialog->getAmsBaudRate();

    qDebug() << "MainWindow: Попытка подключения к АМС на" << m_amsComPort
             << "со скоростью" << m_amsBaudRate;

    if (m_amsHandler->connectToAMS(
            m_amsComPort,
            m_amsBaudRate,
            sensorSettingsDialog->getAmsDataBits(),
            sensorSettingsDialog->getAmsParity(),
            sensorSettingsDialog->getAmsStopBits())) {

        qDebug() << "MainWindow: АМС подключение инициировано";
        sensorSettingsDialog->setAmsConnectionStatus("Подключение...", false);
    } else {
        qDebug() << "MainWindow: Ошибка подключения к АМС";
        sensorSettingsDialog->setAmsConnectionStatus("Ошибка подключения", false);
        showNotice("Не удалось подключиться к АМС. Проверьте порт и настройки.", NotificationToast::Error);
    }
}

void MainWindow::onAmsDisconnectFromSettings()
{
    if (!m_amsHandler) return;

    qDebug() << "MainWindow: Отключение от АМС";
    m_amsHandler->disconnectFromAMS();

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setAmsConnectionStatus("Отключено", false);
        sensorSettingsDialog->setAmsConnectionEnabled(true);
    }
}

void MainWindow::onAmsConnected()
{
    qDebug() << "MainWindow: АМС подключена успешно";
    m_amsLastError.clear();

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setAmsConnectionStatus("Подключено", true);
        sensorSettingsDialog->setAmsConnectionEnabled(false);
    }

    statusBar()->showMessage("АМС подключена успешно", 5000);
    updateAmsStatusLabel(true);
    updateMeasureReadinessLabel();
}

void MainWindow::onAmsDisconnected()
{
    qDebug() << "MainWindow: АМС отключена";

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setAmsConnectionStatus("Отключено", false);
        sensorSettingsDialog->setAmsConnectionEnabled(true);
    }

    statusBar()->showMessage("АМС отключена", 3000);
    updateAmsStatusLabel(false);
    updateMeasureReadinessLabel();

    if (m_functionalControlDialog->isVisible()) {
        m_functionalControlDialog->setDisconnectedState();
    }
}

void MainWindow::onAmsError(const QString &error)
{
    qWarning() << "MainWindow: Ошибка АМС:" << error;
    m_amsLastError = error;
    statusBar()->showMessage("Ошибка АМС: " + error, 10000);

    // Обновляем плашку статуса АМС и плитку готовности — причина уже
    // доступна в шторке датчика (клик по "АМС:" наверху → sensorProblemReason()
    // читает m_amsLastError). Отдельное модальное окно здесь больше НЕ
    // показываем: при обрыве соединения (физическое отключение, ошибка
    // порта, health-check без ответа) оно только мешает — оператор и так
    // увидит красную плашку "АМС: отключен" наверху и причину в шторке.
    updateAmsStatusLabel(m_amsHandler && m_amsHandler->isConnected());
    updateMeasureReadinessLabel();

    // Диалог функционального контроля, если открыт, получает ошибку отдельно
    if (m_functionalControlDialog->isVisible()) {
        m_functionalControlDialog->setErrorState(error);
    }
}

void MainWindow::onAmsStatusMessage(const QString &message)
{
    qDebug() << "MainWindow: Статус АМС:" << message;
    statusBar()->showMessage("АМС: " + message, 3000);
}

void MainWindow::onAmsDataWritten(int recordId)
{
    qDebug() << "MainWindow: Данные АМС записаны в архив, record_id:" << recordId;
    statusBar()->showMessage(
        QString("АМС: Данные записаны в архив (ID: %1)").arg(recordId), 5000);

    if (m_iwsDeviceActive) {
        // Железный ИВС подключён — запрашиваем у него свежие данные.
        // Ответ придёт в onIwsFinalDataReceived, который вызовет tryFinalizeMeasurement.
        requestIwsDataForRecord(recordId);
    } else {
        // Железного ИВС нет. Возможно, оператор ввёл приземные данные вручную —
        // тогда они уже в кеше GroundMeteoParams. Финализируем без запроса к ИВС.
        qDebug() << "MainWindow: ИВС не подключён — пробуем ручные приземные данные";
        tryFinalizeMeasurement(recordId);
    }
}

void MainWindow::onAmsDatabaseError(const QString &error)
{
    qCritical() << "MainWindow: Ошибка БД АМС:" << error;
    statusBar()->showMessage("Ошибка БД АМС: " + error, 10000);
    showNotice("Не удалось записать данные АМС в базу данных: " + error, NotificationToast::Error);
}

// ========= БИНС =========
void MainWindow::setupBinsHandler()
{
    if (!m_binsHandler) return;

    connect(m_binsHandler, &BINSHandler::connected, this, &MainWindow::onBinsConnected);
    connect(m_binsHandler, &BINSHandler::disconnected, this, &MainWindow::onBinsDisconnected);
    connect(m_binsHandler, &BINSHandler::errorOccurred, this, &MainWindow::onBinsError);
    connect(m_binsHandler, &BINSHandler::statusMessage, this, &MainWindow::onBinsStatusMessage);
    connect(m_binsHandler, &BINSHandler::dataReceived, this, &MainWindow::onBinsDataReceived);

    qDebug() << "MainWindow: БИНС обработчик настроен";
}

void MainWindow::onBinsConnectFromSettings()
{
    if (!sensorSettingsDialog || !m_binsHandler) return;

    m_binsComPort = sensorSettingsDialog->getBinsComPort();
    m_binsBaudRate = sensorSettingsDialog->getBinsBaudRate();

    qDebug() << "MainWindow: Попытка подключения к БИНС на" << m_binsComPort << "со скоростью" << m_binsBaudRate;

    if (m_binsHandler->connectToBINS(
            m_binsComPort,
            m_binsBaudRate,
            sensorSettingsDialog->getBinsDataBits(),
            sensorSettingsDialog->getBinsParity(),
            sensorSettingsDialog->getBinsStopBits())) {
        qDebug() << "MainWindow: БИНС подключение инициализировано";
        sensorSettingsDialog->setBinsConnectionStatus("Ожидание данных...", false);
    } else {
        qDebug() << "MainWindow: Ошибка подключения к БИНС";
        sensorSettingsDialog->setBinsConnectionStatus("Ошибка подключения", false);
        showNotice("Не удалось подключиться к БИНС. Проверьте порт или настройки.", NotificationToast::Error);
    }
}

void MainWindow::onBinsDisconnectFromSettings()
{
    if (!m_binsHandler) return;

    qDebug() << "MainWindow: Отключение от БИНС";
    m_binsHandler->disconnectFromBINS();

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setBinsConnectionStatus("Отключено", false);
        sensorSettingsDialog->setBinsConnectionEnabled(true);
    }
}

void MainWindow::onBinsConnected()
{
    qDebug() << "MainWindow: БИНС подключен успешно";
    m_binsLastError.clear();
    m_binsLastDataAt = QDateTime::currentDateTime();

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setBinsConnectionStatus("Подключено", true);
        sensorSettingsDialog->setBinsConnectionEnabled(false);
    }

    statusBar()->showMessage("БИНС подключен успешно", 5000);
    updateBinsStatusLabel(true);
}

void MainWindow::onBinsDisconnected()
{
    qDebug() << "MainWindow: БИНС отключен";

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setBinsConnectionStatus("Отключено", false);
        sensorSettingsDialog->setBinsConnectionEnabled(true);
    }

    statusBar()->showMessage("БИНС отключен", 3000);
    updateBinsStatusLabel(false);
}

void MainWindow::onBinsError(const QString &error)
{
    qWarning() << "MainWindow: Ошибка БИНС:" << error;
    m_binsLastError = error;
    statusBar()->showMessage("Ошибка БИНС: " + error, 10000);
}

void MainWindow::onBinsStatusMessage(const QString &message)
{
    qDebug() << "MainWindow: Статус БИНС:" << message;
    statusBar()->showMessage("БИНС: " + message, 3000);
}

void MainWindow::onBinsDataReceived(const BINSData &data)
{
    if (!data.valid) return;

    // Метка "последние данные получены" — для health-check сторожа.
    m_binsLastDataAt = QDateTime::currentDateTime();

    // Обновляем поля в интерфейсе
    ui->editDirectionAngle->setText(QString::number(data.heading, 'f', 2));
    ui->editRollAngle->setText(QString::number(data.roll, 'f', 2));
    ui->editPitchAngle->setText(QString::number(data.pitch, 'f', 2));

    // Реальные данные получены с датчика.
    m_hasBinsOrientation = true;

    // Свежие данные с датчика перекрыли ручной ввод — подсветка снимается.
    if (m_binsManualHighlight) {
        m_binsManualHighlight = false;
        updateBinsStatusLabel(true);
    }
    updateOverallReadiness();

    // Обновляем строку состояния
    statusBar()->showMessage(
        QString("БИНС: Курс %1 град. | Крен %2 град. | Тангаж %3 град.")
            .arg(data.heading, 0, 'f', 1)
            .arg(data.roll, 0, 'f', 1)
            .arg(data.pitch, 0, 'f', 1),
        2000);
}

void MainWindow::onConnectSensorsClicked()
{
    bool gnssOk = m_gnssHandler->isConnected();
    bool amsOk  = m_amsHandler && m_amsHandler->isConnected();
    bool binsOk = m_binsHandler && m_binsHandler->isConnected();
    bool iwsOk  = m_iwsDeviceActive;
    if (gnssOk && amsOk && binsOk && iwsOk) {
        m_toastTitle->setText("Датчики подключены");
        m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #1C1F22; border: none; background: transparent;");
        m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #0F6B4F; border: none; background: transparent;");
        m_toastProgress->setStyleSheet(
            "QProgressBar { background-color: #EFF1F1; border: none; border-radius: 3px; }"
            "QProgressBar::chunk { background-color: #0F6B4F; border-radius: 3px; }"
            );
        m_toastText->setText("Все датчики подключены.");
        m_toastPercent->setText("100%");
        m_toastProgress->setValue(100);
        showToast();
        m_toastHideTimer->start(4000);
        return;
    }
    if (m_autoConnector->isDetecting()) return;
    m_autoConnector->startDetection();
}

void MainWindow::onConnectAllClicked()
{
    // Клик по иконке в статус-панели теперь только открывает/закрывает
    // шторку с пояснением (как у датчиков) — сам поиск запускается кнопкой
    // внутри неё, см. onConnectAllPopupActionClicked().
    showConnectAllPopup();
}

void MainWindow::onConnectAllPopupActionClicked()
{
    // Отдельная проверка "все уже подключены" не нужна: шторка и так
    // доступна только когда ни один датчик не подключён (см.
    // updateConnectAllButtonVisibility()).
    if (m_autoConnector->isDetecting()) {
        hideConnectAllPopup();
        return;
    }
    hideConnectAllPopup();
    m_autoConnector->startDetection();
    updateConnectAllButtonVisibility();
}

bool MainWindow::connectIwsPort(const QString &port, int baudRate, QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                                int protocol, quint8 address, int pollInterval)
{
    // Создаём serial port если ещё не создан
    if (!serialPort) {
        serialPort = new QSerialPort(this);
        connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReceived);
        connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::onSerialError);
    }

    // Закрываем если уже открыт
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    // Настраиваем порт
    serialPort->setPortName(port);
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(dataBits);
    serialPort->setParity(parity);
    serialPort->setStopBits(stopBits);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Пытаемся открыть порт
    if (serialPort->open(QIODevice::ReadWrite)) {
        // Порт открыт, но устройство ещё не подтверждено —
        // "Подключено" выставим только после первого ответа
        m_iwsDeviceActive = false;
        sensorSettingsDialog->setIwsConnectionStatus("Ожидание ответа...", false);
        sensorSettingsDialog->setIwsConnectionEnabled(false);

        // Запускаем таймаут: если за 3 с нет ответа — считаем, что устройства нет
        if (!m_iwsConnectTimer) {
            m_iwsConnectTimer = new QTimer(this);
            m_iwsConnectTimer->setSingleShot(true);
            connect(m_iwsConnectTimer, &QTimer::timeout, this, &MainWindow::onIwsConnectTimeout);
        }
        m_iwsConnectTimer->start(3000);

        // Запускаем таймер прогрева ИВС (3 минуты)
        m_iwsWarmupDone = false;
        // Переключаемся на 6 мин., если сейчас выбрано 3 мин.
        if (ui->btnAvg3->isChecked())
            ui->btnAvg6->setChecked(true);
        // Серим (блокируем) кнопку "3 мин" — не скрываем, чтобы не сбивать с толку
        ui->btnAvg3->setEnabled(false);
        ui->btnAvg3->setToolTip("Режим 3 мин. станет доступен через 3 минуты после подключения ИВС");
        m_iwsWarmupTimer->start(3 * 60 * 1000);
        statusBar()->showMessage("ИВС подключён. Режим усреднения 3 мин. станет доступен через 3 минуты.", 8000);

        // Настраиваем протокол в GroundMeteoParams
        GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
        if (meteoParams) {
            int protocolToUse = protocol;

            GroundMeteoParams::RS485Protocol rs485Protocol =
                (protocolToUse == 0) ?
                    GroundMeteoParams::UMB_PROTOCOL :
                    GroundMeteoParams::MODBUS_RTU;

            meteoParams->setProtocol(rs485Protocol);
            meteoParams->setDeviceAddress(address);

            qDebug() << "IWS: Configured"
                     << (protocolToUse == 0 ? "UMB" : "Modbus RTU")
                     << "protocol, address" << QString("0x%1").arg(address, 2, 16, QChar('0'))
                     << (protocolToUse == 1 ? "(AVERAGE values)" : "");
        } else {
            qDebug() << "GroundMeteoParams not created yet. Will be configured when 'Initial Data' is opened.";
        }

        // Запускаем таймер опроса
        if (!pollTimer) {
            pollTimer = new QTimer(this);
            connect(pollTimer, &QTimer::timeout, this, &MainWindow::pollMeteoStation);
        }
        pollTimer->start(pollInterval * 1000);
        // Немедленный зондирующий запрос для быстрой верификации устройства
        QTimer::singleShot(200, this, &MainWindow::pollMeteoStation);

        qDebug() << "RS485 port opened on" << port << "— waiting for device response";
        return true;
    } else {
        sensorSettingsDialog->setIwsConnectionStatus("Ошибка подключения", false);
        return false;
    }
}

void MainWindow::onConnectRequested()
{
    if (sensorSettingsDialog->getIwsComPort().isEmpty() ||
        sensorSettingsDialog->getIwsComPort() == "Нет доступных портов") {
        showNotice("Нет доступных COM-портов", NotificationToast::Error);
        return;
    }

    if (!connectIwsPort(sensorSettingsDialog->getIwsComPort(),
                        sensorSettingsDialog->getIwsBaudRate(),
                        sensorSettingsDialog->getIwsDataBits(),
                        sensorSettingsDialog->getIwsParity(),
                        sensorSettingsDialog->getIwsStopBits(),
                        IWS_PROTOCOL,
                        sensorSettingsDialog->getIwsDeviceAddress(),
                        sensorSettingsDialog->getIwsPollInterval())) {
        showNotice(QString("Не удалось открыть порт: %1").arg(serialPort->errorString()),
                   NotificationToast::Error);
    }
}

void MainWindow::onIwsWarmupFinished()
{
    m_iwsWarmupDone = true;
    // Разблокируем кнопку "3 мин"
    ui->btnAvg3->setEnabled(true);
    ui->btnAvg3->setToolTip("");
    statusBar()->showMessage("ИВС: режим усреднения 3 минуты теперь доступен", 5000);
    qDebug() << "MainWindow: ИВС прогрев завершён, пункт '3 мин' разблокирован";
}

void MainWindow::onDisconnectRequested()
{
    m_iwsDeviceActive = false;
    if (m_iwsConnectTimer) m_iwsConnectTimer->stop();

    // Сбрасываем прогрев ИВС; ИВС отключён — восстанавливаем кнопку "3 мин"
    if (m_iwsWarmupTimer) {
        m_iwsWarmupTimer->stop();
    }
    m_iwsWarmupDone = false;
    // При отключении ИВС — разблокируем кнопку "3 мин" обратно
    ui->btnAvg3->setEnabled(true);
    ui->btnAvg3->setToolTip("");

    if (pollTimer) {
        pollTimer->stop();
    }

    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }

    sensorSettingsDialog->setIwsConnectionStatus("Отключено", false);
    sensorSettingsDialog->setIwsConnectionEnabled(true);
    updateIwsStatusLabel(false);

    qDebug() << "RS485 disconnected";
}

void MainWindow::onSerialDataReceived()
{
    if (!serialPort) return;

    QByteArray data = serialPort->readAll();
    qDebug() << "Received data:" << data.toHex(' ');

    // Первый ответ от устройства — подтверждаем подключение ИВС
    if (!m_iwsDeviceActive) {
        m_iwsDeviceActive = true;
        m_iwsLastError.clear();
        if (m_iwsConnectTimer) m_iwsConnectTimer->stop();
        sensorSettingsDialog->setIwsConnectionStatus("Подключено", true);
        updateIwsStatusLabel(true);
        qDebug() << "IWS device confirmed (first response received)";
    }
    m_iwsLastDataAt = QDateTime::currentDateTime();

    // Передаём данные в GroundMeteoParams если он открыт
    GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
    if (meteoParams) {
        meteoParams->onDataReceived(data);
    }
}

void MainWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
        qDebug() << "Serial port error:" << serialPort->errorString();
        m_iwsDeviceActive = false;
        m_iwsLastError = QString("Ошибка порта: %1").arg(serialPort->errorString());

        if (sensorSettingsDialog) {
            sensorSettingsDialog->setIwsConnectionStatus(
                QString("Ошибка: %1").arg(serialPort->errorString()), false);
        }

        if (serialPort->isOpen()) {
            onDisconnectRequested();
        }
    }
}

void MainWindow::onIwsConnectTimeout()
{
    // Таймаут истёк — устройство не отвечает, порт открыт впустую
    if (!m_iwsDeviceActive && serialPort && serialPort->isOpen()) {
        qDebug() << "IWS connect timeout — no response, closing port";
        m_iwsLastError = "Устройство не отвечает (таймаут при подключении)";
        onDisconnectRequested();
        sensorSettingsDialog->setIwsConnectionStatus("Нет ответа от устройства", false);

        // Показываем предупреждение только если AutoConnector уже не работает
        // (чтобы не дублировать сообщение из onAutoConnectorFinished)
        if (!m_autoConnector->isDetecting()) {
            showNotice("Не удалось подключить ИВС: устройство не отвечает. "
                       "Проверьте физическое подключение кабеля и нажмите "
                       "«Подключить датчики» для повторной попытки.",
                       NotificationToast::Error);
        }
    }
}

void MainWindow::pollMeteoStation()
{
    if (!serialPort || !serialPort->isOpen()) {
        qDebug() << "Serial port not open, skipping poll";
        return;
    }

    GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
    if (!meteoParams) {
        qDebug() << "GroundMeteoParams instance not found";
        return;
    }

    // Получаем параметры для запроса
    QList<quint16> params = getRequestParameters();

    if (params.isEmpty()) {
        qWarning() << "No parameters to request";
        return;
    }

    // Создаём запрос через GroundMeteoParams
    QByteArray request;

    // ИСПОЛЬЗУЕМ КОНСТАНТУ IWS_PROTOCOL (определена в начале файла)
    int protocolToUse = IWS_PROTOCOL;

    // Если нужно, можно переопределить из настроек
    // protocolToUse = sensorSettingsDialog->getIwsProtocolIndex();

    // Получаем адрес устройства из настроек
    quint8 deviceAddress = sensorSettingsDialog ?
                               sensorSettingsDialog->getIwsDeviceAddress() :
                               (protocolToUse == 0 ? 0x70 : 0x01);

    if (protocolToUse == 0) { // UMB
        meteoParams->setProtocol(GroundMeteoParams::UMB_PROTOCOL);
        meteoParams->setDeviceAddress(deviceAddress);
        request = meteoParams->createUmbReadRequest(params);
        qDebug() << "Polling IWS with UMB protocol (current values), address"
                 << QString("0x%1").arg(deviceAddress, 2, 16, QChar('0'));
    } else { // MODBUS RTU
        meteoParams->setProtocol(GroundMeteoParams::MODBUS_RTU);
        meteoParams->setDeviceAddress(deviceAddress);
        request = meteoParams->createModbusReadRequest(params);
        qDebug() << "Polling IWS with Modbus RTU protocol (AVERAGE values), address"
                 << QString("0x%1").arg(deviceAddress, 2, 16, QChar('0'));
    }

    if (request.isEmpty()) {
        qDebug() << "Failed to create request";
        return;
    }

    qint64 written = serialPort->write(request);
    if (written != -1) {
        qDebug() << "Request sent (" << written << "bytes):" << request.toHex(' ');
    } else {
        qDebug() << "Failed to write to serial port";
    }
}

QList<quint16> MainWindow::getRequestParameters()
{
    QList<quint16> params;

    // ИСПОЛЬЗУЕМ КОНСТАНТУ IWS_PROTOCOL (определена в начале файла)
    int protocolToUse = IWS_PROTOCOL;

    // Если нужно, можно переопределить из настроек
    // if (sensorSettingsDialog) {
    //     protocolToUse = sensorSettingsDialog->getIwsProtocolIndex();
    // }

    if (protocolToUse == 0) { // UMB - текущие значения
        params << 0x0064  // Temperature (текущая)
               << 0x00C8  // Humidity (текущая)
               << 0x012C  // Pressure (текущее)
               << 0x0190  // Wind Speed (текущая)
               << 0x01F4; // Wind Direction (текущее)
        qDebug() << "Request parameters: UMB current values";
    } else { // MODBUS RTU - СРЕДНИЕ значения (функция 0x04)
        // Оптимизированный запрос: читаем близкие регистры вместе
        // Группируем регистры чтобы не читать 70 штук сразу

        // ВАРИАНТ 1: Все 5 регистров (читает 13-82 = 70 регистров)
        // Работает, но читает много лишнего
        params << 13  // Humidity Avg (влажность средняя)
               << 21  // Wind Direction Avg (направление ветра среднее)
               << 34  // Temperature Avg (температура средняя)
               << 45  // Wind Speed Avg (скорость ветра средняя)
               << 82; // Pressure Avg (давление среднее)

        // ВАРИАНТ 2: Если хотите читать меньше за раз, раскомментируйте это:
        // params << 13 << 21 << 34 << 45;  // 4 параметра (13-45 = 33 регистра)
        // В следующем цикле добавить: params << 82;  // 1 параметр

        qDebug() << "Request parameters: Modbus RTU (0x04) AVERAGE values, registers:" << params;
    }

    return params;
}

// ==================== Авто-подключение датчиков ====================

void MainWindow::connectSensorsFromConfig()
{
    // Стартовый опрос при запуске программы ещё не завершён — пока не знаем
    // итог, кнопка "Подключить всё" должна молчать (не мигать видимой).
    ui->btnConnectAll->setVisible(false);

    // Try each sensor from sensorSettingsDialog (already loaded from QSettings)
    // Track which ones need AutoConnector
    QStringList needAutoSearch;

    // GNSS
    QString gnssPort = sensorSettingsDialog->getGnssComPort();
    if (!gnssPort.isEmpty() && gnssPort != "Нет доступных портов") {
        // ВАЖНО: сначала фиксируем порт/скорость в полях класса. Иначе
        // setChecked(true) ниже синхронно вызовет onGnssCheckboxToggled(),
        // тот увидит пустой m_gnssComPort, снимет галку и через ветку
        // disconnectFromGnss() закроет только что открытый порт — связь
        // рвётся в том же стеке вызовов, до прихода первых данных.
        m_gnssComPort  = gnssPort;
        m_gnssBaudRate = sensorSettingsDialog->getGnssBaudRate();
        if (m_gnssHandler->connectToGnss(m_gnssComPort, m_gnssBaudRate)) {
            m_gnssEnabled = true;
            checkAndDisableConflictingSources("gnss");
            // Галку выставляем без сигнала — обработчик toggled уже не нужен,
            // подключение выполнено выше.
            ui->checkboxGnss->blockSignals(true);
            ui->checkboxGnss->setChecked(true);
            ui->checkboxGnss->blockSignals(false);
            syncGnssPosCheckbox();
        } else { needAutoSearch << "gnss"; }
    } else { needAutoSearch << "gnss"; }

    // АМС
    QString amsPort = sensorSettingsDialog->getAmsComPort();
    if (!amsPort.isEmpty() && amsPort != "Нет доступных портов" && m_amsHandler) {
        if (!m_amsHandler->connectToAMS(amsPort, sensorSettingsDialog->getAmsBaudRate(),
                sensorSettingsDialog->getAmsDataBits(), sensorSettingsDialog->getAmsParity(),
                sensorSettingsDialog->getAmsStopBits()))
            needAutoSearch << "ams";
    } else { needAutoSearch << "ams"; }

    // БИНС (no auto-search for BINS)
    QString binsPort = sensorSettingsDialog->getBinsComPort();
    if (!binsPort.isEmpty() && binsPort != "Нет доступных портов" && m_binsHandler) {
        m_binsHandler->connectToBINS(binsPort, sensorSettingsDialog->getBinsBaudRate(),
            sensorSettingsDialog->getBinsDataBits(), sensorSettingsDialog->getBinsParity(),
            sensorSettingsDialog->getBinsStopBits());
        // BINS is async, don't track failure here
    }

    // ИВС
    QString iwsPort = sensorSettingsDialog->getIwsComPort();
    if (!iwsPort.isEmpty() && iwsPort != "Нет доступных портов") {
        if (!connectIwsPort(iwsPort, sensorSettingsDialog->getIwsBaudRate(),
                sensorSettingsDialog->getIwsDataBits(), sensorSettingsDialog->getIwsParity(),
                sensorSettingsDialog->getIwsStopBits(), IWS_PROTOCOL,
                sensorSettingsDialog->getIwsDeviceAddress(), sensorSettingsDialog->getIwsPollInterval()))
            needAutoSearch << "iws";
    } else { needAutoSearch << "iws"; }

    bool anyNeedSearch = needAutoSearch.contains("gnss") || needAutoSearch.contains("ams") || needAutoSearch.contains("iws");
    if (anyNeedSearch) {
        m_autoConnector->startDetection();
    } else {
        // Автопоиск не понадобился — все настроенные датчики поднялись
        // напрямую из конфига. Стартовый опрос считается завершённым.
        m_startupSensorCheckDone = true;
        updateConnectAllButtonVisibility();
    }
    // Note: if no auto-search needed, we're done (BINS failure shown after AutoConnector)
}

void MainWindow::onAutoConnectorDeviceDetected(AutoConnector::DeviceType type, const QString &port, int baudRate)
{
    switch (type) {
    case AutoConnector::DEVICE_GNSS:
        if (!m_gnssHandler->isConnected()) {
            // Порт/скорость — в поля класса до setChecked(true), иначе
            // onGnssCheckboxToggled() увидит пустой m_gnssComPort и закроет
            // только что открытый порт (см. connectSensorsFromConfig()).
            m_gnssComPort  = port;
            m_gnssBaudRate = baudRate;
            if (m_gnssHandler->connectToGnss(m_gnssComPort, m_gnssBaudRate)) {
                m_gnssEnabled = true;
                checkAndDisableConflictingSources("gnss");
                ui->checkboxGnss->blockSignals(true);
                ui->checkboxGnss->setChecked(true);
                ui->checkboxGnss->blockSignals(false);
                syncGnssPosCheckbox();
            }
        }
        break;
    case AutoConnector::DEVICE_AMS:
        if (m_amsHandler && !m_amsHandler->isConnected()) {
            m_amsHandler->connectToAMS(port, baudRate, QSerialPort::Data8, QSerialPort::NoParity, QSerialPort::OneStop);
        }
        break;
    case AutoConnector::DEVICE_IWS:
        if (!m_iwsDeviceActive && !(serialPort && serialPort->isOpen())) {
            connectIwsPort(port, baudRate, QSerialPort::Data8, QSerialPort::NoParity, QSerialPort::OneStop,
                           IWS_PROTOCOL, 0, 5); // default address=0, pollInterval=5s
        }
        break;
    case AutoConnector::DEVICE_BINS:
        if (m_binsHandler && !m_binsHandler->isConnected()) {
            m_binsHandler->connectToBINS(port, baudRate);
        }
        break;
    default: break;
    }
}

void MainWindow::onAutoConnectorFinished()
{
    ui->btnConnectSensors->setEnabled(true);
    statusBar()->clearMessage();

    m_toastCloseBtn->hide();

    m_toastProgress->setValue(100);
    m_toastPercent->setText("100%");

    // АМС может быть УЖЕ найден AutoConnector'ом (свой отдельный временный
    // порт AutoConnector'а успешно прошёл LINE_TEST), но m_amsHandler ещё
    // не успел подтвердить связь своим ОТДЕЛЬНЫМ подключением — внутри
    // AMSHandler::connectToAMS() есть встроенная пауза перед отправкой
    // LINE_TEST плюс время на сам обмен, итого может занять больше времени,
    // чем разница между "нашли устройство" и "поиск полностью завершён".
    // Без этой отсрочки toast мог написать "не найден" за мгновение до
    // того, как соединение реально подтвердится — короткая пауза убирает
    // эту гонку.
    const auto detected = m_autoConnector->getDetectedDevices();
    const bool amsPendingConfirm = detected.contains(AutoConnector::DEVICE_AMS) &&
                                   m_amsHandler && !m_amsHandler->isConnected();

    if (amsPendingConfirm) {
        QTimer::singleShot(1500, this, &MainWindow::finalizeAutoConnectorFinished);
        return;
    }

    finalizeAutoConnectorFinished();
}

void MainWindow::finalizeAutoConnectorFinished()
{
    // Поиск (полный или одиночный) завершён — пересчитываем видимость
    // "Подключить всё" по актуальному состоянию подключений.
    m_startupSensorCheckDone = true;
    updateConnectAllButtonVisibility();

    const AutoConnector::DeviceType singleTarget = m_autoConnector->singleSearchTarget();

    if (singleTarget != AutoConnector::DEVICE_UNKNOWN) {
        // Поиск ОДНОГО датчика, запущенный из шторки — короткое тайловое
        // сообщение вместо общего диалога/списка "не найдено".
        const QString name = sensorDisplayName(singleTarget);
        const bool found = isSensorConnected(singleTarget);

        if (found) {
            m_toastTitle->setText("Датчик найден");
            m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #1C1F22; border: none; background: transparent;");
            m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #0F6B4F; border: none; background: transparent;");
            m_toastProgress->setStyleSheet(
                "QProgressBar { background-color: #EFF1F1; border: none; border-radius: 3px; }"
                "QProgressBar::chunk { background-color: #0F6B4F; border-radius: 3px; }"
                );
            m_toastText->setText(name + " обнаружен и подключён!");
        } else {
            m_toastTitle->setText("Датчик не найден");
            m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #B71C1C; border: none; background: transparent;");
            m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #B71C1C; border: none; background: transparent;");
            m_toastProgress->setStyleSheet(
                "QProgressBar { background-color: #FFEBEE; border: none; border-radius: 3px; }"
                "QProgressBar::chunk { background-color: #C62828; border-radius: 3px; }"
                );
            m_toastText->setText(name + " не найден. Проверьте подключение кабеля.");
        }

        // Если шторка этого же датчика ещё открыта — освежаем её (кнопка
        // "Идёт поиск..." должна смениться на актуальное состояние).
        if (m_sensorPopup && m_sensorPopup->isVisible() && m_currentPopupSensor == singleTarget)
            populateSensorPopupContent();

        m_toastHideTimer->start(4000);

        // Если пока искали этот датчик, в очередь встали другие заявки —
        // запускаем следующую. Небольшая пауза даёт порту время освободиться
        // и не даёт тосту "найдено/не найдено" смениться мгновенно.
        if (!m_sensorSearchQueue.isEmpty())
            QTimer::singleShot(500, this, &MainWindow::startNextQueuedSearch);

        return;
    }

    QStringList failed;
    if (!m_gnssHandler->isConnected())                    failed << "GNSS";
    if (m_amsHandler && !m_amsHandler->isConnected())     failed << "АМС";
    if (m_binsHandler && !m_binsHandler->isConnected())   failed << "БИНС";
    if (!m_iwsDeviceActive)                               failed << "ИВС";

    if (!failed.isEmpty()) {
        m_toastTitle->setText("Поиск завершнен с ошибками");
        m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #B71C1C; border: none; background: transparent;");
        m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #B71C1C; border: none; background: transparent;");
        m_toastProgress->setStyleSheet(
            "QProgressBar { background-color: #FFEBEE; border: none; border-radius: 3px; }"
            "QProgressBar::chunk { background-color: #C62828; border-radius: 3px }"
        );
        m_toastText->setText("Не подключены: " + failed.join(", "));
    } else {
        m_toastTitle->setText("Поиск успешно завершен");
        m_toastText->setText("Все датчики обнаружены и подключены!");
    }

    m_toastHideTimer->start(4000);
}

void MainWindow::startSingleSensorSearch(AutoConnector::DeviceType type)
{
    hideSensorPopup();

    if (m_autoConnector->isDetecting()) {
        if (m_autoConnector->singleSearchTarget() == type) {
            // Этот же датчик уже ищется — просто ждём, дублировать не нужно.
            statusBar()->showMessage(sensorDisplayName(type) + ": поиск уже выполняется", 3000);
            return;
        }
        // Поиск другого датчика уже идёт — AutoConnector умеет искать
        // только один тип за раз, поэтому не блокируем кнопку, а ставим
        // заявку в очередь: она стартует сама, как только текущий поиск
        // завершится (см. startNextQueuedSearch, вызывается из
        // finalizeAutoConnectorFinished). Уведомления по-прежнему идут в
        // один и тот же тост снизу — просто по очереди.
        if (!m_sensorSearchQueue.contains(type)) {
            m_sensorSearchQueue.append(type);
            statusBar()->showMessage(sensorDisplayName(type) + ": добавлено в очередь поиска", 3000);
        }
        return;
    }

    m_autoConnector->startDetection(type);
    // Поиск одного датчика тоже "поиск" — прячем "Подключить всё" на время,
    // даже если до этого было 0 из 4 подключено.
    updateConnectAllButtonVisibility();
}

// Запускает следующий поиск из очереди (см. startSingleSensorSearch) —
// вызывается из finalizeAutoConnectorFinished сразу после завершения
// предыдущего одиночного поиска. Пропускает датчики, которые тем временем
// уже подключились другим путём (вручную/автопоиском).
void MainWindow::startNextQueuedSearch()
{
    while (!m_sensorSearchQueue.isEmpty()) {
        const AutoConnector::DeviceType next = m_sensorSearchQueue.takeFirst();
        if (isSensorConnected(next))
            continue;
        if (!m_autoConnector->isDetecting())
            m_autoConnector->startDetection(next);
        return;
    }
}

// Вызывается ТОЛЬКО из health-check (onSilenceWatchdogTimer) при реальной
// потере связи — кнопки отключения в шторке больше нет, отключение
// отслеживается автоматически.
void MainWindow::disconnectSensor(AutoConnector::DeviceType type)
{
    switch (type) {
    case AutoConnector::DEVICE_GNSS: onGnssDisconnectFromSettings(); break;
    case AutoConnector::DEVICE_AMS:  onAmsDisconnectFromSettings();  break;
    case AutoConnector::DEVICE_BINS: onBinsDisconnectFromSettings(); break;
    case AutoConnector::DEVICE_IWS:  onDisconnectRequested();        break;
    default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Health-check: опрос "жив ли датчик" для уже подключённых
// ─────────────────────────────────────────────────────────────────────────

void MainWindow::setupHealthChecks()
{
    m_amsHealthTimer = new QTimer(this);
    m_amsHealthTimer->setInterval(kHealthCheckIntervalMs);
    connect(m_amsHealthTimer, &QTimer::timeout, this, &MainWindow::onAmsHealthCheckTimer);
    m_amsHealthTimer->start();

    m_silenceWatchdogTimer = new QTimer(this);
    m_silenceWatchdogTimer->setInterval(kHealthCheckIntervalMs);
    connect(m_silenceWatchdogTimer, &QTimer::timeout, this, &MainWindow::onSilenceWatchdogTimer);
    m_silenceWatchdogTimer->start();
}

void MainWindow::onAmsHealthCheckTimer()
{
    if (!m_amsHandler || !m_amsHandler->isConnected())
        return; // не подключён — нечего проверять

    if (m_amsHandler->getMeasurementStatus() == STATUS_RUNNING)
        return; // идёт измерение — живость и так подтверждается обменом данных

    // pingConnection() сама разберётся: если ответа не будет — AMSHandler
    // сам вызовет disconnectFromAMS() и эмитит disconnected() (см.
    // AMSHandler::onResponseTimeout) — обычный путь onAmsDisconnected()
    // обновит UI, никакой дополнительной обработки здесь не требуется.
    m_amsHandler->pingConnection();
}

void MainWindow::onSilenceWatchdogTimer()
{
    const QDateTime now = QDateTime::currentDateTime();
    bool anyChanged = false;

    if (m_gnssHandler && m_gnssHandler->isConnected() && m_gnssLastDataAt.isValid() &&
        m_gnssLastDataAt.msecsTo(now) > kSilenceTimeoutMs) {
        qWarning() << "MainWindow: GNSS молчит дольше" << kSilenceTimeoutMs
                   << "мс — считаем отключённым";
        m_gnssLastError = QString("Нет данных более %1 сек — проверьте кабель/антенну")
                              .arg(kSilenceTimeoutMs / 1000);
        disconnectSensor(AutoConnector::DEVICE_GNSS);
        anyChanged = true;
    }

    if (m_binsHandler && m_binsHandler->isConnected() && m_binsLastDataAt.isValid() &&
        m_binsLastDataAt.msecsTo(now) > kSilenceTimeoutMs) {
        qWarning() << "MainWindow: БИНС молчит дольше" << kSilenceTimeoutMs
                   << "мс — считаем отключённым";
        m_binsLastError = QString("Нет данных более %1 сек — проверьте кабель")
                              .arg(kSilenceTimeoutMs / 1000);
        disconnectSensor(AutoConnector::DEVICE_BINS);
        anyChanged = true;
    }

    if (m_iwsDeviceActive && m_iwsLastDataAt.isValid() &&
        m_iwsLastDataAt.msecsTo(now) > kSilenceTimeoutMs) {
        qWarning() << "MainWindow: ИВС молчит дольше" << kSilenceTimeoutMs
                   << "мс — считаем отключённым";
        m_iwsLastError = QString("Нет ответа более %1 сек — проверьте кабель/устройство")
                              .arg(kSilenceTimeoutMs / 1000);
        disconnectSensor(AutoConnector::DEVICE_IWS);
        anyChanged = true;
    }

    // Если шторка сейчас открыта на одном из этих датчиков — освежаем её,
    // чтобы причина/статус не оставались устаревшими.
    if (anyChanged && m_sensorPopup && m_sensorPopup->isVisible())
        populateSensorPopupContent();
}

void MainWindow::onAutoConnectorStarted()
{
    ui->btnConnectSensors->setEnabled(false);

    const AutoConnector::DeviceType singleTarget = m_autoConnector->singleSearchTarget();
    const QString title = (singleTarget == AutoConnector::DEVICE_UNKNOWN)
        ? "Автопоиск датчиков"
        : QString("Поиск: %1").arg(sensorDisplayName(singleTarget));

    statusBar()->showMessage(title + "...", 0);

    // Сбрасываем стили к дефолтным (зеленым) на случай, если прошлая попытка завершилась ошибкой
    m_toastTitle->setText(title);
    m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #1C1F22; border: none; background: transparent;");
    m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #0F6B4F; border: none; background: transparent;");
    m_toastProgress->setStyleSheet(
        "QProgressBar { background-color: #EFF1F1; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background-color: #0F6B4F; border-radius: 3px; }"
        );

    m_toastPercent->setText("0%");
    m_toastProgress->setValue(0);
    m_toastText->setText("Инициализация портов...");

    m_toastCloseBtn->show();
    m_toastCloseBtn->raise();

    showToast();
}

void MainWindow::onAutoConnectorProgress(int current, int total)
{
    if (total > 0) {
        int percent = (current * 100) / total;
        m_toastProgress->setValue(percent);
        m_toastPercent->setText(QString("%1%").arg(percent));
    }
}

void MainWindow::onAutoConnectorLog(const QString &msg)
{
    QString cleanMsg = msg.trimmed();

    // Игнорируем пустые строки и чисто декоративные разделители от AutoConnector
    if (cleanMsg.isEmpty() || cleanMsg.startsWith("===") || cleanMsg == "---") {
        return;
    }

    // Очищаем дефисы из строк типа "--- Проверка порта COM1 (1/5) ---"
    if (cleanMsg.startsWith("---")) {
        cleanMsg.replace("-", "");
        cleanMsg = cleanMsg.trimmed();
    }

    m_toastText->setText(cleanMsg);
}
