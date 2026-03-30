#include "MeasurementExporter.h"
#include "amsprotocol.h"        // WindProfileData, MeasuredWindData
#include "WindShearCalculator.h" // WindShearData

#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ─────────────────────────────────────────────────────────────────────────────
// Публичный интерфейс
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::generate(const MeasurementSnapshot &snap,
                                      const ExportOptions       &opts,
                                      QString                   &errorMsg)
{
    errorMsg.clear();

    // Базовая проверка: есть ли хоть что-то для экспорта?
    if (snap.recordId <= 0) {
        errorMsg = "Нет загруженных данных для экспорта.\n"
                   "Выберите дату и время с доступными измерениями.";
        return {};
    }

    bool hasAnyData = snap.coordinatesValid
                      || snap.surfaceMeteoValid
                      || !snap.avgWind.isEmpty()
                      || !snap.actualWind.isEmpty()
                      || !snap.measuredWind.isEmpty()
                      || !snap.windShear.isEmpty()
                      || snap.meteo11Updated.valid
                      || snap.meteo11Approximate.valid;

    if (!hasAnyData) {
        errorMsg = QString("Запись ID %1 от %2 не содержит данных ни в одном разделе.")
                       .arg(snap.recordId)
                       .arg(snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
        return {};
    }

    switch (opts.format) {
    case ExportOptions::TXT:  return generateTxt (snap, opts);
    case ExportOptions::CSV:  return generateCsv (snap, opts);
    case ExportOptions::JSON: return generateJson(snap, opts);
    }
    return {};

}

QString MeasurementExporter::suggestedFileName(const MeasurementSnapshot &snap,
                                               ExportOptions::Format       format)
{
    QString dt = snap.measurementTime.isValid()
    ? snap.measurementTime.toString("yyyyMMdd_HHmmss")
    : "unknown";
    return QString("measurement_%1_ID%2.%3")
        .arg(dt)
        .arg(snap.recordId)
        .arg(formatExt(format));
}

// ─────────────────────────────────────────────────────────────────────────────
// TXT — читаемый отчёт
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::generateTxt(const MeasurementSnapshot &snap,
                                         const ExportOptions       &opts)
{
    const QString sep(80, '=');
    const QString dash(80, '-');
    QStringList out;

    auto h1 = [&](const QString &title) {
        out << sep << QString("  %1").arg(title) << sep;
    };
    auto h2 = [&](const QString &title) {
        out << "" << dash << QString("  %1").arg(title) << dash;
    };
    auto row = [&](const QString &label, const QString &value) {
        out << QString("  %-45s %1").arg(value).arg(label, -45);
    };

    // ── Шапка ──────────────────────────────────────────────────────────────
    h1("РЕЗУЛЬТАТЫ ИЗМЕРЕНИЙ — МЕТЕОКОМПЛЕКС");
    out << QString("  Запись ID : %1").arg(snap.recordId);
    out << QString("  Дата/время: %1").arg(
        snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
    if (!snap.stationNumber.isEmpty())
        out << QString("  Ном. ст.  : %1").arg(snap.stationNumber);
    out << "";

    // ── Координаты ─────────────────────────────────────────────────────────
    if (opts.includeCoordinates) {
        h2("КООРДИНАТЫ СТАНЦИИ");
        if (snap.coordinatesValid) {
            row("Широта",  latToString(snap.latitude));
            row("Долгота", lonToString(snap.longitude));
            row("Высота над УМ", QString("%1 м").arg(snap.altitude, 0, 'f', 1));
        } else {
            out << "  — нет данных —";
        }
    }

    // ── Наземные метеоусловия ───────────────────────────────────────────────
    if (opts.includeSurfaceMeteo) {
        h2("НАЗЕМНЫЕ МЕТЕОРОЛОГИЧЕСКИЕ УСЛОВИЯ");
        if (snap.surfaceMeteoValid) {
            row("Давление (P), мм рт.ст.",
                QString::number(snap.pressureHpa, 'f', 1));
            row("Температура (T), °C",
                QString::number(snap.temperatureC, 'f', 1));
            row("Относит. влажность (r), %",
                QString::number(snap.humidityPct, 'f', 1));
            row("Направление ветра (A), °",
                QString::number(snap.surfaceWindDir, 'f', 0));
            row("Скорость ветра (V), м/с",
                QString::number(snap.surfaceWindSpeed, 'f', 1));
        } else {
            out << "  — нет данных —";
        }
    }

    // ── Средний ветер ───────────────────────────────────────────────────────
    if (opts.includeAvgWind) {
        h2("СРЕДНИЙ ВЕТЕР");
        if (!snap.avgWind.isEmpty()) {
            out << QString("  %-12s  %-12s  %s").arg("Высота, м", "Скорость, м/с", "Направление, °");
            out << QString("  %1  %2  %3").arg(QString(12, '-'), QString(12, '-'), QString(14, '-'));
            for (const WindProfileData &p : snap.avgWind) {
                out << QString("  %-12s  %-12s  %s")
                .arg(QString::number(p.height,       'f', 0), 12)
                    .arg(QString::number(p.windSpeed,    'f', 2), 12)
                    .arg(QString::number(p.windDirection),        14);
            }
        } else {
            out << "  — нет данных —";
        }
    }

    // ── Действительный ветер ────────────────────────────────────────────────
    if (opts.includeActualWind) {
        h2("ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР");
        if (!snap.actualWind.isEmpty()) {
            out << QString("  %-12s  %-12s  %s").arg("Высота, м", "Скорость, м/с", "Направление, °");
            out << QString("  %1  %2  %3").arg(QString(12, '-'), QString(12, '-'), QString(14, '-'));
            for (const WindProfileData &p : snap.actualWind) {
                out << QString("  %-12s  %-12s  %s")
                .arg(QString::number(p.height,       'f', 0), 12)
                    .arg(QString::number(p.windSpeed,    'f', 2), 12)
                    .arg(QString::number(p.windDirection),        14);
            }
        } else {
            out << "  — нет данных —";
        }
    }

    // ── Измеренный ветер ────────────────────────────────────────────────────
    if (opts.includeMeasuredWind) {
        h2("ИЗМЕРЕННЫЙ ВЕТЕР");
        if (!snap.measuredWind.isEmpty()) {
            out << QString("  %-12s  %-12s  %s").arg("Высота, м", "Скорость, м/с", "Направление, °");
            out << QString("  %1  %2  %3").arg(QString(12, '-'), QString(12, '-'), QString(14, '-'));
            for (const MeasuredWindData &p : snap.measuredWind) {
                out << QString("  %-12s  %-12s  %s")
                .arg(QString::number(p.height,       'f', 0), 12)
                    .arg(QString::number(p.windSpeed,    'f', 2), 12)
                    .arg(QString::number(p.windDirection),        14);
            }
        } else {
            out << "  — нет данных —";
        }
    }

    // ── Сдвиг ветра ─────────────────────────────────────────────────────────
    if (opts.includeWindShear) {
        h2("СДВИГ ВЕТРА");
        if (!snap.windShear.isEmpty()) {
            out << QString("  %-12s  %-18s  %-22s  %s")
            .arg("Высота, м", "Скорость, м/с/30м", "Изм. направления, °", "Уровень");
            out << QString("  %1  %2  %3  %4")
                       .arg(QString(12, '-'), QString(18, '-'),
                            QString(22, '-'), QString(10, '-'));
            for (const WindShearData &s : snap.windShear) {
                out << QString("  %-12s  %-18s  %-22s  %s")
                .arg(QString::number(static_cast<int>(s.height)),    12)
                    .arg(QString::number(s.shearPer30m,    'f', 2),     18)
                    .arg(QString::number(s.shearDirection, 'f', 1),     22)
                    .arg(WindShearCalculator::getSeverityText(s.severityLevel));
            }
        } else {
            out << "  — нет данных —";
        }
    }

    // ── Метео-11 ────────────────────────────────────────────────────────────
    auto writeMeteo11 = [&](const QString &title,
                            const MeasurementSnapshot::Meteo11Export &m11,
                            bool include) {
        if (!include) return;
        h2(title);
        if (!m11.valid) {
            out << "  — нет данных —";
            return;
        }
        out << QString("  %1").arg(m11.bulletinString);
        out << "";
        row("Номер ст. (NNNNN)",    m11.stationNumber);
        row("Дата/время (ДДЧЧМ)",
            QString("%1%2%3")
                .arg(m11.day, 2, 10, QChar('0'))
                .arg(m11.hour, 2, 10, QChar('0'))
                .arg(m11.tenMinutes, 1, 10, QChar('0')));
        row("Высота станции (BBBB)",
            QString("%1").arg(m11.stationAltitude, 4, 10, QChar('0')));
        row("Откл. давления (БББ)",
            QString("%1").arg(m11.pressureDev, 3, 10, QChar('0')));
        row("Откл. вирт. темп. (ТТ)",
            QString("%1").arg(m11.tempVirtDev, 2, 10, QChar('0')));
        row("Выс. темп. зонд., км",  QString::number(m11.reachedTempKm));
        row("Выс. ветр. зонд., км",  QString::number(m11.reachedWindKm));
    };

    writeMeteo11("БЮЛЛЕТЕНЬ МЕТЕО-11 (УТОЧНЁННЫЙ)",
                 snap.meteo11Updated,      opts.includeMeteo11Updated);
    writeMeteo11("БЮЛЛЕТЕНЬ МЕТЕО-11 (ПРИБЛИЖЁННЫЙ)",
                 snap.meteo11Approximate,  opts.includeMeteo11Approx);
    writeMeteo11("БЮЛЛЕТЕНЬ МЕТЕО-11 (ОТ МЕТЕОСТАНЦИИ)",
                 snap.meteo11FromStation,  opts.includeMeteo11Station);

    // ── Подвал ──────────────────────────────────────────────────────────────
    out << "" << sep
        << QString("  Сформировано: %1  |  Метеокомплекс АМС")
               .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"))
        << sep;

    return out.join('\n');

}

// ─────────────────────────────────────────────────────────────────────────────
// CSV
// ─────────────────────────────────────────────────────────────────────────────

static QString csvRow(QChar sep, const QStringList &cols)
{
    QStringList quoted;
    for (const QString &c : cols) {
        QString s = c;
        s.replace('"', """");
        if (s.contains(sep) || s.contains('\n') || s.contains('"'))
            s = """ + s + """;
        quoted << s;
    }
    return quoted.join(sep);
}

QString MeasurementExporter::generateCsv(const MeasurementSnapshot &snap,
                                         const ExportOptions       &opts)
{
    const QChar S = opts.csvSeparator;
    QStringList out;

    // Служебная секция — метаданные записи
    out << csvRow(S, {"[ЗАПИСЬ]", ""});
    out << csvRow(S, {"record_id", QString::number(snap.recordId)});
    out << csvRow(S, {"datetime",  snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss")});
    if (!snap.stationNumber.isEmpty())
        out << csvRow(S, {"station_number", snap.stationNumber});
    out << "";

    // Координаты
    if (opts.includeCoordinates) {
        out << csvRow(S, {"[КООРДИНАТЫ]", ""});
        if (snap.coordinatesValid) {
            out << csvRow(S, {"latitude",  latToString(snap.latitude)});
            out << csvRow(S, {"longitude", lonToString(snap.longitude)});
            out << csvRow(S, {"altitude_m", QString::number(snap.altitude, 'f', 1)});
        } else {
            out << csvRow(S, {"", "нет данных"});
        }
        out << "";
    }

    // Наземные метеоусловия
    if (opts.includeSurfaceMeteo) {
        out << csvRow(S, {"[НАЗЕМНЫЕ МЕТ. УСЛОВИЯ]", ""});
        if (snap.surfaceMeteoValid) {
            out << csvRow(S, {"pressure_mmhg",   QString::number(snap.pressureHpa,     'f', 1)});
            out << csvRow(S, {"temperature_c",   QString::number(snap.temperatureC,    'f', 1)});
            out << csvRow(S, {"humidity_pct",    QString::number(snap.humidityPct,     'f', 1)});
            out << csvRow(S, {"wind_dir_deg",    QString::number(snap.surfaceWindDir,  'f', 0)});
            out << csvRow(S, {"wind_speed_ms",   QString::number(snap.surfaceWindSpeed,'f', 1)});
        } else {
            out << csvRow(S, {"", "нет данных"});
        }
        out << "";
    }

    // Средний ветер
    if (opts.includeAvgWind) {
        out << csvRow(S, {"[СРЕДНИЙ ВЕТЕР]", "", ""});
        out << csvRow(S, {"height_m", "speed_ms", "direction_deg"});
        if (!snap.avgWind.isEmpty()) {
            for (const WindProfileData &p : snap.avgWind)
                out << csvRow(S, {QString::number(p.height, 'f', 0),
                                  QString::number(p.windSpeed, 'f', 2),
                                  QString::number(p.windDirection)});
        } else {
            out << csvRow(S, {"нет данных", "", ""});
        }
        out << "";
    }

    // Действительный ветер
    if (opts.includeActualWind) {
        out << csvRow(S, {"[ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР]", "", ""});
        out << csvRow(S, {"height_m", "speed_ms", "direction_deg"});
        if (!snap.actualWind.isEmpty()) {
            for (const WindProfileData &p : snap.actualWind)
                out << csvRow(S, {QString::number(p.height, 'f', 0),
                                  QString::number(p.windSpeed, 'f', 2),
                                  QString::number(p.windDirection)});
        } else {
            out << csvRow(S, {"нет данных", "", ""});
        }
        out << "";
    }

    // Измеренный ветер
    if (opts.includeMeasuredWind) {
        out << csvRow(S, {"[ИЗМЕРЕННЫЙ ВЕТЕР]", "", ""});
        out << csvRow(S, {"height_m", "speed_ms", "direction_deg"});
        if (!snap.measuredWind.isEmpty()) {
            for (const MeasuredWindData &p : snap.measuredWind)
                out << csvRow(S, {QString::number(p.height, 'f', 0),
                                  QString::number(p.windSpeed, 'f', 2),
                                  QString::number(p.windDirection)});
        } else {
            out << csvRow(S, {"нет данных", "", ""});
        }
        out << "";
    }

    // Сдвиг ветра
    if (opts.includeWindShear) {
        out << csvRow(S, {"[СДВИГ ВЕТРА]", "", "", ""});
        out << csvRow(S, {"height_m", "shear_ms_per_30m",
                          "dir_change_deg", "severity"});
        if (!snap.windShear.isEmpty()) {
            for (const WindShearData &s : snap.windShear)
                out << csvRow(S, {
                                     QString::number(static_cast<int>(s.height)),
                                     QString::number(s.shearPer30m,    'f', 2),
                                     QString::number(s.shearDirection, 'f', 1),
                                     WindShearCalculator::getSeverityText(s.severityLevel)
                                 });
        } else {
            out << csvRow(S, {"нет данных", "", "", ""});
        }
        out << "";
    }

    // Метео-11 (уточнённый)
    if (opts.includeMeteo11Updated) {
        out << csvRow(S, {"[МЕТЕО-11 УТОЧНЁННЫЙ]", ""});
        if (snap.meteo11Updated.valid) {
            out << csvRow(S, {"bulletin", snap.meteo11Updated.bulletinString});
            out << csvRow(S, {"NNNNN",    snap.meteo11Updated.stationNumber});
            out << csvRow(S, {"BBBB",     QString::number(snap.meteo11Updated.stationAltitude)});
            out << csvRow(S, {"BBB",      QString::number(snap.meteo11Updated.pressureDev)});
            out << csvRow(S, {"T0T0",     QString::number(snap.meteo11Updated.tempVirtDev)});
            out << csvRow(S, {"reached_temp_km",
                              QString::number(snap.meteo11Updated.reachedTempKm)});
            out << csvRow(S, {"reached_wind_km",
                              QString::number(snap.meteo11Updated.reachedWindKm)});
        } else {
            out << csvRow(S, {"", "нет данных"});
        }
        out << "";
    }

    // Метео-11 (приближённый)
    if (opts.includeMeteo11Approx) {
        out << csvRow(S, {"[МЕТЕО-11 ПРИБЛИЖЁННЫЙ]", ""});
        if (snap.meteo11Approximate.valid) {
            out << csvRow(S, {"bulletin", snap.meteo11Approximate.bulletinString});
        } else {
            out << csvRow(S, {"", "нет данных"});
        }
        out << "";
    }

    out << csvRow(S, {"[СФОРМИРОВАНО]",
                      QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")});

    return out.join('\n');

}

// ─────────────────────────────────────────────────────────────────────────────
// JSON
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::generateJson(const MeasurementSnapshot &snap,
                                          const ExportOptions       &opts)
{
    QJsonObject root;

    // Метаданные
    QJsonObject meta;
    meta["record_id"]  = snap.recordId;
    meta["datetime"]   = snap.measurementTime.toString(Qt::ISODate);
    meta["station_number"] = snap.stationNumber;
    meta["exported_at"]    = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["meta"] = meta;

    // Координаты
    if (opts.includeCoordinates) {
        QJsonObject coords;
        coords["valid"]       = snap.coordinatesValid;
        if (snap.coordinatesValid) {
            coords["latitude"]  = snap.latitude;
            coords["longitude"] = snap.longitude;
            coords["altitude_m"]= snap.altitude;
        }
        root["coordinates"] = coords;
    }

    // Наземные метеоусловия
    if (opts.includeSurfaceMeteo) {
        QJsonObject sm;
        sm["valid"] = snap.surfaceMeteoValid;
        if (snap.surfaceMeteoValid) {
            sm["pressure_hpa"]     = snap.pressureHpa;
            sm["temperature_c"]    = snap.temperatureC;
            sm["humidity_pct"]     = snap.humidityPct;
            sm["wind_dir_deg"]     = snap.surfaceWindDir;
            sm["wind_speed_ms"]    = snap.surfaceWindSpeed;
        }
        root["surface_meteo"] = sm;
    }

    // Профиль ветра (общий шаблон)
    auto windProfileToJson = [](const QVector<WindProfileData> &data) {
        QJsonArray arr;
        for (const WindProfileData &p : data) {
            QJsonObject o;
            o["height_m"]      = p.height;
            o["speed_ms"]      = p.windSpeed;
            o["direction_deg"] = p.windDirection;
            arr.append(o);
        }
        return arr;
    };

    if (opts.includeAvgWind) {
        QJsonObject obj;
        obj["count"] = snap.avgWind.size();
        obj["data"]  = windProfileToJson(snap.avgWind);
        root["avg_wind"] = obj;
    }

    if (opts.includeActualWind) {
        QJsonObject obj;
        obj["count"] = snap.actualWind.size();
        obj["data"]  = windProfileToJson(snap.actualWind);
        root["actual_wind"] = obj;
    }

    if (opts.includeMeasuredWind) {
        QJsonArray arr;
        for (const MeasuredWindData &p : snap.measuredWind) {
            QJsonObject o;
            o["height_m"]      = p.height;
            o["speed_ms"]      = p.windSpeed;
            o["direction_deg"] = p.windDirection;
            o["reliability"]   = p.reliability;
            arr.append(o);
        }
        QJsonObject obj;
        obj["count"] = snap.measuredWind.size();
        obj["data"]  = arr;
        root["measured_wind"] = obj;
    }

    if (opts.includeWindShear) {
        QJsonArray arr;
        for (const WindShearData &s : snap.windShear) {
            QJsonObject o;
            o["height_m"]        = s.height;
            o["shear_ms_30m"]    = s.shearPer30m;
            o["dir_change_deg"]  = s.shearDirection;
            o["severity_level"]  = s.severityLevel;
            o["severity_text"]   = WindShearCalculator::getSeverityText(s.severityLevel);
            arr.append(o);
        }
        QJsonObject obj;
        obj["count"] = snap.windShear.size();
        obj["data"]  = arr;
        root["wind_shear"] = obj;
    }

    // Метео-11
    auto meteo11ToJson = [](const MeasurementSnapshot::Meteo11Export &m11) {
        QJsonObject o;
        o["valid"] = m11.valid;
        if (m11.valid) {
            o["bulletin_string"]   = m11.bulletinString;
            o["station_number"]    = m11.stationNumber;
            o["BBBB"]              = m11.stationAltitude;
            o["BBB_pressure_dev"]  = m11.pressureDev;
            o["T0T0_temp_dev"]     = m11.tempVirtDev;
            o["reached_temp_km"]   = m11.reachedTempKm;
            o["reached_wind_km"]   = m11.reachedWindKm;
        }
        return o;
    };

    if (opts.includeMeteo11Updated)
        root["meteo11_updated"]     = meteo11ToJson(snap.meteo11Updated);
    if (opts.includeMeteo11Approx)
        root["meteo11_approximate"] = meteo11ToJson(snap.meteo11Approximate);
    if (opts.includeMeteo11Station)
        root["meteo11_from_station"]= meteo11ToJson(snap.meteo11FromStation);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);

}

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные методы
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::latToString(double lat)
{
    double absVal = qAbs(lat);
    int    deg    = static_cast<int>(absVal);
    double minF   = (absVal - deg) * 60.0;
    int    min    = static_cast<int>(minF);
    double sec    = (minF - min) * 60.0;
    return QString("%1°%2'%3\" %4")
        .arg(deg)
        .arg(min,  2, 10, QChar('0'))
        .arg(sec,  5, 'f', 2, QChar('0'))
        .arg(lat >= 0 ? "С" : "Ю");
}

QString MeasurementExporter::lonToString(double lon)
{
    double absVal = qAbs(lon);
    int    deg    = static_cast<int>(absVal);
    double minF   = (absVal - deg) * 60.0;
    int    min    = static_cast<int>(minF);
    double sec    = (minF - min) * 60.0;
    return QString("%1°%2'%3\" %4")
        .arg(deg)
        .arg(min,  2, 10, QChar('0'))
        .arg(sec,  5, 'f', 2, QChar('0'))
        .arg(lon >= 0 ? "В" : "З");
}

QString MeasurementExporter::formatExt(ExportOptions::Format fmt)
{
    switch (fmt) {
    case ExportOptions::TXT:  return "txt";
    case ExportOptions::CSV:  return "csv";
    case ExportOptions::JSON: return "json";
    }
    return "txt";
}