#include "functionalcontroldialog.h"
#include "ui_functionalcontroldialog.h"
#include <QListWidgetItem>
#include <QDateTime>
#include <QShowEvent>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QButtonGroup>
#include <QTimer>
#include <QEvent>
#include <QStyle>
#include <QAbstractItemView>

// Таблица неисправностей АМС — Таблица 2 протокола ГИЕФ.00515-01 90 01.
// Нумерация битов 0-индексированная (протокол даёт 1-индекс).
// Бит = 0 → устройство НЕ работает (неисправность).
// Бит = 1 → устройство исправно.
const QVector<FunctionalControlDialog::FaultEntry> FunctionalControlDialog::s_amsFaultTable = {
    { 0, "Превышено время ожидания завершения вращения"      },
    { 1, "Аварийная остановка открытия/закрытия антенны"     },
    { 2, "Превышено время ожидания открытия антенны"         },
    { 3, "Превышено время ожидания закрытия антенны"         },
    { 4, "Нет сбора данных"                                  },
    { 5, "СЧ не пошёл контроль"                             },
    { 6, "Не готов передатчик"                              },
    { 7, "Ошибка ПО"                                        },
    { 8, "Неверное значение даты и времени"                 },
};

// ── Небольшой хелпер: строка списка в стиле макета —
//    цветная полоска слева + текст + цветная метка-тег справа. ──────────────
static QWidget *buildEntryRowWidget(const QString &text, bool isFault)
{
    const QString barColor = isFault ? "#B71C1C" : "#8D5B00";
    const QString tagBg    = isFault ? "#FFEBEE" : "#FFF8E1";
    const QString tagFg    = isFault ? "#B71C1C" : "#8D5B00";
    const QString tagText  = isFault ? "НЕИСПРАВНОСТЬ" : "ОШИБКА";

    QWidget *row = new QWidget();
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(14, 11, 14, 11);
    layout->setSpacing(12);

    QFrame *bar = new QFrame(row);
    bar->setFixedWidth(4);
    bar->setMinimumHeight(20);
    bar->setStyleSheet(QString("background:%1; border-radius:2px;").arg(barColor));
    layout->addWidget(bar);

    QLabel *title = new QLabel(text, row);
    title->setWordWrap(true);
    title->setStyleSheet("font-size: 10pt; font-weight: 600; color: #1C1F22; background: transparent;");
    layout->addWidget(title, 1);

    QLabel *tag = new QLabel(tagText, row);
    tag->setStyleSheet(QString(
        "font-size: 7.5pt; font-weight: bold; letter-spacing: .3px; "
        "background:%1; color:%2; border-radius: 8px; padding: 3px 9px;")
        .arg(tagBg, tagFg));
    tag->setAlignment(Qt::AlignCenter);
    layout->addWidget(tag, 0, Qt::AlignVCenter);

    return row;
}

FunctionalControlDialog::FunctionalControlDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FunctionalControlDialog)
    , m_sensorType(AMS)
{
    ui->setupUi(this);

    connect(ui->btnBackFromFuncControl, &QPushButton::clicked,
            this, &FunctionalControlDialog::backRequested);

    connect(ui->btnRefresh, &QPushButton::clicked,
            this, &FunctionalControlDialog::onRefreshButtonClicked);

    // ── фильтр-чипы: взаимоисключающий выбор ──
    QButtonGroup *chipGroup = new QButtonGroup(this);
    chipGroup->setExclusive(true);
    chipGroup->addButton(ui->chipAll);
    chipGroup->addButton(ui->chipFaults);
    chipGroup->addButton(ui->chipErrors);
    connect(ui->chipAll,    &QPushButton::toggled, this, &FunctionalControlDialog::onChipToggled);
    connect(ui->chipFaults, &QPushButton::toggled, this, &FunctionalControlDialog::onChipToggled);
    connect(ui->chipErrors, &QPushButton::toggled, this, &FunctionalControlDialog::onChipToggled);

    // ── баннер неисправностей кликабелен (обычный QFrame + фильтр событий) ──
    ui->frameAlertBanner->installEventFilter(this);
    ui->frameAlertBanner->setVisible(false);

    // ── пульсация индикатора автоопроса ──
    m_pollDotEffect = new QGraphicsOpacityEffect(ui->lblPollDot);
    ui->lblPollDot->setGraphicsEffect(m_pollDotEffect);
    m_pollDotAnim = new QPropertyAnimation(m_pollDotEffect, "opacity", this);
    m_pollDotAnim->setDuration(900);
    m_pollDotAnim->setStartValue(1.0);
    m_pollDotAnim->setKeyValueAt(0.5, 0.25);
    m_pollDotAnim->setEndValue(1.0);
    m_pollDotAnim->setLoopCount(-1);

    // ── автоопрос: пока страница видна, повторяем refreshRequested раз в 30 с ──
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(30000);
    connect(m_pollTimer, &QTimer::timeout,
            this, &FunctionalControlDialog::refreshRequested);

    updateSensorTitle();
    resetDisplay();
}

FunctionalControlDialog::~FunctionalControlDialog()
{
    delete ui;
}

bool FunctionalControlDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->frameAlertBanner && event->type() == QEvent::MouseButtonRelease) {
        onAlertBannerClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void FunctionalControlDialog::onAlertBannerClicked()
{
    // Прокрутить/подсветить первую неисправность (а если их нет — первую ошибку)
    if (ui->listFaults->count() > 0) {
        ui->chipAll->setChecked(true);
        QListWidgetItem *item = ui->listFaults->item(0);
        ui->listFaults->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        flashItem(item, ui->listFaults);
    } else if (ui->listErrors->count() > 0) {
        ui->chipAll->setChecked(true);
        QListWidgetItem *item = ui->listErrors->item(0);
        ui->listErrors->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        flashItem(item, ui->listErrors);
    }
}

void FunctionalControlDialog::flashItem(QListWidgetItem *item, QListWidget *list)
{
    if (!item || !list) return;
    list->setCurrentItem(item);
    item->setSelected(true);
    // Контекст-объект list: если список успеет очиститься (обновление данных)
    // до срабатывания таймера, соединение само отключится — на "item" здесь
    // не ссылаемся, чтобы не обращаться к уже удалённому указателю.
    QTimer::singleShot(1200, list, [list]() {
        list->clearSelection();
    });
}

void FunctionalControlDialog::onChipToggled()
{
    applyFilter();
}

void FunctionalControlDialog::applyFilter()
{
    // Пока не получен ни один реальный ответ от устройства (страница только
    // открыта / идёт ожидание / нет подключения) — списки всегда пусты.
    // Раньше в этом случае область контента была просто пустой (белый экран).
    // Теперь явно поясняем происходящее, даже на вкладке "Все".
    if (!m_hasData) {
        ui->groupFaults->setVisible(false);
        ui->groupErrors->setVisible(false);
        ui->lblAllOk->setVisible(false);
        ui->lblFilterEmpty->setText(
            ui->pillDisconnected->isVisible()
                ? "Нет подключения к устройству — данные недоступны"
                : "Данные ещё не получены");
        ui->lblFilterEmpty->setVisible(true);
        return;
    }

    const bool showFaults = ui->chipAll->isChecked() || ui->chipFaults->isChecked();
    const bool showErrors = ui->chipAll->isChecked() || ui->chipErrors->isChecked();
    const bool faultsVisible = showFaults && ui->listFaults->count() > 0;
    const bool errorsVisible = showErrors && ui->listErrors->count() > 0;
    ui->groupFaults->setVisible(faultsVisible);
    ui->groupErrors->setVisible(errorsVisible);

    // Если общее состояние не "всё исправно", но выбранный фильтр ничего
    // не показывает (например, чип "Ошибки" при отсутствии ошибок) — поясняем.
    // Также покрывает вкладку "Все", если вдруг она ничего не показывает.
    const bool nothingToShow = !faultsVisible && !errorsVisible;
    const bool shouldShowEmptyHint = nothingToShow && !ui->lblAllOk->isVisible();
    if (shouldShowEmptyHint) {
        ui->lblFilterEmpty->setText("По выбранному фильтру записей нет");
    }
    ui->lblFilterEmpty->setVisible(shouldShowEmptyHint);
}

void FunctionalControlDialog::onRefreshButtonClicked()
{
    emit refreshRequested();
    // визуально перезапускаем автотаймер, чтобы следующий автоопрос не наступил
    // сразу вслед за ручным обновлением
    if (m_pollTimer->isActive()) {
        m_pollTimer->start();
    }
}

void FunctionalControlDialog::setRefreshBusy(bool busy)
{
    ui->btnRefresh->setProperty("busy", busy);
    ui->btnRefresh->style()->unpolish(ui->btnRefresh);
    ui->btnRefresh->style()->polish(ui->btnRefresh);
}

void FunctionalControlDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // При каждом появлении страницы — автоматически запрашиваем данные
    emit refreshRequested();
    m_pollTimer->start();
    m_pollDotAnim->start();
}

void FunctionalControlDialog::hideEvent(QHideEvent *event)
{
    m_pollTimer->stop();
    m_pollDotAnim->stop();
    QWidget::hideEvent(event);
}

void FunctionalControlDialog::setSensorType(SensorType type)
{
    m_sensorType = type;
    updateSensorTitle();
    resetDisplay();
}

void FunctionalControlDialog::updateSensorTitle()
{
    QString name;
    switch (m_sensorType) {
        case AMS:  name = "АМС";  break;
        case GNSS: name = "GNSS"; break;
        case BINS: name = "БИНС"; break;
        case IWS:  name = "ИВС";  break;
    }
    ui->lblDialogTitle->setText(
        QString("Функциональный контроль %1").arg(name));
}

void FunctionalControlDialog::resetDisplay()
{
    ui->lblPowerOnCountHeader->setText("Включений: —");
    ui->lblPowerOnCountHeader->setVisible(true);
    ui->pillDisconnected->setVisible(false);

    ui->listFaults->clear();
    ui->listErrors->clear();
    ui->lblAllOk->setVisible(false);
    ui->frameAlertBanner->setVisible(false);

    updateChipCounts(0, 0);

    ui->lblLastPoll->setText("Последний опрос: —");
    ui->lblStatusBar->setText("Ожидание данных");

    ui->btnRefresh->setEnabled(true);
    setRefreshBusy(false);

    m_hasData = false;
    applyFilter(); // покажет "Данные ещё не получены" вместо пустого экрана
}

void FunctionalControlDialog::setWaitingState()
{
    ui->pillDisconnected->setVisible(false);
    ui->lblPowerOnCountHeader->setVisible(true);
    ui->lblStatusBar->setText("Запрос отправлен, ожидание ответа...");
    ui->btnRefresh->setEnabled(false);
    setRefreshBusy(true);
}

void FunctionalControlDialog::setDisconnectedState()
{
    // Раньше текст "Нет подключения к устройству" шёл обычной строкой в
    // lblStatusBar — теперь это отдельный красный пилл в шапке страницы,
    // как остальные индикаторы подключения (по образцу топбара).
    ui->lblPowerOnCountHeader->setVisible(false);
    ui->pillDisconnected->setVisible(true);

    ui->listFaults->clear();
    ui->listErrors->clear();
    ui->frameAlertBanner->setVisible(false);
    ui->lblAllOk->setVisible(false);

    updateChipCounts(0, 0);

    ui->lblStatusBar->clear();
    ui->btnRefresh->setEnabled(false);
    setRefreshBusy(false);

    m_hasData = false;
    applyFilter(); // покажет "Нет подключения к устройству — данные недоступны"
}

void FunctionalControlDialog::setErrorState(const QString &errorText)
{
    ui->lblStatusBar->setText("Ошибка: " + errorText);
    ui->btnRefresh->setEnabled(true);
    setRefreshBusy(false);
    applyFilter(); // если данных ещё не было — подсказка в контенте актуальна и здесь
}

void FunctionalControlDialog::setAmsData(quint32 bitMask, quint32 powerOnCount)
{
    ui->pillDisconnected->setVisible(false);
    ui->lblPowerOnCountHeader->setVisible(true);
    ui->lblPowerOnCountHeader->setText(
        QString("Включений: %1").arg(powerOnCount));

    FuncControlResult fc = AMSProtocol::funcControlDetails(bitMask);
    populateFromResult(fc);
    updateLastPollTime();

    if (fc.allOk()) {
        ui->lblStatusBar->setText("Всё оборудование исправно");
    } else {
        ui->lblStatusBar->setText(
            QString("Неисправностей: %1, ошибок: %2")
                .arg(fc.faults.size()).arg(fc.errors.size()));
    }
    ui->btnRefresh->setEnabled(true);
    setRefreshBusy(false);
}

void FunctionalControlDialog::updateLastPollTime()
{
    ui->lblLastPoll->setText(
        QString("Последний опрос: %1")
            .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")));
}

void FunctionalControlDialog::updateAlertBanner(int faultCount, int errorCount)
{
    if (faultCount == 0 && errorCount == 0) {
        ui->frameAlertBanner->setVisible(false);
        return;
    }
    ui->lblAlertSub->setText(
        QString("Неисправностей: %1 · Ошибок: %2").arg(faultCount).arg(errorCount));
    ui->frameAlertBanner->setVisible(true);
}

void FunctionalControlDialog::updateChipCounts(int faultCount, int errorCount)
{
    const int total = faultCount + errorCount;
    ui->chipAll->setText(QString("Все (%1)").arg(total));
    ui->chipFaults->setText(QString("Неисправности (%1)").arg(faultCount));
    ui->chipErrors->setText(QString("Ошибки (%1)").arg(errorCount));
}

void FunctionalControlDialog::populateFromResult(const FuncControlResult &fc)
{
    m_hasData = true; // получен настоящий ответ — дальше applyFilter() судит по данным

    // --- Неисправности ---
    ui->listFaults->clear();
    for (const QString &desc : fc.faults) {
        QListWidgetItem *item = new QListWidgetItem(ui->listFaults);
        QWidget *row = buildEntryRowWidget(desc, /*isFault=*/true);
        item->setSizeHint(row->sizeHint());
        ui->listFaults->setItemWidget(item, row);
    }
    ui->lblFaultsSectionLabel->setText(
        QString("ОБНАРУЖЕННЫЕ НЕИСПРАВНОСТИ · %1").arg(fc.faults.size()));

    // --- Ошибки ---
    ui->listErrors->clear();
    for (const QString &desc : fc.errors) {
        QListWidgetItem *item = new QListWidgetItem(ui->listErrors);
        QWidget *row = buildEntryRowWidget(desc, /*isFault=*/false);
        item->setSizeHint(row->sizeHint());
        ui->listErrors->setItemWidget(item, row);
    }
    ui->lblErrorsSectionLabel->setText(
        QString("ОБНАРУЖЕННЫЕ ОШИБКИ · %1").arg(fc.errors.size()));

    ui->lblAllOk->setVisible(fc.allOk());
    updateChipCounts(fc.faults.size(), fc.errors.size());
    updateAlertBanner(fc.faults.size(), fc.errors.size());
    applyFilter();
}
