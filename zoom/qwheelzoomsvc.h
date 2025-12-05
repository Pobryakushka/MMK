#ifndef QWHEELZOOMSVC_H
#define QWHEELZOOMSVC_H

#include "qwtchartzoom.h"

class QWheelZoomSvc : public QObject
{
    Q_OBJECT

public:
    // конструктор
    explicit QWheelZoomSvc();

    // прикрепление интерфейса к менеджеру масштабирования
    void attach(QwtChartZoom *);

    // задание коэффициента масштабирования графика
    // при вращении колеса мыши
    void setWheelFactor(double);

protected:
    // обработчик всех событий
    bool eventFilter(QObject *,QEvent *);

private:
    QwtChartZoom *zoom;     // Опекаемый менеджер масштабирования
    double sfact;           // Коэффициент, определяющий изменение масштаба графика
                            // при вращении колеса мыши (по умолчанию равен 1.2)

    // обработчик нажатия/отпускания клавиши Ctrl
    void switchWheel(QEvent *);
    // применение изменений по вращении колеса мыши
    void applyWheel(QEvent *,bool,bool);
    // обработчик вращения колеса мыши
    void procWheel(QEvent *);
};

#endif // QWHEELZOOMSVC_H
