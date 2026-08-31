#ifndef ARCHIVEWRITER_H
#define ARCHIVEWRITER_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>

// WindProfileData, MeasuredWindData и StationCoordinates описаны в протоколе
// АМС — это общая для программы форма данных измерения. Отдельного слоя
// моделей для них пока нет, поэтому берём их оттуда же, откуда брал драйвер.
#include "devices/ams/amsprotocol.h"

// ─────────────────────────────────────────────────────────────────────────────
// Запись результатов измерения в архив (PostgreSQL).
//
// Раньше эти двадцать девять запросов выполнял AMSHandler — драйвер линии
// RS-485. Один класс одновременно разбирал пакеты протокола АМС и писал
// в базу, из-за чего ни то ни другое нельзя было тронуть по отдельности.
//
// Функции перенесены дословно: запросы, транзакции, порядок проверок и тексты
// сообщений в журнале сохранены буква в букву (включая префикс "AMSHandler:"
// в qCritical и qInfo), чтобы вывод программы не изменился.
//
// Изменилось только то, что обязано было измениться при выносе из QObject:
//   * номер записи архива приходит параметром, а не из поля драйвера;
//   * причина ошибки возвращается через errorMsg, а сигнал databaseError()
//     эмитит вызывающий драйвер — сигналов у свободных функций нет.
// ─────────────────────────────────────────────────────────────────────────────
namespace ArchiveWriter {

/** Бюллетень Метео-11 для записи архива (upsert по record_id). */
bool saveMeteo11Bulletin(int recordId,
                         const QJsonObject &bulletinJson,
                         const QDateTime   &bulletinTime,
                         const QString     &validityPeriod);

/** Новая запись в main_archive. Возвращает record_id или -1. */
int createMainArchiveRecord(const QString &notes, QString &errorMsg);

/** Удалить ранее рассчитанные профили (средний и действительный) записи. */
bool deleteCalculatedWindProfiles(int recordId);

bool saveAvgWindProfile(int recordId, const QVector<WindProfileData> &data,
                        QString &errorMsg);

bool saveActualWindProfile(int recordId, const QVector<WindProfileData> &data,
                           QString &errorMsg);

bool saveMeasuredWindProfile(int recordId, const QVector<MeasuredWindData> &data,
                             QString &errorMsg);

bool saveStationCoordinates(int recordId, const StationCoordinates &coords,
                            QString &errorMsg);

bool saveCriticalMessage(int recordId, const QString &message, const QString &severity,
                         QString &errorMsg);

} // namespace ArchiveWriter

#endif // ARCHIVEWRITER_H
