#include "inspectionpage.h"
#include "ui_inspectionpage.h"

#include <QDebug>

InspectionPage::InspectionPage(AMSHandler *amsHandler, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InspectionPage)
    , m_amsHandler(amsHandler)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    ui->lblStatus->setVisible(false);

    connect(ui->btnBackFromInspection, &QPushButton::clicked,
            this, &InspectionPage::backRequested);
    connect(ui->btnAntennaOpen, &QPushButton::clicked,
            this, &InspectionPage::onOpenAntenna);
    connect(ui->btnAntennaClose, &QPushButton::clicked,
            this, &InspectionPage::onCloseAntenna);

    if (m_amsHandler) {
        connect(m_amsHandler, &AMSHandler::antennaStatusReceived,
                this, &InspectionPage::onAntennaStatus);
        connect(m_amsHandler, &AMSHandler::connected,
                this, &InspectionPage::onAmsConnected);
        connect(m_amsHandler, &AMSHandler::disconnected,
                this, &InspectionPage::onAmsDisconnected);
    }

    updateAmsBanner();
    setControlsEnabled(m_amsHandler && m_amsHandler->isConnected());
}

InspectionPage::~InspectionPage()
{
    delete ui;
}

void InspectionPage::onOpenAntenna()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", true);
        return;
    }

    qDebug() << "InspectionPage: Команда — открыть антенну (0xAD, cmd=0x00)";
    setControlsEnabled(false);
    showStatus("Отправка команды открытия антенны…", false);

    if (!m_amsHandler->openAntenna()) {
        showStatus("Ошибка отправки команды", true);
        setControlsEnabled(true);
    }
}

void InspectionPage::onCloseAntenna()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", true);
        return;
    }

    qDebug() << "InspectionPage: Команда — закрыть антенну (0xAD, cmd=0x01)";
    setControlsEnabled(false);
    showStatus("Отправка команды закрытия антенны…", false);

    if (!m_amsHandler->closeAntenna()) {
        showStatus("Ошибка отправки команды", true);
        setControlsEnabled(true);
    }
}

void InspectionPage::onAntennaStatus(quint8 status)
{
    switch (status) {
    case ANTENNA_IN_PROGRESS:
        showStatus("Выполняется… ожидание ответа привода", false);
        setControlsEnabled(false);
        break;
    case ANTENNA_SUCCESS:
        setControlsEnabled(true);
        showStatus("Операция выполнена успешно", false);
        break;
    case ANTENNA_FAULT:
        setControlsEnabled(true);
        showStatus("Аварийная остановка антенны!", true);
        break;
    default:
        setControlsEnabled(true);
        showStatus(QString("Неизвестный статус: 0x%1").arg(status, 2, 16, QChar('0')), true);
        break;
    }
}

void InspectionPage::setControlsEnabled(bool enabled)
{
    ui->btnAntennaOpen->setEnabled(enabled);
    ui->btnAntennaClose->setEnabled(enabled);
}

void InspectionPage::showStatus(const QString &text, bool error)
{
    ui->lblStatus->setText(text);
    ui->lblStatus->setStyleSheet(
        error
        ? "font-size: 11pt; font-weight: 600; color: #B71C1C; background-color: #FFEBEE; "
          "border: 1px solid #FFCDD2; padding: 12px 16px; border-radius: 12px;"
        : "font-size: 11pt; font-weight: 600; color: #8D5B00; background-color: #FFF8E1; "
          "border: 1px solid #FFE0B2; padding: 12px 16px; border-radius: 12px;");
    ui->lblStatus->setVisible(true);
}

void InspectionPage::onAmsConnected()
{
    updateAmsBanner();
    setControlsEnabled(true);
}

void InspectionPage::onAmsDisconnected()
{
    updateAmsBanner();
    setControlsEnabled(false);
    showStatus("АМС не подключён", true);
}

void InspectionPage::updateAmsBanner()
{
    const bool connected = m_amsHandler && m_amsHandler->isConnected();
    if (connected) {
        ui->amsBannerFrame->setStyleSheet(
            "QFrame#amsBannerFrame { background-color: #E8F5E9; border: 1px solid #A5D6A7; border-radius: 14px; }");
        ui->lblAmsBannerText->setStyleSheet(
            "color: #2E7D32; font-weight: 600; font-size: 10.5pt; background: transparent; border: none;");
        ui->lblAmsBannerText->setText("АМС подключён — команды доступны");
    } else {
        ui->amsBannerFrame->setStyleSheet(
            "QFrame#amsBannerFrame { background-color: #FFEBEE; border: 1px solid #FFCDD2; border-radius: 14px; }");
        ui->lblAmsBannerText->setStyleSheet(
            "color: #B71C1C; font-weight: 600; font-size: 10.5pt; background: transparent; border: none;");
        ui->lblAmsBannerText->setText("АМС не подключён — команды недоступны");
    }
}
