#include "ArchiveDatePopup.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QStyle>
#include <QStyleFactory>
#include <QVariant>
#include <QScreen>
#include <QGuiApplication>
#include <algorithm>

namespace {
const char *kMonthNames[12] = {
    "Январь", "Февраль", "Март", "Апрель", "Май", "Июнь",
    "Июль", "Август", "Сентябрь", "Октябрь", "Ноябрь", "Декабрь"
};
const char *kDowNames[7] = { "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс" };
}

ArchiveDatePopup::ArchiveDatePopup(QWidget *parent)
    : QWidget(parent)
    , m_calMonthOffset(0)
    , m_calendarShown(false)
{
    setWindowFlags(Qt::Popup);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(296);   // ещё немного уже (было 340)
    // Popup — отдельное top-level окно, стиль родителя на него не наследуется,
    // а QSS ниже рассчитан на Fusion (см. MeasurementResults::applyArchiveStyle).
    setStyle(QStyleFactory::create("Fusion"));
    setStyleSheet(
        "ArchiveDatePopup { background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 14px; }"
        "QLabel { color: #1B211F; font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; }"
        "QLabel#dpHead { color: #0B5A41; font-size: 9px; font-weight: 700; }"
        "QLabel#dpBigDate { font-size: 14px; font-weight: 700; font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace; }"
        "QLabel#dpTimesLabel { color: #6E7876; font-size: 9px; font-style: italic; }"
        "QLabel#dpCalMonth { font-size: 11px; font-weight: 700; }"
        "QLabel[class=\"dow\"] { color: #6E7876; font-size: 8px; font-weight: 700; qproperty-alignment: AlignCenter; }"
        "QPushButton { font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; }"
        "QPushButton[class=\"navbtn\"] { background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 6px; font-weight: 700; color: #0B5A41; padding: 0px; font-size: 12px; }"
        "QPushButton[class=\"navbtn\"]:hover { background: #E4F1EC; }"
        "QPushButton[class=\"quick\"] { font-size: 9px; padding: 4px 7px; border-radius: 999px; border: 1px solid #DDE1E3; background: #F7F8F8; color: #6E7876; }"
        "QPushButton[class=\"quick\"]:hover { border-color: #0F6B4F; color: #0B5A41; }"
        "QPushButton#dpCalToggle[on=\"true\"] { background: #E4F1EC; border-color: #0F6B4F; color: #0B5A41; font-weight: 600; }"
        "QPushButton[class=\"calday\"] { border-radius: 7px; border: 1px solid transparent; background: #F1F3F2; color: #C1C8C5; font-weight: 600; min-height: 25px; font-size: 10px; }"
        "QPushButton[class=\"calday\"][avail=\"true\"] { background: #FFFFFF; border-color: #DDE1E3; color: #1B211F; }"
        "QPushButton[class=\"calday\"][complete=\"full\"][avail=\"true\"] { background: #E4F1EC; border-color: #A9D3C3; }"
        "QPushButton[class=\"calday\"][complete=\"partial\"][avail=\"true\"] { background: #FFF8E8; border-color: #F0D28C; }"
        "QPushButton[class=\"calday\"][avail=\"true\"]:hover { border-color: #0F6B4F; background: #E4F1EC; }"
        // Выбранный день должен перебивать раскраску по полноте данных: в QSS при
        // равной специфичности выигрывает более «нагруженный» селектор, поэтому
        // здесь перечислены все три свойства.
        "QPushButton[class=\"calday\"][avail=\"true\"][selected=\"true\"] { background: #0F6B4F; border-color: #0F6B4F; color: #FFFFFF; }"
        "QPushButton[class=\"calday\"][today=\"true\"] { border-color: #F9A825; border-width: 2px; }"
        "QPushButton[class=\"timechip\"] { font-size: 10px; padding: 4px 7px 4px 6px; border-radius: 7px; border: 1px solid #DDE1E3; border-left: 4px solid #DDE1E3; background: #FFFFFF; color: #1B211F; font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace; }"
        "QPushButton[class=\"timechip\"]:hover { border-color: #0F6B4F; }"
        "QPushButton[class=\"timechip\"][complete=\"full\"] { border-left-color: #0F6B4F; background: #E4F1EC; }"
        "QPushButton[class=\"timechip\"][complete=\"partial\"] { border-left-color: #F9A825; background: #FFF8E8; }"
        "QPushButton[class=\"timechip\"][complete=\"empty\"] { border-left-color: #C1C8C5; color: #6E7876; }"
        "QPushButton[class=\"timechip\"][active=\"true\"] { background: #0F6B4F; border-color: #0F6B4F; border-left-color: #0B5A41; color: #FFFFFF; }"
        "QPushButton#dpDoneBtn { background: #0F6B4F; border: none; border-radius: 7px; color: #FFFFFF; font-weight: 600; padding: 5px 12px; font-size: 10px; }"
        "QPushButton#dpDoneBtn:hover { background: #0B5A41; }"
        "QLabel#dpLegend { color: #6E7876; font-size: 8px; }"
        );

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(6);

    auto *head = new QLabel("ВЫБОР ДАТЫ И ВРЕМЕНИ", this);
    head->setObjectName("dpHead");
    root->addWidget(head);

    auto *nav = new QHBoxLayout();
    nav->setSpacing(8);
    auto *prevBtn = new QPushButton("◄", this);
    prevBtn->setProperty("class", "navbtn");
    prevBtn->setFixedSize(25, 25);   // ещё немного меньше (было 28x28)
    connect(prevBtn, &QPushButton::clicked, this, [this] { stepDate(-1); });
    m_bigDate = new QLabel(this);
    m_bigDate->setObjectName("dpBigDate");
    m_bigDate->setAlignment(Qt::AlignCenter);
    auto *nextBtn = new QPushButton("►", this);
    nextBtn->setProperty("class", "navbtn");
    nextBtn->setFixedSize(25, 25);
    connect(nextBtn, &QPushButton::clicked, this, [this] { stepDate(1); });
    nav->addWidget(prevBtn);
    nav->addWidget(m_bigDate, 1);
    nav->addWidget(nextBtn);
    root->addLayout(nav);

    auto *quick = new QHBoxLayout();
    quick->setSpacing(5);
    auto *todayBtn = new QPushButton("Сегодня", this);
    todayBtn->setProperty("class", "quick");
    connect(todayBtn, &QPushButton::clicked, this, [this] { jumpToday(); });
    auto *latestBtn = new QPushButton("Последняя запись", this);
    latestBtn->setProperty("class", "quick");
    connect(latestBtn, &QPushButton::clicked, this, [this] { jumpLatest(); });
    m_calToggleBtn = new QPushButton("Календарь", this);
    m_calToggleBtn->setObjectName("dpCalToggle");
    m_calToggleBtn->setProperty("class", "quick");
    connect(m_calToggleBtn, &QPushButton::clicked, this, [this] { toggleCalendar(); });
    quick->addWidget(todayBtn);
    quick->addWidget(latestBtn);
    quick->addWidget(m_calToggleBtn);
    root->addLayout(quick);

    m_calendarBox = new QWidget(this);
    auto *calLayout = new QVBoxLayout(m_calendarBox);
    calLayout->setContentsMargins(0, 6, 0, 6);
    calLayout->setSpacing(6);

    auto *calHead = new QHBoxLayout();
    calHead->setSpacing(8);
    auto *calPrev = new QPushButton("◄", m_calendarBox);
    calPrev->setProperty("class", "navbtn");
    calPrev->setFixedSize(24, 24);
    connect(calPrev, &QPushButton::clicked, this, [this] { stepMonth(-1); });
    m_calMonthLabel = new QLabel(m_calendarBox);
    m_calMonthLabel->setObjectName("dpCalMonth");
    m_calMonthLabel->setAlignment(Qt::AlignCenter);
    auto *calNext = new QPushButton("►", m_calendarBox);
    calNext->setProperty("class", "navbtn");
    calNext->setFixedSize(24, 24);
    connect(calNext, &QPushButton::clicked, this, [this] { stepMonth(1); });
    calHead->addWidget(calPrev);
    calHead->addWidget(m_calMonthLabel, 1);
    calHead->addWidget(calNext);
    calLayout->addLayout(calHead);

    auto *dowRow = new QHBoxLayout();
    dowRow->setSpacing(4);
    for (int i = 0; i < 7; ++i) {
        auto *dow = new QLabel(kDowNames[i], m_calendarBox);
        dow->setProperty("class", "dow");
        dow->setAlignment(Qt::AlignCenter);
        dowRow->addWidget(dow);
    }
    calLayout->addLayout(dowRow);

    m_calGrid = new QGridLayout();
    m_calGrid->setSpacing(4);
    for (int c = 0; c < 7; ++c)
        m_calGrid->setColumnStretch(c, 1);
    calLayout->addLayout(m_calGrid);

    m_calendarBox->setVisible(false);
    root->addWidget(m_calendarBox);

    m_timesLabel = new QLabel("Доступные записи на эту дату:", this);
    m_timesLabel->setObjectName("dpTimesLabel");
    root->addWidget(m_timesLabel);

    m_legend = new QLabel(
        "<span style=\"color:#0F6B4F;\">&#9679;</span> полные данные&nbsp;&nbsp;"
        "<span style=\"color:#F9A825;\">&#9679;</span> частичные&nbsp;&nbsp;"
        "<span style=\"color:#C1C8C5;\">&#9679;</span> нет данных ветра",
        this);
    m_legend->setObjectName("dpLegend");
    root->addWidget(m_legend);

    m_timesBox = new QWidget(this);
    m_timesGrid = new QGridLayout(m_timesBox);
    m_timesGrid->setContentsMargins(0, 0, 0, 0);
    m_timesGrid->setSpacing(6);
    root->addWidget(m_timesBox);

    auto *foot = new QHBoxLayout();
    foot->addStretch(1);
    auto *doneBtn = new QPushButton("Готово", this);
    doneBtn->setObjectName("dpDoneBtn");
    connect(doneBtn, &QPushButton::clicked, this, &ArchiveDatePopup::close);
    foot->addWidget(doneBtn);
    root->addLayout(foot);
}

QList<QDate> ArchiveDatePopup::sortedDates() const
{
    QList<QDate> dates = m_available.keys();
    std::sort(dates.begin(), dates.end());
    return dates;
}

void ArchiveDatePopup::setAvailable(const QMap<QDate, QVector<ArchiveRecordInfo>> &available)
{
    m_available = available;
    for (auto it = m_available.begin(); it != m_available.end(); ++it)
        std::sort(it.value().begin(), it.value().end(),
                   [](const ArchiveRecordInfo &a, const ArchiveRecordInfo &b) { return a.time < b.time; });
}

namespace {
// Агрегированная полнота данных за день: "full", если хотя бы одна запись
// содержит все три профиля ветра, "partial", если данные есть, но неполные,
// "empty" — если ни у одной записи дня нет ни одного профиля.
QString dayCompleteness(const QVector<ArchiveRecordInfo> &records)
{
    bool anyFull = false, anyData = false;
    for (const ArchiveRecordInfo &r : records) {
        if (r.isComplete()) anyFull = true;
        if (!r.isEmpty_())  anyData = true;
    }
    if (anyFull) return "full";
    if (anyData) return "partial";
    return "empty";
}
}

void ArchiveDatePopup::setCurrent(const QDateTime &dt)
{
    m_currentDateTime = dt;
    m_currentDate = dt.date();
    m_calMonthOffset = 0;
    refreshBigDate();
    rebuildTimeChips();
    if (m_calendarShown)
        rebuildCalendar();
}

void ArchiveDatePopup::popupNear(QWidget *anchor)
{
    if (!anchor) return;
    m_calMonthOffset = 0;
    m_calendarShown = false;
    m_calendarBox->setVisible(false);
    m_calToggleBtn->setText("Календарь");
    m_calToggleBtn->setProperty("on", false);
    refreshBigDate();
    rebuildTimeChips();
    if (layout())
        layout()->activate();
    adjustSize();

    const QPoint pos = anchor->mapToGlobal(QPoint(0, anchor->height() + 4));
    move(pos);
    repositionWithinScreen();
    show();
    raise();
}

void ArchiveDatePopup::refreshBigDate()
{
    m_bigDate->setText(m_currentDate.isValid() ? m_currentDate.toString("dd.MM.yyyy") : "—");
}

// Планшет портретный (1200x1920), а попап открывается под кнопкой выбора
// даты, которая может оказаться в нижней части экрана — без этой проверки
// разворачивающийся календарь легко уезжает за нижний (или боковой) край
// экрана. Сдвигаем попап целиком внутрь видимой области, не трогая его
// размер.
void ArchiveDatePopup::repositionWithinScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    const QRect avail = screen->availableGeometry();
    QRect geo(pos(), size());
    if (geo.bottom() > avail.bottom()) geo.moveBottom(avail.bottom() - 8);
    if (geo.right()  > avail.right())  geo.moveRight(avail.right() - 8);
    if (geo.top()    < avail.top())    geo.moveTop(avail.top() + 8);
    if (geo.left()   < avail.left())   geo.moveLeft(avail.left() + 8);
    move(geo.topLeft());
}

void ArchiveDatePopup::rebuildTimeChips()
{
    QLayoutItem *item;
    while ((item = m_timesGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    const QVector<ArchiveRecordInfo> records = m_available.value(m_currentDate);
    const int columns = 4;
    for (int i = 0; i < records.size(); ++i) {
        const ArchiveRecordInfo &rec = records[i];
        const QDateTime dt = rec.time;
        auto *chip = new QPushButton(dt.time().toString("HH:mm"), m_timesBox);
        chip->setProperty("class", "timechip");
        chip->setProperty("active", dt == m_currentDateTime);
        chip->setProperty("complete", rec.isComplete() ? "full" : (rec.isEmpty_() ? "empty" : "partial"));
        chip->setToolTip(rec.isComplete()
            ? "Есть все профили ветра (средний, действительный, измеренный)"
            : rec.isEmpty_()
                ? "Нет данных профилей ветра"
                : QString("Частично: %1%2%3")
                      .arg(rec.hasAvgWind      ? "средний " : "")
                      .arg(rec.hasActualWind   ? "действительный " : "")
                      .arg(rec.hasMeasuredWind ? "измеренный" : ""));
        connect(chip, &QPushButton::clicked, this, [this, dt] { chooseDateTime(dt); });
        m_timesGrid->addWidget(chip, i / columns, i % columns);
        chip->style()->unpolish(chip);
        chip->style()->polish(chip);
    }

    m_timesLabel->setVisible(!records.isEmpty());
    m_legend->setVisible(!records.isEmpty());
    if (records.isEmpty()) {
        auto *empty = new QLabel("Нет записей на эту дату", m_timesBox);
        empty->setStyleSheet("color: #6E7876; font-style: italic; font-size: 12px;");
        m_timesGrid->addWidget(empty, 0, 0, 1, columns);
    }
}

void ArchiveDatePopup::rebuildCalendar()
{
    int m = m_currentDate.isValid() ? (m_currentDate.month() - 1) : QDate::currentDate().month() - 1;
    int y = m_currentDate.isValid() ? m_currentDate.year() : QDate::currentDate().year();
    m += m_calMonthOffset;
    while (m < 0)  { m += 12; --y; }
    while (m > 11) { m -= 12; ++y; }

    m_calMonthLabel->setText(QString("%1 %2").arg(kMonthNames[m]).arg(y));

    QLayoutItem *item;
    while ((item = m_calGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    const QDate firstOfMonth(y, m + 1, 1);
    const int daysInMonth = firstOfMonth.daysInMonth();
    int firstWeekday = firstOfMonth.dayOfWeek(); // 1=Пн..7=Вс
    const QDate today = QDate::currentDate();

    int row = 0, col = firstWeekday - 1;
    for (int d = 1; d <= daysInMonth; ++d) {
        QDate cellDate(y, m + 1, d);
        auto *btn = new QPushButton(QString::number(d), m_calendarBox);
        btn->setProperty("class", "calday");
        const bool avail = m_available.contains(cellDate);
        btn->setProperty("avail", avail);
        btn->setProperty("selected", avail && cellDate == m_currentDate);
        btn->setProperty("today", cellDate == today);
        if (avail) {
            const QString state = dayCompleteness(m_available.value(cellDate));
            btn->setProperty("complete", state);
            btn->setToolTip(state == "full" ? "Есть записи с полным набором данных"
                             : state == "partial" ? "Есть записи, но данные неполные"
                                                   : "Записи есть, но без профилей ветра");
        }
        if (avail)
            connect(btn, &QPushButton::clicked, this, [this, cellDate] { selectCalendarDate(cellDate); });
        else
            btn->setEnabled(false);
        m_calGrid->addWidget(btn, row, col);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);

        ++col;
        if (col > 6) { col = 0; ++row; }
    }
}

void ArchiveDatePopup::stepDate(int delta)
{
    const QList<QDate> dates = sortedDates();
    if (dates.isEmpty()) return;

    int idx = dates.indexOf(m_currentDate);
    if (idx < 0) {
        // текущая дата не найдена среди доступных — ищем ближайшую
        idx = 0;
        int best = -1;
        for (int i = 0; i < dates.size(); ++i) {
            int diff = qAbs(dates[i].toJulianDay() - m_currentDate.toJulianDay());
            if (best < 0 || diff < best) { best = diff; idx = i; }
        }
    }
    idx = qBound(0, idx + delta, dates.size() - 1);

    const QDate newDate = dates[idx];
    const QVector<ArchiveRecordInfo> records = m_available.value(newDate);
    if (records.isEmpty()) return;

    // Сохраняем время суток по возможности ближе к текущему
    QDateTime best = records.first().time;
    int bestDiff = qAbs(best.time().msecsSinceStartOfDay() - m_currentDateTime.time().msecsSinceStartOfDay());
    for (const ArchiveRecordInfo &r : records) {
        int diff = qAbs(r.time.time().msecsSinceStartOfDay() - m_currentDateTime.time().msecsSinceStartOfDay());
        if (diff < bestDiff) { bestDiff = diff; best = r.time; }
    }

    m_currentDate = newDate;
    m_currentDateTime = best;
    refreshBigDate();
    rebuildTimeChips();
    if (m_calendarShown) rebuildCalendar();
    emit dateTimeSelected(m_currentDateTime);
}

void ArchiveDatePopup::stepMonth(int delta)
{
    m_calMonthOffset += delta;
    rebuildCalendar();
}

void ArchiveDatePopup::jumpToday()
{
    const QDate today = QDate::currentDate();
    if (!m_available.contains(today)) {
        // Раньше здесь молча подставлялась ближайшая по времени дата — это
        // выглядело как случайный выбор. Теперь просто сообщаем, что данных
        // за сегодня нет, и ничего не меняем.
        emit noDataForDate(today);
        return;
    }
    selectCalendarDate(today);
}

void ArchiveDatePopup::jumpLatest()
{
    const QList<QDate> dates = sortedDates();
    if (dates.isEmpty()) return;
    const QDate last = dates.last();
    const QVector<ArchiveRecordInfo> records = m_available.value(last);
    if (records.isEmpty()) return;
    chooseDateTime(records.last().time);
}

void ArchiveDatePopup::toggleCalendar()
{
    m_calendarShown = !m_calendarShown;
    m_calendarBox->setVisible(m_calendarShown);
    m_calToggleBtn->setText(m_calendarShown ? "‹ Скрыть календарь" : "Календарь");
    m_calToggleBtn->setProperty("on", m_calendarShown);
    m_calToggleBtn->style()->unpolish(m_calToggleBtn);
    m_calToggleBtn->style()->polish(m_calToggleBtn);
    m_calMonthOffset = 0;
    if (m_calendarShown)
        rebuildCalendar();
    // При самом первом раскрытии календаря за время жизни попапа (виджет
    // создаётся один раз в конструкторе MeasurementResults и живёт всю
    // сессию) layout()->sizeHint() сразу после setVisible(true)/rebuild ещё
    // не учитывал добавленные ячейки календаря — activate() принудительно
    // пересчитывает геометрию синхронно, до adjustSize(), иначе попап не
    // дорастал до нужной высоты и календарь визуально вылезал за его край.
    if (layout())
        layout()->activate();
    // Попап — top-level окно, оно один раз укладывается по layout при первом
    // show() и дальше не считает размер заново само, поэтому при скрытии
    // календаря высота оставалась "растянутой" под него. adjustSize()
    // пересчитывает её под текущий набор видимых виджетов.
    adjustSize();
    repositionWithinScreen();
}

void ArchiveDatePopup::selectCalendarDate(const QDate &d)
{
    const QVector<ArchiveRecordInfo> records = m_available.value(d);
    if (records.isEmpty()) return;

    m_currentDate = d;
    m_currentDateTime = records.first().time;
    refreshBigDate();
    rebuildTimeChips();

    if (m_calendarShown)
        toggleCalendar(); // сворачиваем календарь обратно к чипам времени, как в макете

    emit dateTimeSelected(m_currentDateTime);
}

void ArchiveDatePopup::chooseDateTime(const QDateTime &dt)
{
    m_currentDateTime = dt;
    m_currentDate = dt.date();
    emit dateTimeSelected(dt);
    close();
}
