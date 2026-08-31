#pragma once
#include <QWidget>

/**
 * Виджет-компас положения РПВ.
 * Рисует циферблат с делениями каждые 10° (подписи каждые 30°),
 * вращающуюся стрелку и цифровое значение угла под циферблатом.
 * 0° — вверх, угол растёт по часовой стрелке.
 * Публичный API не менялся — setAngle(double) вызывается из MainWindow
 * точно так же, как и раньше.
 */
class RpvIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double angle READ angle WRITE setAngle)

public:
    explicit RpvIndicator(QWidget *parent = nullptr);

    double angle() const { return m_angle; }
    void   setAngle(double degrees);

    QSize sizeHint()        const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_angle = 0.0;
};
