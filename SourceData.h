#ifndef SOURCEDATA_H
#define SOURCEDATA_H
#include <QWidget>
#include <QAbstractButton>
#include <QColor>
#include <QJsonObject>
#include <QDateTime>

// Forward declaration
class GroundMeteoParams;
class Meteo11;
class QShowEvent;
class QPaintEvent;

namespace Ui {
class SourceData;
}

// ─────────────────────────────────────────────────────────────────────────
// SourceDataTile — плитка-строка источника данных (заголовок, описание,
// цветной бейдж статуса справа, шеврон). В отличие от прошлой версии — это
// ОДИН виджет без единого дочернего элемента: всё рисуется в paintEvent(),
// а клики обрабатываются штатным механизмом QAbstractButton (тот же самый,
// на котором работают обычные QPushButton в "Расчётах"). Никакого ручного
// перехвата мышиных событий (installEventFilter) — раз это стало давать
// зависание на планшете, лучше вообще не строить на этом логику.
// ─────────────────────────────────────────────────────────────────────────
class SourceDataTile : public QAbstractButton
{
    Q_OBJECT
public:
    explicit SourceDataTile(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setDescription(const QString &description);
    void setBadge(const QString &text, const QColor &fg, const QColor &bg);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_description;
    QString m_badgeText;
    QColor  m_badgeFg = QColor("#C62828");
    QColor  m_badgeBg = QColor("#FFEBEE");
};

// Экран "Исходные данные". Встраивается страницей в общий стек MainWindow
// (QStackedWidget), как и AlgorithmsCalculation — не открывается отдельным
// всплывающим окном. Наружу отдаёт сигналы навигации; переключение страниц
// делает MainWindow.
class SourceData : public QWidget {
    Q_OBJECT

public:
    explicit SourceData(QWidget *parent = nullptr);
    ~SourceData();

    // Доступ к данным бюллетеня Метео-11
    bool        hasMeteo11Bulletin() const;
    QJsonObject meteo11BulletinJson() const;
    QDateTime   meteo11BulletinTime() const;
    QString     meteo11ValidityPeriod() const;
    void        resetMeteo11Applied();   // вызвать после сохранения в БД

    // Виджеты страниц для встраивания в QStackedWidget MainWindow
    // (Meteo11 больше не всплывающее окно — MainWindow добавляет их
    // в стек так же, как это уже сделано для GroundMeteoParams).
    Meteo11           *meteo11Widget() const { return m_meteo11Dialog; }
    GroundMeteoParams *groundMeteoParamsWidget() const { return groundMeteoParams; }

signals:
    void backRequested();
    void openGroundParamsRequested();
    void openMeteo11Requested();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void updateBadges();

private:
    Ui::SourceData *ui;
    GroundMeteoParams *groundMeteoParams;
    Meteo11           *m_meteo11Dialog;  // постоянный экземпляр — данные не теряются
};

#endif // SOURCEDATA_H
