#include "SourceData.h"
#include "ui_SourceData.h"
#include "Meteo11.h"
#include "GroundMeteoParams.h"

SourceData::SourceData(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::SourceData)
{
    ui->setupUi(this);

    connect(ui->btnCloseSourceData, &QPushButton::clicked, this, [this](){
        close();
    });
    connect(ui->btnMeteo11, &QPushButton::clicked, this, [this](){
        Meteo11 dialog(this);
        dialog.exec();
    });
    connect(ui->btnGroundLayerParam, &QPushButton::clicked, this, [this](){
       GroundMeteoParams dialog(this);
       dialog.exec();
    });
}

SourceData::~SourceData()
{
    delete ui;
}

