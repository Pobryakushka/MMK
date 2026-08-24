#include "workregulationhubpage.h"
#include "ui_workregulationhubpage.h"

WorkRegulationHubPage::WorkRegulationHubPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WorkRegulationHubPage)
{
    ui->setupUi(this);

    connect(ui->btnBackFromWorkRegulationHub, &QPushButton::clicked,
            this, &WorkRegulationHubPage::backRequested);
    connect(ui->hubTileKO, &ClickableFrame::clicked,
            this, &WorkRegulationHubPage::openInspectionRequested);
    connect(ui->hubTileAngle, &ClickableFrame::clicked,
            this, &WorkRegulationHubPage::openAngleCheckRequested);
}

WorkRegulationHubPage::~WorkRegulationHubPage()
{
    delete ui;
}
