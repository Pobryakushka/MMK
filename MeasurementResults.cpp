#include "MeasurementResults.h"
#include "CoordHelper.h"
#include "ui_MeasurementResults.h"
#include "databasemanager.h"
#include "sensors/amsprotocol.h"
#include "ui/MeasurementExporter.h"
#include "ui/ExportDialog.h"
#include "ui/ArchiveDatePopup.h"
#include "ui/ArchiveExportView.h"
#include "ui/FlowLayout.h"
#include <qwt_plot_renderer.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <QPushButton>
#include <QLabel>
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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QGridLayout>
#include <QTabBar>
#include <QStyle>
#include <QHeaderView>
#include <QResizeEvent>
#include <QShowEvent>
#include <QScrollArea>
#include <limits>
#include <algorithm>  // Для std::sort
#include <cmath>
#include <QtMath>
#include <QTextDocument>       // Для std::pow (расчёт давления МСА в Метео-11)

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

// Переключение визуального состояния виджета через динамическое свойство.
// Именно так (а не через setStyleSheet() на самом виджете) состояние можно
// менять, не стирая остальные правила из общего стилшита диалога: setStyleSheet()
// на конкретном виджете имеет приоритет над стилшитом предка и заменяет собой
// весь его набор правил для этого виджета.
void MeasurementResults::setWidgetState(QWidget *w, const QString &state)
{
    if (!w) return;
    if (w->property("state").toString() == state) return;
    w->setProperty("state", state);
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

// Бейдж годности бюллетеня — пилюля шириной по тексту, цвет задаётся
// состоянием (ok/warn/bad/none), правила для которых лежат в applyArchiveStyle().
void MeasurementResults::setBulletinBadge(const QString &text, const QString &state)
{
    QLineEdit *badge = ui->lineEdit_bulleten;
    if (!badge) return;
    badge->setText(text);
    setWidgetState(badge, state);
    // QLineEdit по умолчанию просит ширину «под 17 символов» независимо от
    // содержимого — для пилюли считаем ширину по фактическому тексту.
    badge->setFixedWidth(badge->fontMetrics().horizontalAdvance(text) + 30);
}

// Блок бюллетеня в макете — карточка по высоте текста, а не поле во всю
// панель. QTextEdit сам по себе всегда занимает всё доступное место, поэтому
// после каждой смены содержимого подгоняем высоту под документ.
void MeasurementResults::fitMeteo11TextHeight()
{
    QTextEdit *view = ui->textEdit_meteo11;
    if (!view) return;

    view->document()->setTextWidth(view->viewport()->width());
    const int docHeight = qCeil(view->document()->size().height());
    const int frame     = 2 * view->frameWidth() + 28; // рамка + вертикальные отступы из QSS
    view->setFixedHeight(qBound(90, docHeight + frame, 460));

    // Иначе единственный элемент сетки центрируется по вертикали и карточка
    // с бюллетенем «висит» посреди пустой вкладки вместо верха, как в макете.
    if (ui->gridLayout_meteo11_string)
        ui->gridLayout_meteo11_string->setAlignment(view, Qt::AlignTop);
}

// Табличный вид Метео-11: слева компактная сетка расшифрованных полей,
// справа таблица ПП/ТТДДСС во всю оставшуюся ширину. В .ui у сетки крупные
// шрифты и растянутые строки, из-за чего вкладка выглядела разреженной.
void MeasurementResults::setupMeteo11TableLayout()
{
    if (QGridLayout *g = ui->gridLayout_Meteo11Params) {
        g->setVerticalSpacing(6);
        g->setHorizontalSpacing(12);
        g->setContentsMargins(0, 0, 16, 0);
        g->setColumnStretch(0, 1);
        g->setColumnStretch(1, 0);
        g->setRowStretch(g->rowCount(), 1);   // прижать поля к верху
    }
    if (QGridLayout *g = ui->gridLayout_4)
        g->setContentsMargins(0, 0, 0, 0);
    if (QTableWidget *t = ui->tableWidget_meteo11Formalize)
        t->setMinimumWidth(280);
}

// Табличный вид Метео-11: расшифрованные поля и таблица ПП/ТТДДСС стоят рядом
// в широком окне и друг под другом в узком. Бок о бок на планшете каждой
// колонке достаётся около 270 точек — подписи полей обрезаются, а значения
// сжимаются до одного символа.
void MeasurementResults::setMeteo11TableStacked(bool stacked)
{
    QGridLayout  *outer  = ui->gridLayout_4;
    QGridLayout  *params = ui->gridLayout_Meteo11Params;
    QTableWidget *table  = ui->tableWidget_meteo11Formalize;
    if (!outer || !params || !table) return;

    outer->removeItem(params);
    outer->removeWidget(table);

    if (stacked) {
        outer->addLayout(params, 0, 0);
        outer->addWidget(table,  1, 0);
        outer->setColumnStretch(0, 1);
        outer->setColumnStretch(2, 0);
        outer->setRowStretch(0, 0);
        outer->setRowStretch(1, 1);
    } else {
        outer->addLayout(params, 0, 0);
        outer->addWidget(table,  0, 2);
        outer->setColumnStretch(0, 3);
        outer->setColumnStretch(2, 2);
        outer->setRowStretch(0, 1);
        outer->setRowStretch(1, 0);
    }
    outer->invalidate();
}

// Меняет готовую QHBoxLayout из формы на FlowLayout с теми же виджетами.
// Проще, чем переносить ряды кнопок в .ui: Qt Designer не умеет пользовательские
// раскладки, а поведение нужно только одно — перенос по ширине.
void MeasurementResults::replaceWithFlowLayout(QLayout *source, int spacing)
{
    if (!source) return;
    QWidget *host = source->parentWidget();
    if (!host) return;

    const QMargins margins = source->contentsMargins();

    QVector<QWidget *> widgets;
    while (QLayoutItem *item = source->takeAt(0)) {
        if (QWidget *w = item->widget())
            widgets.append(w);
        delete item;   // распорки из ряда кнопок больше не нужны
    }

    // Раскладку нельзя просто заменить у виджета, пока старая жива:
    // QWidget::setLayout() ругается, если layout уже установлен.
    QLayout *parentLayout = nullptr;
    if (host->layout() != source) {
        // вложенная раскладка — ищем её владельца, чтобы вставить новую на то же место
        parentLayout = host->layout();
    }

    if (parentLayout) {
        auto *flow = new FlowLayout(nullptr, 0, spacing, spacing);
        flow->setContentsMargins(margins);
        for (QWidget *w : qAsConst(widgets))
            flow->addWidget(w);
        if (auto *box = qobject_cast<QBoxLayout *>(parentLayout)) {
            const int index = box->indexOf(source);
            box->removeItem(source);
            delete source;
            box->insertLayout(qMax(0, index), flow);
        } else {
            delete flow;
        }
        return;
    }

    delete source;
    auto *flow = new FlowLayout(host, 0, spacing, spacing);
    flow->setContentsMargins(margins);
    for (QWidget *w : qAsConst(widgets))
        flow->addWidget(w);
}

// Планшетная (узкая) компоновка. На 1200x1920 при масштабе 150% окну достаётся
// 800 логических точек по ширине: два графика рядом превращаются в две
// нечитаемые полоски, а боковая панель съедает треть экрана. Ниже эти места
// переключаются по фактической ширине окна, а не по признаку устройства —
// так один и тот же код работает и в портретной, и в альбомной ориентации.
void MeasurementResults::applyResponsiveLayout(int width)
{
    const bool narrow = (width < kNarrowWidthThreshold);
    if (m_narrowLayout == narrow && m_responsiveApplied) return;
    m_narrowLayout = narrow;
    m_responsiveApplied = true;

    // Боковая панель: в узком окне отдаём основной области больше места, но не
    // настолько, чтобы в кнопку выбора даты перестала помещаться строка
    // "22.08.2026 11:54" — вместе с уменьшенным шрифтом 250 точек хватает.
    // Ширину панели задаём жёстко: одного максимума мало — при нехватке места
    // раскладка сжимает её до минимума, и в кнопку выбора даты перестаёт
    // помещаться строка вида "06.04.2026 15:10".
    if (ui->titleWidget) {
        const int railWidth = narrow ? 250 : 270;
        ui->titleWidget->setMinimumWidth(railWidth);
        ui->titleWidget->setMaximumWidth(railWidth);
    }
    setWidgetState(ui->btnSelectDate, narrow ? "narrow" : "");

    // Тулбар Метео-11: в узком окне группы «тип бюллетеня» и «формат вывода»
    // встают друг под друга — иначе группе типа достаётся половина ширины и
    // её четыре кнопки вытягиваются в столбец по одной. Распорка между
    // группами при этом схлопывается: в вертикальном ряду она превращается в
    // пустой промежуток в десятки точек.
    if (QHBoxLayout *toolbar = ui->horizontalLayout_meteo11_buttons) {
        toolbar->setDirection(narrow ? QBoxLayout::TopToBottom
                                     : QBoxLayout::LeftToRight);
        for (int i = 0; i < toolbar->count(); ++i) {
            if (QSpacerItem *sp = toolbar->itemAt(i)->spacerItem()) {
                sp->changeSize(narrow ? 0 : 40, narrow ? 8 : 20,
                               narrow ? QSizePolicy::Minimum : QSizePolicy::Expanding,
                               QSizePolicy::Minimum);
            }
        }
        toolbar->invalidate();
    }

    // Графики: всегда рядом по горизонтали, в любой ширине окна. Данные —
    // это профиль по высотам, значение (скорость/направление) вторично, а
    // высота — по оси Y; поставленные друг под другом графики в узком окне
    // раньше отдавали всю ширину под малоинформативную ось X и вдвое ужимали
    // высоту, на которой как раз и нужно читать показания. Бок о бок каждому
    // графику достаётся уже вертикаль вкладки почти целиком.
    const QList<QHBoxLayout *> chartRows = {
        ui->chartsRow_avgWind, ui->chartsRow_realWind,
        ui->chartsRow_measWind, ui->chartsRow_shear
    };
    for (QHBoxLayout *row : chartRows)
        if (row)
            row->setDirection(QBoxLayout::LeftToRight);

    // Немного сузили ещё раз по просьбе пользователя: графики и так уже
    // всегда рядом по горизонтали (см. выше), но 380/440 всё же оставляли
    // таблице по высотам маловато места на экране — снизили, чтобы таблица
    // под графиками открывалась пошире, до появления собственной прокрутки.
    const QList<QFrame *> cards = {
        ui->cardFrame_avgWindSpeed, ui->cardFrame_avgWindDir,
        ui->cardFrame_realWindSpeed, ui->cardFrame_realWindDir,
        ui->cardFrame_measWindSpeed, ui->cardFrame_measWindDir,
        ui->cardFrame_shearSpeed, ui->cardFrame_shearDir
    };
    for (QFrame *card : cards)
        if (card)
            card->setMaximumHeight(narrow ? 320 : 360);

    setMeteo11TableStacked(narrow);

    // «Наземные условия»: подпись параметра живёт в вертикальном заголовке, и
    // его ширину QHeaderView берёт по самой длинной подписи. На планшете это
    // съедало почти всю панель, и от колонки значения оставалась полоска у
    // правого края — поэтому в узком окне ширина жёстко ограничивается, а
    // шрифт подписи уменьшается, чтобы текст в неё помещался.
    if (QTableWidget *t = ui->tableWidget_parm1b65) {
        QHeaderView *vh = t->verticalHeader();
        if (narrow) {
            vh->setFixedWidth(340);
        } else {
            vh->setMaximumWidth(QWIDGETSIZE_MAX);
            vh->setMinimumWidth(360);
        }
        setWidgetState(t, narrow ? "narrow" : "");
    }

    // В таблице сдвига ветра четыре колонки; на планшете полные заголовки в
    // них не помещаются и обрезаются на середине слова.
    if (QTableWidget *t = ui->table_windShear) {
        const QStringList wide   = { "Высота, м", "Скорость, м/с/30м",
                                     "Изменение направления, °", "Уровень" };
        const QStringList compact = { "Высота, м", "Скор., м/с/30м",
                                      "Δ напр., °", "Уровень" };
        t->setHorizontalHeaderLabels(narrow ? compact : wide);
    }

    // Статусная строка над вкладками в узком окне переносится на две строки
    if (ui->lblDataStatus)
        ui->lblDataStatus->setWordWrap(narrow);
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
}

void MeasurementResults::clearStationCoordinates()
{
    ui->valLatitude->setText("—");
    ui->valLongitude->setText("—");
    ui->valAltitude->setText("—");
    m_stationCoordsValid = false;
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

// ─────────────────────────────────────────────────────────────────────────────
// Визуальный стиль архива — зелёная палитра/типографика по макету
// "archive v1 docked sidebar.html". Здесь задаются только статические
// (не зависящие от состояния) правила; переключаемые состояния (выбранная
// вкладка бюллетеня Метео-11, активный формат экспорта и т.п.) по-прежнему
// переключаются точечно через setStyleSheet() конкретных виджетов — как это
// уже было принято в остальном коде этого класса.
// ─────────────────────────────────────────────────────────────────────────────
void MeasurementResults::applyArchiveStyle()
{
    // Стиль архива — QSS поверх Fusion (фиксируется глобально в main.cpp).
    // Fusion выбран потому, что ничего не дорисовывает там, где оформление
    // задано стилшитом; системные темы (например cleanlooks на Astra/Fly)
    // игнорируют часть правил и рисуют поверх свой объёмный chrome.
    //
    // Правила ниже покрывают и "служебные" части виджетов — полосы прокрутки,
    // индикаторы чекбоксов, стрелку комбобокса, шапки таблиц. Если их не
    // покрыть, именно через них и проступает вид Fusion.
    //
    // Здесь задаются только статические правила. Переключаемые состояния
    // (выбранный тип бюллетеня Метео-11, активная вкладка, наличие записей
    // за дату) выражены динамическими свойствами и селекторами вида
    // [state="..."] — так состояние не требует setStyleSheet() на конкретном
    // виджете, который стирал бы остальные правила для него.
    setStyleSheet(
        // ── палитра макета ────────────────────────────────────────────────
        // green #0F6B4F · green-dark #0B5A41 · green-soft #E4F1EC
        // bg #EFF1F1 · card #FFFFFF · border #DDE1E3
        // text #1B211F · mute #6E7876 · amber #F9A825
        "QWidget#MeasurementResults, QWidget#archivePage, QStackedWidget#rootStack {"
        "  background-color: #EFF1F1;"
        "}"
        "QWidget { font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; color: #1B211F; }"

        // ── левая панель ─────────────────────────────────────────────────
        "QFrame#titleWidget { background: #FFFFFF; border: none; border-right: 1px solid #DDE1E3; }"
        "QFrame#railHead, QFrame#sectionDateTime, QFrame#sectionParams, QFrame#railFoot {"
        "  background: transparent; border: none;"
        "}"
        "QFrame#sectionDateTime, QFrame#sectionParams, QFrame#railFoot {"
        "  border-top: 1px solid #DDE1E3;"
        "}"
        "QLabel#lblTitle { color: #1B211F; font-size: 15px; font-weight: 700; }"
        "QLabel#lblSubtitle { color: #6E7876; font-size: 11px; }"
        "QLabel#capDateTime, QLabel#capParams {"
        "  color: #6E7876; font-size: 11px; font-weight: 600; padding-bottom: 2px;"
        "}"
        "QPushButton#btnPrevDate, QPushButton#btnNextDate {"
        "  background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 6px;"
        "  color: #0B5A41; font-size: 13px; padding: 0px;"
        "}"
        "QPushButton#btnPrevDate:hover, QPushButton#btnNextDate:hover { background: #E4F1EC; }"
        "QPushButton#btnPrevDate:disabled, QPushButton#btnNextDate:disabled {"
        "  color: #B7BEBB; background: #F7F8F8;"
        "}"
        "QPushButton#btnSelectDate {"
        "  background: #E4F1EC; border: 2px solid #0F6B4F; border-radius: 6px;"
        "  color: #0B5A41; font-weight: 700; font-size: 13px;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "}"
        "QPushButton#btnSelectDate:hover { background: #D8ECE3; }"
        "QPushButton#btnSelectDate[state=\"narrow\"] { font-size: 12px; padding: 0px 1px; }"
        "QLabel#lblAvailableRecords { color: #6E7876; font-size: 11px; font-style: italic; }"
        "QLabel#lblAvailableRecords[state=\"empty\"] { color: #B03A2E; }"

        // Строки параметров станции: подпись слева, значение справа моноширинным,
        // разделитель пунктиром — как param-row в макете. У последней строки
        // разделителя нет.
        "QFrame#rowLatitude, QFrame#rowLongitude, QFrame#rowAltitude {"
        "  background: transparent; border: none; border-bottom: 1px dashed #DDE1E3;"
        "}"
        "QFrame#rowDirectionAngle { background: transparent; border: none; }"
        "QLabel#lblLatitude, QLabel#lblLongitude, QLabel#lblAltitude, QLabel#lblDirectionAngle {"
        "  color: #6E7876; font-size: 12px;"
        "}"
        "QLabel#valLatitude, QLabel#valLongitude, QLabel#valAltitude, QLabel#valDirectionAngle {"
        "  color: #1B211F; font-size: 12px; font-weight: 600;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "}"

        "QPushButton#btnExport, QPushButton#btnClose {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; background: #FFFFFF;"
        "  font-weight: 600; font-size: 13px; color: #1B211F;"
        "}"
        "QPushButton#btnExport:hover { background: #F3F5F4; }"
        "QPushButton#btnClose { background: #0F6B4F; border-color: #0F6B4F; color: #FFFFFF; }"
        "QPushButton#btnClose:hover { background: #0B5A41; border-color: #0B5A41; }"

        // ── статусная строка над вкладками ────────────────────────────────
        "QLabel#lblDataStatus {"
        "  background: #FFFFFF; border-bottom: 1px solid #DDE1E3; color: #6E7876;"
        "  font-weight: 600; font-size: 10px; padding: 6px 14px;"
        "}"
        "QLabel#lblDataStatus[state=\"empty\"] { color: #B03A2E; }"

        // ── самодельная строка вкладок (нативная QTabBar скрыта) ──────────
        "QWidget#customTabBar { background: #EFF1F1; }"
        "QPushButton#archiveTabButton {"
        "  background: #E4E7E6; color: #6E7876; font-weight: 600; font-size: 10px;"
        "  padding: 5px 9px; border: none;"
        "  border-top-left-radius: 8px; border-top-right-radius: 8px;"
        "  border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;"
        "}"
        "QPushButton#archiveTabButton:hover { color: #0B5A41; }"
        "QPushButton#archiveTabButton[active=\"true\"] {"
        "  background: #FFFFFF; color: #0B5A41; border-bottom: 2px solid #0F6B4F;"
        "}"
        "QFrame#dataGroup { background: #EFF1F1; border: none; }"
        "QTabWidget::pane { border: none; background: #FFFFFF; }"
        "QTabWidget > QWidget { background: #FFFFFF; }"

        // ── карточки графиков (chart-card из макета) ──────────────────────
        "QFrame#cardFrame_avgWindSpeed, QFrame#cardFrame_avgWindDir,"
        "QFrame#cardFrame_realWindSpeed, QFrame#cardFrame_realWindDir,"
        "QFrame#cardFrame_measWindSpeed, QFrame#cardFrame_measWindDir,"
        "QFrame#cardFrame_shearSpeed, QFrame#cardFrame_shearDir {"
        "  border: 1px solid #DDE1E3; border-radius: 10px; background: #FFFFFF;"
        "}"
        "QwtPlot { border: none; background: transparent; }"
        "QLabel#label_avgWindSpeed, QLabel#label_avgWindDirection,"
        "QLabel#label_realWindSpeed, QLabel#label_realWindDirection,"
        "QLabel#label_measuredWindSpeed, QLabel#label_measuredWindDirection,"
        "QLabel#label_shearSpeed, QLabel#label_shearDirection {"
        "  background: #E4F1EC; color: #0B5A41; font-weight: 600; font-size: 12px;"
        "  border-top-left-radius: 9px; border-top-right-radius: 9px;"
        "  border-bottom: 1px solid #DDE1E3; padding: 9px 12px;"
        "}"

        // ── таблицы ──────────────────────────────────────────────────────
        "QTableWidget, QTableView {"
        "  border: 1px solid #DDE1E3; border-radius: 8px; gridline-color: #EEF0EF;"
        "  background: #FFFFFF; alternate-background-color: #F7F8F8; font-size: 12px;"
        "  selection-background-color: #E4F1EC; selection-color: #0B5A41;"
        "}"
        "QTableWidget::item, QTableView::item { padding: 5px 8px; border: none; }"
        "QTableWidget::item:selected, QTableView::item:selected { background: #E4F1EC; color: #0B5A41; }"
        "QHeaderView { background: transparent; border: none; }"
        "QHeaderView::section {"
        "  background: #E4F1EC; color: #0B5A41; font-weight: 600; font-size: 12px;"
        "  border: none; border-bottom: 1px solid #DDE1E3; border-right: 1px solid #EEF0EF;"
        "  padding: 7px 8px;"
        "}"
        "QHeaderView::section:last { border-right: none; }"
        "QTableCornerButton::section { background: #E4F1EC; border: none; border-bottom: 1px solid #DDE1E3; }"
        // В «Наземных условиях» подпись параметра живёт в вертикальном
        // заголовке, но по макету это обычная ячейка, а не шапка таблицы.
        "QTableWidget#tableWidget_parm1b65 QHeaderView::section {"
        "  background: transparent; color: #1B211F; font-weight: 400;"
        "  border: none; border-bottom: 1px solid #EEF0EF; padding: 7px 10px;"
        "}"
        "QTableWidget#tableWidget_parm1b65[state=\"narrow\"] QHeaderView::section {"
        "  font-size: 11px; padding: 7px 6px;"
        "}"

        // ── поля ввода и выпадающие списки ───────────────────────────────
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; padding: 4px 6px; background: #FFFFFF;"
        "  selection-background-color: #E4F1EC; selection-color: #0B5A41;"
        "}"
        "QLineEdit:read-only { background: #F7F8F8; }"
        "QComboBox {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; padding: 4px 8px; background: #FFFFFF;"
        "}"
        "QComboBox:disabled { background: #F7F8F8; color: #9AA3A0; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox QAbstractItemView {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; background: #FFFFFF;"
        "  selection-background-color: #E4F1EC; selection-color: #0B5A41; outline: none;"
        "}"

        // ── кнопки по умолчанию ──────────────────────────────────────────
        "QPushButton {"
        "  background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 6px;"
        "  padding: 7px 13px; color: #1B211F; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #F3F5F4; border-color: #0F6B4F; }"
        "QPushButton:disabled { color: #B7BEBB; background: #F1F3F2; border-color: #E6E9E8; }"

        // Тулбар Метео-11 — плоские контейнеры без рамки и заголовка,
        // сами кнопки-пилюли переключаются через свойство [pressed].
        "QGroupBox#groupBox_bulletenType, QGroupBox#groupBox_bulletenFormat {"
        "  border: none; background: transparent; margin: 0; padding: 0;"
        "}"
        "QGroupBox { border: 1px solid #DDE1E3; border-radius: 10px; background: #FFFFFF;"
        "  margin-top: 10px; padding-top: 8px; }"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 6px;"
        "  color: #6E7876; font-size: 11px; font-weight: 600; background: transparent;"
        "}"
        "QPushButton[pill=\"true\"] {"
        "  background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 8px;"
        "  color: #6E7876; font-weight: 600; font-size: 12px; padding: 8px 13px;"
        "}"
        "QPushButton[pill=\"true\"]:hover { border-color: #0F6B4F; color: #0B5A41; }"
        "QPushButton[pill=\"true\"][pressed=\"true\"] {"
        "  background: #0F6B4F; border-color: #0F6B4F; color: #FFFFFF;"
        "}"
        "QPushButton[pill=\"true\"]:disabled { background: #F1F3F2; border-color: #E6E9E8; color: #B7BEBB; }"
        // Бюллетень старше 12 ч — янтарная пилюля-предупреждение
        "QPushButton[pill=\"true\"][state=\"stale\"] {"
        "  background: #FFF4DC; border-color: #F9A825; color: #8A6100;"
        "}"
        "QPushButton[pill=\"true\"][state=\"stale\"][pressed=\"true\"] {"
        "  background: #E65100; border-color: #E65100; color: #FFFFFF;"
        "}"

        // Бейдж годности бюллетеня и время составления
        "QLineEdit#lineEdit_bulleten {"
        "  border: none; border-radius: 10px; padding: 5px 12px; font-weight: 700;"
        "  font-size: 11px; background: #E4F1EC; color: #0B5A41;"
        "}"
        "QLineEdit#lineEdit_bulleten[state=\"warn\"] { background: #FFF4DC; color: #8A6100; }"
        "QLineEdit#lineEdit_bulleten[state=\"none\"] { background: #F1F3F2; color: #9AA3A0; }"
        "QLineEdit#lineEdit_bulleten[state=\"bad\"]  { background: #FBE4E4; color: #B3261E; }"
        "QLineEdit#lineEdit_bulletenTime {"
        "  border: none; background: transparent; color: #6E7876; font-size: 11px;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "}"
        "QLineEdit#lineEdit_bulletenTime[state=\"stale\"] {"
        "  background: #FFF4DC; color: #8A6100; border-radius: 6px; padding: 2px 8px; font-weight: 700;"
        "}"
        // Табличный вид Метео-11: компактные подписи и узкие поля значений —
        // без этого подписи наследуют крупный шрифт из .ui и строки расползаются.
        "QWidget#page_meteo11_table QLabel { font-size: 10px; color: #6E7876; font-weight: 400; }"
        "QWidget#page_meteo11_table QLineEdit {"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "  font-size: 11px; font-weight: 600; color: #1B211F; max-width: 130px; padding: 1px 4px;"
        "}"
        "QWidget#page_meteo11_approximate QLabel { font-size: 10px; color: #6E7876; }"
        "QWidget#page_meteo11_approximate QLineEdit {"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "  font-size: 11px; color: #1B211F; padding: 1px 4px;"
        "}"
        "QTextEdit#textEdit_meteo11, QTextEdit#textEdit_meteo11_updated {"
        "  background: #F7F8F8; border: 1px solid #DDE1E3; border-radius: 8px; padding: 12px 14px;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace; font-size: 13px;"
        "}"

        // ── чекбоксы ─────────────────────────────────────────────────────
        "QCheckBox { spacing: 8px; font-size: 12px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px;"
        "  border: 1px solid #C7CDCB; background: #FFFFFF; }"
        "QCheckBox::indicator:hover { border-color: #0F6B4F; }"
        // Галочку QSS сам не рисует, если фон индикатора задан стилшитом,
        // поэтому подкладываем её изображением из ресурсов.
        "QCheckBox::indicator:checked { background: #0F6B4F; border-color: #0F6B4F;"
        "  image: url(:/icons/checkmark_white.svg); }"
        "QCheckBox::indicator:disabled { background: #F1F3F2; border-color: #E6E9E8; }"

        // ── полосы прокрутки (иначе через них проступает Fusion) ─────────
        "QScrollBar:vertical { background: transparent; width: 11px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: #C7CDCB; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #A9B2AF; }"
        "QScrollBar:horizontal { background: transparent; height: 11px; margin: 0px; }"
        "QScrollBar::handle:horizontal { background: #C7CDCB; border-radius: 5px; min-width: 30px; }"
        "QScrollBar::handle:horizontal:hover { background: #A9B2AF; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; border: none; background: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: none; }"
        "QAbstractScrollArea::corner { background: transparent; border: none; }"

        "QToolTip { background: #1B211F; color: #FFFFFF; border: none; padding: 5px 8px; }"
        );
}

// Единая настройка таблиц архива. В макете таблица занимает всю ширину
// панели, строки равной высоты чередуются полосами, вертикального заголовка
// нет (высота вынесена в обычную колонку), выделение — светло-зелёное.
void MeasurementResults::setupArchiveTables()
{
    const QList<QTableWidget *> tables = {
        ui->tableWidget_AverageWind, ui->tableWidget_realWind,
        ui->tableWidget_izmWind_2,   ui->table_windShear,
        ui->tableWidget_meteo11Formalize
    };

    for (QTableWidget *t : tables) {
        if (!t) continue;
        t->verticalHeader()->setVisible(false);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        t->setFocusPolicy(Qt::NoFocus);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setSelectionMode(QAbstractItemView::SingleSelection);
        t->setWordWrap(false);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->horizontalHeader()->setHighlightSections(false);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->verticalHeader()->setDefaultSectionSize(32);
        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    // Наземные условия: подпись параметра остаётся в вертикальном заголовке,
    // поэтому он виден и занимает основную ширину, а колонка значения узкая.
    if (QTableWidget *t = ui->tableWidget_parm1b65) {
        t->verticalHeader()->setVisible(true);
        t->horizontalHeader()->setVisible(false);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        t->setFocusPolicy(Qt::NoFocus);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionMode(QAbstractItemView::NoSelection);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        t->verticalHeader()->setDefaultSectionSize(36);
        t->verticalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->verticalHeader()->setMinimumWidth(360);
        // Таблица наземных условий короткая и фиксированной длины — прижимаем
        // её к верху вкладки по фактической высоте строк, как в макете, вместо
        // растягивания пустой рамки на всю панель.
        t->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        t->setFixedHeight(t->rowCount() * 36 + 2 * t->frameWidth() + 2);
        t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // Иначе единственный элемент сетки центрируется по вертикали и
        // таблица «висит» посреди пустой вкладки.
        if (ui->gridLayout_12)
            ui->gridLayout_12->setAlignment(t, Qt::AlignTop);
    }
}

// Оформление графиков Qwt под макет: карточка рисуется рамкой QFrame вокруг,
// поэтому сам график остаётся плоским и прозрачным. Ось высот вертикальная —
// привычный вид профиля; из макета взяты только цвета, пунктирная сетка и
// отсутствие подписей осей (название вынесено в зелёную шапку карточки).
void MeasurementResults::styleArchivePlot(QwtPlot *plot)
{
    if (!plot) return;

    plot->setAutoFillBackground(false);
    plot->setCanvasBackground(Qt::white);
    plot->setContentsMargins(0, 0, 0, 0);
    plot->plotLayout()->setCanvasMargin(0);
    plot->plotLayout()->setAlignCanvasToScales(true);

    if (auto *canvas = qobject_cast<QwtPlotCanvas *>(plot->canvas())) {
        canvas->setFrameStyle(QFrame::NoFrame);
        canvas->setBorderRadius(0);
        canvas->setPalette(QColor(Qt::white));
    }

    // Заголовок и подписи осей убраны — их роль играет шапка карточки
    plot->setTitle(QwtText());
    plot->setAxisTitle(QwtPlot::xBottom, QwtText());
    plot->setAxisTitle(QwtPlot::yLeft,   QwtText());

    QFont tickFont = plot->font();
    tickFont.setPixelSize(10);
    for (int axis : {QwtPlot::xBottom, QwtPlot::yLeft}) {
        plot->setAxisFont(axis, tickFont);
        if (QwtScaleWidget *sw = plot->axisWidget(axis)) {
            QPalette pal = sw->palette();
            pal.setColor(QPalette::WindowText, QColor("#C7CDCB")); // линия оси и засечки
            pal.setColor(QPalette::Text,       QColor("#6E7876")); // цифры на оси
            sw->setPalette(pal);
            sw->setMargin(0);
            sw->setSpacing(4);
        }
    }
}

// Единая пунктирная сетка макета (#EEF0EF) — заменяет чёрно-серую по умолчанию.
QwtPlotGrid *MeasurementResults::makeArchiveGrid()
{
    auto *grid = new QwtPlotGrid();
    grid->setMajorPen(QPen(QColor("#E2E6E4"), 0, Qt::DashLine));
    grid->enableXMin(false);
    grid->enableYMin(false);
    return grid;
}

// Линия профиля: тонкая цветная кривая с белыми точками-кружками, как в макете.
void MeasurementResults::styleArchiveCurve(QwtPlotCurve *curve, const QColor &color)
{
    if (!curve) return;
    curve->setPen(QPen(color, 2));
    curve->setStyle(QwtPlotCurve::Lines);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                   QBrush(Qt::white), QPen(color, 2), QSize(6, 6)));
}

// Строит самодельную строку вкладок поверх скрытой нативной QTabBar (см.
// комментарий у объявления m_customTabBar в .h) — единственный способ
// гарантированно получить плоские скруглённые сверху вкладки макета
// независимо от системной темы/QStyle.
void MeasurementResults::setupCustomTabBar()
{
    QTabWidget *tabs = ui->tabWidget;
    if (!tabs) return;

    // Одного hide() недостаточно: QTabWidget при перекладке (setUpLayout)
    // возвращает своей QTabBar видимость, и системные вкладки снова
    // проступают поверх самодельной строки. Дополнительно зажимаем её
    // высоту в ноль, чтобы она не занимала места ни в каком случае.
    tabs->tabBar()->hide();
    tabs->tabBar()->setFixedHeight(0);

    m_customTabBar = new QWidget(this);
    m_customTabBar->setObjectName("customTabBar");
    // Раньше здесь был FlowLayout, переносивший лишние вкладки на вторую
    // строку — из-за этого высота строки вкладок "прыгала" (1 или 2 строки)
    // и съедала место у контента под ней. Вкладки должны быть в один ряд
    // всегда; если все шесть подряд не помещаются по ширине — строка вкладок
    // скроллится по горизонтали (обёрнута в scrollArea_tabBar ниже), а не
    // переносится вниз.
    auto *layout = new QHBoxLayout(m_customTabBar);
    layout->setSpacing(2);
    layout->setContentsMargins(16, 8, 16, 0);

    m_tabButtons.clear();
    for (int i = 0; i < tabs->count(); ++i) {
        auto *btn = new QPushButton(tabs->tabText(i), m_customTabBar);
        btn->setObjectName("archiveTabButton");
        btn->setProperty("active", i == tabs->currentIndex());
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(btn, &QPushButton::clicked, this, [tabs, i] { tabs->setCurrentIndex(i); });
        layout->addWidget(btn);
        m_tabButtons.append(btn);
    }
    layout->addStretch(1);

    connect(tabs, &QTabWidget::currentChanged, this, &MeasurementResults::updateCustomTabBarHighlight);

    // Строка вкладок — сама по себе узкая полоска фиксированной высоты
    // внутри горизонтального scrollArea без рамки и без вертикальной
    // прокрутки: обёртка нужна только на случай, если сумма ширин вкладок
    // всё же превысит ширину экрана.
    auto *tabBarScroll = new QScrollArea(this);
    tabBarScroll->setObjectName("scrollArea_tabBar");
    tabBarScroll->setWidget(m_customTabBar);
    tabBarScroll->setWidgetResizable(false);
    tabBarScroll->setFrameShape(QFrame::NoFrame);
    tabBarScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabBarScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tabBarScroll->setFixedHeight(36);
    tabBarScroll->setStyleSheet("QScrollArea#scrollArea_tabBar { background: #EFF1F1; border: none; }");

    // Вставляем строку кнопок между статусной строкой (row 0) и самим
    // QTabWidget (row 1) в gridLayout_2, сдвигая tabWidget на row 2.
    QGridLayout *grid = ui->gridLayout_2;
    if (grid) {
        grid->removeWidget(tabs);
        grid->addWidget(tabBarScroll, 1, 0);
        grid->addWidget(tabs, 2, 0);
    }
}

void MeasurementResults::updateCustomTabBarHighlight(int currentIndex)
{
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        QPushButton *btn = m_tabButtons[i];
        if (!btn) continue;
        btn->setProperty("active", i == currentIndex);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

// Дополнительные поля АМС/зонда (координаты, поправки, даты) на странице
// "приближённый" бюллетень Метео-11 отсутствуют в новом макете архива —
// оставляем функциональность, но по умолчанию сворачиваем блок за кнопкой.
void MeasurementResults::setupAmsProbeCollapse()
{
    m_amsProbeWidgets = {
        ui->label_amsDegrees1, ui->lineEdit_AMSDegrees1, ui->label_amsMinutes1, ui->lineEdit_AMSMinutes1,
        ui->label_amsSeconds1, ui->lineEdit_AMSSeconds1, ui->label_amsLongitude, ui->lineEdit_AMSLongtitude,
        ui->label_amsDegrees2, ui->lineEdit_AMSDegrees2, ui->label_amsMinutes2, ui->lineEdit_AMSMinutes2,
        ui->label_amsSeconds2, ui->lineEdit_AMSSeconds2, ui->label_amsLatitude, ui->lineEdit_AMSLatitude,
        ui->label_ams_title,
        ui->label_minutesFromProbe, ui->lineEdit_AMSMinutesFromProbe, ui->label_probeDistance, ui->lineEdit_ProbeDistance,
        ui->label_bulletinType, ui->lineEdit_AMSTypeBl, ui->label_bulletinDate, ui->lineEdit_AMSDateBl,
        ui->label_amsBias, ui->lineEdit_AMSBias,
        ui->label_lastProbeDate, ui->lineEdit_DateLastUTC, ui->label_biasUTC, ui->lineEdit_BiasUTC,
        ui->label_avgTime, ui->lineEdit_avgTime,
        ui->label_probeDegrees1, ui->lineEdit_probeDegrees1, ui->label_probeMinutes1, ui->lineEdit_probeMinutes1,
        ui->label_probeSeconds1, ui->lineEdit_probeSeconds1, ui->label_probeLongitude, ui->lineEdit_probeLongtitude,
        ui->label_probeDegrees2, ui->lineEdit_probeDegrees2, ui->label_probeMinutes2, ui->lineEdit_probeMinutes2,
        ui->label_probeSeconds2, ui->lineEdit_probeSeconds2, ui->label_probeLatitude, ui->lineEdit_probeLatitude,
        ui->label_probe_title,
    };

    for (QWidget *w : qAsConst(m_amsProbeWidgets))
        if (w) w->setVisible(false);

    auto *toggleBtn = new QPushButton("Показать доп. поля АМС/зонда ▾", ui->page_meteo11_approximate);
    toggleBtn->setStyleSheet(
        "QPushButton { border: 1px solid #DDE1E3; border-radius: 8px; background: #F7F8F8;"
        " color: #6E7876; font-size: 11px; font-weight: 600; padding: 6px 12px; }"
        "QPushButton:hover { border-color: #0F6B4F; color: #0B5A41; }");
    connect(toggleBtn, &QPushButton::clicked, this, [this, toggleBtn] {
        m_amsProbeFieldsVisible = !m_amsProbeFieldsVisible;
        for (QWidget *w : qAsConst(m_amsProbeWidgets))
            if (w) w->setVisible(m_amsProbeFieldsVisible);
        toggleBtn->setText(m_amsProbeFieldsVisible
                                ? "Скрыть доп. поля АМС/зонда ▴"
                                : "Показать доп. поля АМС/зонда ▾");
    });

    // Добавляем новой строкой в конец существующей сетки — не задевает
    // расположение уже имеющихся полей.
    if (ui->gridLayout_approximate)
        ui->gridLayout_approximate->addWidget(toggleBtn, ui->gridLayout_approximate->rowCount(), 0, 1, -1);
}

void MeasurementResults::onExportBackRequested()
{
    ui->rootStack->setCurrentWidget(ui->archivePage);
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

    // Загружаем ВСЕ записи архива (без ограничения по дате — требование
    // хранения не менее года). Наличие профилей ветра определяется тем же
    // запросом через LEFT JOIN на wind_profiles_references — это убирает
    // N отдельных запросов в цикле и делает открытие архива быстрым даже
    // на больших объёмах.
    //
    // CASE WHEN ... IS NOT NULL — флаг наличия соответствующего профиля.
    QString sql =
        "SELECT "
        "   ma.record_id, "
        "   ma.completion_time, "
        "   ma.notes, "
        "   (wpr.avg_wind_profile_id      IS NOT NULL) AS has_avg, "
        "   (wpr.actual_wind_profile_id   IS NOT NULL) AS has_actual, "
        "   (wpr.measured_wind_profile_id IS NOT NULL) AS has_measured "
        "FROM main_archive ma "
        "LEFT JOIN wind_profiles_references wpr "
        "       ON wpr.record_id = ma.record_id "
        "ORDER BY ma.completion_time DESC";

    qDebug() << "MeasurementResults: Выполняем запрос к main_archive (весь архив)...";

    if (!query.exec(sql)) {
        qCritical() << "MeasurementResults: Ошибка SQL:" << query.lastError().text();
        qDebug() << "SQL запрос:" << sql;
        return;
    }

    int totalRecords = 0;

    while (query.next()) {
        MeasurementRecord record;
        record.recordId        = query.value(0).toInt();
        record.measurementTime = query.value(1).toDateTime();
        record.notes           = query.value(2).toString();

        // Флаги наличия профилей пришли тем же запросом — без доп. обращений к БД
        record.hasAvgWind      = query.value(3).toBool();
        record.hasActualWind   = query.value(4).toBool();
        record.hasMeasuredWind = query.value(5).toBool();

        const QDate date = record.measurementTime.date();
        availableMeasurements[date].append(record);
        totalRecords++;
    }

    qInfo() << "MeasurementResults: Загружено" << totalRecords
            << "записей из main_archive (весь архив)";

    // Сортируем записи внутри каждой даты по времени (новые сверху)
    for (auto it = availableMeasurements.begin(); it != availableMeasurements.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                  [](const MeasurementRecord &a, const MeasurementRecord &b) {
                      return a.measurementTime > b.measurementTime;
                  });
    }

    qInfo() << "MeasurementResults: Данные распределены по"
            << availableMeasurements.size() << "датам";

    // Список дат для отладки
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
        "SELECT height, wind_speed, wind_direction, reliability "
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
        // Достоверность от АМС (1 - достоверная, 0 - недостоверная)
        point.reliability   = query.value(3).toInt();
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
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
        clearStationCoordinates();
        return;
    }

    double lat = query.value(0).toDouble();
    double lon = query.value(1).toDouble();
    double alt = query.value(2).toDouble();

    m_currentLatitude = lat;
    m_currentLongitude = lon;

    // Значения выводятся одной строкой с буквой полушария — как в макете
    // ("55°45'21\" N"), поэтому отдельные выпадающие списки больше не нужны.
    ui->valLatitude->setText(CoordHelper::toDisplayDMS(qAbs(lat)) + (lat >= 0 ? " N" : " S"));
    ui->valLongitude->setText(CoordHelper::toDisplayDMS(qAbs(lon)) + (lon >= 0 ? " E" : " W"));
    ui->valAltitude->setText(QString::number(alt, 'f', 1) + " м");
    m_stationCoordsValid = true;

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

void MeasurementResults::updateAvailableRecordsLabel()
{
    QDate date = currentDateTime.date();

    int recordCount = 0;
    if (availableMeasurements.contains(date)) {
        recordCount = availableMeasurements[date].size();
    }

    // Цвет задаётся не setStyleSheet() на самой метке (это стёрло бы остальные
    // правила для неё), а динамическим свойством, на которое есть селектор в
    // applyArchiveStyle().
    if (recordCount > 0) {
        ui->lblAvailableRecords->setText(
            QString("Доступно записей: %1 · нажмите на дату для выбора").arg(recordCount));
        setWidgetState(ui->lblAvailableRecords, "ok");
    } else {
        ui->lblAvailableRecords->setText("Нет данных за выбранную дату");
        setWidgetState(ui->lblAvailableRecords, "empty");
    }
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

    if (enabled) {
        ui->btnPrevDate->setEnabled(false);
        ui->btnNextDate->setEnabled(false);
        ui->btnSelectDate->setEnabled(false);
    } else {
        ui->btnPrevDate->setEnabled(true);
        ui->btnNextDate->setEnabled(true);
        ui->btnSelectDate->setEnabled(true);
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
    snap.coordinatesValid = m_stationCoordsValid;
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
            snap.pressureHpa      = cell(0);
            snap.temperatureC     = cell(1);
            snap.humidityPct      = cell(2);
            snap.surfaceWindDir   = cell(3);
            snap.surfaceWindSpeed = cell(4);
        }
    }

    // ── Профили ветра ────────────────────────────────────────────────────────
    // Загружаем из БД (лёгкий повторный запрос — данные кешируются на уровне БД)
    snap.avgWind      = m_currentAvgWind;
    snap.actualWind   = m_currentActualWind;
    snap.measuredWind = m_currentMeasuredWind;

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

    const int CW = 400, CH = 280;
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
    // Собираем снимок данных и переключаемся на встроенный экран экспорта
    // (страница 1 rootStack) вместо модального ExportDialog.
    MeasurementSnapshot snap = buildSnapshot();

    if (snap.recordId <= 0) {
        showStatus("Нет данных для экспорта: выберите дату и время с доступными измерениями, "
                   "чтобы данные были загружены из архива.", NotificationToast::Error);
        return;
    }

    m_exportView->setSnapshot(snap);
    ui->rootStack->setCurrentWidget(m_exportView);
}

void MeasurementResults::onExportSubmitted(const MeasurementSnapshot &snap, const ExportOptions &opts)
{
    // Диалог выбора пути
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
            showStatus("Ошибка экспорта: " + errorMsg, NotificationToast::Error);
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            showStatus(QString("Не удалось открыть файл: %1. Ошибка: %2")
                           .arg(path, file.errorString()),
                       NotificationToast::Error);
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
            showStatus("Ошибка экспорта: " + errorMsg, NotificationToast::Error);
        return;
    }

    // Успешный экспорт — возвращаемся к архиву, чтобы не держать пользователя
    // на экране экспорта; при ошибке остаёмся, чтобы можно было поправить опции.
    ui->rootStack->setCurrentWidget(ui->archivePage);

    m_toast->showMessageWithAction(
        QString("Файл успешно сохранён: %1").arg(QFileInfo(path).fileName()),
        NotificationToast::Success,
        "Открыть папку",
        [path]() {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
        });
}