#include "core/grib/MushroomResultParser.h"
#include <QRegularExpression>
#include <QSet>

QVector<MushroomMessage> MushroomResultParser::parse(const QString &rawOutput)
{
    QVector<MushroomMessage> result;

    // Пример строки вывода (значение в скобках — необязательное,
    // это "сырое" ближайшее значение сетки до интерполяции):
    //
    // <file> <centre> Latitude: 55 (55.75) Longitude: 37 (37.5)
    // Temperature: 291.82 (292.157) Distance: -1 Level: 0
    // validityDate: 20260803 validityTime: 0 dataDate: 20260803 dataTime: 0
    //
    // Программа печатает каждую точку дважды: один раз внутри
    // ProcessMessage() (с "сырым" значением в скобках), ещё раз в
    // финальном списке main() (без скобок) — обе формы покрываются
    // одним и тем же регулярным выражением за счёт необязательной
    // группы "(?:\s*\([^)]*\))?".
    static const QRegularExpression lineRe(
        R"(^(\S+)\s+(.+?)\s+Latitude:\s*([-\d.]+)(?:\s*\([^)]*\))?\s+)"
        R"(Longitude:\s*([-\d.]+)(?:\s*\([^)]*\))?\s+)"
        R"(([A-Za-z_]+):\s*([-\d.]+)(?:\s*\([^)]*\))?\s+)"
        R"(Distance:\s*([-\d.]+)\s+Level:\s*([-\d.]+)\s+)"
        R"(validityDate:\s*(\d+)\s+validityTime:\s*(\d+)\s+)"
        R"(dataDate:\s*(\d+)\s+dataTime:\s*(\d+))"
    );

    // Ключ для отсева дублей (см. комментарий выше про двойную печать)
    QSet<QString> seenKeys;

    const auto lines = rawOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        const auto match = lineRe.match(line);
        if (!match.hasMatch())
            continue; // строки диагностики ("всего сообщений=...") сюда не попадают

        MushroomMessage msg;
        msg.fileName       = match.captured(1);
        msg.centre         = match.captured(2).trimmed();
        msg.latitude       = match.captured(3).toDouble();
        msg.longitude      = match.captured(4).toDouble();
        msg.parameterName  = match.captured(5);
        msg.parameterValue = match.captured(6).toDouble();
        msg.distance       = match.captured(7).toDouble();
        msg.level          = match.captured(8).toLong();
        msg.validityDate   = match.captured(9).toLong();
        msg.validityTime   = match.captured(10).toLong();
        msg.dataDate       = match.captured(11).toLong();
        msg.dataTime       = match.captured(12).toLong();

        const QString key = QStringLiteral("%1|%2|%3|%4|%5")
            .arg(msg.fileName, msg.parameterName)
            .arg(msg.level)
            .arg(msg.validityDate)
            .arg(msg.validityTime);

        if (seenKeys.contains(key))
            continue;
        seenKeys.insert(key);

        result.push_back(msg);
    }

    return result;
}
