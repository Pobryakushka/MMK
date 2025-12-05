#include "InitialParameters.h"

#ifndef FORMMAPVIEW_H
#define FORMMAPVIEW_H

namespace Ui {
class FormMapView;
}

class FormMapView : public QDialog
{
    Q_OBJECT

public:
//    explicit FormMapView(int xCoordMainWindow, int yCoordMainWindow, QWidget *parent = 0);
    explicit FormMapView(QWidget *parent = 0);
    ~FormMapView();
    void reject();

protected:
    //void paintEvent(QPaintEvent *event); // переопределение виртуальной функции
    void resizeEvent(QResizeEvent *fmv);

private:
    Ui::FormMapView *ui;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QImage *image;

//    QAction *fitToWindowAct;
};

#endif // FORMMAPVIEW_H
