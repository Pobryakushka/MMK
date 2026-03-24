#include "functionalcontroldialog.h"
#include "ui_functionalcontroldialog.h"
#include <QListWidgetItem>
#include <QDateTime>
#include <QShowEvent>

void FunctionalControlDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // При каждом открытии — автоматически запрашиваем данные
    emit refreshRequested();
}

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

FunctionalControlDialog::FunctionalControlDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FunctionalControlDialog)
    , m_sensorType(AMS)
{
    ui->setupUi(this);
    setModal(false);

    connect(ui->btnRefresh, &QPushButton::clicked,
            this, &FunctionalControlDialog::refreshRequested);

    updateSensorTitle();
    resetDisplay();
}

FunctionalControlDialog::~FunctionalControlDialog()
{
    delete ui;
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
    setWindowTitle(QString("Функциональный контроль — %1").arg(name));
    ui->lblDialogTitle->setText(
        QString("Функциональный контроль %1").arg(name));
}

void FunctionalControlDialog::resetDisplay()
{
    ui->lblComplexNumberValue->setText("—");

    ui->listFaults->clear();
    ui->listFaults->addItem("Неисправностей не обнаружено");

    ui->listErrors->clear();
    ui->listErrors->addItem("Ошибок не обнаружено");

    ui->lblLastPoll->setText("Последний опрос: —");
    ui->lblStatusBar->setText("Ожидание данных");

    ui->btnRefresh->setEnabled(true);
}

void FunctionalControlDialog::setWaitingState()
{
    ui->lblStatusBar->setText("Запрос отправлен, ожидание ответа...");
    ui->btnRefresh->setEnabled(false);

    ui->listFaults->clear();
    ui->listFaults->addItem("Ожидание данных от устройства...");

    ui->listErrors->clear();
    ui->listErrors->addItem("Ожидание данных от устройства...");
}

void FunctionalControlDialog::setDisconnectedState()
{
    ui->lblComplexNumberValue->setText("—");

    ui->listFaults->clear();
    ui->listFaults->addItem("Устройство не подключено");

    ui->listErrors->clear();
    ui->listErrors->addItem("Устройство не подключено");

    ui->lblStatusBar->setText("Нет подключения к устройству");
    ui->btnRefresh->setEnabled(false);
}

void FunctionalControlDialog::setErrorState(const QString &errorText)
{
    ui->lblStatusBar->setText("Ошибка: " + errorText);
    ui->btnRefresh->setEnabled(true);
}

void FunctionalControlDialog::setAmsData(quint32 bitMask, quint32 powerOnCount)
{
    ui->lblComplexNumberValue->setText(QString::number(powerOnCount));

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
}

void FunctionalControlDialog::updateLastPollTime()
{
    ui->lblLastPoll->setText(
        QString("Последний опрос: %1")
            .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")));
}

void FunctionalControlDialog::populateFromResult(const FuncControlResult &fc)
{
    // --- Неисправности ---
    ui->listFaults->clear();
    if (fc.faults.isEmpty()) {
        ui->listFaults->addItem("Неисправностей не обнаружено");
    } else {
        for (const QString &desc : fc.faults) {
            ui->listFaults->addItem(desc);
        }
    }

    // --- Ошибки ---
    ui->listErrors->clear();
    if (fc.errors.isEmpty()) {
        ui->listErrors->addItem("Ошибок не обнаружено");
    } else {
        for (const QString &desc : fc.errors) {
            ui->listErrors->addItem(desc);
        }
    }
}
