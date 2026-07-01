#include "AlgorithmsCalc.h"
#include "ui_AlgorithmsCalc.h"
#include "LandingCalculation.h"
#include "CoordHelper.h"
#include <QMessageBox>
#include <QLineEdit>

AlgorithmsCalculation::AlgorithmsCalculation(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::AlgorithmsCalculation)
    , m_mainWindow(qobject_cast<MainWindow *>(parent))
{
    ui->setupUi(this);

    connect(ui->btnAlgClose, &QPushButton::clicked, this, [this](){
        close();
    });
    connect(ui->btnAlgCalcWithoutScan, &QPushButton::clicked, this, [](){

    });
    connect(ui->btnAlgLandingCalc, &QPushButton::clicked,
            this, &AlgorithmsCalculation::onLandingCalcClicked);
}

AlgorithmsCalculation::~AlgorithmsCalculation()
{
    delete ui;
}

void AlgorithmsCalculation::onLandingCalcClicked()
{
    LandingCalculation dialog(this);

    if (m_mainWindow) {
        QLineEdit *editLat = m_mainWindow->findChild<QLineEdit *>("editLatitude");
        QLineEdit *editLon = m_mainWindow->findChild<QLineEdit *>("editLongitude");
        QLineEdit *editAlt = m_mainWindow->findChild<QLineEdit *>("editAltitude");

        if (editLat && editLon && editAlt) {
            double lat = 0.0, lon = 0.0;
            bool ok1 = CoordHelper::parseDMS(editLat->text(), lat);
            bool ok2 = CoordHelper::parseDMS(editLon->text(), lon);
            bool ok3;
            double alt = editAlt->text().toDouble(&ok3);

            if (ok1 && ok2 && ok3) {
                dialog.setMeteoStationCoords(lat, lon, alt);
            } else {
                QMessageBox::information(this, "Координаты метеокомплекса",
                                         "Координаты метеокомплекса не заданы или введены некорректно.\n"
                                         "Расстояние и углы от М до ТПП/ТНВ не будут рассчитаны.\n\n"
                                         "Введите координаты на главном экране и повторите расчёт.");
            }
        }
    }

    dialog.exec();
}
