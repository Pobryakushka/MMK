#include "core/grib/Meteo11ProfileBuilder.h"
#include <QMap>

Meteo11ProfileBuilder::Result Meteo11ProfileBuilder::build(const QVector<MushroomMessage> &messages)
{
    Result res;

    QMap<long, double> heightByLevel; // "Height" (HGT)
    QMap<long, double> uByLevel;      // "U_ComponentOfWind"
    QMap<long, double> vByLevel;      // "V_ComponentOfWind"

    for (const auto &m : messages) {
        if (m.parameterName == QLatin1String("Height"))
            heightByLevel[m.level] = m.parameterValue;
        else if (m.parameterName == QLatin1String("U_ComponentOfWind"))
            uByLevel[m.level] = m.parameterValue;
        else if (m.parameterName == QLatin1String("V_ComponentOfWind"))
            vByLevel[m.level] = m.parameterValue;
    }

    if (!heightByLevel.contains(0)) {
        res.error = QStringLiteral(
            "Не найдена приземная геопотенциальная высота (Height, level=0). "
            "Проверьте, что HGT для уровня surface реально присутствует в скачанном "
            "файле (grib_ls -p shortName,level,typeOfLevel ... | grep -i gh).");
        return res;
    }
    const double surfaceHeight = heightByLevel.value(0);

    for (auto it = heightByLevel.constBegin(); it != heightByLevel.constEnd(); ++it) {
        const long level = it.key();
        if (level == 0)
            continue; // это сама приземная точка, а не уровень профиля

        if (!uByLevel.contains(level) || !vByLevel.contains(level))
            continue; // нет пары U/V для этого уровня — пропускаем его

        const double heightAgl = it.value() - surfaceHeight;
        if (heightAgl <= 0.0)
            continue; // некорректная точка (высота ниже/на уровне земли)

        LevelWind lw;
        lw.heightAglM = heightAgl;
        lw.vz = uByLevel.value(level);
        lw.vm = vByLevel.value(level);
        res.profile.push_back(lw);
    }

    if (res.profile.isEmpty()) {
        res.error = QStringLiteral(
            "Не удалось построить ни одной точки профиля — нет уровней, на которых "
            "одновременно присутствуют Height, U_ComponentOfWind и V_ComponentOfWind.");
        return res;
    }

    res.ok = true;
    return res;
}
