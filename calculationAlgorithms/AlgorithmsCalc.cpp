#include "AlgorithmsCalc.h"
#include "ui_AlgorithmsCalc.h"

AlgorithmsCalculation::AlgorithmsCalculation(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlgorithmsCalculation)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);

    connect(ui->btnAlgBack, &QPushButton::clicked, this, [this](){
        emit backRequested();
    });
    connect(ui->btnAlgCalcWithoutScan, &QPushButton::clicked, this, [this](){
        // Расчёт без сканирования пока не реализован
    });
    connect(ui->btnAlgLandingCalc, &QPushButton::clicked, this, [this](){
        emit landingCalculationRequested();
    });
}

AlgorithmsCalculation::~AlgorithmsCalculation()
{
    delete ui;
}
