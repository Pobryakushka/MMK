#include "RpvIndicator.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

RpvIndicator::RpvIndicator(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setToolTip("Положение РПВ (угол антенны)");
}

void RpvIndicator::setAngle(double degrees)
{
    m_angle = degrees;
    update();
}

QSize RpvIndicator::sizeHint()        const { return {72, 72}; }
QSize RpvIndicator::minimumSizeHint() const { return {56, 56}; }

void RpvIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r    = rect();
    const double side = qMin(r.width(), r.height());
    // Область под цифровой подпись: нижние ~14px
    const double labelH = 14.0;
    const double dialD  = side - labelH;    // диаметр циферблата
    const double cx     = r.width()  / 2.0;
    const double cy     = dialD / 2.0;      // центр циферблата
    const double rad    = dialD / 2.0 - 2.0;// радиус окружности

    // ── Циферблат ──────────────────────────────────────────────────────────
    QRectF dialRect(cx - rad, cy - rad, rad * 2, rad * 2);

    // Фон
    p.setPen(QPen(QColor(0xBD, 0xBD, 0xBD), 1.2));
    p.setBrush(QColor(0xFA, 0xFA, 0xFA));
    p.drawEllipse(dialRect);

    // Риски: 4 длинных (0°/90°/180°/270°) + 4 коротких (45° и т.д.)
    p.setPen(QPen(QColor(0x90, 0x90, 0x90), 1.0));
    for (int i = 0; i < 8; ++i) {
        const double a  = qDegreesToRadians(i * 45.0 - 90.0); // 0° вверх
        const bool   major = (i % 2 == 0);
        const double rIn   = major ? rad * 0.72 : rad * 0.82;
        p.drawLine(QLineF(cx + rIn  * qCos(a), cy + rIn  * qSin(a),
                          cx + rad  * qCos(a), cy + rad  * qSin(a)));
    }

    // Метки сторон (маленький шрифт)
    {
        QFont f = p.font();
        f.setPointSizeF(qMax(5.5, side * 0.085));
        p.setFont(f);
        p.setPen(QColor(0x55, 0x55, 0x55));

        const double lbl = rad * 0.58;
        struct { const char *txt; double a; } marks[] = {
            {"0",   -90}, {"90",   0}, {"180",  90}, {"270", 180}
        };
        QFontMetricsF fm(f);
        for (auto &m : marks) {
            const double a = qDegreesToRadians(m.a);
            const QSizeF sz = fm.size(0, m.txt);
            p.drawText(QRectF(cx + lbl * qCos(a) - sz.width() / 2,
                              cy + lbl * qSin(a) - sz.height() / 2,
                              sz.width(), sz.height()),
                       Qt::AlignCenter, m.txt);
        }
    }

    // ── Стрелка ─────────────────────────────────────────────────────────────
    // Угол: 0° вверх, по часовой
    const double needleAngle = qDegreesToRadians(m_angle - 90.0);
    const double tipR   = rad * 0.80;
    const double tailR  = rad * 0.28;

    const double tipX   = cx + tipR  * qCos(needleAngle);
    const double tipY   = cy + tipR  * qSin(needleAngle);
    const double tailX  = cx - tailR * qCos(needleAngle);
    const double tailY  = cy - tailR * qSin(needleAngle);

    // Боковые вершины наконечника (треугольник)
    const double perpA  = needleAngle + M_PI / 2.0;
    const double hw     = rad * 0.09;  // полуширина наконечника
    const double hbLen  = rad * 0.20;  // длина основания треугольника

    QPainterPath arrow;
    arrow.moveTo(tipX, tipY);
    arrow.lineTo(tipX - hbLen * qCos(needleAngle) + hw * qCos(perpA),
                 tipY - hbLen * qSin(needleAngle) + hw * qSin(perpA));
    arrow.lineTo(tailX, tailY);
    arrow.lineTo(tipX - hbLen * qCos(needleAngle) - hw * qCos(perpA),
                 tipY - hbLen * qSin(needleAngle) - hw * qSin(perpA));
    arrow.closeSubpath();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x15, 0x65, 0xC0));   // синий как lblProgressPercent
    p.drawPath(arrow);

    // Хвост стрелки (тонкая линия)
    p.setPen(QPen(QColor(0x15, 0x65, 0xC0), 1.5));
    p.drawLine(QLineF(tailX, tailY,
                      tipX - hbLen * qCos(needleAngle),
                      tipY - hbLen * qSin(needleAngle)));

    // Центральная точка
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x15, 0x65, 0xC0));
    p.drawEllipse(QPointF(cx, cy), 2.8, 2.8);

    // ── Цифровое значение угла ──────────────────────────────────────────────
    {
        QFont f = p.font();
        f.setPointSizeF(qMax(6.0, side * 0.115));
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0x15, 0x65, 0xC0));

        const QString txt = QString("%1°").arg(m_angle, 0, 'f', 1);
        const QRectF  lr(0, side - labelH, r.width(), labelH);
        p.drawText(lr, Qt::AlignHCenter | Qt::AlignVCenter, txt);
    }
}
