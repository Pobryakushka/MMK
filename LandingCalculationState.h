#ifndef LANDINGCALCULATIONSTATE_H
#define LANDINGCALCULATIONSTATE_H

#include <QString>

struct LandingCalculationState
{
    // Исходные данные
    QString latCUP; // Широта ТПП
    QString lonCUP; // Долгота ТПП
    int comboLatCUP; // 0=N, 1=S
    int comboLonCUP; // 0=E, 1=W
    QString heightUM; // Высота ТПП

    QString timeVP; // tпад
    QString lossPV; // hнап
    QString speedDescent; // Vсн
    QString staffTime; // A0
    QString hvhod; // Hдес
    QString trueBearing; // Угол площадки

    bool manualInput; // Чекбокс ручного ввода ветра
    QString distance; // Uср
    QString direction; // Delta ср

    // Результаты
    QString latTNV;
    QString lonTNV;
    int comboLatTNV;
    int comboLonTNV;
    QString heightTNV;
    QString distanceTNV;
    QString angleTNV;
    QString distanceTPP;
    QString angleTPP;

    bool hasData; // true если диалог открывался

    static LandingCalculationState &instance()
    {
        static LandingCalculationState s;
        return s;
    }

private:
    LandingCalculationState()
        : comboLatCUP(0), comboLonCUP(0)
        , manualInput(false)
        , comboLatTNV(0), comboLonTNV(0)
        , hasData(false)
    {}
};

#endif // LANDINGCALCULATIONSTATE_H
