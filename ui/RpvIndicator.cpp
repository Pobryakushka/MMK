#include "RpvIndicator.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>

// Внутренняя граница кольца крупных рисок (доля радиуса). Подписи градусов
// размещаются строго внутри неё — см. paintEvent().
static constexpr double kTickInnerMajor = 0.90;
// Нижняя граница подбора размера подписи (px). Ниже неё шрифт не
// уменьшаем — лучше едва заметное касание, чем нечитаемые цифры.
static constexpr int kMinLabelPx = 7;

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
QSize RpvIndicator::minimumSizeHint() const { return {180, 180}; }

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
    // kTickInnerMajor — внутренняя граница кольца рисок. От неё же считается
    // радиус подписей ниже, поэтому значение вынесено в константу: подписи и
    // риски не должны пересекаться ни при каком размере виджета.
    for (int deg = 0; deg < 360; deg += 10) {
        const bool   major = (deg % 30 == 0);
        const double a     = qDegreesToRadians(deg - 90.0); // 0° вверх
        const double rOut  = rad;
        const double rIn   = major ? rad * kTickInnerMajor : rad * 0.945;
        p.setPen(QPen(major ? QColor(0x9A, 0xA0, 0xA3) : QColor(0xC7, 0xCB, 0xCC),
                      major ? 1.6 : 1.0));
        p.drawLine(QLineF(cx + rIn  * qCos(a), cy + rIn  * qSin(a),
                          cx + rOut * qCos(a), cy + rOut * qSin(a)));
    }

    // ── Подписи градусов каждые 30°, крупнее на 0/90/180/270 ────────────
    {
        // Насколько подпись выступает наружу по радиусу, зависит от того, под
        // каким углом она стоит: у верхней/нижней это половина высоты, у
        // боковых — половина ширины, у диагональных — промежуточное значение.
        // Считаем точно (опорная функция прямоугольника), а не по половине
        // диагонали: завышенная оценка уводит подписи к центру, где они
        // начинают наползать уже друг на друга.
        auto radialExtent = [](const QSizeF &sz, double a) {
            return 0.5 * (sz.width() * qFabs(qCos(a)) + sz.height() * qFabs(qSin(a)));
        };

        // ПИКСЕЛЬНЫЙ размер шрифта, а не пунктовый: пункты Qt переводит в
        // пиксели ещё раз, через DPI экрана, и на планшете (масштаб 150%)
        // подписи росли дважды, а сам циферблат — один раз.
        //
        // Размер ПОДБИРАЕТСЯ так, чтобы по дуге поместились ВСЕ двенадцать
        // подписей. Раньше вместо подбора был запасной вариант «показать
        // только 0/90/180/270»: при более широком системном шрифте (DejaVu
        // Sans на Astra) он срабатывал уже на обычном размере тайла, и шкала
        // молча теряла восемь подписей. Подписи каждые 30° несут смысл —
        // уменьшаем шрифт, но показываем их все.
        const double gap = rad * 0.05;
        QFont fMinor = p.font();
        QFont fMajor = p.font();
        double rLabel = rad * 0.35;
        int minorPx = qMax(kMinLabelPx, qRound(side * 0.052));
        int majorPx = qMax(kMinLabelPx + 1, qRound(side * 0.068));

        for (;;) {
            fMinor.setPixelSize(minorPx); fMinor.setBold(false);
            fMajor.setPixelSize(majorPx); fMajor.setBold(true);
            const QFontMetricsF fmMinor(fMinor), fmMajor(fMajor);

            double outward = 0.0, wCardinal = 0.0, wMinor = 0.0;
            for (int deg = 0; deg < 360; deg += 30) {
                const bool cardinal = (deg % 90 == 0);
                const QSizeF sz = (cardinal ? fmMajor : fmMinor).size(0, QString::number(deg));
                outward = qMax(outward, radialExtent(sz, qDegreesToRadians(deg - 90.0)));
                if (cardinal) wCardinal = qMax(wCardinal, sz.width());
                else          wMinor    = qMax(wMinor,    sz.width());
            }

            rLabel = qMax(rad * 0.35, rad * kTickInnerMajor - gap - outward);

            // Соседние подписи — всегда «основная + промежуточная», поэтому
            // нужен полусумма их ширин, а не удвоенная максимальная.
            const double arcPerLabel = 2.0 * M_PI * rLabel / 12.0;
            const double needed      = (wCardinal + wMinor) / 2.0 + rad * 0.04;
            if (arcPerLabel >= needed || minorPx <= kMinLabelPx)
                break;

            --minorPx;
            majorPx = qMax(minorPx + 1, majorPx - 1);
        }

        const QFontMetricsF fmMinor(fMinor), fmMajor(fMajor);
        for (int deg = 0; deg < 360; deg += 30) {
            const bool cardinal = (deg % 90 == 0);
            p.setFont(cardinal ? fMajor : fMinor);
            p.setPen(cardinal ? QColor(0x5B, 0x62, 0x66) : QColor(0x8A, 0x90, 0x94));

            const double a = qDegreesToRadians(deg - 90.0);
            const QString txt = QString::number(deg);
            const QSizeF sz = (cardinal ? fmMajor : fmMinor).size(0, txt);
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
    // Наконечник узкий и вытянутый: широкий и короткий перекрывал подписи
    // на циферблате и читался как пятно, а не как стрелка.
    const double hw     = rad * 0.052; // полуширина наконечника
    const double hbLen  = rad * 0.24;  // длина основания треугольника

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

    // Толщина хвоста — долей радиуса: при фиксированных 2 px на крупном
    // циферблате широкий наконечник и волосяной хвост выглядели как две
    // разные фигуры.
    p.setPen(QPen(needleColor, qMax(2.0, rad * 0.028)));
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
        // Тоже в пикселях — по той же причине, что и подписи градусов выше.
        f.setPixelSize(qMax(12, qRound(side * 0.115)));
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0x1C, 0x1F, 0x22));

        const QString txt = QString("%1°").arg(m_angle, 0, 'f', 1);
        const QRectF  lr(0, dialD, r.width(), labelH);
        p.drawText(lr, Qt::AlignHCenter | Qt::AlignVCenter, txt);
    }
}
