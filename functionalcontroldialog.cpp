#include "functionalcontroldialog.h"
#include "ui_functionalcontroldialog.h"
#include <QListWidgetItem>
#include <QDateTime>

// Таблица неисправностей АМС
// Бит = 0 -> устройство не работает (неисправность)
// Бит = 1 -> устройство исправно
const QVector<FunctionalControlDialog::FaultEntry> FunctionalControlDialog::s_amsFaultTable = {
    { 0, "Превышено время ожидания завершения вращения" },
    { 1, "Аварийная остановка открытия/закрытия антенны" },
    { 2, "Превышено время ожидания открытия антенны" },
    { 3, "Превышено время ожидания закрытия антенны" },
    { 4, "Нет сбора данных" },
    { 5, "СЧ не пошел контроль" },
    { 6, "Не готов передатчик" },
    { 7, "Ошибка ПО" },
    { 8, "Неверное значение даты и времени" },
};

FunctionalControlDialog::FunctionalControlDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FunctionalControlDialog)
    , m_sensorType(AMS)
{
    ui->setupUi(this);
    setModal(false);


}
