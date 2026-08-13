#ifndef SOURCEDATA_H
#define SOURCEDATA_H
#include <QWidget>
#include <QJsonObject>
#include <QDateTime>

// Forward declaration
class GroundMeteoParams;
class Meteo11;
class QShowEvent;

namespace Ui {
class SourceData;
}

// Экран "Исходные данные". Как и AlgorithmsCalculation — встраивается
// страницей в общий стек MainWindow (QStackedWidget), а не открывается
// отдельным всплывающим окном. Наружу отдаёт только сигнал навигации
// backRequested(); переключение видимости страниц делает MainWindow.
//
// Плитки-строки (rowMeteo11 / rowGroundParams) полностью описаны в
// SourceData.ui — так же, как кнопки-плитки в AlgorithmsCalc.ui — а не
// создаются динамически в коде. Кликабельность реализована через
// installEventFilter(), т.к. в отличие от QPushButton у обычного
// QWidget-контейнера нет сигнала clicked().
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

signals:
    void backRequested();
    void openGroundParamsRequested();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void updateBadges();

private:
    Ui::SourceData *ui;
    GroundMeteoParams *groundMeteoParams;
    Meteo11           *m_meteo11Dialog;  // постоянный экземпляр — данные не теряются

    void setRowPressed(QWidget *row, bool pressed);
};

#endif // SOURCEDATA_H
