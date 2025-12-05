#ifndef ALGORITHMSCALC_H
#define ALGORITHMSCALC_H

#include <QDialog>

namespace Ui {
class AlgorithmsCalculation;
}

class AlgorithmsCalculation : public QDialog {
    Q_OBJECT

public:
    explicit AlgorithmsCalculation(QWidget *parent = nullptr);
    ~AlgorithmsCalculation();

private:
    Ui::AlgorithmsCalculation *ui;


};

#endif // ALGORITHMSCALC_H
