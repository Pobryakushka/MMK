#include "qwheelzoomsvc.h"

#ifdef Q_WS_WIN
//    #define L_SHIFT 42
    #define R_SHIFT 54
#endif

#ifdef Q_WS_X11
//    #define L_SHIFT 50
    #define R_SHIFT 62
#endif

// Конструктор
QWheelZoomSvc::QWheelZoomSvc() :
    QObject()
{
    // назначаем коэффициент, определяющий изменение масштаба графика
    // при вращении колеса мыши
    sfact = 1.2;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QWheelZoomSvc::attach(QwtChartZoom *zm)
{
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zoom->plot()->installEventFilter(this);
}

// Задание коэффициента масштабирования графика
// при вращении колеса мыши (по умолчанию он равен 1.2)
void QWheelZoomSvc::setWheelFactor(double fact) {
    sfact = fact;
}

// Обработчик всех событий
bool QWheelZoomSvc::eventFilter(QObject *target,QEvent *event)
{
    // если событие произошло для графика, то
    if (target == zoom->plot())
    {
        // если произошло одно из событий от клавиатуры, то
        if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease)
            switchWheel(event); // вызываем соответствующий обработчик
        // если событие вызвано вращением колеса мыши, то
        if (event->type() == QEvent::Wheel)
        {
            procWheel(event);   // вызываем соответствующий обработчик
            zoom->updatePlot(); // обновляем график
        }
    }
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(target,event);
}

// Обработчик нажатия/отпускания клавиши Ctrl или Shift
void QWheelZoomSvc::switchWheel(QEvent *event)
{
    // читаем режим масштабирования
    QwtChartZoom::QConvType ct = zoom->regim();
    // создаем указатель на событие от клавиатуры
    QKeyEvent *kEvent = static_cast<QKeyEvent *>(event);
    // переключаем режим в зависимости от клавиши
    switch (kEvent->key())
    {
    // клавиша Ctrl
    case Qt::Key_Control:
        // в зависимости от события
        switch (event->type())
        {
        // клавиша нажата
        case QEvent::KeyPress:
            // если не включен никакой другой режим,
            if (ct == QwtChartZoom::ctNone)
                // то включаем режим Wheel
                zoom->setRegim(QwtChartZoom::ctWheel);
        // клавиша отпущена
        case QEvent::KeyRelease:
            // если включен режим Wheel,
            if (ct == QwtChartZoom::ctWheel)
                // то выключаем его
                zoom->setRegim(QwtChartZoom::ctNone);
            break;
        // иначе ничего не делаем
        default: ;
        }
        break;
    // клавиша Shift
    case Qt::Key_Shift:
        // в зависимости от события
        switch (event->type())
        {
        // клавиша нажата
        case QEvent::KeyPress:

#ifdef R_SHIFT
// платформа Win или X11
            // если не включен никакой другой режим, то
            if (ct == QwtChartZoom::ctNone)
            {
                // если нажат правый Shift,
                if (kEvent->nativeScanCode() == R_SHIFT)
                    // то включаем режим ctHorWheel
                    zoom->setRegim(QwtChartZoom::ctHorWheel);
                // иначе (нажат левый Shift) включаем режим ctVerWheel
                else zoom->setRegim(QwtChartZoom::ctVerWheel);
            }
#else
// неизвестная платформа
            // если не включен никакой другой режим,
            if (ct == QwtChartZoom::ctNone)
                // то включаем режим ctVerWheel
                zoom->setRegim(QwtChartZoom::ctVerWheel);
#endif

            break;
        // клавиша отпущена
        case QEvent::KeyRelease:

#ifdef R_SHIFT
// платформа Win или X11
            // если отпущен правый Shift,
            if (kEvent->nativeScanCode() == R_SHIFT)
            {
                // если включен режим ctHorWheel,
                if (ct == QwtChartZoom::ctHorWheel)
                    // то выключаем его
                    zoom->setRegim(QwtChartZoom::ctNone);
            }
            // иначе (отпущен левый Shift)
            // если включен режим ctVerWheel,
            else if (ct == QwtChartZoom::ctVerWheel)
                // то выключаем его
                zoom->setRegim(QwtChartZoom::ctNone);
#else
// неизвестная платформа
            // если включен режим ctVerWheel,
            if (ct == QwtChartZoom::ctVerWheel)
                // то выключаем его
                zoom->setRegim(QwtChartZoom::ctNone);
#endif

            break;
        // иначе ничего не делаем
        default: ;
        }
        break;
        // для остальных ничего не делаем
    default: ;
    }
}

// Применение изменений по вращении колеса мыши
void QWheelZoomSvc::applyWheel(QEvent *event,bool ax,bool ay)
{
    // приводим тип QEvent к QWheelEvent
    QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);
    // если вращается вертикальное колесо мыши
    if (wEvent->orientation() == Qt::Vertical)
    {
        // определяем угол поворота колеса мыши
        // (значение 120 соответствует углу поворота 15°)
        int wd = wEvent->delta();
        // вычисляем масштабирующий множитель
        // (во сколько раз будет увеличен/уменьшен график)
        double kw = sfact*wd/120;
        if (wd != 0)    // если колесо вращалось, то
        {
            // фиксируем исходные границы графика (если этого еще не было сделано)
            zoom->fixBounds();
            // получаем указатель на график
            QwtPlot *plt = zoom->plot();
            if (ax) // если задано масштабирование по горизонтали
            {
                // получаем карту основной горизонтальной шкалы
                QwtScaleMap sm = plt->canvasMap(zoom->masterH());
                // определяем центр отображаемого на шкале x интервала
                double mx = (sm.s1()+sm.s2())/2;
                // и полуширину интервала
                double dx = (sm.s2()-sm.s1())/2;
                // в зависимости от знака угла поворота колеса мыши
                // уменьшаем полуширину отображаемых интервалов в kw раз
                if (wd > 0) dx /= kw;
                // или увеличиваем полуширину отображаемых интервалов в -kw раз
                else dx *= -kw;
                // устанавливаем новые левую и правую границы шкалы для оси x
                // (центр изображаемой части графика остается на месте,
                // а границы удаляются от центра, т.о. изображение графика уменьшается)
                zoom->isb_x->set(mx-dx,mx+dx);
            }
            if (ay) // если задано масштабирование по вертикали
            {
                // получаем карту основной вертикальной шкалы
                QwtScaleMap sm = plt->canvasMap(zoom->masterV());
                // определяем центр отображаемого на шкале y интервала
                double my = (sm.s1()+sm.s2())/2;
                // и полуширину интервала
                double dy = (sm.s2()-sm.s1())/2;
                // в зависимости от знака угла поворота колеса мыши
                // уменьшаем полуширину отображаемых интервалов в kw раз
                if (wd > 0) dy /= kw;
                // увеличиваем полуширину отображаемых интервалов в -kw раз
                else dy *= -kw;
                // устанавливаем новые нижнюю и верхнюю границы вертикальной шкалы
                // (центр изображаемой части графика остается на месте,
                // а границы удаляются от центра, т.о. изображение графика уменьшается)
                zoom->isb_y->set(my-dy,my+dy);
            }
            // перестраиваем график (синхронно с остальными)
            plt->replot();
        }
    }
}

// Обработчик вращения колеса мыши
void QWheelZoomSvc::procWheel(QEvent *event)
{
    // в зависимости от включенного режима вызываем
    // масштабирование с соответствующими параметрами
    switch (zoom->regim())
    {
        // Wheel - была нажата клавиша Ctrl -
            // масштабирование по обеим осям
    case QwtChartZoom::ctWheel: applyWheel(event,true,true); break;
        //VerWheel - была нажата левая клавиша Shift -
            // масштабирование только по вертикальной оси
    case QwtChartZoom::ctVerWheel: applyWheel(event,false,true); break;
        // HorWheel - была нажата правая клавиша Shift -
            // масштабирование только по горизонтальной оси
    case QwtChartZoom::ctHorWheel: applyWheel(event,true,false); break;
        // для прочих режимов ничего не делаем
    default: ;
    }
}
