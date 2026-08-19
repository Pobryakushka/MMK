#include "RpvIndicator.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

RpvIndicator::RpvIndicator(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setToolTip("Положение РПВ (угол антенны)");
}

void RpvIndicator::setAngle(double degrees)
{
    m_angle = degrees;
    update();
}

// Компас — крупный элемент интерфейса (тайл "Угол РПВ"), поэтому и
// предпочтительный, и минимальный размер заметно больше, чем у прежнего
// компактного индикатора. Реальный размер всё равно диктуется layout'ом
// (см. minimumSize/maximumSize тайла в mainwindow.ui).
QSize RpvIndicator::sizeHint()        const { return {260, 260}; }
QSize RpvIndicator::minimumSizeHint() const { return {160, 160}; }

void RpvIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r    = rect();
    const double side = qMin(r.width(), r.height());
    // Область под цифровую подпись под циферблатом.
    const double labelH = qMax(28.0, side * 0.16);
    const double dialD  = side - labelH;    // диаметр циферблата
    const double cx     = r.width()  / 2.0;
    const double cy     = dialD / 2.0 + 2.0;// центр циферблата
    const double rad    = dialD / 2.0 - 4.0;// радиус окружности

    QRectF dialRect(cx - rad, cy - rad, rad * 2, rad * 2);

    // ── Циферблат: фон + внешняя рамка ───────────────────────────────────
    p.setPen(QPen(QColor(0xE4, 0xE6, 0xE7), 1.4));
    p.setBrush(QColor(0xFB, 0xFB, 0xFB));
    p.drawEllipse(dialRect);

    // ── Риски: каждые 10°, крупные каждые 30° ───────────────────────────
    for (int deg = 0; deg < 360; deg += 10) {
        const bool   major = (deg % 30 == 0);
        const double a     = qDegreesToRadians(deg - 90.0); // 0° вверх
        const double rOut  = rad;
        const double rIn   = major ? rad * 0.83 : rad * 0.90;
        p.setPen(QPen(major ? QColor(0x9A, 0xA0, 0xA3) : QColor(0xC7, 0xCB, 0xCC),
                      major ? 1.6 : 1.0));
        p.drawLine(QLineF(cx + rIn  * qCos(a), cy + rIn  * qSin(a),
                          cx + rOut * qCos(a), cy + rOut * qSin(a)));
    }

    // ── Подписи градусов каждые 30°, крупнее на 0/90/180/270 ────────────
    {
        const double rLabel = rad * 0.70;
        for (int deg = 0; deg < 360; deg += 30) {
            const bool   cardinal = (deg % 90 == 0);
            QFont f = p.font();
            f.setPointSizeF(qMax(6.5, side * (cardinal ? 0.052 : 0.040)));
            f.setBold(cardinal);
            p.setFont(f);
            p.setPen(cardinal ? QColor(0x5B, 0x62, 0x66) : QColor(0x8A, 0x90, 0x94));

            const double a = qDegreesToRadians(deg - 90.0);
            const QString txt = QString::number(deg);
            QFontMetricsF fm(f);
            const QSizeF sz = fm.size(0, txt);
            p.drawText(QRectF(cx + rLabel * qCos(a) - sz.width() / 2,
                              cy + rLabel * qSin(a) - sz.height() / 2,
                              sz.width(), sz.height()),
                       Qt::AlignCenter, txt);
        }
    }

    // ── Стрелка (0° вверх, растёт по часовой) ────────────────────────────
    const double needleAngle = qDegreesToRadians(m_angle - 90.0);
    const double tipR   = rad * 0.80;
    const double tailR  = rad * 0.26;

    const double tipX   = cx + tipR  * qCos(needleAngle);
    const double tipY   = cy + tipR  * qSin(needleAngle);
    const double tailX  = cx - tailR * qCos(needleAngle);
    const double tailY  = cy - tailR * qSin(needleAngle);

    const double perpA  = needleAngle + M_PI / 2.0;
    const double hw     = rad * 0.075; // полуширина наконечника
    const double hbLen  = rad * 0.16;  // длина основания треугольника

    QPainterPath arrow;
    arrow.moveTo(tipX, tipY);
    arrow.lineTo(tipX - hbLen * qCos(needleAngle) + hw * qCos(perpA),
                 tipY - hbLen * qSin(needleAngle) + hw * qSin(perpA));
    arrow.lineTo(tailX, tailY);
    arrow.lineTo(tipX - hbLen * qCos(needleAngle) - hw * qCos(perpA),
                 tipY - hbLen * qSin(needleAngle) - hw * qSin(perpA));
    arrow.closeSubpath();

    const QColor needleColor(0x0F, 0x6B, 0x4F); // основной акцентный зелёный приложения

    p.setPen(Qt::NoPen);
    p.setBrush(needleColor);
    p.drawPath(arrow);

    p.setPen(QPen(needleColor, 2.0));
    p.drawLine(QLineF(tailX, tailY,
                      tipX - hbLen * qCos(needleAngle),
                      tipY - hbLen * qSin(needleAngle)));

    // Втулка в центре
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(QColor(0x1C, 0x1F, 0x22));
    p.drawEllipse(QPointF(cx, cy), rad * 0.045 + 2.0, rad * 0.045 + 2.0);

    // ── Цифровое значение угла под циферблатом ───────────────────────────
    {
        QFont f = p.font();
        f.setPointSizeF(qMax(9.0, side * 0.10));
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0x1C, 0x1F, 0x22));

        const QString txt = QString("%1°").arg(m_angle, 0, 'f', 1);
        const QRectF  lr(0, dialD, r.width(), labelH);
        p.drawText(lr, Qt::AlignHCenter | Qt::AlignVCenter, txt);
    }
}
