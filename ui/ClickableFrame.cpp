#include "ClickableFrame.h"
#include <QMouseEvent>

ClickableFrame::ClickableFrame(QWidget *parent)
    : QFrame(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickableFrame::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = true;
    QFrame::mousePressEvent(event);
}

void ClickableFrame::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        if (rect().contains(event->pos()))
            emit clicked();
    }
    QFrame::mouseReleaseEvent(event);
}
