#ifndef FUNCTIONALCONTROLDIALOG_H
#define FUNCTIONALCONTROLDIALOG_H

#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QListWidget>
#include "sensors/amsprotocol.h"

namespace Ui {
class FunctionalControlDialog;
}

// ПРИМЕЧАНИЕ: класс сохранил имя FunctionalControlDialog и имя файла для
// минимального диффа при встраивании в проект (mainwindow.h/.cpp и .ui
// ссылаются на это имя). По сути это больше не диалог, а страница,
// встраиваемая в общий ui->stackedWidget — как SourceData/GroundMeteoParams/
// Meteo11. При желании класс можно переименовать через рефакторинг IDE.
class FunctionalControlDialog : public QWidget
{
    Q_OBJECT

public:
    // Тип датчика — для заголовка и таблицы неисправностей
    enum SensorType {
        AMS,
        GNSS,
        BINS,
        IWS
    };

    explicit FunctionalControlDialog(QWidget *parent = nullptr);
    ~FunctionalControlDialog();

    void setSensorType(SensorType type);

    // Вызывается из MainWindow при получении данных от АМС
    void setAmsData(quint32 bitMask, quint32 powerOnCount);

    // Состояние "ожидание ответа"
    void setWaitingState();

    // Состояние "АМС не подключена"
    void setDisconnectedState();

    // Ошибка при запросе
    void setErrorState(const QString &errorText);

    // Обновить время последнего опроса
    void updateLastPollTime();

signals:
    // Кнопка "Обновить" / автотаймер / открытие страницы — MainWindow должен
    // инициировать запрос функционального контроля у устройства
    void refreshRequested();

    // Нажата "‹ Назад" — MainWindow должен вернуть предыдущую страницу стека
    void backRequested();

private slots:
    void onChipToggled();
    void onAlertBannerClicked();
    void onRefreshButtonClicked();

private:
    Ui::FunctionalControlDialog *ui;

    SensorType m_sensorType;

    struct FaultEntry {
        int     bit;          // 0-индексированный номер бита
        QString description;  // Описание из протокола
    };

    // Таблица неисправностей АМС согласно Таблице 2 протокола.
    // ВАЖНО: бит = 0 означает НЕИСПРАВНОСТЬ, бит = 1 — устройство исправно.
    static const QVector<FaultEntry> s_amsFaultTable;

    // Автоматический опрос, пока страница видна (см. showEvent/hideEvent)
    QTimer *m_pollTimer = nullptr;

    // Пульсация индикатора автоопроса (lblPollDot)
    QGraphicsOpacityEffect *m_pollDotEffect = nullptr;
    QPropertyAnimation     *m_pollDotAnim   = nullptr;

    // true только после первого реально полученного ответа от устройства.
    // Пока false — на странице ещё нет данных (открыли/ждём/отключены), и
    // "нет записей" нужно объяснять иначе, чем "по этому фильтру пусто".
    bool m_hasData = false;

    void resetDisplay();
    void populateFromResult(const FuncControlResult &fc);
    void updateSensorTitle();
    void updateAlertBanner(int faultCount, int errorCount);
    void updateChipCounts(int faultCount, int errorCount);
    void applyFilter();
    void setRefreshBusy(bool busy);
    void flashItem(QListWidgetItem *item, QListWidget *list);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // FUNCTIONALCONTROLDIALOG_H
