#ifndef ZOOMSCONTAINER_H
#define ZOOMSCONTAINER_H

#include "qwtchartzoom.h"
#include "qwheelzoomsvc.h"
#include "qaxiszoomsvc.h"
#include <list>
#include <QObject>


class ZoomsContainer : public QObject
{
    Q_OBJECT
public:
    ZoomsContainer();
    ~ZoomsContainer();
    QwtChartZoom *attachZoom(QwtPlot *plot, QColor rubberBandColor = Qt::white);
    void connectXZooms();
    void disconnectXZooms();
    void setAxisesScale(int axisId, double min, double max, double step = 0);


protected:

    std::list<QwtChartZoom*> chartZoomList;
    std::list<QWheelZoomSvc*> whlZoomList;
    std::list<QAxisZoomSvc*> axzZoomList;
};

#endif // ZOOMSCONTAINER_H
