#include "zoomscontainer.h"

ZoomsContainer::ZoomsContainer() : QObject()
{
}

ZoomsContainer::~ZoomsContainer()
{
    for (std::list<QwtChartZoom*>::iterator it = chartZoomList.begin(); it != chartZoomList.end(); ++it) {
        delete *it;
    }
    for (std::list<QWheelZoomSvc*>::iterator it = whlZoomList.begin(); it != whlZoomList.end(); ++it) {
        delete *it;
    }
    for (std::list<QAxisZoomSvc*>::iterator it = axzZoomList.begin(); it != axzZoomList.end(); ++it) {
        delete *it;
    }
    chartZoomList.clear();
    whlZoomList.clear();
    axzZoomList.clear();
}

QwtChartZoom* ZoomsContainer::attachZoom(QwtPlot *plot, QColor rubberBandColor)
{
    QwtChartZoom* reflectZoom = new QwtChartZoom(plot);
    reflectZoom->setRubberBandColor(rubberBandColor);
    QWheelZoomSvc* whlzmsvc = new QWheelZoomSvc();
    whlzmsvc->attach(reflectZoom);
    QAxisZoomSvc* axzmsvc = new QAxisZoomSvc();
    axzmsvc->attach(reflectZoom);
//    reflectZoom->setLightMode(true);

    chartZoomList.push_back(reflectZoom);
    whlZoomList.push_back(whlzmsvc);
    axzZoomList.push_back(axzmsvc);
    return reflectZoom;
}

void ZoomsContainer::setAxisesScale(int axisId, double min, double max, double step)
{
    for (std::list<QwtChartZoom*>::iterator it = chartZoomList.begin(); it != chartZoomList.end(); ++it) {
        (*it)->plot()->setAxisScale(axisId, min, max, step);
        (*it)->fixBoundaries();
    }
}

void ZoomsContainer::connectXZooms()
{
    if (chartZoomList.size() < 2)
        return;
    std::list<QwtChartZoom*>::iterator itprev, ite;
    ite = chartZoomList.end();
    --ite;
    for (std::list<QwtChartZoom*>::iterator it = chartZoomList.begin(); it != ite; /*++it*/) {
        itprev = it;
        ++it;
        connect((*(itprev))->isb_x, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
                (*(it))->isb_x, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
//        connect((*(itprev))->isb_y, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
//                (*(it))->isb_y, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
    }
    std::list<QwtChartZoom*>::iterator it = chartZoomList.end();
    --it;
    connect((*it)->isb_x, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
            (*chartZoomList.begin())->isb_x, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
//    connect((*it)->isb_y, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
//            (*chartZoomList.begin())->isb_y, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
}

void ZoomsContainer::disconnectXZooms()
{
    if (chartZoomList.size() < 2)
        return;
    std::list<QwtChartZoom*>::iterator itprev, ite;
    ite = chartZoomList.end();
    --ite;
    for (std::list<QwtChartZoom*>::iterator it = chartZoomList.begin(); it != ite; /*++it*/) {
        itprev = it;
        ++it;
//        disconnect((*(itprev))->isb_x, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
//                (*(it))->isb_x, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
        disconnect((*(itprev))->isb_y, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
                (*(it))->isb_y, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
    }
    std::list<QwtChartZoom*>::iterator it = chartZoomList.end();
    --it;
//    disconnect((*it)->isb_x, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
//            (*chartZoomList.begin())->isb_x, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
    disconnect((*it)->isb_y, SIGNAL(axisScaleChanged(QwtPlot::Axis,double,double)),
            (*chartZoomList.begin())->isb_y, SLOT(setAxisScale(QwtPlot::Axis,double,double)));
}
