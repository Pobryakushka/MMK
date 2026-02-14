#ifndef ALGORITHMSCALC_H
#define ALGORITHMSCALC_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class AlgorithmsCalculation;
}

class AlgorithmsCalculation : public QDialog {
    Q_OBJECT

public:
    explicit AlgorithmsCalculation(QWidget *parent = nullptr);
    ~AlgorithmsCalculation();

private slots:
    void onLandingCalcClicked();

private:
    Ui::AlgorithmsCalculation *ui;
    MainWindow *m_mainWindow;

};

#endif // ALGORITHMSCALC_H
