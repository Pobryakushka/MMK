#include "anglecheckpage.h"
#include "ui_anglecheckpage.h"

#include <QDoubleValidator>
#include <QDebug>

AngleCheckPage::AngleCheckPage(AMSHandler *amsHandler, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AngleCheckPage)
    , m_amsHandler(amsHandler)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);

    m_toast = new NotificationToast(this);

    ui->editTargetAngle->setValidator(new QDoubleValidator(0.0, 330.0, 1, this));
    ui->btnStop->setEnabled(false);

    connect(ui->btnBackFromAngleCheck, &QPushButton::clicked,
            this, &AngleCheckPage::backRequested);
    connect(ui->btnStart, &QPushButton::clicked,
            this, &AngleCheckPage::onStart);
    connect(ui->btnStop, &QPushButton::clicked,
            this, &AngleCheckPage::onStop);

    if (m_amsHandler) {
        connect(m_amsHandler, &AMSHandler::rotateStatusReceived,
                this, &AngleCheckPage::onRotateStatus);
        connect(m_amsHandler, &AMSHandler::connected,
                this, &AngleCheckPage::onAmsConnected);
        connect(m_amsHandler, &AMSHandler::disconnected,
                this, &AngleCheckPage::onAmsDisconnected);
    }

    updateAmsBanner();
    setControlsEnabled(m_amsHandler && m_amsHandler->isConnected());
}

AngleCheckPage::~AngleCheckPage()
{
    delete ui;
}

void AngleCheckPage::onStart()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", NotificationToast::Error);
        return;
    }

    bool ok = false;
    float angle = ui->editTargetAngle->text().replace(',', '.').toFloat(&ok);
    if (!ok || angle < 0.0f || angle > 330.0f) {
        showStatus("Введите корректный угол (0–330°)", NotificationToast::Error);
        return;
    }

    qDebug() << "AngleCheckPage: Поворот антенны на угол" << angle << "° (0xAF, cmd=start)";

    m_rotating = true;
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
    showStatus(QString("Поворот антенны на %1°…").arg(angle), NotificationToast::Info);

    if (!m_amsHandler->rotateAntenna(angle)) {
        showStatus("Ошибка отправки команды поворота", NotificationToast::Error);
        m_rotating = false;
        ui->btnStart->setEnabled(true);
        ui->btnStop->setEnabled(false);
    }
}

void AngleCheckPage::onStop()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", NotificationToast::Error);
        return;
    }

    qDebug() << "AngleCheckPage: Остановка вращения антенны (0xAF, cmd=stop)";

    if (!m_amsHandler->stopAntennaRotation()) {
        showStatus("Ошибка отправки команды остановки", NotificationToast::Error);
    } else {
        showStatus("Команда остановки отправлена", NotificationToast::Info);
    }

    m_rotating = false;
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
}

void AngleCheckPage::onRotateStatus(quint8 status, float angle)
{
    // Живое положение РПВ — всегда на компас, независимо от статуса
    // (полезно видеть последнее известное положение и при аварии).
    ui->rpvAngleIndicator->setAngle(angle);

    switch (status) {
    case ROTATE_RUNNING:
        showStatus(QString("Вращение… текущий угол: %1°").arg(angle, 0, 'f', 1), NotificationToast::Info);
        m_rotating = true;
        ui->btnStart->setEnabled(false);
        ui->btnStop->setEnabled(true);
        break;
    case ROTATE_IDLE_OK:
        showStatus("Завершено", NotificationToast::Success);
        m_rotating = false;
        ui->btnStart->setEnabled(true);
        ui->btnStop->setEnabled(false);
        break;
    case ROTATE_FAULT:
        showStatus("Аварийная остановка привода вращения!", NotificationToast::Error);
        m_rotating = false;
        ui->btnStart->setEnabled(true);
        ui->btnStop->setEnabled(false);
        break;
    default:
        showStatus(QString("Неизвестный статус: 0x%1").arg(status, 2, 16, QChar('0')), NotificationToast::Error);
        m_rotating = false;
        ui->btnStart->setEnabled(true);
        ui->btnStop->setEnabled(false);
        break;
    }
}

void AngleCheckPage::setControlsEnabled(bool enabled)
{
    ui->editTargetAngle->setEnabled(enabled);
    ui->btnStart->setEnabled(enabled && !m_rotating);
    ui->btnStop->setEnabled(enabled && m_rotating);
}

void AngleCheckPage::showStatus(const QString &text, NotificationToast::Kind kind)
{
    m_toast->showMessage(text, kind);
}

void AngleCheckPage::onAmsConnected()
{
    updateAmsBanner();
    setControlsEnabled(true);
}

void AngleCheckPage::onAmsDisconnected()
{
    updateAmsBanner();
    setControlsEnabled(false);

    if (m_rotating) {
        m_rotating = false;
        showStatus("АМС отключён во время выполнения операции", NotificationToast::Error);
    } else {
        showStatus("АМС не подключён", NotificationToast::Error);
    }
}

void AngleCheckPage::updateAmsBanner()
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
