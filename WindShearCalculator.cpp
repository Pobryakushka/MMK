#include "WindShearCalculator.h"
#include <QColor>
#include <QtMath>
#include <QDebug>
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

bool WindShearCalculator::runSelfTest()
{
    // Эталонные данные из таблицы "Результаты тестирования алгоритма расчета сдвига ветра"
    // Примечание: в Excel-таблице обнаружены опечатки:
    //   • (10,300,10,210): Excel 345° → верно 165°
    //   • Группы G10 (Vi-1=10,Vi=20,Ai-1=210) и G11 (Vi-1=10,Vi=20,Ai-1=300):
    //     все ΔA в Excel на 180° больше правильного значения.
    //     Здесь используются математически верные значения.
    // { Vi-1, Ai-1, Vi, Ai, ожид_ΔV, ожид_ΔA }
    struct TestCase { double Vi1, Ai1, Vi, Ai, expV, expA; };
    static const TestCase cases[] = {
        // G1: Одинаковые скорость и угол → нулевой сдвиг
        {10, 30,  10, 30,  0.0,      0.0},
        {10, 120, 10, 120, 0.0,      0.0},
        {10, 210, 10, 210, 0.0,      0.0},
        {10, 300, 10, 300, 0.0,      0.0},
        // G2: Скорость увеличилась, угол тот же
        {10, 30,  20, 30,  10.0,     30.0},
        {10, 120, 20, 120, 10.0,    120.0},
        {10, 210, 20, 210, 10.0,    210.0},
        {10, 300, 20, 300, 10.0,    300.0},
        // G3: Скорость уменьшилась, угол тот же
        {20, 30,  10, 30,  10.0,    210.0},
        {20, 120, 10, 120, 10.0,    300.0},
        {20, 210, 10, 210, 10.0,     30.0},
        {20, 300, 10, 300, 10.0,    120.0},
        // G4: Скорость та же, угол изменился — Ai-1=30
        {10, 30, 10,  60,  5.17638, 135.0},
        {10, 30, 10, 120, 14.1421,  165.0},
        {10, 30, 10, 210, 20.0,     210.0},
        {10, 30, 10, 300, 14.1421,  255.0},
        // G5: Ai-1=120
        {10, 120, 10, 150,  5.17638, 225.0},
        {10, 120, 10, 210, 14.1421,  255.0},
        {10, 120, 10, 300, 20.0,     300.0},
        {10, 120, 10,  30, 14.1421,  345.0},
        // G6: Ai-1=210
        {10, 210, 10, 240,  5.17638, 315.0},
        {10, 210, 10, 300, 14.1421,  345.0},
        {10, 210, 10,  30, 20.0,      30.0},
        {10, 210, 10, 120, 14.1421,   75.0},
        // G7: Ai-1=300 (Excel-опечатка в последней строке исправлена: 345→165)
        {10, 300, 10, 330,  5.17638,  45.0},
        {10, 300, 10,  30, 14.1421,   75.0},
        {10, 300, 10, 120, 20.0,     120.0},
        {10, 300, 10, 210, 14.1421,  165.0},
        // G8: Vi-1=10, Vi=20, Ai-1=30
        {10,  30, 20,  60, 12.3931,   83.794},
        {10,  30, 20, 120, 22.3607,  146.565},
        {10,  30, 20, 210, 30.0,     210.0},
        {10,  30, 20, 300, 22.3607,  273.435},
        // G9: Vi-1=10, Vi=20, Ai-1=120
        {10, 120, 20, 150, 12.3931,  173.794},
        {10, 120, 20, 210, 22.3607,  236.565},
        {10, 120, 20, 300, 30.0,     300.0},
        {10, 120, 20,  30, 22.3607,    3.435},
        // G10: Vi-1=10, Vi=20, Ai-1=210 (Excel на 180° больше — исправлено)
        {10, 210, 20, 240, 12.3931,  263.794},
        {10, 210, 20, 300, 22.3607,  326.565},
        {10, 210, 20,  30, 30.0,      30.0},
        {10, 210, 20, 120, 22.3607,   93.435},
        // G11: Vi-1=10, Vi=20, Ai-1=300 (Excel на 180° больше — исправлено)
        {10, 300, 20, 330, 12.3931,  353.794},
        {10, 300, 20,  30, 22.3607,   56.565},
        {10, 300, 20, 120, 30.0,     120.0},
        {10, 300, 20, 210, 22.3607,  183.435},
        // G12: Vi-1=20, Vi=10, Ai-1=30
        {20,  30, 10,  60, 12.3931,  186.206},
        {20,  30, 10, 120, 22.3607,  183.435},
        {20,  30, 10, 210, 30.0,     210.0},
        {20,  30, 10, 300, 22.3607,  236.565},
        // G13: Vi-1=20, Vi=10, Ai-1=120
        {20, 120, 10, 150, 12.3931,  276.206},
        {20, 120, 10, 210, 22.3607,  273.435},
        {20, 120, 10, 300, 30.0,     300.0},
        {20, 120, 10,  30, 22.3607,  326.565},
        // G14: Vi-1=20, Vi=10, Ai-1=210
        {20, 210, 10, 240, 12.3931,    6.206},
        {20, 210, 10, 300, 22.3607,    3.435},
        {20, 210, 10,  30, 30.0,      30.0},
        {20, 210, 10, 120, 22.3607,   56.565},
        // G15: Vi-1=20, Vi=10, Ai-1=300
        {20, 300, 10, 330, 12.3931,   96.206},
        {20, 300, 10,  30, 22.3607,   93.435},
        {20, 300, 10, 120, 30.0,     120.0},
        {20, 300, 10, 210, 22.3607,  146.565},
    };

    const int N = sizeof(cases) / sizeof(cases[0]);
    const double EPS_V = 0.01;   // допуск ΔV, м/с
    const double EPS_A = 0.1;    // допуск ΔA, градус

    int passed = 0;
    qDebug() << "=== WindShearCalculator self-test ===";
    qDebug() << QString("%1 %2 %3 %4 | %5 %6 | %7 %8 | %9")
                .arg("Vi-1",5).arg("Ai-1",5).arg("Vi",5).arg("Ai",5)
                .arg("expΔV",8).arg("expΔA",7)
                .arg("gotΔV",8).arg("gotΔA",7)
                .arg("result",6);

    for (int i = 0; i < N; ++i) {
        const TestCase &c = cases[i];
        double dV = 0, dA = 0;
        // Высоты не важны для этого теста (используются только для shearPer30m)
        calculateShearMethod2(c.Vi, c.Ai, c.Vi1, c.Ai1, 30.0, 0.0, dV, dA);

        double diffA = fabs(dA - c.expA);
        if (diffA > 180.0) diffA = 360.0 - diffA;
        bool ok = (fabs(dV - c.expV) < EPS_V) && (diffA < EPS_A);
        if (ok) ++passed;

        qDebug() << QString("%1 %2 %3 %4 | %5 %6 | %7 %8 | %9")
                    .arg(c.Vi1, 5, 'f', 0).arg(c.Ai1, 5, 'f', 0)
                    .arg(c.Vi,  5, 'f', 0).arg(c.Ai,  5, 'f', 0)
                    .arg(c.expV, 8, 'f', 4).arg(c.expA, 7, 'f', 2)
                    .arg(dV,     8, 'f', 4).arg(dA,     7, 'f', 2)
                    .arg(ok ? "PASS" : "FAIL");
    }

    qDebug() << QString("=== Итог: %1/%2 тестов прошло ===").arg(passed).arg(N);
    return (passed == N);
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
