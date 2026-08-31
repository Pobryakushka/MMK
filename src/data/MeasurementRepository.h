#ifndef MEASUREMENTREPOSITORY_H
#define MEASUREMENTREPOSITORY_H

#include <QDateTime>
#include <QString>
#include <QVector>

// WindProfileData и MeasuredWindData описаны в протоколе АМС — это общая для
// всей программы форма записи профиля ветра, а не что-то специфичное для
// драйвера. Отдельного «слоя моделей» для них в проекте пока нет, поэтому
// репозиторий берёт их оттуда же, откуда брал виджет архива до выноса.
#include "devices/ams/amsprotocol.h"

// ─────────────────────────────────────────────────────────────────────────────
// Чтение архива измерений из базы.
//
// Раньше все эти запросы выполнял сам виджет MeasurementResults: страница
// интерфейса напрямую собирала QSqlQuery, разбирала строки результата и тут же
// раскладывала их по таблицам и подписям. Теперь SQL живёт здесь, а виджету
// достаются готовые структуры — он больше не подключает <QSqlQuery> вовсе.
//
// Функции перенесены дословно: запросы, порядок проверок и тексты
// диагностических сообщений сохранены буква в букву (включая префикс
// "MeasurementResults:" в qDebug), чтобы вывод программы не изменился.
// ─────────────────────────────────────────────────────────────────────────────

// Одна запись архива: когда измеряли и какие профили к ней привязаны.
struct MeasurementRecord {
    int recordId;
    QDateTime measurementTime;
    bool hasAvgWind;
    bool hasActualWind;
    bool hasMeasuredWind;
    QString notes;

    MeasurementRecord() : recordId(-1), hasAvgWind(false),
        hasActualWind(false), hasMeasuredWind(false) {}
};

namespace MeasurementRepository {

/** Подключиться к БД, если ещё не подключены. */
bool ensureConnected();

/**
 * То же, но с записью в журнал — как это делала загрузка списка измерений.
 * Вынесено отдельным шагом сознательно: при недоступной БД архив НЕ сбрасывал
 * ранее загруженный список, а при неудачном запросе — сбрасывал. Разделение
 * подключения и запроса сохраняет это различие в точности.
 */
bool ensureConnectedLogged();

/**
 * Весь архив одним запросом (main_archive + LEFT JOIN на
 * wind_profiles_references для флагов наличия профилей), новые сверху.
 * Вызывать после успешного ensureConnectedLogged().
 * Возвращает false, если запрос не удался, — от «архив пуст» это отличается.
 */
bool loadAllRecords(QVector<MeasurementRecord> &out);

QVector<WindProfileData>  loadAvgWindProfile(int recordId);
QVector<WindProfileData>  loadActualWindProfile(int recordId);
QVector<MeasuredWindData> loadMeasuredWindProfile(int recordId);

/** Приземные метеоданные измерения (таблица surface_meteo). */
struct SurfaceMeteo {
    double temperatureC     = 0.0;
    double humidityPct      = 0.0;
    double pressureMmHg     = 0.0;  // в БД уже в мм рт. ст.
    double windSpeedSurface = 0.0;
    // Направление приземного ветра архив читал из одного и того же столбца
    // ДВАЖДЫ и по-разному: целым — для показа в таблице, дробным — для
    // последующего расчёта Метео-11. Оба значения сохранены как есть, чтобы
    // вынос запросов не потерял дробную часть и не изменил округление.
    double windDirSurface     = 0.0;
    int    windDirSurfaceCell = 0;
};
bool loadSurfaceMeteo(int recordId, SurfaceMeteo &out);

/**
 * Координаты станции для измерения (таблица station_coordinates).
 * Имя намеренно не StationCoordinates — так называется структура протокола
 * АМС из amsprotocol.h, и совпадение имён запутывало бы обе стороны.
 */
struct StationPosition {
    double latitude  = 0.0;
    double longitude = 0.0;
    double altitude  = 0.0;
};
bool loadStationPosition(int recordId, StationPosition &out);

/**
 * Исходный бюллетень Метео-11 от метеостанции (таблица meteo_11_bulletin):
 * JSON с полями бюллетеня и время его составления. false — бюллетень для
 * этой записи не вводился, это нормальная ситуация.
 */
bool loadMeteo11Bulletin(int recordId, QString &bulletinJson, QDateTime &bulletinTime);

} // namespace MeasurementRepository

#endif // MEASUREMENTREPOSITORY_H
