#ifndef FUNCTIONALCONTROLDIALOG_H
#define FUNCTIONALCONTROLDIALOG_H

#include <QDialog>
#include <QVector>
#include <QDateTime>
#include "amsprotocol.h"

namespace Ui {
class FunctionalControlDialog;
}

class FunctionalControlDialog : public QDialog
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
    // Кнопка "Обновить" нажата — MainWindow должен инициировать запрос
    void refreshRequested();

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

    void resetDisplay();
    void populateFromResult(const FuncControlResult &fc);
    void updateSensorTitle();

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // FUNCTIONALCONTROLDIALOG_H
