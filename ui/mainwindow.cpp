#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ClickableLabel.h"
#include <QApplication>
#include "RpvIndicator.h"
#include "SourceData.h"
#include "Meteo11.h"
#include "calculationAlgorithms/AlgorithmsCalc.h"
#include "MeasurementResults.h"
#include "sensors/GroundMeteoParams.h"
#include "VirtualKeyboard.h"
#include "calculationAlgorithms/LandingCalculation.h"
#include "sensors/amshandler.h"
#include "databasemanager.h"
#include "CoordHelper.h"
#include "surfacemeteosaver.h"
#include <QDateTime>
#include <QTimer>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QtPositioning/QGeoCoordinate>
#include <QPushButton>
#include <QCheckBox>
#include <QIcon>
#include <QStatusBar>
#include <QDebug>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QDir>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QUrl>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QEasingCurve>
#include <cmath>


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
    , m_windProfileCalculator(new WindProfileCalculator(QStringLiteral("climatData/climat/")))
{
    ui->setupUi(this);

    setupToastUI();

    m_notifyToast = new NotificationToast(this);

    configureAmsDatabase();

    fMapView = new FormMapView(this);

    setupMapCoordinatesButton();
    setupGnssCheckbox();

    //    setupGnssSettingsButton();

    updateMapCoordinatesButtonStyle();

    // Навигация лаунчера (главный экран -> страницы -> назад)
    ui->stackedWidget->setCurrentWidget(ui->page_home);
    connect(ui->btnOpenPosition, &QPushButton::clicked, this, &MainWindow::onOpenPositionPage);
    connect(ui->btnOpenMap, &QPushButton::clicked, this, &MainWindow::onOpenMapPage);
    connect(ui->btnOpenMeasure, &QPushButton::clicked, this, &MainWindow::onOpenMeasurePage);
    connect(ui->btnBackFromPosition, &QPushButton::clicked, this, &MainWindow::onBackToHome);
    connect(ui->btnBackFromMap, &QPushButton::clicked, this, &MainWindow::onBackToHome);
    connect(ui->btnBackFromMeasure, &QPushButton::clicked, this, &MainWindow::onBackToHome);

    // Плавающие элементы управления над картой (маркер координат, тип карты, GNSS)
    ui->mapCanvas->installEventFilter(this);
    QTimer::singleShot(0, this, &MainWindow::repositionMapFloatingControls);

    // Экран "Расчёты"
    m_algorithmsCalcWidget = new AlgorithmsCalculation(this);
    ui->page_calculations->layout()->addWidget(m_algorithmsCalcWidget);
    connect(m_algorithmsCalcWidget, &AlgorithmsCalculation::backRequested,
            this, &MainWindow::onBackToHome);
    connect(m_algorithmsCalcWidget, &AlgorithmsCalculation::landingCalculationRequested,
            this, [this](){
        ui->stackedWidget->setCurrentWidget(ui->page_landing);
    });

    // Экран "Расчёт на десантирование"
    m_landingCalcWidget = new LandingCalculation(this);
    ui->page_landing->layout()->addWidget(m_landingCalcWidget);
    connect(m_landingCalcWidget, &LandingCalculation::backRequested,
            this, [this](){
        ui->stackedWidget->setCurrentWidget(ui->page_calculations);
    });

    // Подключение сигналов к слотам
    connect(ui->btnFunctionalControl, &QPushButton::clicked, this, &MainWindow::onFunctionalControlClicked);
    connect(ui->btnWorkRegulation, &QPushButton::clicked, this, &MainWindow::onWorkRegulationClicked);
    connect(ui->btnManualInput, &QPushButton::clicked, this, &MainWindow::onManualInputClicked);
    connect(ui->btnInitialData, &QPushButton::clicked, this, &MainWindow::onInitialDataClicked);
    connect(ui->btnCalculations, &QPushButton::clicked, this, &MainWindow::onCalculationsClicked);
    // Раздел "Расчёты" пока не нужен — прячем кнопку из главного меню, не
    // удаляя саму реализацию (может понадобиться в будущем).
    ui->btnCalculations->setVisible(false);
    connect(ui->btnMeasurementResults, &QPushButton::clicked, this, &MainWindow::onMeasurementResultsClicked);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    // connect(ui->btnModeWorking, &QPushButton::toggled, this, &MainWindow::onWorkModeChanged);
    // connect(ui->btnModeStandby, &QPushButton::toggled, this, &MainWindow::onStandbyModeChanged);
    // NoFocus: кнопка часто переключается setEnabled(false/true) во время
    // поиска датчиков — если она в этот момент в фокусе, Qt перекидывает
    // фокус на следующий по табуляции виджет ("Функциональный контроль"),
    // и там появляется видимая рамка фокуса. См. также m_toastCloseBtn и
    // кнопки попапов ниже — та же причина.
    ui->btnConnectSensors->setFocusPolicy(Qt::NoFocus);
    connect(ui->btnConnectSensors, &QPushButton::clicked, this, &MainWindow::onConnectSensorsClicked);
    // Кнопка "Подключить датчики" убрана из главного меню — её роль теперь
    // играет btnConnectAll в статус-панели (см. onConnectAllClicked()).
    // Реализация и сигнал выше оставлены нетронутыми на будущее.
    ui->btnConnectSensors->setVisible(false);

    ui->btnConnectAll->setFocusPolicy(Qt::NoFocus);
    connect(ui->btnConnectAll, &QPushButton::clicked, this, &MainWindow::onConnectAllClicked);
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

    // Поля положения/ориентации ОСТАЮТСЯ пустыми до реальных данных (карта/
    // ГНСС/БИНС/подтверждённый ручной ввод) — раньше здесь стоял
    // setCoordField(...) с демо-координатами Москвы, из-за чего поля
    // выглядели заполненными ещё до старта программы, а индикатор "Отказ"
    // при этом (справедливо) не видел готовых данных — путаница для
    // оператора. См. m_hasGnssPosition/m_hasBinsOrientation.

    // Виртуальная клавиатура для ручного ввода положения/ориентации — тот
    // же механизм, что и у ячеек таблицы GroundMeteoParams. Поля постоянные
    // (не пересоздаются как редакторы делегата), поэтому attach() вызывается
    // один раз здесь и живёт всё время жизни MainWindow — VirtualKeyboard
    // сама показывается только когда поле получает фокус (а получить фокус
    // получится только когда поле enabled, см. onManualInputClicked).
    {
        VirtualKeyboard::Constraints coordC;
        coordC.allowNegative = true;   // широта/долгота могут быть отрицательными
        coordC.allowDecimal  = true;
        coordC.maxDecimals   = 6;
        coordC.maxLength     = 12;
        coordC.allowModeSwitch = false;
        VirtualKeyboard::attach(ui->editLatitude,  VirtualKeyboard::Mode::Numeric, coordC);
        VirtualKeyboard::attach(ui->editLongitude, VirtualKeyboard::Mode::Numeric, coordC);

        VirtualKeyboard::Constraints altC;
        altC.allowNegative = true;     // высота над УМ теоретически может быть отрицательной
        altC.allowDecimal  = true;
        altC.maxDecimals   = 2;
        altC.maxLength     = 8;
        altC.allowModeSwitch = false;
        VirtualKeyboard::attach(ui->editAltitude, VirtualKeyboard::Mode::Numeric, altC);

        VirtualKeyboard::Constraints angleC;
        angleC.allowNegative = true;   // крен/тангаж — знаковые; дир. угол оставляем так же
        angleC.allowDecimal  = true;
        angleC.maxDecimals   = 2;
        angleC.maxLength     = 8;
        angleC.allowModeSwitch = false;
        VirtualKeyboard::attach(ui->editDirectionAngle, VirtualKeyboard::Mode::Numeric, angleC);
        VirtualKeyboard::attach(ui->editRollAngle,       VirtualKeyboard::Mode::Numeric, angleC);
        VirtualKeyboard::attach(ui->editPitchAngle,      VirtualKeyboard::Mode::Numeric, angleC);
    }

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

    // Когда QML сообщает о доступных типах карт — обновляем comboBox
    connect(&qcp, &QmlCoordinateProxy::mapTypesChanged, this, [this](const QStringList &list) {
        qDebug() << "mapTypesChanged:" << list;
        m_osmMapTypeNames = list;
        refreshMapCombo();
    });

    connect(&qcp, &QmlCoordinateProxy::coordinateFromChanged, [=](const QGeoCoordinate &c){
        if (m_mapCoordinatesEnabled){
            updateCoordinatesFromMap(c.latitude(), c.longitude());
        }
    });

    // comboBox → onMapComboChanged (вместо прямой связи с qcp)
    connect(ui->comboBox_mapTypes,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMapComboChanged);

    // Пути к директориям тайлов карты
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    m_mapCacheDir = appData + "/MapCache";
    QDir().mkpath(m_mapCacheDir);

    // Директория провайдеров тайлов (Qt OSM-плагин читает отдельный JSON-файл на тип)
    QString providersDir = appData + "/osm_providers";
    QDir().mkpath(providersDir);

    // Запускаем локальный тайл-сервер ОДИН РАЗ — порт фиксирован на всё время работы.
    // При переключении режима (онлайн ↔ офлайн) меняется только БД через switchTo(),
    // URL провайдеров и сам плагин карты не пересоздаются.
    m_tileServer = new LocalTileServer(this);
    if (m_tileServer->start()) {
        // Провайдеры пишутся один раз с фиксированным портом
        writeProvidersJson(providersDir, m_tileServer->tileUrlTemplate());
        // Онлайн-режим по умолчанию: Street Map с кэшированием в MBTiles
        m_tileServer->switchTo(m_mapCacheDir + "/Street Map.mbtiles",
                               "https://a.tile.openstreetmap.org/%1/%2/%3.png");
    } else {
        // Фолбек: прямой OSM без кэширования
        writeProvidersJson(providersDir,
                           "https://a.tile.openstreetmap.org/%z/%x/%y.png");
    }

    // Следим за новыми .mbtiles-файлами в MapCache
    auto *watcher = new QFileSystemWatcher(QStringList{m_mapCacheDir}, this);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        refreshMapCombo();
    });

    // Базовый URL директории провайдеров (не меняется после старта)
    QString osmProvidersUrl = QUrl::fromLocalFile(providersDir + "/").toString();

    ui->quickWidget->engine()->rootContext()->setContextProperty("coord",           &qcp);
    ui->quickWidget->engine()->rootContext()->setContextProperty("osmProvidersUrl", osmProvidersUrl);
    ui->quickWidget->setSource(QUrl("qrc:/qml/Main.qml"));
    createMapComponent("osm");

    // Создаём окно настроек датчиков
    sensorSettingsDialog = new SensorSettings(this);
    // Подгоняем размер под содержимое и фиксируем минимум
    sensorSettingsDialog->adjustSize();
    sensorSettingsDialog->setMinimumSize(sensorSettingsDialog->sizeHint());
    sensorSettingsDialog->setSizeGripEnabled(true);
    connect(sensorSettingsDialog, &SensorSettings::connectRequested, this, &MainWindow::onConnectRequested);
    connect(sensorSettingsDialog, &SensorSettings::disconnectRequested, this, &MainWindow::onDisconnectRequested);

    connect(sensorSettingsDialog, &SensorSettings::gnssConnectRequested, this, &MainWindow::onGnssConnectFromSettings);
    connect(sensorSettingsDialog, &SensorSettings::gnssDisconnectRequested, this, &MainWindow::onGnssDisconnectFromSettings);

    connect(sensorSettingsDialog, &SensorSettings::amsConnectRequested, this, &MainWindow::onAmsConnectFromSettings);
    connect(sensorSettingsDialog, &SensorSettings::amsDisconnectRequested, this, &MainWindow::onAmsDisconnectFromSettings);

    connect(sensorSettingsDialog, &SensorSettings::binsConnectRequested, this, &MainWindow::onBinsConnectFromSettings);
    connect(sensorSettingsDialog, &SensorSettings::binsDisconnectRequested, this, &MainWindow::onBinsDisconnectFromSettings);

    // Функциональный контроль теперь встроен страницей в общий stackedWidget —
    // как SourceData/GroundMeteoParams/Meteo11 — а не всплывающим QDialog.
    m_functionalControlDialog = new FunctionalControlDialog(this);
    m_functionalControlDialog->setSensorType(FunctionalControlDialog::AMS);
    ui->stackedWidget->addWidget(m_functionalControlDialog);

    // "‹ Назад" на странице функционального контроля — возврат на главный экран
    connect(m_functionalControlDialog, &FunctionalControlDialog::backRequested,
            this, &MainWindow::onBackToHome);

    // ПРИМЕЧАНИЕ: старое дублирующее подключение
    // connect(m_functionalControlDialog, refreshRequested, this, onFunctionalControlClicked)
    // отсюда убрано — оно было лишним (onFunctionalControlClicked больше не
    // занимается запросом данных, см. патч №2). Реальный обработчик
    // refreshRequested — лямбда чуть ниже по файлу
    // (setWaitingState()/requestFunctionalControl()) — она не меняется.

    // Создаём постоянный экземпляр SourceData (внутри создастся GroundMeteoParams)
    // Встраиваем страницей в общий stackedWidget — как экран "Расчёты"
    // (AlgorithmsCalculation) — а не показываем отдельным всплывающим окном.
    sourceDataInstance = new SourceData(this);
    ui->stackedWidget->addWidget(sourceDataInstance);
    connect(sourceDataInstance, &SourceData::backRequested,
            this, &MainWindow::onBackToHome);
    qDebug() << "SourceData instance created (with GroundMeteoParams inside)";

    // Регламентные работы
    m_workRegulationHubPage = new WorkRegulationHubPage(this);
    m_inspectionPage = new InspectionPage(m_amsHandler, this);
    m_angleCheckPage = new AngleCheckPage(m_amsHandler, this);

    ui->stackedWidget->addWidget(m_workRegulationHubPage);
    ui->stackedWidget->addWidget(m_inspectionPage);
    ui->stackedWidget->addWidget(m_angleCheckPage);

    connect(m_workRegulationHubPage, &WorkRegulationHubPage::backRequested,
            this, &MainWindow::onBackToHome);
    connect(m_workRegulationHubPage, &WorkRegulationHubPage::openInspectionRequested,
            this, [this]() { ui->stackedWidget->setCurrentWidget(m_inspectionPage); });
    connect(m_workRegulationHubPage, &WorkRegulationHubPage::openAngleCheckRequested,
            this, [this]() { ui->stackedWidget->setCurrentWidget(m_angleCheckPage); });

    connect(m_inspectionPage, &InspectionPage::backRequested,
            this, [this]() { ui->stackedWidget->setCurrentWidget(m_workRegulationHubPage); });
    connect(m_angleCheckPage, &AngleCheckPage::backRequested,
            this, [this]() { ui->stackedWidget->setCurrentWidget(m_workRegulationHubPage); });

    // ── Подписка на состояние приземных данных + встраивание страницы ──────
    // GroundMeteoParams является единой точкой правды о готовности приземных
    // данных. MainWindow отображает: lblStatus + доступность btnStart, и
    // держит саму страницу в общем стеке (как "Расчёты") — отдельным
    // всплывающим QDialog она больше не является.
    if (GroundMeteoParams *gmp = GroundMeteoParams::instance()) {
        ui->stackedWidget->addWidget(gmp);
        connect(sourceDataInstance, &SourceData::openGroundParamsRequested,
                this, [this, gmp]() {
            ui->stackedWidget->setCurrentWidget(gmp);
        });
        connect(gmp, &GroundMeteoParams::backRequested, this, [this]() {
            ui->stackedWidget->setCurrentWidget(sourceDataInstance);
        });

        connect(gmp, &GroundMeteoParams::surfaceStateChanged,
                this, &MainWindow::onSurfaceStateChanged);
        // Начальное состояние — данных ещё нет → NoData, кнопка пуска
        // должна быть заблокирована с самого старта программы.
        onSurfaceStateChanged(gmp->surfaceState());
    } else {
        qWarning() << "MainWindow: GroundMeteoParams::instance() == nullptr "
                      "при создании SourceData — кнопка пуска не получит "
                      "блокировку до первого появления экземпляра";
    }

    // ── Метео-11: та же схема, что и у GroundMeteoParams выше, но экземпляр
    // не синглтон, а живёт внутри SourceData (данные не теряются между
    // открытиями страницы) и отдаётся через SourceData::meteo11Widget().
    if (Meteo11 *met11 = sourceDataInstance->meteo11Widget()) {
        ui->stackedWidget->addWidget(met11);
        connect(sourceDataInstance, &SourceData::openMeteo11Requested,
                this, [this, met11]() {
            ui->stackedWidget->setCurrentWidget(met11);
        });
        connect(met11, &Meteo11::backRequested, this, [this]() {
            ui->stackedWidget->setCurrentWidget(sourceDataInstance);
        });
    } else {
        qWarning() << "MainWindow: SourceData::meteo11Widget() == nullptr "
                      "при создании SourceData — страница Метео-11 не будет доступна";
    }

    // Всплывающая карточка у индикатора состояния приземных данных
    // ("Состояние: ..."), выезжает из readinessIndicatorFrame по клику.
    setupReadinessPopup();
    connect(ui->readinessIndicatorFrame, &ClickableFrame::clicked,
            this, &MainWindow::onReadinessIndicatorClicked);

    // Шторка состояния/управления датчиком — клик по любой из 4 плашек
    // статуса (GNSS/АМС/БИНС/ИВС). lblGnssStatus/lblAmsStatus/lblBinsStatus/
    // lblIwsStatus должны быть промоутнуты в Designer до класса
    // ClickableLabel (Правой кнопкой → Promote to... → ClickableLabel,
    // header ClickableLabel.h), иначе clicked() у них не будет.
    setupSensorPopup();
    setupConnectAllPopup();
    connect(ui->lblGnssStatus, &ClickableLabel::clicked, this, [this]() {
        showSensorPopup(AutoConnector::DEVICE_GNSS);
    });
    connect(ui->lblAmsStatus, &ClickableLabel::clicked, this, [this]() {
        showSensorPopup(AutoConnector::DEVICE_AMS);
    });
    connect(ui->lblBinsStatus, &ClickableLabel::clicked, this, [this]() {
        showSensorPopup(AutoConnector::DEVICE_BINS);
    });
    connect(ui->lblIwsStatus, &ClickableLabel::clicked, this, [this]() {
        showSensorPopup(AutoConnector::DEVICE_IWS);
    });

    setupAmsHandler();
    //    configureAmsDatabase();

    m_binsHandler = new BINSHandler(this);
    setupBinsHandler();

    m_autoConnector = new AutoConnector(this);

    connect(m_autoConnector, &AutoConnector::deviceDetected, this,
            &MainWindow::onAutoConnectorDeviceDetected);
    connect(m_autoConnector, &AutoConnector::detectionFinished, this,
            &MainWindow::onAutoConnectorFinished);
    connect(m_autoConnector, &AutoConnector::detectionStarted, this,
            &MainWindow::onAutoConnectorStarted);
    connect(m_autoConnector, &AutoConnector::progressUpdated, this,
            &MainWindow::onAutoConnectorProgress);
    connect(m_autoConnector, &AutoConnector::logMessage, this,
            &MainWindow::onAutoConnectorLog);

    QTimer::singleShot(800, this, &MainWindow::connectSensorsFromConfig);

    // Таймер прогрева ИВС (3 минуты)
    m_iwsWarmupTimer = new QTimer(this);
    m_iwsWarmupTimer->setSingleShot(true);
    connect(m_iwsWarmupTimer, &QTimer::timeout, this, &MainWindow::onIwsWarmupFinished);

    // 3 мин доступны по умолчанию; блокируется только после подключения ИВС
    // на 3 минуты прогрева (см. onConnectRequested / onDisconnectRequested).
    // comboAvgTime/comboLitera заменены сегментированными кнопками
    // (btnAvg3/6/9, btnLitera1/2/3) — логика выбора не изменилась, изменился
    // только визуальный виджет.
    ui->btnAvg3->setEnabled(true);
    ui->btnAvg3->setToolTip("");

    // Литера 2 по умолчанию (совпадает с исходным setCurrentIndex(1))
    ui->btnLitera2->setChecked(true);

    // Инициализация панели статуса датчиков
    updateSensorStatusPanel();

    // Health-check: опрос "жив ли датчик" для уже подключённых (см.
    // setupHealthChecks()) — в самом конце конструктора, когда
    // m_gnssHandler/m_amsHandler/m_binsHandler уже точно созданы.
    setupHealthChecks();

    runPlowSelfTest();
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
    delete m_windProfileCalculator;
    m_windProfileCalculator = nullptr;
    delete ui;
}

// =================================================
// =================================================
// Навигация лаунчера (главный экран планшета)
// =================================================

void MainWindow::onOpenPositionPage()
{
    ui->stackedWidget->setCurrentWidget(ui->page_position);
}

void MainWindow::onOpenMapPage()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void MainWindow::onOpenMeasurePage()
{
    ui->stackedWidget->setCurrentWidget(ui->page_measure);
}

void MainWindow::onBackToHome()
{
    ui->stackedWidget->setCurrentWidget(ui->page_home);
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

// ── Готовность положения (ГНСС) / ориентации (БИНС) ─────────────────────
// ВАЖНО: это флаги "данные реально получены", а НЕ парсинг текста полей —
// поля editLatitude/../editPitchAngle в .ui изначально содержат непустые
// демонстрационные значения (Designer), поэтому "текст непустой" не значит
// "данные реальные". Флаги выставляются только настоящими источниками —
// см. onGnssDataReceived/updateCoordinatesFromMap/onBinsDataReceived/
// updateManualHighlightAfterManualInput.
bool MainWindow::hasPositionData() const
{
    return m_hasGnssPosition;
}

bool MainWindow::hasOrientationData() const
{
    return m_hasBinsOrientation;
}

// "Сырая" проверка полей — используется ТОЛЬКО для решения о жёлтой
// подсветке при выходе из ручного режима (см. updateManualHighlightAfterManualInput).
bool MainWindow::fieldsLookLikePosition() const
{
    bool ok1 = false, ok2 = false;
    getCoordField(ui->editLatitude, ok1);
    getCoordField(ui->editLongitude, ok2);
    const bool altOk = !ui->editAltitude->text().trimmed().isEmpty();
    return ok1 && ok2 && altOk;
}

bool MainWindow::fieldsLookLikeOrientation() const
{
    bool ok = false;
    ui->editDirectionAngle->text().toDouble(&ok);
    if (!ok) return false;
    ui->editRollAngle->text().toDouble(&ok);
    if (!ok) return false;
    ui->editPitchAngle->text().toDouble(&ok);
    return ok;
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
    ui->btnMapCoordinates->setIconSize(QSize(20, 20));

    connect(ui->btnMapCoordinates, &QPushButton::clicked, this, &MainWindow::onMapCoordinatesToggled);
    // Дубликат-чип на странице "Положение" — тот же обработчик (он не
    // принимает параметров, просто флипает m_mapCoordinatesEnabled и красит
    // оба виджета через updateMapCoordinatesButtonStyle()).
    connect(ui->btnMapCoordinatesPos, &QPushButton::clicked, this, &MainWindow::onMapCoordinatesToggled);
}

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

    // Есть ДВА виджета этой кнопки — оригинал на карте (плавающий маркер,
    // по которому и правда тапают) и чип-дубликат на странице "Положение"
    // (см. .ui). Одно состояние m_mapCoordinatesEnabled — оба отражают его
    // одинаково, каждый в своём стиле.
    ui->btnMapCoordinates->setIcon(markerIcon);
    ui->btnMapCoordinates->setIconSize(QSize(20, 20));
    ui->btnMapCoordinates->setChecked(m_mapCoordinatesEnabled);
    if (ui->btnMapCoordinatesPos)
        ui->btnMapCoordinatesPos->setChecked(m_mapCoordinatesEnabled);

    if (m_mapCoordinatesEnabled) {
        ui->btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: #0F6B4F;"
            "   border: 2px solid #0B5A41;"
            "   border-radius: 12px;"
            "}"
            "QPushButton:hover { background-color: #0B5A41; }"
            );
        ui->btnMapCoordinates->setToolTip("Режим координат с карты активен — тапните точку на карте");
        if (ui->btnMapCoordinatesPos) {
            ui->btnMapCoordinatesPos->setStyleSheet(
                "QPushButton {"
                "   background-color: #0F6B4F; color: #FFFFFF; border: 1px solid #0F6B4F;"
                "   border-radius: 8px; font-size: 9pt; font-weight: 700; padding: 6px 12px;"
                "}"
                );
        }
    } else {
        ui->btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(255,255,255,235);"
            "   border: none;"
            "   border-radius: 12px;"
            "}"
            "QPushButton:hover { background-color: #f0f0f0; }"
            );
        ui->btnMapCoordinates->setToolTip("Использовать координаты с карты (нажмите, затем тапните точку на карте)");
        if (ui->btnMapCoordinatesPos) {
            ui->btnMapCoordinatesPos->setStyleSheet(
                "QPushButton {"
                "   background-color: #FFFFFF; color: #1C1F22; border: 1px solid #DDE1E3;"
                "   border-radius: 8px; font-size: 9pt; font-weight: 700; padding: 6px 12px;"
                "}"
                );
        }
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

// Выравнивает checkboxGnssPos (дубликат-чип на странице "Положение") по
// текущему checked-состоянию "настоящего" checkboxGnss (на карте).
// blockSignals — чтобы не спровоцировать повторный вызов пересылки на
// checkboxGnss (см. лямбду в setupUi/конструкторе) и не зациклиться.
void MainWindow::syncGnssPosCheckbox()
{
    if (!ui->checkboxGnssPos) return;
    const bool checked = ui->checkboxGnss->isChecked();
    if (ui->checkboxGnssPos->isChecked() == checked) return;
    ui->checkboxGnssPos->blockSignals(true);
    ui->checkboxGnssPos->setChecked(checked);
    ui->checkboxGnssPos->blockSignals(false);
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
    updateMapCoordDisplay();

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

void MainWindow::checkAndDisableConflictingSources(const QString &activeSource)
{
    if (activeSource == "map") {
        // Отключаем GNSS и ручной ввод
        if (m_gnssEnabled) {
            ui->checkboxGnss->setChecked(false);
        }
        m_manualInputEnabled = false;
        // Координаты теперь с карты — предыдущая подсветка "введено вручную"
        // (если была) больше не актуальна.
        m_gnssManualHighlight = false;
        updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
        updateOverallReadiness();
    } else if (activeSource == "gnss") {
        // Отключаем карту и ручной ввод
        if (m_mapCoordinatesEnabled) {
            ui->btnMapCoordinates->setChecked(false);
            m_mapCoordinatesEnabled = false;
            updateMapCoordinatesButtonStyle();
        }
        m_manualInputEnabled = false;
        m_gnssManualHighlight = false;
        updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
        updateOverallReadiness();
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

void MainWindow::onAmsDatabaseError(const QString &error)
{
    qCritical() << "MainWindow: Ошибка БД АМС:" << error;
    statusBar()->showMessage("Ошибка БД АМС: " + error, 10000);
    showNotice("Не удалось записать данные АМС в базу данных: " + error, NotificationToast::Error);
}

void MainWindow::onFunctionalControlClicked()
{
    // Переключаемся на страницу функционального контроля в общем стеке.
    // showEvent внутри страницы сам вызовет refreshRequested → лямбда на
    // functionalControlDialog->refreshRequested (см. setupConnections) сама
    // разберётся, подключены ли мы, и вызовет setDisconnectedState() или
    // setWaitingState()+requestFunctionalControl() — дублировать эту
    // проверку здесь больше не нужно.
    ui->stackedWidget->setCurrentWidget(m_functionalControlDialog);
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

    // Раньше здесь стоял точечный setStyleSheet(...) с серым фоном "не
    // редактируется" — он перебивал общий стиль карточки положения
    // (QSS страницы page_position, см. .ui). Визуальное отличие
    // редактируемо/нередактируемо теперь достаточно даёт enabled/disabled
    // самих полей (см. onManualInputClicked) — readOnly используется здесь
    // только для источников "карта"/"ГНСС", когда поля остаются enabled,
    // но не должны принимать ручной ввод.
}

void MainWindow::updateCoordinatesFromMap(double latitude, double longitude)
{
    setCoordField(ui->editLatitude, latitude);
    setCoordField(ui->editLongitude, longitude);

    // Координаты выбраны на карте — это реальные данные положения (высота
    // от карты не приходит, но широта/долгота — основа "положения").
    m_hasGnssPosition = true;
    updateMapCoordDisplay();
    updateOverallReadiness();

    // Передаем сигнал другим окнам
    emit coordinatesUpdatedFromMap(latitude, longitude);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if(m_toastWidget) {
        repositionToast();
    }

    if (m_stopConfirmOverlay) {
        m_stopConfirmOverlay->setGeometry(rect());
        if (m_stopConfirmCard && m_stopConfirmCard->isVisible()) {
            m_stopConfirmCard->move((width() - m_stopConfirmCard->width()) / 2,
                                    (height() - m_stopConfirmCard->height()) / 2);
        }
    }

    if (m_readinessPopup && m_readinessPopup->isVisible()) {
        // Без анимации — просто пересчитываем позицию под индикатором,
        // чтобы не "гонять" карточку туда-сюда на каждый ресайз.
        const QPoint frameBottomLeft = ui->readinessIndicatorFrame->mapTo(
            this, QPoint(0, ui->readinessIndicatorFrame->height()));
        m_readinessPopup->move(frameBottomLeft.x(), frameBottomLeft.y() + 8);
    }

    if (m_sensorPopup && m_sensorPopup->isVisible()) {
        QWidget *indicator = sensorIndicatorWidget(m_currentPopupSensor);
        if (indicator) {
            const QPoint bottomLeft = indicator->mapTo(this, QPoint(0, indicator->height()));
            m_sensorPopup->move(bottomLeft.x(), bottomLeft.y() + 8);
        }
    }

    if (m_connectAllPopup && m_connectAllPopup->isVisible()) {
        const QPoint bottomLeft = ui->btnConnectAll->mapTo(this, QPoint(0, ui->btnConnectAll->height()));
        m_connectAllPopup->move(bottomLeft.x(), bottomLeft.y() + 8);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->mapCanvas && event->type() == QEvent::Resize) {
        repositionMapFloatingControls();
    }

    // Закрытие карточки-уведомления о приземных данных по клику мимо неё и
    // мимо самого индикатора. Событие НЕ поглощается — клик продолжает
    // обрабатываться как обычно, закрытие попапа лишь побочный эффект.
    //
    // ВАЖНО: при qApp->installEventFilter(this) в watched приходит
    // КОНКРЕТНЫЙ виджет-получатель события (кнопка, поле и т.д.), а не сам
    // qApp — раньше здесь стояла проверка "watched == qApp", которая была
    // всегда false, и клик "мимо" вообще не отслеживался.
    if (event->type() == QEvent::MouseButtonPress &&
        m_readinessPopup && m_readinessPopup->isVisible()) {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint globalPos = me->globalPos();
        const bool insidePopup = m_readinessPopup->rect().contains(
            m_readinessPopup->mapFromGlobal(globalPos));
        const bool insideIndicator = ui->readinessIndicatorFrame->rect().contains(
            ui->readinessIndicatorFrame->mapFromGlobal(globalPos));
        if (!insidePopup && !insideIndicator)
            hideReadinessPopup();
    }

    // То же самое для шторки датчика.
    if (event->type() == QEvent::MouseButtonPress &&
        m_sensorPopup && m_sensorPopup->isVisible()) {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint globalPos = me->globalPos();
        QWidget *indicator = sensorIndicatorWidget(m_currentPopupSensor);
        const bool insidePopup = m_sensorPopup->rect().contains(
            m_sensorPopup->mapFromGlobal(globalPos));
        const bool insideIndicator = indicator && indicator->rect().contains(
            indicator->mapFromGlobal(globalPos));
        if (!insidePopup && !insideIndicator)
            hideSensorPopup();
    }

    // То же самое для шторки "Подключить всё".
    if (event->type() == QEvent::MouseButtonPress &&
        m_connectAllPopup && m_connectAllPopup->isVisible()) {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint globalPos = me->globalPos();
        const bool insidePopup = m_connectAllPopup->rect().contains(
            m_connectAllPopup->mapFromGlobal(globalPos));
        const bool insideIndicator = ui->btnConnectAll->rect().contains(
            ui->btnConnectAll->mapFromGlobal(globalPos));
        if (!insidePopup && !insideIndicator)
            hideConnectAllPopup();
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::repositionMapFloatingControls()
{
    if (!ui->mapCanvas) return;

    const int margin = 16;
    const int gap = 8;
    const int canvasWidth = ui->mapCanvas->width();

    // Строка 1 слева: текущие выбранные координаты — подсказка при выборе
    // точки маркером (см. updateMapCoordDisplay()).
    if (ui->lblMapCoordDisplay) {
        ui->lblMapCoordDisplay->adjustSize();
        ui->lblMapCoordDisplay->move(margin, margin);
    }

    // Строка 1 справа: маркер (выбор координат с карты) + GNSS справа от него
    const int markerSize = ui->btnMapCoordinates->width();
    const int gnssWidth = ui->checkboxGnss->width();
    const int row1Height = ui->btnMapCoordinates->height();

    const int gnssX = canvasWidth - gnssWidth - margin;
    const int markerX = gnssX - gap - markerSize;
    ui->checkboxGnss->move(gnssX, margin);
    ui->btnMapCoordinates->move(markerX, margin);

    // Строка 2: выбор типа карты — под строкой 1, прижат к правому краю
    const int comboWidth = ui->comboBox_mapTypes->width();
    const int y2 = margin + row1Height + gap;
    ui->comboBox_mapTypes->move(canvasWidth - comboWidth - margin, y2);

    // Поднимаем плавающие элементы над картой в порядке отрисовки
    if (ui->lblMapCoordDisplay)
        ui->lblMapCoordDisplay->raise();
    ui->btnMapCoordinates->raise();
    ui->checkboxGnss->raise();
    ui->comboBox_mapTypes->raise();
}

// Обновляет текст плавающей подсказки над картой (lblMapCoordDisplay) в
// соответствии с текущими editLatitude/editLongitude — они уже хранят
// последнее выбранное значение в отображаемом DMS-формате (см. setCoordField,
// вызывается и из onGnssDataReceived, и из updateCoordinatesFromMap).
// m_hasGnssPosition отличает "реальные данные когда-либо получены" от
// демо-значений полей из Designer (см. комментарий у hasPositionData()).
void MainWindow::updateMapCoordDisplay()
{
    if (!ui->lblMapCoordDisplay) return;

    if (!m_hasGnssPosition) {
        ui->lblMapCoordDisplay->setText("Координаты не выбраны");
    } else {
        ui->lblMapCoordDisplay->setText(
            QString("Ш: %1   Д: %2").arg(ui->editLatitude->text(), ui->editLongitude->text()));
    }

    repositionMapFloatingControls();
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
    ui->lblTopDateTime->setText(timeString);
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
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #E8F5E9;");
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
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #FFFACD;");
    } else {
        // Время введено некоррктно - возвращаем предыдущее значение
        ui->editDateTime->setText(m_manualDateTime.toString("dd.MM.yyyy hh:mm:ss"));

        // Кратковременная визуальная индикация ошибки
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #FFB6C1;");
        QTimer::singleShot(500, this, [this]() {
            ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #FFFACD;");
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
    ui->stackedWidget->setCurrentWidget(m_workRegulationHubPage);
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
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt; background-color: #FFFACD;");
    } else {
        // При выходе из режима ручного ввода возвращаемся к автоматическому времени
        m_manualDateTimeSet = true;
        m_isEditingDateTime = false;
        ui->editDateTime->setStyleSheet("font-weight: bold; font-size: 11pt;");

        QDateTime currentDisplayed = QDateTime::fromString(ui->editDateTime->text(), "dd.MM.yyyy hh:mm:ss");
        if (currentDisplayed.isValid()) {
            m_manualDateTime = currentDisplayed;
        }

        updateDateTime();

        // Ручной ввод положения/ориентации только что "зафиксирован" (поля
        // снова стали readonly-по-факту) — определяем по итоговому
        // содержимому полей, какую из групп (ГНСС/БИНС) подсвечивать жёлтым.
        // Независимо: заполнили только положение — БИНС не загорается, и
        // наоборот (см. header, вариант B).
        updateManualHighlightAfterManualInput();
    }
}

// Вызывается сразу после выхода из режима ручного ввода (onManualInputClicked,
// переход enabled→disabled). Смотрит на итоговое содержимое полей — какая
// из групп (положение/ориентация) реально заполнена — и подсвечивает
// соответствующую плашку датчика жёлтым. Группы независимы (вариант B).
void MainWindow::updateManualHighlightAfterManualInput()
{
    const bool posOk = fieldsLookLikePosition();
    const bool oriOk = fieldsLookLikeOrientation();

    m_gnssManualHighlight = posOk;
    m_binsManualHighlight = oriOk;

    // Ручной ввод подтверждён (кнопка "Ручной ввод" выключена) — если поля
    // валидны, это ТЕПЕРЬ реальные данные положения/ориентации, а не просто
    // текст в поле. Не сбрасываем в false при !posOk/!oriOk — оператор мог
    // выключить ручной режим, не тронув эту группу полей вовсе, тогда как
    // они были данные с датчика ранее.
    if (posOk) m_hasGnssPosition = true;
    if (oriOk) m_hasBinsOrientation = true;

    updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
    updateBinsStatusLabel(m_binsHandler && m_binsHandler->isConnected());
    updateOverallReadiness();
}

void MainWindow::onInitialDataClicked()
{
    ui->stackedWidget->setCurrentWidget(sourceDataInstance);
}

void MainWindow::onCalculationsClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_calculations);
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

    dialog->adjustSize();
    // Минимум ниже реального экрана планшета (1200x1920 при масштабе 150% —
    // это 800x1280 логических точек): иначе окно не может сузиться до ширины
    // экрана и правый край содержимого уезжает за границу.
    dialog->setMinimumSize(720, 560);
    dialog->showMaximized();
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
        if (m_gnssHandler->connectToGnss(gnssPort, sensorSettingsDialog->getGnssBaudRate())) {
            m_gnssEnabled = true;
            ui->checkboxGnss->setChecked(true);
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
            if (m_gnssHandler->connectToGnss(port, baudRate)) {
                m_gnssEnabled = true;
                ui->checkboxGnss->setChecked(true);
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

// ── MBTiles / LocalTileServer методы ─────────────────────────────────────────

void MainWindow::writeProvidersJson(const QString &providersDir, const QString &urlTemplate)
{
    // Строим JSON через replace, чтобы %z/%x/%y в urlTemplate не мешал QString::arg()
    QString tmpl =
        "{\n"
        "    \"UrlTemplate\":      \"TILE_URL\",\n"
        "    \"ImageFormat\":      \"png\",\n"
        "    \"MapCopyRight\":     \"<a href='https://www.openstreetmap.org/copyright'>OpenStreetMap</a>\",\n"
        "    \"DataCopyRight\":    \"<a href='https://www.openstreetmap.org/copyright'>OpenStreetMap contributors</a>\",\n"
        "    \"MinimumZoomLevel\": 0,\n"
        "    \"MaximumZoomLevel\": 19\n"
        "}";
    QByteArray json = tmpl.replace("TILE_URL", urlTemplate).toUtf8();

    static const QStringList types = {
        "street", "terrain", "street-hires", "terrain-hires",
        "satellite", "satellite-hires", "cycle", "cycle-hires",
        "transit", "transit-hires", "night-transit", "night-transit-hires",
        "hiking", "hiking-hires"
    };
    for (const QString &type : types) {
        QFile f(providersDir + "/" + type);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
            f.write(json);
    }
}

void MainWindow::refreshMapCombo()
{
    QComboBox *combo = ui->comboBox_mapTypes;
    QSignalBlocker blocker(combo);

    // Запоминаем текущий выбор, чтобы восстановить его после перестройки
    QString savedMbtiles = m_currentMbtilesPath;
    int     savedOsmIdx  = m_osmCurrentIndex;

    combo->clear();

    // 1. OSM-типы
    for (const QString &name : m_osmMapTypeNames)
        combo->addItem(name);

    // 2. .mbtiles-файлы из MapCache
    QDir dir(m_mapCacheDir);
    QStringList files = dir.entryList({"*.mbtiles"}, QDir::Files, QDir::Name);
    if (!files.isEmpty()) {
        combo->insertSeparator(combo->count());
        for (const QString &file : files) {
            QString label = "[офлайн] " + QFileInfo(file).completeBaseName();
            combo->addItem(label, dir.absoluteFilePath(file));
        }
    }

    // 3. Восстанавливаем выбор
    if (!savedMbtiles.isEmpty()) {
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemData(i).toString() == savedMbtiles) {
                combo->setCurrentIndex(i);
                return;
            }
        }
    }
    if (savedOsmIdx >= 0 && savedOsmIdx < m_osmMapTypeNames.size())
        combo->setCurrentIndex(savedOsmIdx);
}

void MainWindow::onMapComboChanged(int index)
{
    if (index < 0) return;

    QString mbtilesPath = ui->comboBox_mapTypes->itemData(index).toString();

    if (!mbtilesPath.isEmpty()) {
        // Офлайн-режим: тайлы только из MBTiles-файла
        m_currentMbtilesPath = mbtilesPath;
        applyMbtilesFile(mbtilesPath);
    } else if (index < m_osmMapTypeNames.size()) {
        // Онлайн-режим: OSM с кэшированием в MBTiles
        m_currentMbtilesPath.clear();
        m_osmCurrentIndex = index;
        applyOnlineMapType(index);
    }
}

void MainWindow::applyOnlineMapType(int osmIndex)
{
    // Только переключаем БД в сервере — карту не пересоздаём (порт не меняется)
    QString mapName     = m_osmMapTypeNames.value(osmIndex, "Street Map");
    QString mbtilesPath = m_mapCacheDir + "/" + mapName + ".mbtiles";

    if (m_tileServer)
        m_tileServer->switchTo(mbtilesPath,
                               "https://a.tile.openstreetmap.org/%1/%2/%3.png");

    // Переключаем активный тип карты в QML (сетка тайлов та же, данные из нашего сервера)
    qcp.setCurrentMapType(osmIndex);
}

void MainWindow::applyMbtilesFile(const QString &mbtilesPath)
{
    // Только переключаем БД в сервере — карту не пересоздаём (порт не меняется)
    if (m_tileServer)
        m_tileServer->switchTo(mbtilesPath, QString()); // офлайн: нет upstream
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

// Разовое уведомление об ошибке/успехе поверх главного окна — тот же
// плавающий тост, что и на страницах ТО (AngleCheckPage/InspectionPage),
// пришедший на замену модальным QMessageBox.
void MainWindow::showNotice(const QString &text, NotificationToast::Kind kind)
{
    m_notifyToast->showMessage(text, kind);
}

// =====================================================
// Методы работы с уведомлением о подключении датчиков
// =====================================================

void MainWindow::setupToastUI()
{
    m_toastWidget = new QWidget(this);
    m_toastWidget->setFixedSize(340, 105);
    m_toastWidget->setStyleSheet(
        "QWidget#toastWidget {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 12px;"
        "}"
        );
    m_toastWidget->setObjectName("toastWidget");

    // Тень для окна
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    m_toastWidget->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_toastWidget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_toastTitle = new QLabel("Автопоиск датчиков", m_toastWidget);
    m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #1C1F22; border: none; background: transparent;");

    m_toastPercent = new QLabel("0%", m_toastWidget);
    m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #0F6B4F; border: none; background: transparent;");
    m_toastPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(m_toastTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_toastPercent);

    m_toastText = new QLabel("Инициализация...", m_toastWidget);
    m_toastText->setStyleSheet("font-size: 9pt; color: #666; font-family: monospace; border: none; background: transparent;");
    m_toastText->setWordWrap(true);
    m_toastText->setFixedHeight(32);

    m_toastProgress = new QProgressBar(m_toastWidget);
    m_toastProgress->setFixedHeight(6);
    m_toastProgress->setTextVisible(false);
    m_toastProgress->setStyleSheet(
        "QProgressBar {"
        "   background-color: #EFF1F1;"
        "   border: none;"
        "   border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #0F6B4F;"
        "   border-radius: 3px;"
        "}"
        );

    layout->addLayout(headerLayout);
    layout->addWidget(m_toastText);
    layout->addWidget(m_toastProgress);

    // Маленькая красная кнопка остановки поиска — в правом верхнем углу окна
    m_toastCloseBtn = new QPushButton("\u2715", m_toastWidget);
    m_toastCloseBtn->setFocusPolicy(Qt::NoFocus);
    m_toastCloseBtn->setFixedSize(12, 12);
    m_toastCloseBtn->setCursor(Qt::PointingHandCursor);
    m_toastCloseBtn->setToolTip("Остановить поиск датчиков");
    m_toastCloseBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #E53935;"
        "   color: #FFFFFF;"
        "   border: none;"
        "   border-radius: 6px;"
        "   font-size: 10pt;"
        "   font-weight: bold;"
        "   padding: 0px;"
        "}"
        "QPushButton:hover { background-color: #C62828; }"
        "QPushButton:pressed { background-color: #B71C1C; }"
        );
    // Позиционируем поверх карточки, в углу — вне общего layout
    m_toastCloseBtn->move(m_toastWidget->width() - m_toastCloseBtn->width() - 8, 8);
    m_toastCloseBtn->raise();
    m_toastCloseBtn->hide(); // видна только пока идёт активный поиск
    connect(m_toastCloseBtn, &QPushButton::clicked, this, &MainWindow::onToastCloseClicked);

    setupConfirmOverlay();

    m_toastAnimation = new QPropertyAnimation(m_toastWidget, "pos", this);
    m_toastAnimation->setDuration(400);
    m_toastAnimation->setEasingCurve(QEasingCurve::OutBack);

    m_toastHideTimer = new QTimer(this);
    m_toastHideTimer->setSingleShot(true);
    connect(m_toastHideTimer, &QTimer::timeout, this, &MainWindow::hideToast);

    // Прячем окно за пределы экрана при создании
    m_toastWidget->move(width() - m_toastWidget->width() - 20, height() + 10);
    m_toastWidget->show();
}

void MainWindow::showToast()
{
    // Если toast прятался по таймеру после предыдущей отмены/завершения
    // поиска (см. onToastCloseClicked/onAutoConnectorFinished), а поиск
    // запустили заново раньше, чем этот таймер сработал — он всё равно
    // "выстрелит" через оставшееся время и спрячет уже НОВЫЙ toast. Поэтому
    // при каждом показе гарантированно останавливаем отложенное скрытие.
    m_toastHideTimer->stop();

    m_toastWidget->raise();

    int targetX = width() - m_toastWidget->width() - 20;
    int startY = height() + 10;
    int endY = height() - m_toastWidget->height() - 20;

    m_toastAnimation->stop();
    m_toastWidget->move(targetX, startY);
    m_toastAnimation->setStartValue(QPoint(targetX, startY));
    m_toastAnimation->setEndValue(QPoint(targetX, endY));
    m_toastAnimation->start();
}

void MainWindow::hideToast()
{
    int targetX = width() - m_toastWidget->width() - 20;
    int startY = m_toastWidget->y();
    int endY = height() + 10;

    m_toastAnimation->stop();
    m_toastAnimation->setStartValue(QPoint(targetX, startY));
    m_toastAnimation->setEndValue(QPoint(targetX, endY));
    m_toastAnimation->start();
}

void MainWindow::repositionToast()
{
    if (m_toastAnimation->state() == QAbstractAnimation::Running) {
        return;
    }

    int targetX = width() - m_toastWidget->width() - 20;
    int targetYVisible = height() - m_toastWidget->height() - 20;
    int targetYHidden = height() + 10;

    // Если Toast сейчас на экране, корректируем его видимую позицию
    if (m_toastWidget->y() < height()) {
        m_toastWidget->move(targetX, targetYVisible);
    } else {
        m_toastWidget->move(targetX, targetYHidden);
    }
}

// =====================================================
// Окно подтверждения остановки поиска датчиков
// =====================================================

void MainWindow::setupConfirmOverlay()
{
    // Полупрозрачный оверлей на всё окно — затемняет фон, перехватывает клики "мимо"
    m_stopConfirmOverlay = new QWidget(this);
    m_stopConfirmOverlay->setObjectName("stopConfirmOverlay");
    m_stopConfirmOverlay->setStyleSheet(
        "QWidget#stopConfirmOverlay { background-color: rgba(28, 31, 34, 140); }"
        );
    m_stopConfirmOverlay->setAttribute(Qt::WA_StyledBackground, true);
    m_stopConfirmOverlay->setGeometry(rect());
    m_stopConfirmOverlay->hide();

    // Плавное появление/исчезновение затемнения. ВАЖНО: этот эффект висит
    // ТОЛЬКО на затемняющем фоне, а не на карточке с кнопками (см. ниже) —
    // иначе Qt рендерит весь поддерево через закэшированный офскрин-буфер,
    // который не обновляется на :hover у кнопок, и они визуально "пропадают"
    // при наведении курсора. Это была причина бага.
    m_stopConfirmOpacity = new QGraphicsOpacityEffect(m_stopConfirmOverlay);
    m_stopConfirmOverlay->setGraphicsEffect(m_stopConfirmOpacity);
    m_stopConfirmOpacity->setOpacity(0.0);

    m_stopConfirmAnimation = new QPropertyAnimation(m_stopConfirmOpacity, "opacity", this);
    m_stopConfirmAnimation->setDuration(200);
    connect(m_stopConfirmAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_stopConfirmOpacity->opacity() < 0.01)
            m_stopConfirmOverlay->hide();
    });

    // Карточка подтверждения — ОБЩАЯ для "остановить поиск" и "отключить
    // датчик": текст/кнопки/колбэк выставляются в showConfirmOverlay().
    // Дочерний виджет MainWindow (НЕ m_stopConfirmOverlay!) — намеренно, по
    // причине, описанной выше. Позиционируется/поднимается поверх оверлея
    // вручную в showConfirmOverlay()/resizeEvent().
    m_stopConfirmCard = new QWidget(this);
    m_stopConfirmCard->setObjectName("stopConfirmCard");
    m_stopConfirmCard->setFixedSize(360, 210);
    m_stopConfirmCard->setStyleSheet(
        "QWidget#stopConfirmCard {"
        "   background-color: #FFFFFF;"
        "   border-radius: 20px;"
        "}"
        );
    m_stopConfirmCard->hide();

    QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(m_stopConfirmCard);
    cardShadow->setBlurRadius(30);
    cardShadow->setColor(QColor(0, 0, 0, 90));
    cardShadow->setOffset(0, 8);
    m_stopConfirmCard->setGraphicsEffect(cardShadow);

    QVBoxLayout *cardLayout = new QVBoxLayout(m_stopConfirmCard);
    cardLayout->setContentsMargins(24, 22, 24, 20);
    cardLayout->setSpacing(10);

    QLabel *icon = new QLabel("\u26A0", m_stopConfirmCard);
    icon->setFixedSize(48, 48);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(
        "font-size: 20pt; color: #B71C1C; background-color: #FFEBEE; border-radius: 24px;"
        );
    QHBoxLayout *iconRow = new QHBoxLayout();
    iconRow->addStretch();
    iconRow->addWidget(icon);
    iconRow->addStretch();

    m_confirmTitleLabel = new QLabel(m_stopConfirmCard);
    m_confirmTitleLabel->setAlignment(Qt::AlignCenter);
    m_confirmTitleLabel->setWordWrap(true);
    m_confirmTitleLabel->setStyleSheet(
        "font-weight: bold; font-size: 12pt; color: #1C1F22; border: none; background: transparent;"
        );

    m_confirmSubtitleLabel = new QLabel(m_stopConfirmCard);
    m_confirmSubtitleLabel->setAlignment(Qt::AlignCenter);
    m_confirmSubtitleLabel->setWordWrap(true);
    m_confirmSubtitleLabel->setStyleSheet(
        "font-size: 9pt; color: #666666; border: none; background: transparent;"
        );

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_confirmYesBtn = new QPushButton(m_stopConfirmCard);
    m_confirmYesBtn->setFocusPolicy(Qt::NoFocus);
    m_confirmYesBtn->setCursor(Qt::PointingHandCursor);
    m_confirmYesBtn->setFixedHeight(44);
    m_confirmYesBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #FFFFFF;"
        "   color: #B71C1C;"
        "   border: 1px solid #FFCDD2;"
        "   border-radius: 14px;"
        "   font-size: 10pt;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #FFEBEE; }"
        "QPushButton:pressed { background-color: #FFCDD2; }"
        );

    m_confirmNoBtn = new QPushButton(m_stopConfirmCard);
    m_confirmNoBtn->setFocusPolicy(Qt::NoFocus);
    m_confirmNoBtn->setCursor(Qt::PointingHandCursor);
    m_confirmNoBtn->setFixedHeight(44);
    m_confirmNoBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #0F6B4F;"
        "   color: #FFFFFF;"
        "   border: none;"
        "   border-radius: 14px;"
        "   font-size: 10pt;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #0B5A42; }"
        "QPushButton:pressed { background-color: #094B37; }"
        );

    btnRow->addWidget(m_confirmYesBtn);
    btnRow->addWidget(m_confirmNoBtn);

    cardLayout->addLayout(iconRow);
    cardLayout->addWidget(m_confirmTitleLabel);
    cardLayout->addWidget(m_confirmSubtitleLabel);
    cardLayout->addStretch();
    cardLayout->addLayout(btnRow);

    connect(m_confirmYesBtn, &QPushButton::clicked, this, &MainWindow::onConfirmOverlayAccepted);
    connect(m_confirmNoBtn, &QPushButton::clicked, this, &MainWindow::onConfirmOverlayCancelled);
}

void MainWindow::showConfirmOverlay(const QString &title, const QString &subtitle,
                                    std::function<void()> onConfirm,
                                    const QString &yesLabel, const QString &noLabel)
{
    if (!m_stopConfirmOverlay || !m_stopConfirmCard) return;

    m_confirmTitleLabel->setText(title);
    m_confirmSubtitleLabel->setText(subtitle);
    m_confirmSubtitleLabel->setVisible(!subtitle.isEmpty());
    m_confirmYesBtn->setText(yesLabel);
    m_confirmNoBtn->setText(noLabel);
    m_confirmCallback = std::move(onConfirm);

    m_stopConfirmOverlay->setGeometry(rect());
    m_stopConfirmOverlay->show();
    m_stopConfirmOverlay->raise();

    // Карточка больше не в layout'е оверлея (она вообще не его потомок) —
    // центрируем вручную и поднимаем НАД оверлеем.
    m_stopConfirmCard->move((width() - m_stopConfirmCard->width()) / 2,
                            (height() - m_stopConfirmCard->height()) / 2);
    m_stopConfirmCard->show();
    m_stopConfirmCard->raise();

    m_stopConfirmAnimation->stop();
    m_stopConfirmAnimation->setStartValue(m_stopConfirmOpacity->opacity());
    m_stopConfirmAnimation->setEndValue(1.0);
    m_stopConfirmAnimation->start();
}

void MainWindow::hideConfirmOverlay()
{
    if (!m_stopConfirmOverlay) return;

    if (m_stopConfirmCard)
        m_stopConfirmCard->hide(); // сразу — карточка не участвует в фейде фона

    m_stopConfirmAnimation->stop();
    m_stopConfirmAnimation->setStartValue(m_stopConfirmOpacity->opacity());
    m_stopConfirmAnimation->setEndValue(0.0);
    m_stopConfirmAnimation->start();
}

void MainWindow::onConfirmOverlayAccepted()
{
    hideConfirmOverlay();
    if (m_confirmCallback) {
        auto callback = std::move(m_confirmCallback);
        m_confirmCallback = nullptr;
        callback();
    }
}

void MainWindow::onConfirmOverlayCancelled()
{
    hideConfirmOverlay();
    m_confirmCallback = nullptr;
}

void MainWindow::onToastCloseClicked()
{
    // Крестик активен только пока идёт реальный поиск
    if (!m_autoConnector->isDetecting()) return;

    showConfirmOverlay(
        "Остановить поиск датчиков?",
        "Уже найденные датчики останутся подключены,\nостальные придётся искать заново.",
        [this]() {
            m_autoConnector->stopDetection();

            ui->btnConnectSensors->setEnabled(true);
            statusBar()->clearMessage();
            m_toastCloseBtn->hide();

            // Поиск прерван пользователем — пересчитываем видимость
            // "Подключить всё": появится, только если к этому моменту не
            // подключился ни один датчик.
            updateConnectAllButtonVisibility();

            m_toastTitle->setText("Поиск остановлен");
            m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #B71C1C; border: none; background: transparent;");
            m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #B71C1C; border: none; background: transparent;");
            m_toastProgress->setStyleSheet(
                "QProgressBar { background-color: #FFEBEE; border: none; border-radius: 3px; }"
                "QProgressBar::chunk { background-color: #C62828; border-radius: 3px; }"
                );
            m_toastText->setText("Поиск остановлен пользователем");

            m_toastHideTimer->start(2000);
        },
        "Да, остановить", "Нет, продолжить");
}

// ─────────────────────────────────────────────────────────────────────────
// Всплывающая карточка у индикатора состояния приземных данных
// ─────────────────────────────────────────────────────────────────────────

void MainWindow::setupReadinessPopup()
{
    m_readinessPopup = new QWidget(this);
    m_readinessPopup->setObjectName("readinessPopup");
    m_readinessPopup->setFixedWidth(300);
    m_readinessPopup->setStyleSheet(
        "QWidget#readinessPopup {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    m_readinessPopup->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_readinessPopup);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    m_readinessPopupTitle = new QLabel(m_readinessPopup);
    m_readinessPopupTitle->setWordWrap(true);
    m_readinessPopupTitle->setStyleSheet(
        "font-weight: bold; font-size: 10pt; color: #1C1F22; background: transparent; border: none;");

    m_readinessPopupSubtitle = new QLabel(m_readinessPopup);
    m_readinessPopupSubtitle->setWordWrap(true);
    m_readinessPopupSubtitle->setStyleSheet(
        "font-size: 9pt; color: #6B7278; background: transparent; border: none;");

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_readinessPopupNo = new QPushButton("Нет", m_readinessPopup);
    m_readinessPopupNo->setFocusPolicy(Qt::NoFocus);
    m_readinessPopupNo->setStyleSheet(
        "QPushButton { background:#FFFFFF; color:#1C1F22; border:1px solid #DDE1E3;"
        " border-radius:8px; padding:6px 18px; font-weight:600; }"
        "QPushButton:pressed { background:#F0F1F2; }");
    m_readinessPopupYes = new QPushButton("Да", m_readinessPopup);
    m_readinessPopupYes->setFocusPolicy(Qt::NoFocus);
    m_readinessPopupYes->setStyleSheet(
        "QPushButton { background:#0F6B4F; color:#FFFFFF; border:none;"
        " border-radius:8px; padding:6px 18px; font-weight:700; }"
        "QPushButton:pressed { background:#0B5A41; }");
    btnRow->addStretch();
    btnRow->addWidget(m_readinessPopupNo);
    btnRow->addWidget(m_readinessPopupYes);

    layout->addWidget(m_readinessPopupTitle);
    layout->addWidget(m_readinessPopupSubtitle);
    layout->addLayout(btnRow);

    m_readinessPopup->hide();

    m_readinessPopupAnimation = new QPropertyAnimation(m_readinessPopup, "pos", this);
    m_readinessPopupAnimation->setDuration(300);
    m_readinessPopupAnimation->setEasingCurve(QEasingCurve::OutBack);

    connect(m_readinessPopupYes, &QPushButton::clicked, this, [this]() {
        hideReadinessPopup();
        if (GroundMeteoParams *gmp = GroundMeteoParams::instance())
            ui->stackedWidget->setCurrentWidget(gmp);
    });
    connect(m_readinessPopupNo, &QPushButton::clicked, this, [this]() {
        hideReadinessPopup();
    });

    // Закрытие по клику вне уведомления и вне самого индикатора — фильтр
    // ставится один раз на приложение, работает только пока попап виден.
    qApp->installEventFilter(this);
}

// Попап общего индикатора — ИСКЛЮЧИТЕЛЬНО информационный (см. header):
// заголовок + список причин, без единой кнопки перехода к вводу данных.
// Переход к ручному вводу конкретных данных делается из шторки конкретного
// датчика — см. m_sensorPopupManualBtn/onSensorPopupManualClicked.
void MainWindow::populateReadinessPopupContent()
{
    const bool measurementRunning =
        (m_amsHandler && m_amsHandler->getMeasurementStatus() == STATUS_RUNNING);

    m_readinessPopupYes->setVisible(false);
    m_readinessPopupNo->setVisible(false);

    if (measurementRunning) {
        m_readinessPopupTitle->setText("Идёт измерение АМС");
        m_readinessPopupSubtitle->clear();
    } else {
        const QStringList issues = collectReadinessIssues();
        if (!issues.isEmpty()) {
            m_readinessPopupTitle->setText("Отказ");
            m_readinessPopupSubtitle->setText(issues.join("\n"));
        } else if (m_lastKnownSurfaceState == GroundMeteoParams::Stale) {
            m_readinessPopupTitle->setText("Данные устарели");
            m_readinessPopupSubtitle->setText(
                "Приземные данные старше 30 минут. Рекомендуется обновить.");
        } else {
            m_readinessPopupTitle->setText("Система готова");
            m_readinessPopupSubtitle->clear();
        }
    }

    m_readinessPopup->adjustSize();
    m_readinessPopup->setFixedWidth(300); // adjustSize мог сжать по ширине — держим фиксированную
}

void MainWindow::onReadinessIndicatorClicked()
{
    if (m_readinessPopup->isVisible()) {
        hideReadinessPopup();
        return;
    }

    populateReadinessPopupContent();
    showReadinessPopup();
}

void MainWindow::showReadinessPopup()
{
    const QPoint frameBottomLeft = ui->readinessIndicatorFrame->mapTo(
        this, QPoint(0, ui->readinessIndicatorFrame->height()));
    const int targetX = frameBottomLeft.x();
    const int targetY = frameBottomLeft.y() + 8;
    const int startY  = frameBottomLeft.y() - 20; // "выезжает" из-под индикатора

    m_readinessPopup->raise();
    m_readinessPopupAnimation->stop();
    m_readinessPopup->move(targetX, startY);
    m_readinessPopup->show();
    m_readinessPopupAnimation->setStartValue(QPoint(targetX, startY));
    m_readinessPopupAnimation->setEndValue(QPoint(targetX, targetY));
    m_readinessPopupAnimation->start();
}

void MainWindow::hideReadinessPopup()
{
    if (!m_readinessPopup->isVisible())
        return;
    m_readinessPopupAnimation->stop();
    m_readinessPopup->hide();
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

void MainWindow::setupSensorPopup()
{
    m_sensorPopup = new QWidget(this);
    m_sensorPopup->setObjectName("sensorPopup");
    m_sensorPopup->setFixedWidth(280);
    m_sensorPopup->setStyleSheet(
        "QWidget#sensorPopup {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    m_sensorPopup->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_sensorPopup);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    m_sensorPopupTitle = new QLabel(m_sensorPopup);
    m_sensorPopupTitle->setStyleSheet(
        "font-weight: bold; font-size: 11pt; color: #1C1F22; background: transparent; border: none;");

    m_sensorPopupStatus = new QLabel(m_sensorPopup);
    m_sensorPopupStatus->setStyleSheet(
        "font-size: 9pt; font-weight: 600; background: transparent; border: none;");

    m_sensorPopupInfo = new QLabel(m_sensorPopup);
    m_sensorPopupInfo->setWordWrap(true);
    m_sensorPopupInfo->setStyleSheet(
        "font-size: 8.5pt; color: #6B7278; background: transparent; border: none;");

    m_sensorPopupReason = new QLabel(m_sensorPopup);
    m_sensorPopupReason->setWordWrap(true);
    m_sensorPopupReason->setStyleSheet(
        "font-size: 8.5pt; color: #B71C1C; background: transparent; border: none;");

    m_sensorPopupActionBtn = new QPushButton(m_sensorPopup);
    m_sensorPopupActionBtn->setFocusPolicy(Qt::NoFocus);
    m_sensorPopupActionBtn->setCursor(Qt::PointingHandCursor);
    m_sensorPopupActionBtn->setFixedHeight(36);

    // Вторая кнопка — переход к ручному вводу данных этого датчика (ИВС →
    // приземные данные, ГНСС/БИНС → положение/ориентация). Показывается
    // только когда датчик не подключён И соответствующих данных ещё нет —
    // см. populateSensorPopupContent()/sensorHasRequiredData().
    m_sensorPopupManualBtn = new QPushButton(m_sensorPopup);
    m_sensorPopupManualBtn->setFocusPolicy(Qt::NoFocus);
    m_sensorPopupManualBtn->setCursor(Qt::PointingHandCursor);
    m_sensorPopupManualBtn->setFixedHeight(36);
    m_sensorPopupManualBtn->hide();

    layout->addWidget(m_sensorPopupTitle);
    layout->addWidget(m_sensorPopupStatus);
    layout->addWidget(m_sensorPopupInfo);
    layout->addWidget(m_sensorPopupReason);
    layout->addSpacing(4);
    layout->addWidget(m_sensorPopupActionBtn);
    layout->addWidget(m_sensorPopupManualBtn);

    m_sensorPopup->hide();

    m_sensorPopupAnimation = new QPropertyAnimation(m_sensorPopup, "pos", this);
    m_sensorPopupAnimation->setDuration(300);
    m_sensorPopupAnimation->setEasingCurve(QEasingCurve::OutBack);

    connect(m_sensorPopupActionBtn, &QPushButton::clicked, this, &MainWindow::onSensorPopupActionClicked);
    connect(m_sensorPopupManualBtn, &QPushButton::clicked, this, &MainWindow::onSensorPopupManualClicked);
}

void MainWindow::populateSensorPopupContent()
{
    const AutoConnector::DeviceType type = m_currentPopupSensor;
    m_sensorPopupTitle->setText(sensorDisplayName(type));

    const bool connected = isSensorConnected(type);
    const bool searchRunning = m_autoConnector->isDetecting();
    const bool searchingThis = searchRunning && m_autoConnector->singleSearchTarget() == type;

    if (connected) {
        m_sensorPopupStatus->setText("Подключено");
        m_sensorPopupStatus->setStyleSheet(
            "font-size: 9pt; font-weight: 600; color: #0F6B4F; background: transparent; border: none;");

        QString info = sensorExtraInfo(type);
        const QString lastSeen = sensorLastSeenText(type);
        if (!lastSeen.isEmpty())
            info = info.isEmpty() ? lastSeen : (info + "\n" + lastSeen);
        m_sensorPopupInfo->setText(info);
        m_sensorPopupInfo->setVisible(!info.isEmpty());

        // Отключать вручную из шторки не нужно (см. задачу) — отключение
        // отслеживается автоматически через health-check.
        m_sensorPopupReason->setVisible(false);
        m_sensorPopupActionBtn->setVisible(false);
        m_sensorPopupManualBtn->setVisible(false);
    } else {
        m_sensorPopupStatus->setText("Не подключено");
        m_sensorPopupStatus->setStyleSheet(
            "font-size: 9pt; font-weight: 600; color: #C62828; background: transparent; border: none;");
        m_sensorPopupInfo->setVisible(false);

        m_sensorPopupReason->setText(sensorProblemReason(type));
        m_sensorPopupReason->setVisible(true);

        m_sensorPopupActionBtn->setVisible(true);
        const bool queued = m_sensorSearchQueue.contains(type);
        if (searchingThis) {
            m_sensorPopupActionBtn->setText("Идёт поиск...");
            m_sensorPopupActionBtn->setEnabled(false);
        } else if (queued) {
            m_sensorPopupActionBtn->setText("В очереди поиска...");
            m_sensorPopupActionBtn->setEnabled(false);
        } else if (searchRunning) {
            // Другой датчик уже ищется — не блокируем, а ставим в очередь
            // по клику (см. startSingleSensorSearch).
            m_sensorPopupActionBtn->setText("Подключить (в очередь)");
            m_sensorPopupActionBtn->setEnabled(true);
        } else {
            m_sensorPopupActionBtn->setText("Подключить");
            m_sensorPopupActionBtn->setEnabled(true);
        }
        m_sensorPopupActionBtn->setStyleSheet(
            "QPushButton { background:#0F6B4F; color:#FFFFFF; border:none;"
            " border-radius:8px; font-weight:700; }"
            "QPushButton:pressed { background:#0B5A41; }"
            "QPushButton:disabled { background:#B9BFC2; color:#FFFFFF; }");

        // Кнопка перехода к ручному вводу — только для ИВС/ГНСС/БИНС и
        // только пока соответствующих данных ДЕЙСТВИТЕЛЬНО нет (ни с
        // датчика, ни введены вручную ранее). Для АМС не показывается —
        // для него нет ручной замены.
        const QString manualText = sensorManualEntryButtonText(type);
        const bool showManual = !manualText.isEmpty() && !sensorHasRequiredData(type);
        m_sensorPopupManualBtn->setVisible(showManual);
        if (showManual) {
            m_sensorPopupManualBtn->setText(manualText);
            m_sensorPopupManualBtn->setStyleSheet(
                "QPushButton { background:#FFF8E1; color:#8a6100; border:1px solid #FFE082;"
                " border-radius:8px; font-weight:700; font-size:8pt; padding:4px 6px; }"
                "QPushButton:pressed { background:#FFECB3; }");
        }
    }

    m_sensorPopup->adjustSize();
    m_sensorPopup->setFixedWidth(280);
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

// Переход к ручному вводу данных, которые обычно даёт этот датчик.
// ИВС → страница GroundMeteoParams (свой класс). ГНСС/БИНС → поля
// положения/ориентации живут прямо на странице "Исходные данные"
// (mainwindow.ui) — переключаемся туда и, если ручной режим ещё не
// включён, включаем его (onManualInputClicked), чтобы поля сразу стали
// редактируемыми.
void MainWindow::onSensorPopupManualClicked()
{
    const AutoConnector::DeviceType type = m_currentPopupSensor;
    hideSensorPopup();

    switch (type) {
    case AutoConnector::DEVICE_IWS:
        if (GroundMeteoParams *gmp = GroundMeteoParams::instance())
            ui->stackedWidget->setCurrentWidget(gmp);
        break;
    case AutoConnector::DEVICE_GNSS:
    case AutoConnector::DEVICE_BINS:
        // Поля положения/ориентации живут на ui->page_position (см. .ui),
        // а НЕ в sourceDataInstance (тот — для приземных данных/Метео-11).
        ui->stackedWidget->setCurrentWidget(ui->page_position);
        if (!m_manualInputEnabled)
            onManualInputClicked();
        break;
    default:
        break;
    }
}

void MainWindow::showSensorPopup(AutoConnector::DeviceType type)
{
    // Повторный клик по той же плашке — закрыть (как readiness popup)
    if (m_sensorPopup->isVisible() && m_currentPopupSensor == type) {
        hideSensorPopup();
        return;
    }

    QWidget *indicator = sensorIndicatorWidget(type);
    if (!indicator) return;

    m_currentPopupSensor = type;
    populateSensorPopupContent();

    const QPoint bottomLeft = indicator->mapTo(this, QPoint(0, indicator->height()));
    const int targetX = bottomLeft.x();
    const int targetY = bottomLeft.y() + 8;
    const int startY  = bottomLeft.y() - 20;

    m_sensorPopup->raise();
    m_sensorPopupAnimation->stop();
    m_sensorPopup->move(targetX, startY);
    m_sensorPopup->show();
    m_sensorPopupAnimation->setStartValue(QPoint(targetX, startY));
    m_sensorPopupAnimation->setEndValue(QPoint(targetX, targetY));
    m_sensorPopupAnimation->start();
}

void MainWindow::hideSensorPopup()
{
    if (!m_sensorPopup || !m_sensorPopup->isVisible())
        return;
    m_sensorPopupAnimation->stop();
    m_sensorPopup->hide();
}

void MainWindow::onSensorPopupActionClicked()
{
    startSingleSensorSearch(m_currentPopupSensor);
}

// ─────────────────────────────────────────────────────────────────────────
// Шторка кнопки "Подключить всё" — по стилю и механике повторяет шторку
// датчика (m_sensorPopup), только без per-датчикового состояния: она и так
// видна лишь когда ни один датчик не подключён.
// Кнопка сейчас полностью скрыта (см. kConnectAllButtonEnabled в
// updateConnectAllButtonVisibility()), поэтому иконки у неё нет — код ниже
// оставлен нетронутым на случай, если понадобится вернуть.
// ─────────────────────────────────────────────────────────────────────────

void MainWindow::setupConnectAllPopup()
{
    m_connectAllPopup = new QWidget(this);
    m_connectAllPopup->setObjectName("connectAllPopup");
    m_connectAllPopup->setFixedWidth(260);
    m_connectAllPopup->setStyleSheet(
        "QWidget#connectAllPopup {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    m_connectAllPopup->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_connectAllPopup);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    m_connectAllPopupTitle = new QLabel(m_connectAllPopup);
    m_connectAllPopupTitle->setText("Ни один датчик не подключён");
    m_connectAllPopupTitle->setStyleSheet(
        "font-weight: bold; font-size: 11pt; color: #1C1F22; background: transparent; border: none;");
    m_connectAllPopupTitle->setWordWrap(true);

    m_connectAllPopupSubtitle = new QLabel(m_connectAllPopup);
    m_connectAllPopupSubtitle->setText("Запустите поиск, чтобы найти и подключить GNSS, АМС, БИНС и ИВС");
    m_connectAllPopupSubtitle->setWordWrap(true);
    m_connectAllPopupSubtitle->setStyleSheet(
        "font-size: 8.5pt; color: #6B7278; background: transparent; border: none;");

    m_connectAllPopupActionBtn = new QPushButton(m_connectAllPopup);
    m_connectAllPopupActionBtn->setText("Подключить все датчики");
    m_connectAllPopupActionBtn->setFocusPolicy(Qt::NoFocus);
    m_connectAllPopupActionBtn->setCursor(Qt::PointingHandCursor);
    m_connectAllPopupActionBtn->setFixedHeight(36);
    m_connectAllPopupActionBtn->setStyleSheet(
        "QPushButton { background:#0F6B4F; color:#FFFFFF; border:none;"
        " border-radius:8px; font-weight:700; }"
        "QPushButton:pressed { background:#0B5A41; }");

    layout->addWidget(m_connectAllPopupTitle);
    layout->addWidget(m_connectAllPopupSubtitle);
    layout->addSpacing(4);
    layout->addWidget(m_connectAllPopupActionBtn);

    m_connectAllPopup->hide();

    m_connectAllPopupAnimation = new QPropertyAnimation(m_connectAllPopup, "pos", this);
    m_connectAllPopupAnimation->setDuration(300);
    m_connectAllPopupAnimation->setEasingCurve(QEasingCurve::OutBack);

    connect(m_connectAllPopupActionBtn, &QPushButton::clicked, this, &MainWindow::onConnectAllPopupActionClicked);
}

void MainWindow::populateConnectAllPopupContent()
{
    // Состояние сейчас всегда одно и то же (шторка доступна только при
    // 0 из 4 подключённых), но проверка на случай поиска в процессе не
    // помешает — на будущее, если правило показа кнопки изменится.
    const bool searching = m_autoConnector && m_autoConnector->isDetecting();
    m_connectAllPopupActionBtn->setEnabled(!searching);
    m_connectAllPopupActionBtn->setText(searching ? "Идёт поиск..." : "Подключить все датчики");
}

void MainWindow::showConnectAllPopup()
{
    // Повторный клик по иконке — закрыть (как у остальных шторок).
    if (m_connectAllPopup->isVisible()) {
        hideConnectAllPopup();
        return;
    }

    populateConnectAllPopupContent();

    const QPoint bottomLeft = ui->btnConnectAll->mapTo(this, QPoint(0, ui->btnConnectAll->height()));
    const int targetX = bottomLeft.x();
    const int targetY = bottomLeft.y() + 8;
    const int startY  = bottomLeft.y() - 20;

    m_connectAllPopup->raise();
    m_connectAllPopupAnimation->stop();
    m_connectAllPopup->move(targetX, startY);
    m_connectAllPopup->show();
    m_connectAllPopupAnimation->setStartValue(QPoint(targetX, startY));
    m_connectAllPopupAnimation->setEndValue(QPoint(targetX, targetY));
    m_connectAllPopupAnimation->start();
}

void MainWindow::hideConnectAllPopup()
{
    if (!m_connectAllPopup || !m_connectAllPopup->isVisible())
        return;
    m_connectAllPopupAnimation->stop();
    m_connectAllPopup->hide();
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
