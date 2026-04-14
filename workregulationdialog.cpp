#include "workregulationdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QDebug>

// ===================================================================
// Вспомогательная функция — общий стиль кнопок диалогов
// ===================================================================
static void applyButtonStyle(QPushButton *btn)
{
    btn->setMinimumHeight(36);
    btn->setStyleSheet(
        "QPushButton {"
        "  background-color: #f0f0f0;"
        "  color: #000000;"
        "  border: 1px solid #b0b0b0;"
        "  border-radius: 3px;"
        "  padding: 6px 18px;"
        "  font-size: 10pt;"
        "}"
        "QPushButton:hover   { background-color: #e0e0e0; }"
        "QPushButton:pressed { background-color: #c8c8c8; border: 1px solid #888; }"
        "QPushButton:disabled { background-color: #f8f8f8; color: #a0a0a0; border-color: #d0d0d0; }");
}

// ===================================================================
//  InspectionDialog — Контрольный осмотр (КО)
// ===================================================================
InspectionDialog::InspectionDialog(AMSHandler *amsHandler, QWidget *parent)
    : QDialog(parent)
    , m_amsHandler(amsHandler)
{
    setWindowTitle("Контрольный осмотр (КО)");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(320);

    // --- Метка заголовка ---
    auto *lblTitle = new QLabel("Контрольный осмотр (КО)", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("font-size: 11pt; font-weight: bold; padding: 6px;");

    // --- Кнопки управления антенной ---
    m_btnOpen  = new QPushButton("Антенну открыть", this);
    m_btnClose = new QPushButton("Антенну закрыть", this);
    applyButtonStyle(m_btnOpen);
    applyButtonStyle(m_btnClose);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_btnOpen);
    btnLayout->addWidget(m_btnClose);

    // --- Метка статуса ---
    m_lblStatus = new QLabel("", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);
    m_lblStatus->setWordWrap(true);
    m_lblStatus->setStyleSheet("font-size: 10pt; padding: 4px;");
    m_lblStatus->setVisible(false);

    // --- Кнопка закрытия ---
    auto *btnClose = new QPushButton("Закрыть", this);
    applyButtonStyle(btnClose);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    auto *closeLine = new QHBoxLayout;
    closeLine->addStretch();
    closeLine->addWidget(btnClose);
    closeLine->addStretch();

    // --- Общий layout ---
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(lblTitle);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_lblStatus);
    mainLayout->addSpacing(8);
    mainLayout->addLayout(closeLine);

    // --- Соединения ---
    connect(m_btnOpen,  &QPushButton::clicked, this, &InspectionDialog::onOpenAntenna);
    connect(m_btnClose, &QPushButton::clicked, this, &InspectionDialog::onCloseAntenna);

    // Слушаем статус антенны от AMSHandler
    if (m_amsHandler) {
        connect(m_amsHandler, &AMSHandler::antennaStatusReceived,
                this, &InspectionDialog::onAntennaStatus);
    }

    // Если АМС не подключён — блокируем кнопки
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        setControlsEnabled(false);
        showStatus("АМС не подключён", true);
    }
}

void InspectionDialog::onOpenAntenna()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", true);
        return;
    }

    qDebug() << "InspectionDialog: Команда — открыть антенну (0xAD, cmd=0x00)";
    setControlsEnabled(false);
    showStatus("Отправка команды открытия антенны…");

    if (!m_amsHandler->openAntenna()) {
        showStatus("Ошибка отправки команды", true);
        setControlsEnabled(true);
    }
}

void InspectionDialog::onCloseAntenna()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", true);
        return;
    }

    qDebug() << "InspectionDialog: Команда — закрыть антенну (0xAD, cmd=0x01)";
    setControlsEnabled(false);
    showStatus("Отправка команды закрытия антенны…");

    if (!m_amsHandler->closeAntenna()) {
        showStatus("Ошибка отправки команды", true);
        setControlsEnabled(true);
    }
}

void InspectionDialog::onAntennaStatus(quint8 status)
{
    switch (status) {
    case ANTENNA_IN_PROGRESS:
        // Операция ещё идёт — держим кнопки заблокированными, обновляем текст
        showStatus("Выполняется…");
        setControlsEnabled(false);
        break;
    case ANTENNA_SUCCESS:
        setControlsEnabled(true);
        showStatus("Операция выполнена успешно");
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

void InspectionDialog::setControlsEnabled(bool enabled)
{
    m_btnOpen->setEnabled(enabled);
    m_btnClose->setEnabled(enabled);
}

void InspectionDialog::showStatus(const QString &text, bool error)
{
    m_lblStatus->setText(text);
    m_lblStatus->setStyleSheet(
        error
        ? "color: #c0392b; font-size: 10pt; padding: 4px;"
        : "color: #1a6e2a; font-size: 10pt; padding: 4px;");
    m_lblStatus->setVisible(true);
}

// ===================================================================
//  AngleCheckDialog — Проверка правильности установки углов (ТО-2)
// ===================================================================
AngleCheckDialog::AngleCheckDialog(AMSHandler *amsHandler, QWidget *parent)
    : QDialog(parent)
    , m_amsHandler(amsHandler)
{
    setWindowTitle("Проверка правильности установки углов (ТО-2)");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(360);

    // --- Метка заголовка ---
    auto *lblTitle = new QLabel("Проверка правильности установки углов (ТО-2)", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setWordWrap(true);
    lblTitle->setStyleSheet("font-size: 11pt; font-weight: bold; padding: 6px;");

    // --- Ввод угла РПВ ---
    auto *lblAngle = new QLabel("Положение РПВ", this);
    lblAngle->setStyleSheet("font-size: 10pt;");

    m_editAngle = new QLineEdit("000", this);
    m_editAngle->setAlignment(Qt::AlignRight);
    m_editAngle->setMinimumWidth(70);
    m_editAngle->setMaximumWidth(90);
    // Разрешаем ввод от 0.0 до 360.0 градусов
    m_editAngle->setValidator(new QDoubleValidator(0.0, 330.0, 1, this));
    m_editAngle->setStyleSheet("font-size: 11pt; padding: 3px;");

    auto *lblDeg = new QLabel("°", this);
    lblDeg->setStyleSheet("font-size: 11pt;");

    auto *angleLayout = new QHBoxLayout;
    angleLayout->addStretch();
    angleLayout->addWidget(lblAngle);
    angleLayout->addSpacing(8);
    angleLayout->addWidget(m_editAngle);
    angleLayout->addWidget(lblDeg);
    angleLayout->addStretch();

    // --- Кнопки Стоп / Пуск ---
    m_btnStop  = new QPushButton("Стоп  ■", this);
    m_btnStart = new QPushButton("Пуск  ▶", this);
    applyButtonStyle(m_btnStop);
    applyButtonStyle(m_btnStart);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnStop);
    btnLayout->addSpacing(12);
    btnLayout->addWidget(m_btnStart);
    btnLayout->addStretch();

    // --- Метка статуса ---
    m_lblStatus = new QLabel("", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);
    m_lblStatus->setWordWrap(true);
    m_lblStatus->setStyleSheet("font-size: 10pt; padding: 4px;");
    m_lblStatus->setVisible(false);

    // --- Кнопка закрытия ---
    auto *btnClose = new QPushButton("Закрыть", this);
    applyButtonStyle(btnClose);
    connect(btnClose, &QPushButton::clicked, this, [this]() {
        // При закрытии останавливаем вращение, если идёт
        if (m_amsHandler && m_amsHandler->isConnected()) {
            m_amsHandler->stopAntennaRotation();
        }
        accept();
    });

    auto *closeLine = new QHBoxLayout;
    closeLine->addStretch();
    closeLine->addWidget(btnClose);
    closeLine->addStretch();

    // --- Общий layout ---
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(lblTitle);
    mainLayout->addLayout(angleLayout);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_lblStatus);
    mainLayout->addSpacing(8);
    mainLayout->addLayout(closeLine);

    // --- Соединения ---
    connect(m_btnStart, &QPushButton::clicked, this, &AngleCheckDialog::onStart);
    connect(m_btnStop,  &QPushButton::clicked, this, &AngleCheckDialog::onStop);

    // Слушаем статус поворота от AMSHandler
    if (m_amsHandler) {
        connect(m_amsHandler, &AMSHandler::rotateStatusReceived,
                this, &AngleCheckDialog::onRotateStatus);
    }

    // Стоп изначально неактивен
    m_btnStop->setEnabled(false);

    // Если АМС не подключён — блокируем кнопку Пуск
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        m_btnStart->setEnabled(false);
        showStatus("АМС не подключён", true);
    }
}

void AngleCheckDialog::onStart()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", true);
        return;
    }

    bool ok = false;
    float angle = m_editAngle->text().replace(',', '.').toFloat(&ok);
    if (!ok || angle < 0.0f || angle > 330.0f) {
        showStatus("Введите корректный угол (0–330°)", true);
        return;
    }

    qDebug() << "AngleCheckDialog: Поворот антенны на угол" << angle << "° (0xAF, cmd=start)";

    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(true);
    showStatus(QString("Поворот антенны на %1°…").arg(angle));

    if (!m_amsHandler->rotateAntenna(angle)) {
        showStatus("Ошибка отправки команды поворота", true);
        m_btnStart->setEnabled(true);
        m_btnStop->setEnabled(false);
    }
}

void AngleCheckDialog::onStop()
{
    if (!m_amsHandler || !m_amsHandler->isConnected()) {
        showStatus("АМС не подключён", true);
        return;
    }

    qDebug() << "AngleCheckDialog: Остановка вращения антенны (0xAF, cmd=stop)";

    if (!m_amsHandler->stopAntennaRotation()) {
        showStatus("Ошибка отправки команды остановки", true);
    } else {
        showStatus("Команда остановки отправлена");
    }

    m_btnStart->setEnabled(true);
    m_btnStop->setEnabled(false);
}

void AngleCheckDialog::onRotateStatus(quint8 status, float angle)
{
    switch (status) {
    case ROTATE_RUNNING:
        // Вращение продолжается — держим Стоп активным, обновляем угол
        showStatus(QString("Вращение… текущий угол: %1°")
                   .arg(angle, 0, 'f', 1));
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(true);
        break;
    case ROTATE_IDLE_OK:
        // Завершено успешно (или ответ на команду Стоп)
        showStatus(QString("Завершено. Угол: %1°").arg(angle, 0, 'f', 1));
        m_btnStart->setEnabled(true);
        m_btnStop->setEnabled(false);
        break;
    case ROTATE_FAULT:
        showStatus("Аварийная остановка привода вращения!", true);
        m_btnStart->setEnabled(true);
        m_btnStop->setEnabled(false);
        break;
    default:
        showStatus(QString("Неизвестный статус: 0x%1").arg(status, 2, 16, QChar('0')), true);
        m_btnStart->setEnabled(true);
        m_btnStop->setEnabled(false);
        break;
    }
}

void AngleCheckDialog::showStatus(const QString &text, bool error)
{
    m_lblStatus->setText(text);
    m_lblStatus->setStyleSheet(
        error
        ? "color: #c0392b; font-size: 10pt; padding: 4px;"
        : "color: #1a6e2a; font-size: 10pt; padding: 4px;");
    m_lblStatus->setVisible(true);
}

// ===================================================================
//  WorkRegulationDialog — главный диалог «Регламентные работы»
// ===================================================================
WorkRegulationDialog::WorkRegulationDialog(AMSHandler *amsHandler, QWidget *parent)
    : QDialog(parent)
    , m_amsHandler(amsHandler)
{
    setWindowTitle("Регламентные работы");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(340);

    // --- Метка заголовка ---
    auto *lblTitle = new QLabel("Регламентные работы", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("font-size: 11pt; font-weight: bold; padding: 8px;");

    // --- Кнопки режимов ---
    auto *btnInspection  = new QPushButton("Контрольный осмотр (КО)", this);
    auto *btnAngleCheck  = new QPushButton("Проверка правильности установки углов (ТО-2)", this);

    for (auto *btn : {btnInspection, btnAngleCheck}) {
        applyButtonStyle(btn);
        btn->setMinimumHeight(40);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // QPushButton не поддерживает setWordWrap — текст умещается за счёт
        // фиксированной ширины окна и минимальной высоты кнопки
    }

    // --- Кнопка закрытия ---
    auto *btnClose = new QPushButton("Закрыть", this);
    applyButtonStyle(btnClose);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    auto *closeLine = new QHBoxLayout;
    closeLine->addStretch();
    closeLine->addWidget(btnClose);
    closeLine->addStretch();

    // --- Layout ---
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(btnInspection);
    mainLayout->addWidget(btnAngleCheck);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(closeLine);

    // --- Соединения ---
    connect(btnInspection, &QPushButton::clicked,
            this, &WorkRegulationDialog::onInspectionClicked);
    connect(btnAngleCheck, &QPushButton::clicked,
            this, &WorkRegulationDialog::onAngleCheckClicked);
}

void WorkRegulationDialog::onInspectionClicked()
{
    InspectionDialog dlg(m_amsHandler, this);
    dlg.exec();
}

void WorkRegulationDialog::onAngleCheckClicked()
{
    AngleCheckDialog dlg(m_amsHandler, this);
    dlg.exec();
}
