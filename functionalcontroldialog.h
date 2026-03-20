#ifndef FUNCTIONALCONTROLDIALOG_H
#define FUNCTIONALCONTROLDIALOG_H

#include <QDialog>
#include <QVector>
#include <QDateTime>

namespace Ui {
class FunctionalControlDialog;
}

class FunctionalControlDialog : public QDialog
{
    Q_OBJECT

public:
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
    void setAmsData(quint32 bitmask, quint32 powerOnCount);

    // Состояние "Ожидание ответа"
    void setWaitingState();

    // Состояние "АМС не подключена"
    void setDisconnectState();

    // Ошибка при запросе
    void setErrorState(const QString &errorText);

    // Обновить время последнего опроса
    void updateLastPollTime();

signals:
    void refreshRequested();

private:
    Ui::FunctionalControlDialog *ui;

    SensorType m_sensorType;

    struct FaultEntry {
        int bit;
        QString description;
    };

    static const QVector<FaultEntry> s_amsFaultTable;

    void resetDisplay();
    void populateAmsFaults(quint32 bitMask);
    void updateSensorTitle();
};

#endif // FUNCTIONALCONTROLDIALOG_H
