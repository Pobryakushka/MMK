#include "SourceData.h"
#include "ui_SourceData.h"
#include "Meteo11.h"
#include "GroundMeteoParams.h"
#include <QCloseEvent>
#include <QDebug>

SourceData::SourceData(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::SourceData)
    , groundMeteoParams(nullptr)
    , m_meteo11Dialog(nullptr)
{
    ui->setupUi(this);

    groundMeteoParams = new GroundMeteoParams(this);
    qDebug() << "GroundMeteoParams instance created in SourceData";

    // Создаём ПОСТОЯННЫЙ экземпляр Meteo11 — данные не теряются
    // между открытиями диалога и сохраняются до нажатия «Пуск»
    m_meteo11Dialog = new Meteo11(this);

    connect(ui->btnCloseSourceData, &QPushButton::clicked, this, [this](){
        hide();
    });

    connect(ui->btnMeteo11, &QPushButton::clicked, this, [this](){
        m_meteo11Dialog->show();
        m_meteo11Dialog->raise();
        m_meteo11Dialog->activateWindow();
    });

    connect(ui->btnGroundLayerParam, &QPushButton::clicked, this, [this](){
        if (groundMeteoParams) {
            groundMeteoParams->show();
            groundMeteoParams->raise();
            groundMeteoParams->activateWindow();
        }
    });
}

SourceData::~SourceData()
{
    delete ui;
    // m_meteo11Dialog и groundMeteoParams удаляются через parent
}

void SourceData::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

bool SourceData::hasMeteo11Bulletin() const
{
    return m_meteo11Dialog && m_meteo11Dialog->isApplied();
}

QJsonObject SourceData::meteo11BulletinJson() const
{
    return m_meteo11Dialog ? m_meteo11Dialog->bulletinJson() : QJsonObject();
}

QDateTime SourceData::meteo11BulletinTime() const
{
    return m_meteo11Dialog ? m_meteo11Dialog->bulletinTime() : QDateTime();
}

QString SourceData::meteo11ValidityPeriod() const
{
    return m_meteo11Dialog ? m_meteo11Dialog->validityPeriod() : QString();
}

void SourceData::resetMeteo11Applied()
{
    if (m_meteo11Dialog)
        m_meteo11Dialog->resetApplied();
}