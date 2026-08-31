// ─────────────────────────────────────────────────────────────────────────
// Конструктор, деструктор и «скелет» окна: создание страниц, вся проводка
// сигналов и слотов, навигация по QStackedWidget, часы и события окна.
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

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

    // ── Страница «Положение метеокомплекса» в стиле «Архива измерений» ──
    // Тема применяется точечно к самой странице, а не ко всему главному
    // окну: остальные страницы (плитки главного экрана, карта, измерение)
    // сохраняют прежний вид. Роли виджетов помечаем свойствами ДО вызова
    // темы — QSS-селекторы [card]/[toggle]/… должны сработать сразу.
    // Кнопки «Назад» — через общий помощник: одинаковые место, размер и
    // подсветка на всех экранах, включая ещё не переведённые «Карту» и
    // «Пуск измерений».
    setupArchiveBackButton(ui->btnBackFromPosition);
    setupArchiveBackButton(ui->btnBackFromMap);
    setupArchiveBackButton(ui->btnBackFromMeasure);
    ui->btnManualInput->setProperty("primary", true);
    ui->gnssPositionCard->setProperty("card", true);
    ui->binsOrientationCard->setProperty("card", true);
    ui->lblGnssCardTitle->setProperty("cardTitle", true);
    ui->lblBinsCardTitle->setProperty("cardTitle", true);
    // Кнопка «Карта» и чекбокс «ГНСС-датчик» — переключатели источника
    // координат: включённый заливается зелёным (см. [toggle] в теме).
    ui->btnMapCoordinatesPos->setProperty("toggle", true);
    ui->checkboxGnssPos->setProperty("toggle", true);
    for (QLabel *cap : { ui->lblLatitudeLabel,  ui->lblLatitudeType,
                         ui->lblLongitudeLabel, ui->lblLongitudeType,
                         ui->lblAltitudeLabel,  ui->lblDirectionAngleLabel,
                         ui->lblRollAngleLabel, ui->lblPitchAngleLabel })
        cap->setProperty("caption", true);
    applyArchiveScreenTheme(ui->page_position);

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

    // ── Архив измерений: та же схема встраивания, что и у остальных страниц
    // выше — постоянный экземпляр в общем стеке вместо всплывающего QDialog,
    // который раньше пересоздавался (new MeasurementResults(this)) на каждый
    // клик по кнопке архива и открывался поверх главного окна showMaximized().
    m_measurementResults = new MeasurementResults(this);
    ui->stackedWidget->addWidget(m_measurementResults);
    connect(m_measurementResults, &MeasurementResults::backRequested,
            this, &MainWindow::onBackToHome);
    // Те же две связи с координатами карты, что раньше настраивались заново
    // при каждом создании диалога в onMeasurementResultsClicked() — теперь
    // экземпляр один и живёт всё время работы программы, поэтому подписка
    // делается один раз здесь.
    connect(this, &MainWindow::coordinatesUpdatedFromMap,
            m_measurementResults, &MeasurementResults::updateCoordinatesFromMainWindow);
    connect(this, &MainWindow::mapCoordinatesModeChanged,
            m_measurementResults, &MeasurementResults::setMapCoordinatesMode,
            Qt::DirectConnection);
    // Верхняя панель статуса датчиков в архиве не нужна (это отдельный полный
    // экран истории, а не рабочий экран измерения) и просто отнимает у него
    // высоту, которой на планшете и так не хватает. Прячем её, пока активна
    // страница архива, и возвращаем на всех остальных страницах.
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, [this](int) {
        if (ui->topStatusBar)
            ui->topStatusBar->setVisible(ui->stackedWidget->currentWidget() != m_measurementResults);
    });

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

    // Разовая подсказка про выбор точки — только пока точку ни разу не
    // выбирали (m_hasGnssPosition отражает это для обоих источников,
    // см. updateMapCoordDisplay()) и только один раз за сеанс работы, чтобы
    // не надоедать при каждом заходе на страницу.
    if (!m_mapCoordHintShown && !m_hasGnssPosition) {
        m_mapCoordHintShown = true;
        showNotice("Чтобы выбрать точку на карте: нажмите \"Указать точку\", "
                   "затем тапните нужное место на карте.",
                   NotificationToast::Info);
    }
}

void MainWindow::onOpenMeasurePage()
{
    ui->stackedWidget->setCurrentWidget(ui->page_measure);
}

void MainWindow::onBackToHome()
{
    ui->stackedWidget->setCurrentWidget(ui->page_home);
}

void MainWindow::openMeasurementResults(int recordId)
{
    // -1 = просто открыть архив на последней доступной записи (обычное
    // поведение кнопки "Архив измерений" на главном экране, повторяет
    // onMeasurementResultsClicked()); recordId > 0 = сразу перейти к
    // конкретной записи (например, сразу после завершения измерения).
    onMeasurementResultsClicked();
    if (recordId > 0 && m_measurementResults)
        m_measurementResults->navigateToRecord(recordId);
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
    // Раньше здесь создавался новый MeasurementResults(this) на каждый клик
    // (QDialog, setAttribute(WA_DeleteOnClose), showMaximized()). Теперь это
    // постоянная страница общего стека (создана и подключена один раз в
    // конструкторе, см. m_measurementResults) — здесь только освежаем
    // координаты и переключаем стек на неё.
    if (!m_measurementResults)
        return;

    m_measurementResults->setMapCoordinatesMode(m_mapCoordinatesEnabled || m_gnssEnabled);

    if ((m_mapCoordinatesEnabled || m_gnssEnabled) && ui->editLatitude && ui->editLongitude) {
        bool ok1, ok2;
        double lat = getCoordField(ui->editLatitude, ok1);
        double lon = getCoordField(ui->editLongitude, ok2);
        if (ok1 && ok2) {
            m_measurementResults->updateCoordinatesFromMainWindow(lat, lon);
        }
    }

    ui->stackedWidget->setCurrentWidget(m_measurementResults);
}
