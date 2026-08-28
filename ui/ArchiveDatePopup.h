#ifndef ARCHIVEDATEPOPUP_H
#define ARCHIVEDATEPOPUP_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QDate>
#include <QDateTime>

class QLabel;
class QPushButton;
class QGridLayout;

// Данные об одной записи архива, нужные попапу только для того, чтобы
// показать, полный ли это набор данных (есть все три профиля ветра) —
// не тянем сюда полную структуру MeasurementRecord из MeasurementResults.h,
// чтобы не создавать зависимость попапа от диалога-владельца.
struct ArchiveRecordInfo {
    QDateTime time;
    bool hasAvgWind      = false;
    bool hasActualWind   = false;
    bool hasMeasuredWind = false;

    bool isComplete() const { return hasAvgWind && hasActualWind && hasMeasuredWind; }
    bool isEmpty_()   const { return !hasAvgWind && !hasActualWind && !hasMeasuredWind; }
};

// Всплывающее окно выбора даты/времени архива — замена модального QDialog
// с QCalendarWidget. Визуально/структурно повторяет date-popup из макета
// "archive v1 docked sidebar.html": большая дата с навигацией ◄/►,
// быстрые кнопки (Сегодня/Последняя запись/Календарь), сворачиваемая сетка
// календаря месяца и "чипы" доступных записей на выбранную дату. Чипы и
// дни календаря дополнительно подсвечиваются по полноте данных записи
// (зелёный — есть все три профиля ветра, жёлтый — часть данных, серый —
// данных нет вовсе), чтобы было видно ещё до перехода к записи.
class ArchiveDatePopup : public QWidget
{
    Q_OBJECT
public:
    explicit ArchiveDatePopup(QWidget *parent = nullptr);

    // available: дата -> список записей на эту дату (время + полнота данных)
    void setAvailable(const QMap<QDate, QVector<ArchiveRecordInfo>> &available);
    void setCurrent(const QDateTime &dt);

    // Показать popup под указанным виджетом (например, кнопкой выбора даты)
    void popupNear(QWidget *anchor);

signals:
    void dateTimeSelected(const QDateTime &dt);
    // Нажата "Сегодня", а записей за сегодняшнюю дату нет — раньше в этом
    // случае молча подставлялась ближайшая по времени дата, что выглядело
    // как случайный выбор. Теперь тут только уведомление, выбор не меняется.
    void noDataForDate(const QDate &date);

private:
    QMap<QDate, QVector<ArchiveRecordInfo>> m_available;
    QDate     m_currentDate;
    QDateTime m_currentDateTime;
    int       m_calMonthOffset;
    bool      m_calendarShown;

    QLabel      *m_bigDate;
    QPushButton *m_calToggleBtn;
    QWidget     *m_calendarBox;
    QLabel      *m_calMonthLabel;
    QGridLayout *m_calGrid;
    QLabel      *m_timesLabel;
    QLabel      *m_legend;
    QWidget     *m_timesBox;
    QGridLayout *m_timesGrid;

    QList<QDate> sortedDates() const;

    void rebuildTimeChips();
    void rebuildCalendar();
    void refreshBigDate();
    void repositionWithinScreen();

    void stepDate(int delta);
    void stepMonth(int delta);
    void jumpToday();
    void jumpLatest();
    void toggleCalendar();
    void selectCalendarDate(const QDate &d);
    void chooseDateTime(const QDateTime &dt);
};

#endif // ARCHIVEDATEPOPUP_H
