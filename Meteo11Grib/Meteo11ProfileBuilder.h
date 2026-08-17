#pragma once
#include <QVector>
#include <QString>
#include "MushroomMessage.h"
#include "Meteo11Types.h"

// Строит профиль LevelWind (высота над землёй + компоненты ветра) из
// уже распарсенных результатов Mushroom (H_Height, U/V_ComponentOfWind
// на одинаковых уровнях давления). Приземный ветер сюда не входит —
// он вводится отдельно (см. SurfaceWind), так как по методике должен
// приходить с отдельного датчика, а не из GRIB.
//
// ВАЖНОЕ ДОПУЩЕНИЕ: рассчитывает на то, что среди сообщений есть
// параметр "Height" с level=0 — это приземная геопотенциальная высота
// (orography), которую Mushroom печатает под тем же условным "level=0",
// что и приземную температуру (см. обсуждение неоднозначности поля
// Level в выводе Mushroom). Если такой записи нет — build() вернёт
// ok=false с понятным текстом ошибки, а не тихо посчитает неправильно.
class Meteo11ProfileBuilder {
public:
    struct Result {
        bool ok = false;
        QString error;
        QVector<LevelWind> profile;
    };

    static Result build(const QVector<MushroomMessage> &messages);
};
