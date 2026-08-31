#ifndef ALGORITHMSCALC_H
#define ALGORITHMSCALC_H

#include <QWidget>

namespace Ui {
class AlgorithmsCalculation;
}

// Экран выбора расчёта ("Расчёты"). Раньше был отдельным всплывающим
// диалогом (QDialog), теперь встраивается как страница в общий стек
// MainWindow — сам класс и его .ui/.cpp остаются в своих файлах,
// а наружу отдаёт только сигналы навигации.
class AlgorithmsCalculation : public QWidget {
    Q_OBJECT

public:
    explicit AlgorithmsCalculation(QWidget *parent = nullptr);
    ~AlgorithmsCalculation();

signals:
    void backRequested();
    void landingCalculationRequested();

private:
    Ui::AlgorithmsCalculation *ui;
};

#endif // ALGORITHMSCALC_H
