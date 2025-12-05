#include "AlgorithmsCalc.h"
#include "ui_AlgorithmsCalc.h"
#include "LandingCalculation.h"

AlgorithmsCalculation::AlgorithmsCalculation(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::AlgorithmsCalculation)
{
    ui->setupUi(this);

    connect(ui->btnAlgClose, &QPushButton::clicked, this, [this](){
        close();
    });
    connect(ui->btnAlgCalcWithoutScan, &QPushButton::clicked, this, [this](){

    });
    connect(ui->btnAlgLandingCalc, &QPushButton::clicked, this, [this](){
        LandingCalculation dialog(this);
        dialog.exec();
    });
}

AlgorithmsCalculation::~AlgorithmsCalculation()
{
    delete ui;
}
