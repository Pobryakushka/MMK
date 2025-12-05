#include "Meteo11.h"
#include "ui_Meteo11.h"

Meteo11::Meteo11(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint)
    , ui(new Ui::Meteo11)
{
    ui->setupUi(this);

    connect(ui->btnMet11Close, &QPushButton::clicked, this, [this](){
        close();
    });
    connect(ui->btnMet11Clear, &QPushButton::clicked, this, [this](){
        ui->tableWidget_meteo11->clearContents();
        ui->lineEdit_Met11AchievedSensHeight->clear();
        ui->lineEdit_Met11DateTime->clear();
        ui->lineEdit_Met11GroundPresDev->clear();
        ui->lineEdit_Met11GroundVertTempDev->clear();
        ui->lineEdit_Met11StationHeight->clear();
        ui->lineEdit_Met11StationNum->clear();
    });
}

Meteo11::~Meteo11()
{
    delete ui;
}

