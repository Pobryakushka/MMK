#pragma once
#include <QWidget>

/**
 * Компактный виджет-индикатор положения РПВ.
 * Рисует кружок с вращающейся стрелкой и цифровым значением угла.
 * 0° — вверх, угол растёт по часовой стрелке.
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
