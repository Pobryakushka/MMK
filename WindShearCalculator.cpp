#include "WindShearCalculator.h"
#include <QColor>
#include <QtMath>
#include <cmath>

WindShearCalculator::WindShearCalculator()
{
}

QVector<WindShearData> WindShearCalculator::calculateShear(const QVector<WindProfileData> &profile)
{
    QVector<WindShearData> shearData;

    if (profile.size() < 2) {
        return shearData; // Нужно минимум 2 точки для расчета сдвига
    }

    // Вычисляем сдвиг между каждой парой соседних высот
    for (int i = 1; i < profile.size(); ++i) {
        const WindProfileData &upper = profile[i];      // Верхний уровень H_i
        const WindProfileData &lower = profile[i-1];    // Нижний уровень H_{i-1}

        // Пропускаем недействительные данные
        if (!upper.isValid || !lower.isValid) {
            continue;
        }

        WindShearData shear;

        // Средняя высота между уровнями
        shear.height = (upper.height + lower.height) / 2.0;

        double deltaV = 0.0;
        double deltaAlpha = 0.0;

        // Используем второй способ (через составляющие) — даёт верное направление ΔA
        calculateShearMethod2(upper.windSpeed, upper.windDirection,
                             lower.windSpeed, lower.windDirection,
                             upper.height, lower.height,
                             deltaV, deltaAlpha);

        shear.shearMagnitude = deltaV;
        shear.shearDirection = deltaAlpha;

        // Приведение к стандартному интервалу 30м
        double deltaH = upper.height - lower.height;
        if (deltaH > 0) {
            shear.shearPer30m = deltaV * (30.0 / deltaH);
        } else {
            shear.shearPer30m = 0.0;
        }

        // Определение уровня критичности
        shear.severityLevel = getSeverityLevel(shear.shearPer30m);

        shearData.append(shear);
    }

    return shearData;
}

QVector<WindShearData> WindShearCalculator::calculateShear(const QVector<MeasuredWindData> &measuredData)
{
    QVector<WindShearData> shearData;

    if (measuredData.size() < 2) {
        return shearData;
    }

    // Вычисляем сдвиг между каждой парой соседних высот
    for (int i = 1; i < measuredData.size(); ++i) {
        const MeasuredWindData &upper = measuredData[i];
        const MeasuredWindData &lower = measuredData[i-1];

        // Пропускаем недействительные данные (reliability == 1)
        if (upper.reliability == 1 || lower.reliability == 1) {
            continue;
        }

        WindShearData shear;
        shear.height = (upper.height + lower.height) / 2.0;

        double deltaV = 0.0;
        double deltaAlpha = 0.0;

        // Используем второй способ (через составляющие) — даёт верное направление ΔA
        calculateShearMethod2(upper.windSpeed, upper.windDirection,
                             lower.windSpeed, lower.windDirection,
                             upper.height, lower.height,
                             deltaV, deltaAlpha);

        shear.shearMagnitude = deltaV;
        shear.shearDirection = deltaAlpha;

        double deltaH = upper.height - lower.height;
        if (deltaH > 0) {
            shear.shearPer30m = deltaV * (30.0 / deltaH);
        } else {
            shear.shearPer30m = 0.0;
        }

        shear.severityLevel = getSeverityLevel(shear.shearPer30m);

        shearData.append(shear);
    }

    return shearData;
}

void WindShearCalculator::calculateShearMethod1(double V_i, double alpha_i,
                                               double V_i_1, double alpha_i_1,
                                               double H_i, double H_i_1,
                                               double &deltaV, double &deltaAlpha)
{
    // Конвертируем углы в радианы для вычислений
    double alpha_i_rad = degToRad(alpha_i);
    double alpha_i_1_rad = degToRad(alpha_i_1);

    // Формула: ΔV = √(V_i² + V_{i-1}² - 2·V_i·V_{i-1}·cos(α_i - α_{i-1}))
    double cos_diff = cos(alpha_i_rad - alpha_i_1_rad);
    deltaV = sqrt(V_i * V_i + V_i_1 * V_i_1 - 2.0 * V_i * V_i_1 * cos_diff);

    // ИЗМЕНЕНИЕ НАПРАВЛЕНИЯ - это просто разница углов
    deltaAlpha = alpha_i - alpha_i_1;

    // Нормализуем в диапазон [-180, 180] для удобства
    while (deltaAlpha > 180.0) {
        deltaAlpha -= 360.0;
    }
    while (deltaAlpha < -180.0) {
        deltaAlpha += 360.0;
    }
}

void WindShearCalculator::calculateShearMethod2(double V_i, double alpha_i,
                                               double V_i_1, double alpha_i_1,
                                               double H_i, double H_i_1,
                                               double &deltaV, double &deltaAlpha)
{
    // Конвертируем углы в радианы
    double alpha_i_rad = degToRad(alpha_i);
    double alpha_i_1_rad = degToRad(alpha_i_1);

    // Вычисляем зональную (V_Z) и меридиональную (V_M) составляющие
    // V_Z = -V·sin(α), V_M = -V·cos(α)
    double V_Zi = -V_i * sin(alpha_i_rad);
    double V_Mi = -V_i * cos(alpha_i_rad);

    double V_Zi_1 = -V_i_1 * sin(alpha_i_1_rad);
    double V_Mi_1 = -V_i_1 * cos(alpha_i_1_rad);

    // Формула: ΔV = √((V_Zi - V_Z{i-1})² + (V_Mi - V_M{i-1})²)
    double dV_Z = V_Zi - V_Zi_1;
    double dV_M = V_Mi - V_Mi_1;

    deltaV = sqrt(dV_Z * dV_Z + dV_M * dV_M);

    // Вычисление изменения направления
    if (fabs(deltaV) < 1e-6) {
        // Если ΔV = 0, то Δα = 0°
        deltaAlpha = 0.0;
    } else {
        // Формула: Δα = arcsin(-(V_Zi - V_Z{i-1})/ΔV)
        double sin_value = -dV_Z / deltaV;
        // Ограничиваем значение для arcsin в диапазоне [-1, 1]
        sin_value = std::max(-1.0, std::min(1.0, sin_value));

        deltaAlpha = radToDeg(asin(sin_value));

        // Корректировка согласно условиям
        if (V_Mi - V_Mi_1 > 0) {
            // Если V_Mi - V_M{i-1} > 0, то Δα = 180° - arcsin(...)
            deltaAlpha = 180.0 - deltaAlpha;
        }

        // Нормализация
        if (deltaAlpha < 0.0) {
            deltaAlpha = deltaAlpha + 360.0;
        }
    }
}

int WindShearCalculator::getSeverityLevel(double shearPer30m)
{
    double absShear = fabs(shearPer30m);

    if (absShear < 2.0) {
        return 0; // Слабый
    } else if (absShear < 4.0) {
        return 1; // Умеренный
    } else if (absShear < 6.0) {
        return 2; // Сильный
    } else {
        return 3; // Очень сильный
    }
}

QColor WindShearCalculator::getSeverityColor(int severityLevel)
{
    switch (severityLevel) {
        case 0: // Слабый
            return QColor(144, 238, 144); // Светло-зеленый
        case 1: // Умеренный
            return QColor(255, 255, 0);   // Желтый
        case 2: // Сильный
            return QColor(255, 165, 0);   // Оранжевый
        case 3: // Очень сильный
            return QColor(255, 0, 0);     // Красный
        default:
            return QColor(255, 255, 255); // Белый
    }
}

QString WindShearCalculator::getSeverityText(int severityLevel)
{
    switch (severityLevel) {
        case 0:
            return QString::fromUtf8("Слабый");
        case 1:
            return QString::fromUtf8("Умеренный");
        case 2:
            return QString::fromUtf8("Сильный");
        case 3:
            return QString::fromUtf8("Очень сильный");
        default:
            return QString::fromUtf8("Неизвестно");
    }
}

double WindShearCalculator::degToRad(double degrees)
{
    return degrees * M_PI / 180.0;
}

double WindShearCalculator::radToDeg(double radians)
{
    return radians * 180.0 / M_PI;
}

double WindShearCalculator::normalizeAngle(double angle)
{
    while (angle < 0.0) {
        angle += 360.0;
    }
    while (angle >= 360.0) {
        angle -= 360.0;
    }
    return angle;
}

int WindShearCalculator::sign(double y)
{
    return (y >= 0.0) ? 1 : -1;
}
