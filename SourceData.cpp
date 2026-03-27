#include "SourceData.h"
#include "ui_SourceData.h"
#include "Meteo11.h"
#include "GroundMeteoParams.h"
#include <QDebug>

SourceData::SourceData(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::SourceData)
    , groundMeteoParams(nullptr)
{
    ui->setupUi(this);

    // Создаём постоянный экземпляр GroundMeteoParams (не показываем сразу)
    groundMeteoParams = new GroundMeteoParams(this);
    qDebug() << "GroundMeteoParams instance created in SourceData";

    connect(ui->btnCloseSourceData, &QPushButton::clicked, this, [this](){
        hide();
    });

    connect(ui->btnMeteo11, &QPushButton::clicked, this, [this](){
        Meteo11 dialog(this);
        dialog.exec();
    });

    connect(ui->btnGroundLayerParam, &QPushButton::clicked, this, [this](){
        // Показываем уже созданный экземпляр
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
    // groundMeteoParams удалится автоматически через parent
}
