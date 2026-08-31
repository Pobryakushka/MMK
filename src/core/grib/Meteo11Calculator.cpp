#include "core/grib/Meteo11Calculator.h"
#include <algorithm>
#include <cmath>

namespace {

// Собирает единый отсортированный массив высот (с приземной точкой под
// индексом 0) и соответствующего параметра (vz или vm) из surface +
// profile. Совпадающие по высоте точки (в пределах эпсилон) убираются
// (оставляем первую), чтобы не делить на h_{j+1}-h_j = 0 при расчётах.
void buildArrays(const SurfaceWind &surface, const QVector<LevelWind> &sortedProfile,
                  QVector<double> &heights, QVector<double> &vzValues, QVector<double> &vmValues)
{
    heights.clear();
    vzValues.clear();
    vmValues.clear();

    heights.push_back(0.0);
    vzValues.push_back(surface.vz);
    vmValues.push_back(surface.vm);

    const double epsilon = 0.5; // метра — точки ближе этого считаем дублями

    for (const auto &lvl : sortedProfile) {
        if (lvl.heightAglM <= epsilon)
            continue; // совпадает с приземной точкой либо ниже земли — пропускаем
        if (!heights.isEmpty() && (lvl.heightAglM - heights.last()) <= epsilon)
            continue; // дубль по высоте с предыдущей добавленной точкой
        heights.push_back(lvl.heightAglM);
        vzValues.push_back(lvl.vz);
        vmValues.push_back(lvl.vm);
    }
}

} // namespace

double Meteo11Calculator::trapezoidAverage(const QVector<double> &heights,
                                            const QVector<double> &values,
                                            int uptoIndex)
{
    // Формула (1): f_{0÷h_m} = sum_{j=0}^{m-1} (f_j+f_{j+1})*(h_{j+1}-h_j) / (2*h_m)
    if (uptoIndex <= 0)
        return values.value(0, 0.0); // h_m = 0 -> среднее вырождается в приземное значение

    const double hm = heights[uptoIndex];
    if (hm <= 0.0)
        return values[0];

    double sum = 0.0;
    for (int j = 0; j < uptoIndex; ++j) {
        sum += (values[j] + values[j + 1]) * (heights[j + 1] - heights[j]);
    }
    return sum / (2.0 * hm);
}

double Meteo11Calculator::interpolateAverage(double h1, double avg1,
                                              double h2, double avg2,
                                              double targetH)
{
    // Формула (3), работает и как интерполяция (h1 <= targetH <= h2),
    // и как линейная экстраполяция (targetH > h2), так как это просто
    // линейная функция по двум опорным точкам.
    if (std::abs(h2 - h1) < 1e-9)
        return avg1;
    return avg1 * (h2 - targetH) / (h2 - h1) + avg2 * (targetH - h1) / (h2 - h1);
}

QVector<Meteo11LayerResult> Meteo11Calculator::compute(const SurfaceWind &surface,
                                                         QVector<LevelWind> profile)
{
    std::sort(profile.begin(), profile.end(),
              [](const LevelWind &a, const LevelWind &b) { return a.heightAglM < b.heightAglM; });

    QVector<double> heights, vzValues, vmValues;
    buildArrays(surface, profile, heights, vzValues, vmValues);

    QVector<Meteo11LayerResult> results;
    const int n = heights.size();

    for (int targetM : meteo11StandardLayersM()) {
        const double Yl = static_cast<double>(targetM);

        Meteo11LayerResult res;
        res.layerTopM = targetM;

        // Шаг 1: ищем бракет h_m <= Yl <= h_n среди имеющихся точек
        int bracketLow = -1;
        for (int i = 0; i < n - 1; ++i) {
            if (heights[i] <= Yl && Yl <= heights[i + 1]) {
                bracketLow = i;
                break;
            }
        }

        if (bracketLow >= 0) {
            // Шаг 2 (формулы 1-3): нормальная интерполяция
            const double avgM_vz = trapezoidAverage(heights, vzValues, bracketLow);
            const double avgN_vz = trapezoidAverage(heights, vzValues, bracketLow + 1);
            const double avgM_vm = trapezoidAverage(heights, vmValues, bracketLow);
            const double avgN_vm = trapezoidAverage(heights, vmValues, bracketLow + 1);

            res.vzAvg = interpolateAverage(heights[bracketLow], avgM_vz,
                                            heights[bracketLow + 1], avgN_vz, Yl);
            res.vmAvg = interpolateAverage(heights[bracketLow], avgM_vm,
                                            heights[bracketLow + 1], avgN_vm, Yl);
            res.sufficientData = true;
        } else if (n >= 2 && Yl > heights.last()) {
            // Шаг 5-6: Yl выше самого верхнего уровня данных, но есть
            // хотя бы 2 точки — экстраполируем по последнему сегменту
            const double avgPrev_vz = trapezoidAverage(heights, vzValues, n - 2);
            const double avgLast_vz = trapezoidAverage(heights, vzValues, n - 1);
            const double avgPrev_vm = trapezoidAverage(heights, vmValues, n - 2);
            const double avgLast_vm = trapezoidAverage(heights, vmValues, n - 1);

            res.vzAvg = interpolateAverage(heights[n - 2], avgPrev_vz,
                                            heights[n - 1], avgLast_vz, Yl);
            res.vmAvg = interpolateAverage(heights[n - 2], avgPrev_vm,
                                            heights[n - 1], avgLast_vm, Yl);
            res.sufficientData = true;
        } else {
            // Шаг 4/7: нужна климатическая база данных, которой нет —
            // помечаем слой как недостаточно обеспеченный данными.
            res.sufficientData = false;
            res.vzAvg = 0.0;
            res.vmAvg = 0.0;
        }

        if (res.sufficientData) {
            // Формула (6)
            const double V = std::sqrt(res.vzAvg * res.vzAvg + res.vmAvg * res.vmAvg);
            res.speedMs = V;

            if (V < 1e-9) {
                res.directionDeg = 0.0;
            } else {
                const double asinArg = std::clamp(-res.vzAvg / V, -1.0, 1.0);
                const double asinDeg = std::asin(asinArg) * 180.0 / M_PI;
                double theta = (res.vmAvg <= 0.0) ? asinDeg : (180.0 - asinDeg);
                if (theta < 0.0)
                    theta += 360.0;
                res.directionDeg = theta;
            }
        } else {
            res.speedMs = 0.0;
            res.directionDeg = 0.0;
        }

        results.push_back(res);
    }

    return results;
}
