#include "GroundMeteoParams.h"
#include "ui_GroundMeteoParams.h"

GroundMeteoParams::GroundMeteoParams(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint)
    , ui(new Ui::GroundMeteoParams)
{
    ui->setupUi(this);

    connect(ui->btnGroundParamsClose, &QPushButton::clicked, this, [this](){
        close();
    });
    connect(ui->btnGroundParamsClear, &QPushButton::clicked, this, &GroundMeteoParams::deleteDataFromTable);
}

GroundMeteoParams::~GroundMeteoParams()
{
    delete ui;
}

void GroundMeteoParams::deleteDataFromTable(){
    int columnToClear = 1;
    int rowCount = ui->tableWidget_GroundParams->rowCount();

    for (int row = 0; row < rowCount; ++row){
        QTableWidgetItem *item = ui->tableWidget_GroundParams->item(row, columnToClear);
        if (item){
            item->setText("");
        }
    }
}
