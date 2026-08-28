#include "AlgorithmsCalc.h"
#include "ui_AlgorithmsCalc.h"
#include "../ui/ScreenTheme.h"

AlgorithmsCalculation::AlgorithmsCalculation(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlgorithmsCalculation)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);

    // Вид экрана в стиле «Архива измерений» (фон, поля, кнопки,
    // группы). Роли кнопок помечаем ДО темы — селекторы [primary]/[nav]
    // должны сработать сразу при первой полировке стиля.
    setupArchiveBackButton(ui->btnAlgBack);
    applyArchiveScreenTheme(this);

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
