#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

// ─────────────────────────────────────────────────────────────────────────
// ClickableLabel — обычный QLabel с сигналом clicked(). Та же реализация,
// что и у ClickableFrame (прямое переопределение mousePressEvent/
// mouseReleaseEvent, без installEventFilter) — используется для плашек
// статуса датчиков (lblGnssStatus/lblAmsStatus/lblBinsStatus/lblIwsStatus),
// клик по которым открывает шторку с состоянием и управлением датчиком.
// ─────────────────────────────────────────────────────────────────────────
class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_pressed = false;
};

#endif // CLICKABLELABEL_H
