#ifndef METEO11_H
#define METEO11_H

#include <QDialog>

namespace Ui {
class Meteo11;
}

class Meteo11 : public QDialog {
    Q_OBJECT

public:
    explicit Meteo11(QWidget *parent = nullptr);
    ~Meteo11();

private:
    Ui::Meteo11 *ui;

};

#endif // METEO11_H
