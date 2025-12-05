#ifndef GROUNDMETEOPARAMS_H
#define GROUNDMETEOPARAMS_H

#include <QDialog>

namespace Ui {
class GroundMeteoParams;
}

class GroundMeteoParams : public QDialog {
    Q_OBJECT

public:
    explicit GroundMeteoParams(QWidget *parent = nullptr);
    void deleteDataFromTable();
    ~GroundMeteoParams();

private:
    Ui::GroundMeteoParams *ui;

};

#endif // GROUNDMETEOPARAMS_H
