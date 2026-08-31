// ─────────────────────────────────────────────────────────────────────────
// Конструктор, деструктор и «скелет» страницы архива: проводка сигналов,
// перемещение по датам и записям, загрузка выбранного измерения.
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

MeasurementResults::MeasurementResults(QWidget *parent)
    : QWidget(parent)
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
    , m_currentAvgWind()
    , m_currentActualWind()
    , m_currentMeasuredWind()
    , m_gribPipeline(new GribMeteo11Pipeline(this))
    , m_datePopup(nullptr)
    , m_exportView(nullptr)
    , m_amsProbeFieldsVisible(false)
    , m_customTabBar(nullptr)
//    , m_dbPort(5432)
//    , m_dbConfigured(false)
{
    ui->setupUi(this);

    m_toast = new NotificationToast(this);

    applyArchiveStyle();
    setupCustomTabBar();

    // Ряды кнопок Метео-11 в узком окне тоже переносятся по строкам
    replaceWithFlowLayout(ui->horizontalLayout_bulletenTypeBtns, 6);
    replaceWithFlowLayout(ui->verticalLayout_bulletenFormat, 6);
    if (ui->verticalLayout_bulletenType)
        ui->verticalLayout_bulletenType->setContentsMargins(0, 0, 0, 0);

    // Кнопки-переключатели Метео-11 оформлены как «пилюли» из макета —
    // помечаем их свойством, на которое есть правила в applyArchiveStyle().
    for (QPushButton *b : { ui->pushButton_updated, ui->pushButton_approximate,
                            ui->pushButton_fromMeteoStat, ui->pushButton_fromGrib,
                            ui->pushButton_string, ui->pushButton_table }) {
        if (b) {
            b->setProperty("pill", true);
            b->setCursor(Qt::PointingHandCursor);
        }
    }

    // Встроенный экран экспорта — вторая страница rootStack (индекс 1),
    // подменяет содержимое архива вместо модального ExportDialog.
    m_exportView = new ArchiveExportView(this);
    ui->rootStack->addWidget(m_exportView);
    connect(m_exportView, &ArchiveExportView::backRequested,
            this, &MeasurementResults::onExportBackRequested);
    connect(m_exportView, &ArchiveExportView::exportRequested,
            this, &MeasurementResults::onExportSubmitted);

    // Popup выбора даты/времени (взамен модального QDialog с QCalendarWidget)
    m_datePopup = new ArchiveDatePopup(this);
    connect(m_datePopup, &ArchiveDatePopup::dateTimeSelected,
            this, &MeasurementResults::onDatePopupDateTimeSelected);
    connect(m_datePopup, &ArchiveDatePopup::noDataForDate, this, [this](const QDate &date) {
        showStatus(QString("Нет данных за %1").arg(date.toString("dd.MM.yyyy")), NotificationToast::Info);
    });

    // Кнопка "Закрыть" в шапке — единственный способ покинуть архив теперь
    // (кнопка "Назад" убрана как избыточная). accept()/reject() у QDialog
    // больше нет — просто эмитим сигнал, MainWindow сам переключит стек.
    connect(ui->btnClose, &QPushButton::clicked, this, &MeasurementResults::backRequested);

    setupAmsProbeCollapse();

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
    connect(ui->pushButton_fromGrib, &QPushButton::clicked, this, &MeasurementResults::onFromGribButtonClicked);

    // Статус GRIB-расчёта отображается прямо под кнопкой (одна строка,
    // обновляется по мере выполнения) — без отдельного окна лога, чтобы
    // не загромождать вкладку. Полный лог по-прежнему дублируется в
    // qDebug() для отладки из Qt Creator.
    ui->labelGribStatus->setVisible(false);
    connect(m_gribPipeline, &GribMeteo11Pipeline::logLine, this,
            [this](const QString &line) {
                qDebug() << "[GRIB]" << line;
                ui->labelGribStatus->setStyleSheet("color: #666; font-size: 10px;");
                ui->labelGribStatus->setText(line);
                ui->labelGribStatus->setVisible(true);
            });
    connect(m_gribPipeline, &GribMeteo11Pipeline::finished, this,
            [this](bool success, const QVector<WindProfileData> &, const QString &error) {
                ui->pushButton_fromGrib->setEnabled(true);
                ui->pushButton_fromGrib->setText("Из GRIB");
                if (success) {
                    ui->labelGribStatus->setStyleSheet("color: #2e7d32; font-size: 10px;"); // зелёный
                    ui->labelGribStatus->setText("Готово");
                } else {
                    ui->labelGribStatus->setStyleSheet("color: #c62828; font-size: 10px;"); // красный
                    ui->labelGribStatus->setText("Ошибка: " + error);
                }
                ui->labelGribStatus->setVisible(true);
            });
    // Отдельно — реальная обработка результата (сборка Meteo11Data через
    // buildMeteo11 и обновление таблицы/строки, если открыта вкладка GRIB)
    connect(m_gribPipeline, &GribMeteo11Pipeline::finished,
            this, &MeasurementResults::onGribPipelineFinished);

    connect(ui->pushButton_string, &QPushButton::clicked, this, &MeasurementResults::onStringFormatClicked);
    connect(ui->pushButton_table, &QPushButton::clicked, this, &MeasurementResults::onTableFormatClicked);

    connect(ui->btnExport, &QPushButton::clicked, this, &MeasurementResults::onExportClicked);

    // Бейдж годности бюллетеня — пилюля по ширине текста, как в макете,
    // а не поле ввода во всю строку.
    ui->lineEdit_bulleten->setReadOnly(true);
    ui->lineEdit_bulleten->setAlignment(Qt::AlignCenter);
    ui->lineEdit_bulleten->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->lineEdit_bulletenTime->setReadOnly(true);
    ui->lineEdit_bulletenTime->setFrame(false);

    switchMeteo11Display();

    // Все графики инициализируются ДО загрузки данных —
    // иначе displayWindProfile/clearWindShearDisplay обращаются к неготовым виджетам
    setupPlots();
    setupZoom();
    setupWindShearTab();
    setupArchiveTables();
    setupMeteo11TableLayout();
    applyResponsiveLayout(width());

    loadAvailableMeasurements();

    updateDateTimeDisplay();
    updateSliderRange();
}

void MeasurementResults::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyResponsiveLayout(event->size().width());
}

void MeasurementResults::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Страница теперь постоянный виджет в стеке MainWindow, а не диалог,
    // пересоздаваемый заново на каждый клик — поэтому список измерений
    // обновляем здесь, при каждом появлении страницы на экране.
    loadAvailableMeasurements();
    // Шрифты из QSS применяются на полировке (при показе), а от них зависит
    // высота строк таблицы — поэтому её высоту пересчитываем и здесь.
    updateMeteo11TableHeight();
}

void MeasurementResults::showStatus(const QString &text, NotificationToast::Kind kind)
{
    // Раньше Success сам скрывался через 3с, а Info/Error оставались на экране
    // бессрочно (Error можно было закрыть крестиком, Info — вообще ничем,
    // пока его не сменяло следующее уведомление). Теперь любое всплывающее
    // уведомление в архиве само пропадает через несколько секунд; для Error
    // время побольше, чтобы успеть прочитать текст ошибки, и крестик
    // по-прежнему доступен, чтобы закрыть раньше.
    const int autoHideMs = (kind == NotificationToast::Error) ? 6000 : 4000;
    m_toast->showMessage(text, kind, autoHideMs);
}

void MeasurementResults::onDatePopupDateTimeSelected(const QDateTime &dt)
{
    currentDateTime = dt;
    updateDisplay();
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

// ===== ОБНОВЛЕНИЕ ИНТЕРФЕЙСА =====

void MeasurementResults::updateCoordinatesFromMainWindow(double latitude, double longitude)
{
    if (!m_mapCoordinatesMode){
        return;
    }

    m_currentLatitude  = latitude;
    m_currentLongitude = longitude;

    ui->valLatitude->setText(CoordHelper::toDisplayDMS(qAbs(latitude))
                             + (latitude >= 0 ? " N" : " S"));
    ui->valLongitude->setText(CoordHelper::toDisplayDMS(qAbs(longitude))
                              + (longitude >= 0 ? " E" : " W"));
    m_stationCoordsValid = true;

    qDebug() << "MeasurementResults: Координаты обновлены с карты:" << latitude << longitude;
}

void MeasurementResults::setMapCoordinatesMode(bool enabled)
{
    m_mapCoordinatesMode = enabled;

    if (enabled) {
        m_lockedDateTime = currentDateTime;
    }

    // Поля координат всегда только для чтения — независимо от режима карты
    // (данные берутся из архива БД, не от пользователя)

    // Раньше здесь ещё и отключалась вся навигация по датам (стрелки и кнопка
    // выбора даты/времени), пока активен режим координат с карты/GNSS. Из-за
    // этого при входе в архив, когда карта/GNSS уже включены (обычная
    // ситуация на станции), кнопка выбора даты оказывалась неактивной с
    // первого раза — а сам архив как раз и нужен для просмотра ИСТОРИИ
    // записей, так что блокировать переход по датам тут не должно было.
    // Режим координат с карты влияет только на поля координат, не на
    // просмотр архива.
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
    // Архив открывается пустым — ничего не подставляем и не выбираем за
    // пользователя (ни "сейчас", ни последнюю запись): пока дата и время не
    // выбраны явно (стрелками, попапом выбора даты или кнопкой "Последняя
    // запись" внутри него), таблицы и графики остаются пустыми, а статусная
    // строка показывает приглашение выбрать дату — см. "else"-ветку в
    // loadMeasurementData().
    updateDisplay();
}

void MeasurementResults::updateDateTimeDisplay()
{
    ui->btnSelectDate->setText(currentDateTime.toString("dd.MM.yyyy hh:mm"));

    loadMeasurementData(currentDateTime);
}

void MeasurementResults::updateSliderRange()
{
    // Слайдер времени заменён списком доступных времён в попапе выбора даты
    // (ArchiveDatePopup) — здесь осталось только обновить подпись под датой.
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

    m_currentAvgWind.clear();
    m_currentActualWind.clear();
    m_currentMeasuredWind.clear();

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
        // Стрелки "‹ ›" ищут currentDateTime среди реальных записей по точному
        // совпадению времени — если оставить его равным переданному dateTime
        // (который может быть округлён "к ближайшему часу"/не совпадать ни с
        // одной записью), клик по стрелке никогда не найдёт "текущую" запись
        // и ничего не произойдёт. Поэтому синхронизируем его с фактически
        // найденной записью сразу же.
        currentDateTime = record.measurementTime;
        m_currentSondingTime = record.measurementTime;

        QVector<WindProfileData>  avgWind      = loadAvgWindProfile(record.recordId);
        QVector<WindProfileData>  actualWind   = loadActualWindProfile(record.recordId);
        QVector<MeasuredWindData> measuredWind = loadMeasuredWindProfile(record.recordId);

        m_currentAvgWind = avgWind;
        m_currentActualWind = actualWind;
        m_currentMeasuredWind = measuredWind;

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
        ui->lblDataStatus->setText(QString("Запись №%1 · %2 · %3")
                                       .arg(record.recordId)
                                       .arg(record.measurementTime.toString("dd.MM.yyyy hh:mm"))
                                       .arg(info));
        setWidgetState(ui->lblDataStatus, "ok");

    } else {
        ui->lblDataStatus->setText("Нет данных для выбранного времени");
        setWidgetState(ui->lblDataStatus, "empty");

        ui->tableWidget_AverageWind->clearContents();
        ui->tableWidget_realWind->clearContents();
        ui->tableWidget_izmWind_2->clearContents();
        ui->tableWidget_parm1b65->clearContents();
        clearStationCoordinates();

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
    // Собираем доступные даты/времена в лёгком формате для попапа —
    // вместе с флагами полноты данных, чтобы попап мог подсветить,
    // у каких записей есть все профили ветра, а у каких нет.
    QMap<QDate, QVector<ArchiveRecordInfo>> available;
    for (auto it = availableMeasurements.constBegin(); it != availableMeasurements.constEnd(); ++it) {
        QVector<ArchiveRecordInfo> records;
        records.reserve(it.value().size());
        for (const MeasurementRecord &record : it.value()) {
            ArchiveRecordInfo info;
            info.time             = record.measurementTime;
            info.hasAvgWind       = record.hasAvgWind;
            info.hasActualWind    = record.hasActualWind;
            info.hasMeasuredWind  = record.hasMeasuredWind;
            records.append(info);
        }
        available.insert(it.key(), records);
    }

    m_datePopup->setAvailable(available);
    m_datePopup->setCurrent(currentDateTime);
    m_datePopup->popupNear(ui->btnSelectDate);
}
