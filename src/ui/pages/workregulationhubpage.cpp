#include "ui/pages/workregulationhubpage.h"
#include "ui_workregulationhubpage.h"
#include "ui/theme/ScreenTheme.h"

WorkRegulationHubPage::WorkRegulationHubPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WorkRegulationHubPage)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);

    // Вид экрана в стиле «Архива измерений» (фон, поля, кнопки,
    // группы). Роли кнопок помечаем ДО темы — селекторы [primary]/[nav]
    // должны сработать сразу при первой полировке стиля.
    setupArchiveBackButton(ui->btnBackFromWorkRegulationHub);
    applyArchiveScreenTheme(this);

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
