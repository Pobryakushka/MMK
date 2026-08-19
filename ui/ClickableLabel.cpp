#include "ClickableLabel.h"
#include <QMouseEvent>

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = true;
    QLabel::mousePressEvent(event);
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        if (rect().contains(event->pos()))
            emit clicked();
    }
    QLabel::mouseReleaseEvent(event);
}
