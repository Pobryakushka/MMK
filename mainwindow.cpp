#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SourceData.h"
#include "AlgorithmsCalc.h"
#include "MeasurementResults.h"
#include "GroundMeteoParams.h"
#include "LandingCalculation.h"
#include "amshandler.h"
#include "databasemanager.h"
#include "CoordHelper.h"
#include "surfacemeteosaver.h"
#include <QDateTime>
#include <QTimer>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMessageBox>
#include <QtPositioning/QGeoCoordinate>
#include <QPushButton>
#include <QCheckBox>
#include <QIcon>
#include <QStatusBar>
#include <QDebug>

// ====================================================================
// НАСТРОЙКА ПРОТОКОЛА IWS
// ====================================================================
// Измените эту константу для выбора протокола связи с IWS:
// 0 = UMB Protocol (текущие значения)
// 1 = Modbus RTU (средние значения) - рекомендуется для IWS
// ====================================================================
const int IWS_PROTOCOL = 1;  // 1 = Modbus RTU по умолчанию
// ====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pollTimer(nullptr)
    , serialPort(nullptr)
    , sensorSettingsDialog(nullptr)
    , sourceDataInstance(nullptr)
    , m_useManualDateTime(false)
    , m_isEditingDateTime(false)
    , m_manualDateTimeSet(false)
    , m_mapCoordinatesEnabled(false)
    , m_gnssEnabled(false)
    , m_manualInputEnabled(false)
    , m_gnssHandler(new GNSSHandler(this))
    , m_gnssComPort("")
    , m_gnssBaudRate(19200)
    , m_amsHandler(nullptr)
    , m_amsComPort("")
    , m_amsBaudRate(9600)
    , m_iwsWarmupTimer(nullptr)
    , m_iwsWarmupDone(false)
    , m_pendingIwsRecordId(-1)
    , m_iwsFinalRequestTimer(nullptr)
    , m_surfaceMeteoSaver(new SurfaceMeteoSaver(this))
{
    ui->setupUi(this);

    configureAmsDatabase();

    fMapView = new FormMapView(this);

    setupMapCoordinatesButton();
    setupGnssCheckbox();

//    setupGnssSettingsButton();

    updateMapCoordinatesButtonStyle();

    // Подключение сигналов к слотам
    connect(ui->btnFunctionalControl, &QPushButton::clicked, this, &MainWindow::onFunctionalControlClicked);
    connect(ui->btnWorkRegulation, &QPushButton::clicked, this, &MainWindow::onWorkRegulationClicked);
    connect(ui->btnManualInput, &QPushButton::clicked, this, &MainWindow::onManualInputClicked);
    connect(ui->btnInitialData, &QPushButton::clicked, this, &MainWindow::onInitialDataClicked);
    connect(ui->btnCalculations, &QPushButton::clicked, this, &MainWindow::onCalculationsClicked);
    connect(ui->btnMeasurementResults, &QPushButton::clicked, this, &MainWindow::onMeasurementResultsClicked);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(ui->cbWorkMode, &QCheckBox::stateChanged, this, &MainWindow::onWorkModeChanged);
    connect(ui->cbStandbyMode, &QCheckBox::stateChanged, this, &MainWindow::onStandbyModeChanged);
    connect(ui->btnSensorSettings, &QPushButton::clicked, this, &MainWindow::onSensorSettingsClicked);
    connect(ui->btnSyncTime, &QPushButton::clicked, this, &MainWindow::onSyncTimeClicked);
    connect(ui->editDateTime, &QLineEdit::editingFinished, this, &MainWindow::onDateTimeEditingFinished);
    connect(ui->editDateTime, &QLineEdit::textEdited, this, &MainWindow::onDateTimeEditingStarted);

    // GNSS сигналы
    connect(m_gnssHandler, &GNSSHandler::dataReceived, this, &MainWindow::onGnssDataReceived);
    connect(m_gnssHandler, &GNSSHandler::connected, this, &MainWindow::onGnssConnected);
    connect(m_gnssHandler, &GNSSHandler::disconnected, this, &MainWindow::onGnssDisconnected);
    connect(m_gnssHandler, &GNSSHandler::errorOccurred, this, &MainWindow::onGnssError);
    connect(m_gnssHandler, &GNSSHandler::nmeaReceived, this, &MainWindow::onNmeaReceived);

    connect(m_gnssHandler, &GNSSHandler::coordinatesUpdated, this, [this](int id){
        statusBar()->showMessage(
                    QString("GNSS: координаты обновлены в БД (record_id: %1)").arg(id), 5000);
    });
    connect(m_gnssHandler, &GNSSHandler::dbError, this, [this](const QString &err){
        qWarning() << "MainWindow: Ошибка GNSS БД:" << err;
        statusBar()->showMessage("Ошибка GNSS БД: " + err, 8000);
    });

    connect(ui->editLatitude, &QLineEdit::textEdited, this, [this](){ onCoordTextEdited(ui->editLatitude); });
    connect(ui->editLongitude, &QLineEdit::textEdited, this, [this](){ onCoordTextEdited(ui->editLongitude); });

    setCoordField(ui->editLatitude, 55.7558);
    setCoordField(ui->editLongitude, 37.6173);

    ui->editAltitude->setEnabled(false);
    ui->editDirectionAngle->setEnabled(false);
    ui->editLatitude->setEnabled(false);
    ui->editLongitude->setEnabled(false);
    ui->editPitchAngle->setEnabled(false);
    ui->editRollAngle->setEnabled(false);

    // Настройка таймера для обновления времени
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timer->start(1000);

    // Первоначальная установка времени
    updateDateTime();

    connect(&qcp, &QmlCoordinateProxy::mapTypesChanged, [=](const QStringList &list) {
        qDebug() << "mapTypesChanged received with" << list.size() << "items:" << list;
        ui->comboBox_mapTypes->clear();
        ui->comboBox_mapTypes->addItems(list);
    });

    connect(&qcp, &QmlCoordinateProxy::coordinateFromChanged, [=](const QGeoCoordinate &c){
        if (m_mapCoordinatesEnabled){
            updateCoordinatesFromMap(c.latitude(), c.longitude());
        }
    });

    connect(ui->comboBox_mapTypes, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            &qcp, &QmlCoordinateProxy::setCurrentMapType);

    ui->quickWidget->engine()->rootContext()->setContextProperty("coord", &qcp);
    ui->quickWidget->setSource(QUrl("qrc:/qml/Main.qml"));
    createMapComponent("osm");

    // Создаём окно настроек датчиков
    sensorSettingsDialog = new SensorSettings(this);
    connect(sensorSettingsDialog, &SensorSettings::connectRequested, this, &MainWindow::onConnectRequested);
    connect(sensorSettingsDialog, &SensorSettings::disconnectRequested, this, &MainWindow::onDisconnectRequested);

    connect(sensorSettingsDialog, &SensorSettings::gnssConnectRequested, this, &MainWindow::onGnssConnectFromSettings);
    connect(sensorSettingsDialog, &SensorSettings::gnssDisconnectRequested, this, &MainWindow::onGnssDisconnectFromSettings);

    connect(sensorSettingsDialog, &SensorSettings::amsConnectRequested, this, &MainWindow::onAmsConnectFromSettings);
    connect(sensorSettingsDialog, &SensorSettings::amsDisconnectRequested, this, &MainWindow::onAmsDisconnectFromSettings);

    connect(sensorSettingsDialog, &SensorSettings::binsConnectRequested, this, &MainWindow::onBinsConnectFromSettings);
    connect(sensorSettingsDialog, &SensorSettings::binsDisconnectRequested, this, &MainWindow::onBinsDisconnectFromSettings);

    m_functionalControlDialog = new FunctionalControlDialog(this);
    m_functionalControlDialog->setSensorType(FunctionalControlDialog::AMS);

    connect(m_functionalControlDialog, &FunctionalControlDialog::refreshRequested,
            this, &MainWindow::onFunctionalControlClicked);

    // Таймер периодического опроса АМС — каждые 5 минут
    m_functionalControlPollTimer = new QTimer(this);
    m_functionalControlPollTimer->setInterval(5 * 60 * 1000);
    connect(m_functionalControlPollTimer, &QTimer::timeout, this, [this]() {
        if (m_amsHandler && m_amsHandler->isConnected()
            && m_amsHandler->getMeasurementStatus() != STATUS_RUNNING) {
            qDebug() << "MainWindow: Плановый опрос функционального контроля АМС";
            m_amsHandler->requestFunctionalControl();
        }
    });

    // Создаём постоянный экземпляр SourceData (внутри создастся GroundMeteoParams)
    // Не показываем его, просто держим в памяти для доступа к данным
    sourceDataInstance = new SourceData(this);
    qDebug() << "SourceData instance created (with GroundMeteoParams inside)";

    m_amsHandler = new AMSHandler(this);
    setupAmsHandler();
//    configureAmsDatabase();

    m_binsHandler = new BINSHandler(this);
    setupBinsHandler();

    // Таймер прогрева ИВС (3 минуты)
    m_iwsWarmupTimer = new QTimer(this);
    m_iwsWarmupTimer->setSingleShot(true);
    connect(m_iwsWarmupTimer, &QTimer::timeout, this, &MainWindow::onIwsWarmupFinished);

    // rbAvg3 доступен по умолчанию; блокируется только после подключения ИВС
    // на 3 минуты прогрева (см. onConnectRequested / onDisconnectRequested)
    ui->rbAvg3->setEnabled(true);
    ui->rbAvg3->setToolTip("");

    // Инициализация панели статуса датчиков
    updateSensorStatusPanel();
}

MainWindow::~MainWindow()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    if (pollTimer) {
        pollTimer->stop();
    }
    if (m_amsHandler && m_amsHandler->isConnected()) {
        m_amsHandler->disconnectFromAMS();
    }
    if (m_binsHandler && m_binsHandler->isConnected()) {
        m_binsHandler->disconnectFromBINS();
    }
    delete ui;
}

// =================================================
// Методы работы с координатами
// =================================================

void MainWindow::setCoordField(QLineEdit *edit, double dec_deg)
{
    if (!edit) return;

    // Конвертируем десятичные градусы
    QString dmsString = CoordHelper::toDisplayDMS(dec_deg);
    edit->setText(dmsString);
}

double MainWindow::getCoordField(QLineEdit *edit, bool &ok) const
{
    ok = false;
    if (!edit) return 0.0;

    QString text = edit->text().trimmed();
    if (text.isEmpty()) return 0.0;

    double degrees = 0.0;
    ok = CoordHelper::parseDMS(text, degrees);

    return ok ? degrees : 0.0;
}

void MainWindow::onCoordTextEdited(QLineEdit *edit)
{
    if (!edit) return;

    static bool isProcessing = false;
    if (isProcessing) return;
    isProcessing = true;

    QString rawText = edit->text();
    int cursorPos = edit->cursorPosition();

    QString formatted = CoordHelper::formatInput(rawText, cursorPos);

    if (formatted != rawText) {
        edit->setText(formatted);
        edit->setCursorPosition(cursorPos);
    }

    isProcessing = false;
}

void MainWindow::setupMapCoordinatesButton()
{
    // Кнопка теперь в UI файле, просто настраиваем иконку и подключаем сигнал
    QIcon markerIcon(":/dat/images/marker.png");
    ui->btnMapCoordinates->setIcon(markerIcon);
    ui->btnMapCoordinates->setIconSize(QSize(32, 32));

    connect(ui->btnMapCoordinates, &QPushButton::clicked, this, &MainWindow::onMapCoordinatesToggled);
}

void MainWindow::setupGnssCheckbox()
{
    // Чекбокс теперь в UI файле, просто подключаем сигнал
    connect(ui->checkboxGnss, &QCheckBox::toggled, this, &MainWindow::onGnssCheckboxToggled);
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

void MainWindow::updateGnssMarkerOnMap(double latitude, double longitude)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
        QMetaObject::invokeMethod(main, "updateGnssMarker", Qt::DirectConnection,
                                  Q_ARG(QVariant, latitude),
                                  Q_ARG(QVariant, longitude),
                                  Q_ARG(QVariant, m_gnssEnabled));
    }
}

void MainWindow::updateMapCoordinatesButtonStyle()
{
    QIcon markerIcon(":/dat/images/marker.png");
    ui->btnMapCoordinates->setIcon(markerIcon);
    ui->btnMapCoordinates->setIconSize(QSize(32, 32));

    if (m_mapCoordinatesEnabled) {
        ui->btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: #BBBBBB;"
            "   border: 2px solid #555;"
            "   border-radius: 20px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #AAAAAA;"
            "   border: 2px solid #444;"
            "}"
        );
        ui->btnMapCoordinates->setToolTip("Режим координат с карты активен (нажмите для отключения)");
    } else {
        ui->btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: white;"
            "   border: 2px solid gray;"
            "   border-radius: 20px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #f0f0f0;"
            "}"
        );
        ui->btnMapCoordinates->setToolTip("Использовать координаты с карты (нажмите для включения)");
    }
}

void MainWindow::onMapCoordinatesToggled()
{
    m_mapCoordinatesEnabled = !m_mapCoordinatesEnabled;
    updateMapCoordinatesButtonStyle();

    if (m_mapCoordinatesEnabled) {
        checkAndDisableConflictingSources("map");
        updateCoordinateSource("Карта");
    } else {
        updateCoordinateSource("Нет");
    }

    updateFieldsEditability();
    emit mapCoordinatesModeChanged(m_mapCoordinatesEnabled);

    // Выводим сообщение о смене режима
    QString status = m_mapCoordinatesEnabled ?
        "Режим координат с карты ВКЛЮЧЕН" :
        "Режим координат с карты ВЫКЛЮЧЕН";
    statusBar()->showMessage(status, 3000);
}

void MainWindow::onGnssCheckboxToggled(bool checked)
{
    if (checked) {
        if (m_gnssComPort.isEmpty()) {
            qDebug() << "MainWindow: COM-порт не настроен, открываем настройки...";
            ui->checkboxGnss->setChecked(false);
            QMessageBox::information(this, "Настройки GNSS",
                "Пожалуйста, настройте параметры подключения GNSS через кнопку настроек ⚙");
            return;
        }

        connectToGnss();
    } else {
        disconnectFromGnss();
    }
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
        QMessageBox::warning(this, "Ошибка", "Не удалось подключиться к GNSS приемнику");
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
    ui->checkboxGnss->setStyleSheet(
        "QCheckBox { background-color: #DCDCDC; padding: 5px 10px; border: 2px solid #666; border-radius: 5px; }"
    );
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

    ui->checkboxGnss->setStyleSheet(
        "QCheckBox { background-color: #F0F0F0; padding: 5px 10px; border: 1px solid #AAAAAA; border-radius: 5px; }"
    );
    updateFieldsEditability();
    updateGnssStatusLabel(false);
}

void MainWindow::onGnssError(const QString &error)
{
    qDebug() << "Ошибка GNSS:" << error;

    if (m_gnssHandler->isConnected()) {
        statusBar()->showMessage("Ошибка GNSS: " + error, 5000);
    }
}

void MainWindow::checkAndDisableConflictingSources(const QString &activeSource)
{
    if (activeSource == "map") {
        // Отключаем GNSS и ручной ввод
        if (m_gnssEnabled) {
            ui->checkboxGnss->setChecked(false);
        }
        m_manualInputEnabled = false;
    } else if (activeSource == "gnss") {
        // Отключаем карту и ручной ввод
        if (m_mapCoordinatesEnabled) {
            ui->btnMapCoordinates->setChecked(false);
            m_mapCoordinatesEnabled = false;
            updateMapCoordinatesButtonStyle();
        }
        m_manualInputEnabled = false;
    } else if (activeSource == "manual") {
        // Отключаем карту и GNSS
        if (m_mapCoordinatesEnabled) {
            ui->btnMapCoordinates->setChecked(false);
            m_mapCoordinatesEnabled = false;
            updateMapCoordinatesButtonStyle();
        }
        if (m_gnssEnabled) {
            ui->checkboxGnss->setChecked(false);
        }
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
    });

    connect(m_amsHandler, &AMSHandler::functionalControlRequested,
            this, [this]() {
        // При progress==-2 показываем окно ФК
        // showEvent внутри диалога сам инициирует запрос
        m_functionalControlDialog->show();
        m_functionalControlDialog->raise();
        m_functionalControlDialog->activateWindow();
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
}

void MainWindow::configureAmsDatabase()
{
    // Настройка подключения к БД
    // TODO: Загрузить параметры из конфигурационного файла или настроек
    QString dbHost = "localhost";
    int dbPort = 5432;
    QString dbName = "MMK";
    QString dbUser = "postgres";
    QString dbPassword = "123";

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
        QMessageBox::warning(this, "Ошибка БД",
        "Не удалось подключиться к базе данных. \nПроверьте параметры подключения.");
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
        QMessageBox::warning(this, "Ошибка",
            "Не удалось подключиться к АМС. Проверьте порт и настройки.");
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

    if (sensorSettingsDialog) {
        sensorSettingsDialog->setAmsConnectionStatus("Подключено", true);
        sensorSettingsDialog->setAmsConnectionEnabled(false);
    }

    statusBar()->showMessage("АМС подключена успешно", 5000);
    updateAmsStatusLabel(true);

    // Запускаем периодический опрос функционального контроля
    m_functionalControlPollTimer->start();

    // Сразу выполняем первый опрос через 2 секунды после подключения
    QTimer::singleShot(2000, this, [this]() {
        if (m_amsHandler && m_amsHandler->isConnected()) {
            m_amsHandler->requestFunctionalControl();
        }
    });
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

    m_functionalControlPollTimer->stop();
    if (m_functionalControlDialog->isVisible()) {
        m_functionalControlDialog->setDisconnectedState();
    }
}

void MainWindow::onAmsError(const QString &error)
{
    qWarning() << "MainWindow: Ошибка АМС:" << error;
    statusBar()->showMessage("Ошибка АМС: " + error, 10000);

    // Передаём ошибку в диалог функционального контроля если он открыт
    if (m_functionalControlDialog->isVisible()) {
        m_functionalControlDialog->setErrorState(error);
    }

    if (error.contains("Таймаут") || error.contains("контрольной суммы")) {
        qDebug() << "MainWindow: Ошибка связи с АМС, будет повторная попытка";
    } else {
        QMessageBox::warning(this, "Ошибка АМС", error);
    }
}

void MainWindow::onAmsStatusMessage(const QString &message)
{
    qDebug() << "MainWindow: Статус АМС:" << message;
    statusBar()->showMessage("АМС: " + message, 3000);
}

//void MainWindow::onAmsMeasurementProgress(int percent, float angle)
//{
//    if (percent >= 0 && percent <= 100) {
//        qDebug() << "MainWindow: Прогресс измерений АМС:" << percent << "%, угол:" << angle << "°";
//        statusBar()->showMessage(
//            QString("АМС: Измерения %1% (угол: %2°)").arg(percent).arg(angle, 0, 'f', 1),
//            2000);
//    } else if (percent == -1) {
//        qDebug() << "MainWindow: Измерения АМС завершены успешно";
//        statusBar()->showMessage("АМС: Измерения завершены успешно", 5000);

//        // Можно запросить данные
//        QTimer::singleShot(500, this, [this]() {
//            if (m_amsHandler && m_amsHandler->isConnected()) {
//                m_amsHandler->requestAvgWind();
//                m_amsHandler->requestActualWind();
//                m_amsHandler->requestMeasuredWind();
//            }
//        });
//    } else if (percent == -2) {
//        qWarning() << "MainWindow: Ошибка при измерениях АМС";
//        statusBar()->showMessage("АМС: Ошибка при измерениях", 5000);
//        QMessageBox::warning(this, "АМС", "Ошибка при выполнении измерений");
//    }
//}

void MainWindow::onAmsDataWritten(int recordId)
{
    qDebug() << "MainWindow: Данные АМС записаны в архив, record_id:" << recordId;
    statusBar()->showMessage(
        QString("АМС: Данные записаны в архив (ID: %1), запрашиваем ИВС...").arg(recordId), 5000);

    if (serialPort && serialPort->isOpen()) {
        requestIwsDataForRecord(recordId);
    } else {
        qWarning() << "MainWindow: ИВС не подключён — surface_meteo не будет заполнена";
        statusBar()->showMessage("Предупреждение: ИВС не подключён, приземные данные не сохранены", 8000);
    }
}

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

    if (m_surfaceMeteoSaver->hasData()) {
        bool ok = m_surfaceMeteoSaver->saveToDatabase(recordId);
        if (ok) {
            statusBar()->showMessage(
                QString("ИВС: приземные данные сохранены (ID: %1)").arg(recordId), 5000);
        }
    } else {
        qWarning() << "MainWindow: После запроса ИВС данные всё ещё неполные";
    }
}

void MainWindow::onAmsDatabaseError(const QString &error)
{
    qCritical() << "MainWindow: Ошибка БД АМС:" << error;
    statusBar()->showMessage("Ошибка БД АМС: " + error, 10000);
    QMessageBox::critical(this, "Ошибка базы данных АМС",
        "Не удалось записать данные в базу данных:\n" + error);
}

void MainWindow::onFunctionalControlClicked()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        m_functionalControlDialog->setDisconnectedState();
        m_functionalControlDialog->show();
        m_functionalControlDialog->raise();
        m_functionalControlDialog->activateWindow();
        return;
    }

    // Просто показываем диалог — showEvent внутри него сам вызовет
    // refreshRequested → onFunctionalControlRefresh → requestFunctionalControl
    m_functionalControlDialog->show();
    m_functionalControlDialog->raise();
    m_functionalControlDialog->activateWindow();
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
    ui->progressBarMeasurement->setValue(displayPercent);
    ui->lblProgressPercent->setText(QString("%1%").arg(displayPercent));
    ui->lblRpvAngle->setText(QString("%1°").arg(angle, 0, 'f', 1));
    // Синхронизируем поле положения РПВ
    ui->editRPVPosition->setText(QString::number(angle, 'f', 1));

    statusBar()->showMessage(
        QString("Измерение: %1%, Угол РПВ: %2°").arg(displayPercent).arg(angle, 0, 'f', 1)
    );
}

void MainWindow::onAmsMeasurementCompleted(int recordId)
{
    qDebug() << "MainWindow: Измерение завершено успешно, ID записи:" << recordId;

    QMessageBox::information(this, "Успех",
        QString("Измерение завершено успешно!\n\nID записи в БД: %1\n\n"
                "Результаты сохранеRны и доступны в разделе 'Результаты измерений'.")
        .arg(recordId));

    // Обновляем UI
    ui->lblStatus->setText("ГОТОВ");
    ui->lblStatus->setStyleSheet("font-weight: bold; font-size: 14pt; border: 2px solid #888; padding: 5px; border-radius: 4px; background-color: #E8E8E8; color: #222;");

    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);

    // Скрываем прогрессбар
    ui->measurementProgressWidget->setVisible(false);
    ui->progressBarMeasurement->setValue(0);

    statusBar()->showMessage("Измерение завершено успешно", 10000);

    if (m_gnssHandler && m_gnssHandler->isConnected() && m_gnssHandler->hasValidFix()) {
        m_gnssHandler->updateCoordinatesInDb(recordId);
    } else {
        qDebug() << "MainWindow: GNSS недоступен - в БД записываются координаты из UI-полей";
    }
}

void MainWindow::onAmsMeasurementFailed(const QString &reason)
{
    qWarning() << "MainWindow: Измерение не удалось:" << reason;

    QMessageBox::critical(this, "Ошибка измерения",
        QString("Измерение не было завершено:\n\n%1").arg(reason));

    // Обновляем UI
    ui->lblStatus->setText("ОШИБКА");
    ui->lblStatus->setStyleSheet("font-weight: bold; font-size: 14pt; border: 2px solid #333; padding: 5px; border-radius: 4px; background-color: #CCCCCC; color: #000;");

    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);

    // Скрываем прогрессбар
    ui->measurementProgressWidget->setVisible(false);
    ui->progressBarMeasurement->setValue(0);

    statusBar()->showMessage("Ошибка: " + reason, 10000);
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
        QMessageBox::warning(this, "Ошибка",
            "Не удалось отправить промежуточные данные в АМС.");
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
        QMessageBox::warning(this, "Ошибка", "Не удалось подключиться к БИНС. Проверьте порт или настройки.");
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

    // Обновляем поля в интерфейсе
    ui->editDirectionAngle->setText(QString::number(data.heading, 'f', 2));
    ui->editRollAngle->setText(QString::number(data.roll, 'f', 2));
    ui->editPitchAngle->setText(QString::number(data.pitch, 'f', 2));

    // Обновляем строку состояния
    statusBar()->showMessage(
                QString("БИНС: Курс %1 град. | Крен %2 град. | Тангаж %3 град.")
                .arg(data.heading, 0, 'f', 1)
                .arg(data.roll, 0, 'f', 1)
                .arg(data.pitch, 0, 'f', 1),
                2000);
}

void MainWindow::updateCoordinateSource(const QString &source)
{
    QString message = "Источник координат: " + source;
    statusBar()->showMessage(message, 3000);
    qDebug() << message;
}

void MainWindow::updateFieldsEditability()
{
    // Поля редактируемы только если все источники выключены (ручной ввод)
    bool fieldsEditable = !m_mapCoordinatesEnabled && !m_gnssEnabled;

    ui->editLatitude->setReadOnly(!fieldsEditable);
    ui->editLongitude->setReadOnly(!fieldsEditable);
    ui->editAltitude->setReadOnly(!fieldsEditable);

    // Визуальная индикация
    QString style = fieldsEditable ? "" : "background-color: #F5F5F5;";
    ui->editLatitude->setStyleSheet(style);
    ui->editLongitude->setStyleSheet(style);
    ui->editAltitude->setStyleSheet(style);
}

void MainWindow::updateCoordinatesFromMap(double latitude, double longitude)
{
    setCoordField(ui->editLatitude, latitude);
    setCoordField(ui->editLongitude, longitude);

    // Передаем сигнал другим окнам
    emit coordinatesUpdatedFromMap(latitude, longitude);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // Кнопки теперь в layout панели статуса, перемещение не требуется
}

void MainWindow::onSensorSettingsClicked()
{
    if (sensorSettingsDialog) {
        sensorSettingsDialog->show();
        sensorSettingsDialog->raise();
        sensorSettingsDialog->activateWindow();
    }
}

void MainWindow::onConnectRequested()
{
    if (sensorSettingsDialog->getIwsComPort().isEmpty() ||
        sensorSettingsDialog->getIwsComPort() == "Нет доступных портов") {
        QMessageBox::warning(this, "Ошибка", "Нет доступных COM-портов");
        return;
    }

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

    // Настраиваем порт из настроек
    serialPort->setPortName(sensorSettingsDialog->getIwsComPort());
    serialPort->setBaudRate(sensorSettingsDialog->getIwsBaudRate());
    serialPort->setDataBits(sensorSettingsDialog->getIwsDataBits());
    serialPort->setParity(sensorSettingsDialog->getIwsParity());
    serialPort->setStopBits(sensorSettingsDialog->getIwsStopBits());
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Пытаемся открыть порт
    if (serialPort->open(QIODevice::ReadWrite)) {
        sensorSettingsDialog->setIwsConnectionStatus("Подключено", true);
        sensorSettingsDialog->setIwsConnectionEnabled(false);
        updateIwsStatusLabel(true);

        // Запускаем таймер прогрева ИВС (3 минуты)
        m_iwsWarmupDone = false;
        ui->rbAvg3->setEnabled(false);
        ui->rbAvg3->setToolTip("Режим 3 мин. станет доступен через 3 минуты после подключения ИВС");
        if (ui->rbAvg3->isChecked()) {
            ui->rbAvg6->setChecked(true);
        }
        m_iwsWarmupTimer->start(3 * 60 * 1000);
        statusBar()->showMessage("ИВС подключён. Режим усреднения 3 мин. станет доступен через 3 минуты.", 8000);

        // Настраиваем протокол в GroundMeteoParams
        GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
        if (meteoParams) {
            // ИСПОЛЬЗУЕМ КОНСТАНТУ IWS_PROTOCOL (определена в начале файла)
            int protocolToUse = IWS_PROTOCOL;

            // Если нужно, можно переопределить из настроек
            // protocolToUse = sensorSettingsDialog->getIwsProtocolIndex();

            GroundMeteoParams::RS485Protocol protocol =
                (protocolToUse == 0) ?
                GroundMeteoParams::UMB_PROTOCOL :
                GroundMeteoParams::MODBUS_RTU;

            meteoParams->setProtocol(protocol);

            // Получаем адрес устройства из настроек
            quint8 deviceAddress = sensorSettingsDialog->getIwsDeviceAddress();
            meteoParams->setDeviceAddress(deviceAddress);

            qDebug() << "IWS: Configured"
                     << (protocolToUse == 0 ? "UMB" : "Modbus RTU")
                     << "protocol, address" << QString("0x%1").arg(deviceAddress, 2, 16, QChar('0'))
                     << (protocolToUse == 1 ? "(AVERAGE values)" : "");
        } else {
            qDebug() << "GroundMeteoParams not created yet. Will be configured when 'Initial Data' is opened.";
        }

        // Запускаем таймер опроса
        if (!pollTimer) {
            pollTimer = new QTimer(this);
            connect(pollTimer, &QTimer::timeout, this, &MainWindow::pollMeteoStation);
        }
        pollTimer->start(sensorSettingsDialog->getIwsPollInterval() * 1000);

        qDebug() << "RS485 connected on" << sensorSettingsDialog->getIwsComPort();
    } else {
        QMessageBox::critical(this, "Ошибка подключения",
                            QString("Не удалось открыть порт: %1").arg(serialPort->errorString()));
        sensorSettingsDialog->setIwsConnectionStatus("Ошибка подключения", false);
    }
}

void MainWindow::onIwsWarmupFinished()
{
    m_iwsWarmupDone = true;
    ui->rbAvg3->setEnabled(true);
    ui->rbAvg3->setToolTip("");
    statusBar()->showMessage("ИВС: режим усреднения 3 минуты теперь доступен", 5000);
    qDebug() << "MainWindow: ИВС прогрев завершён, rbAvg3 разблокирован";
}

void MainWindow::onDisconnectRequested()
{
    // Сбрасываем прогрев ИВС; ИВС отключён — разблокируем rbAvg3
    if (m_iwsWarmupTimer) {
        m_iwsWarmupTimer->stop();
    }
    m_iwsWarmupDone = false;
    ui->rbAvg3->setEnabled(true);
    ui->rbAvg3->setToolTip("");

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

        if (sensorSettingsDialog) {
            sensorSettingsDialog->setIwsConnectionStatus(
                QString("Ошибка: %1").arg(serialPort->errorString()), false);
        }

        if (serialPort->isOpen()) {
            onDisconnectRequested();
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

void MainWindow::createMapComponent(const QString &pluginName)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
        qcp.setMapTypes(QStringList());
        QMetaObject::invokeMethod(main, "createMapComponent", Qt::DirectConnection,
                                  Q_ARG(QVariant, pluginName));
        setupMapItems(main);
    }
}

void MainWindow::setupMapItems(QQuickItem *item)
{
    if (item) {
        item->update();
    }
}

void MainWindow::updateDateTime()
{
    if (m_isEditingDateTime) {
        return;
    }

    if (ui->editDateTime->hasFocus()) {
        return;
    }

    QString timeString;

    if (m_manualDateTimeSet) {
        // Используем вручную установленное время и продолжаем его инкремент
        m_manualDateTime = m_manualDateTime.addSecs(1);
        timeString = m_manualDateTime.toString("dd.MM.yyyy hh:mm:ss");
    }
    else if (m_gnssEnabled && m_gnssHandler->isConnected()) {
        // Используем время из GNSS
        GNSSData data = m_gnssHandler->getCurrentData();
        if (data.timestamp.isValid()){
            timeString = data.timestamp.toString("dd.MM.yyyy hh:mm:ss");
        } else {
            // Если время из GNSS не валидно, то используем системное
            timeString = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
        }
    } else {
        // Используем системное время
        timeString = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
    }

    ui->editDateTime->setText(timeString);
}

void MainWindow::onSyncTimeClicked()
{
    // Синхронизируем с системным временем
    m_manualDateTime = QDateTime::currentDateTime();
    ui->editDateTime->setText(m_manualDateTime.toString("dd.MM.yyyy hh:mm:ss"));

    // Помечаем, что время было установлено вручную
    m_manualDateTimeSet = true;
    m_useManualDateTime = true;

    statusBar()->showMessage("Время синхронизировано с системным", 3000);

    if (!m_manualInputEnabled) {
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #E0E0E0;");
    }
}

void MainWindow::onDateTimeEditingFinished()
{
    m_isEditingDateTime = false;

    if (!m_useManualDateTime) {
        return; // Валидация только в режиме ручного ввода
    }

    QString inputText = ui->editDateTime->text();
    QDateTime newDateTime = QDateTime::fromString(inputText, "dd.MM.yyyy hh:mm:ss");

    if (newDateTime.isValid()) {
        // Время корректно введено
        m_manualDateTime = newDateTime;
        m_manualDateTimeSet = true;
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 10pt; background-color: #E0E0E0;");
    } else {
        // Время введено некоррктно - возвращаем предыдущее значение
        ui->editDateTime->setText(m_manualDateTime.toString("dd.MM.yyyy hh:mm:ss"));

        // Кратковременная визуальная индикация ошибки
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #FFB6C1;");
        QTimer::singleShot(500, this, [this]() {
            ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 10pt; background-color: #E0E0E0;");
        });
    }
}

void MainWindow::onDateTimeEditingStarted()
{
    if (m_useManualDateTime) {
        m_isEditingDateTime = true;
    }
}

//void MainWindow::onFunctionalControlClicked()
//{
//    // Здесь будет логика функционального контроля
//}

void MainWindow::onWorkRegulationClicked()
{
    // Здесь будет логика регламента работы
}

void MainWindow::onManualInputClicked()
{
    bool enabled = ui->editAltitude->isEnabled() &&
                   ui->editDirectionAngle->isEnabled() &&
                   ui->editLatitude->isEnabled() &&
                   ui->editLongitude->isEnabled() &&
                   ui->editPitchAngle->isEnabled() &&
                   ui->editRollAngle->isEnabled();

    ui->editAltitude->setEnabled(!enabled);
    ui->editDirectionAngle->setEnabled(!enabled);
    ui->editLatitude->setEnabled(!enabled);
    ui->editLongitude->setEnabled(!enabled);
    ui->editPitchAngle->setEnabled(!enabled);
    ui->editRollAngle->setEnabled(!enabled);
    ui->editDateTime->setReadOnly(enabled);

    m_manualInputEnabled = !enabled;

    if (m_manualInputEnabled) {
        checkAndDisableConflictingSources("manual");
        updateCoordinateSource("Ручной ввод");

        m_useManualDateTime = true;
        m_isEditingDateTime = false;

        // Если время еще не было установлено вручную, берем текущее
        if (!m_manualDateTimeSet) {
            QDateTime currentDisplayed = QDateTime::fromString(ui->editDateTime->text(), "dd.MM.yyyy hh:mm:ss");
            if (currentDisplayed.isValid()) {
                m_manualDateTime = currentDisplayed;
            } else {
                m_manualDateTime = QDateTime::currentDateTime();
            }
        }
        // Визуальная индикация редактируемого поля
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 10pt; background-color: #E0E0E0;");
    } else {
        // При выходе из режима ручного ввода возвращаемся к автоматическому времени
        m_manualDateTimeSet = true;
        m_isEditingDateTime = false;
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 10pt;");

        QDateTime currentDisplayed = QDateTime::fromString(ui->editDateTime->text(), "dd.MM.yyyy hh:mm:ss");
        if (currentDisplayed.isValid()) {
            m_manualDateTime = currentDisplayed;
        }

        updateDateTime();
    }
}

void MainWindow::onInitialDataClicked()
{
    // Используем постоянный экземпляр, чтобы GroundMeteoParams (и его данные) жили всё время
    if (sourceDataInstance) {
        sourceDataInstance->show();
        sourceDataInstance->raise();
        sourceDataInstance->activateWindow();
    }
}

void MainWindow::onCalculationsClicked()
{
    AlgorithmsCalculation dialog(this);
    dialog.exec();
}

void MainWindow::onMeasurementResultsClicked()
{
    MeasurementResults *dialog = new MeasurementResults(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

//    if (!DatabaseManager::instance()->isConnected()){
//        DatabaseManager::instance()->connect();
//    }

    connect(this, &MainWindow::coordinatesUpdatedFromMap,
            dialog, &MeasurementResults::updateCoordinatesFromMainWindow);

    connect(this, &MainWindow::mapCoordinatesModeChanged,
            dialog, &MeasurementResults::setMapCoordinatesMode,
            Qt::DirectConnection);

    dialog->setMapCoordinatesMode(m_mapCoordinatesEnabled || m_gnssEnabled);

    if ((m_mapCoordinatesEnabled || m_gnssEnabled) && ui->editLatitude && ui->editLongitude) {
        bool ok1, ok2;
        double lat = getCoordField(ui->editLatitude, ok1);
        double lon = getCoordField(ui->editLongitude, ok2);
        if (ok1 && ok2) {
            dialog->updateCoordinatesFromMainWindow(lat, lon);
        }
    }

    dialog->show();
}

void MainWindow::onStartClicked()
{
    // Проверяем подключение к АМС
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        QMessageBox::warning(this, "Ошибка",
            "АМС не подключен. Подключитесь к АМС через настройки датчиков.");
        return;
    }

    // Проверяем, не выполняется ли уже измерение
    if (m_amsHandler->getMeasurementStatus() == STATUS_RUNNING) {
        QMessageBox::warning(this, "Ошибка",
            "Измерение уже выполняется. Остановите текущее измерение перед запуском нового.");
        return;
    }

    // Обновляем UI
    ui->lblStatus->setText("РАБОТА");
    ui->lblStatus->setStyleSheet("font-weight: bold; font-size: 14pt; border: 2px solid #555; padding: 5px; border-radius: 4px; background-color: #D0D0D0; color: #111;");

    // Получаем параметры для запуска измерения
    WorkMode mode = ui->cbWorkMode->isChecked() ? MODE_WORKING : MODE_STANDBY;
    Litera litera = LITERA_1; // Можно добавить выбор в UI

    // Определяем время усреднения из радиокнопок
    AveragingTime avgTime = AVERAGING_3_MIN;
    if (ui->rbAvg6->isChecked())
        avgTime = AVERAGING_6_MIN;
    else if (ui->rbAvg9->isChecked())
        avgTime = AVERAGING_9_MIN;

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
        QMessageBox::warning(this, "Ошибка",
            "Не удалось запустить измерение АМС. Проверьте подключение.");

        // Возвращаем статус в ГОТОВ
        ui->lblStatus->setText("ГОТОВ");
    ui->lblStatus->setStyleSheet("font-weight: bold; font-size: 14pt; border: 2px solid #888; padding: 5px; border-radius: 4px; background-color: #E8E8E8; color: #222;");
        return;
    }

    // Блокируем кнопку старта, разблокируем стоп
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);

    // Сбрасываем и показываем прогрессбар
    ui->progressBarMeasurement->setValue(0);
    ui->lblProgressPercent->setText("0%");
    ui->lblRpvAngle->setText("0.0°");
    ui->measurementProgressWidget->setVisible(true);

    statusBar()->showMessage("Измерение АМС запущено...", 5000);
}

void MainWindow::onStopClicked()
{
    // Останавливаем измерение АМС если оно выполняется
    if (m_amsHandler && m_amsHandler->getMeasurementStatus() == STATUS_RUNNING) {
        bool stopped = m_amsHandler->stopMeasurement();

        if (stopped) {
            statusBar()->showMessage("Измерение АМС остановлено", 3000);
        } else {
            QMessageBox::warning(this, "Предупреждение",
                "Не удалось корректно остановить измерение АМС.");
        }
    }

    // Обновляем UI
    ui->lblStatus->setText("ГОТОВ");
    ui->lblStatus->setStyleSheet("font-weight: bold; font-size: 14pt; border: 2px solid #888; padding: 5px; border-radius: 4px; background-color: #E8E8E8; color: #222;");

    // Разблокируем кнопку старта, блокируем стоп
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);

    // Скрываем прогрессбар
    ui->measurementProgressWidget->setVisible(false);
    ui->progressBarMeasurement->setValue(0);
}

void MainWindow::onWorkModeChanged(int state)
{
    if (state == Qt::Checked) {
        ui->cbStandbyMode->setChecked(false);
    }
}

void MainWindow::onStandbyModeChanged(int state)
{
    if (state == Qt::Checked) {
        ui->cbWorkMode->setChecked(false);
    }
}

// ==================== Методы обновления статуса датчиков ====================

void MainWindow::updateSensorStatusPanel()
{
    updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
    updateAmsStatusLabel(m_amsHandler && m_amsHandler->isConnected());
    updateBinsStatusLabel(m_binsHandler && m_binsHandler->isConnected());
    updateIwsStatusLabel(serialPort && serialPort->isOpen());
}

void MainWindow::updateGnssStatusLabel(bool connected)
{
    if (connected) {
        ui->lblGnssStatus->setText("GNSS: подключен");
        ui->lblGnssStatus->setStyleSheet(
            "background-color: #D8D8D8; color: #111; border: 1px solid #888; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblGnssStatus->setText("GNSS: отключен");
        ui->lblGnssStatus->setStyleSheet(
            "background-color: #EBEBEB; color: #555; border: 1px solid #AAAAAA; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
}

void MainWindow::updateAmsStatusLabel(bool connected)
{
    if (connected) {
        ui->lblAmsStatus->setText("АМС: подключен");
        ui->lblAmsStatus->setStyleSheet(
            "background-color: #D8D8D8; color: #111; border: 1px solid #888; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblAmsStatus->setText("АМС: отключен");
        ui->lblAmsStatus->setStyleSheet(
            "background-color: #EBEBEB; color: #555; border: 1px solid #AAAAAA; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
}

void MainWindow::updateBinsStatusLabel(bool connected)
{
    if (connected) {
        ui->lblBinsStatus->setText("БИНС: подключен");
        ui->lblBinsStatus->setStyleSheet(
            "background-color: #D8D8D8; color: #111; border: 1px solid #888; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblBinsStatus->setText("БИНС: отключен");
        ui->lblBinsStatus->setStyleSheet(
            "background-color: #EBEBEB; color: #555; border: 1px solid #AAAAAA; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
}

void MainWindow::updateIwsStatusLabel(bool connected)
{
    if (connected) {
        ui->lblIwsStatus->setText("ИВС: подключен");
        ui->lblIwsStatus->setStyleSheet(
            "background-color: #D8D8D8; color: #111; border: 1px solid #888; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    } else {
        ui->lblIwsStatus->setText("ИВС: отключен");
        ui->lblIwsStatus->setStyleSheet(
            "background-color: #EBEBEB; color: #555; border: 1px solid #AAAAAA; font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;"
            "font-size: 10pt; padding: 4px 12px; border-radius: 4px; margin: 2px;");
    }
}
