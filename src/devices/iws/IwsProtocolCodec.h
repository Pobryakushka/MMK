#ifndef IWSPROTOCOLCODEC_H
#define IWSPROTOCOLCODEC_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QtGlobal>

// ─────────────────────────────────────────────────────────────────────────────
// Протокол связи с приземной метеостанцией (ИВС): Modbus RTU и UMB.
//
// Сборка запросов, проверка CRC и разбор ответов раньше были приватными
// методами виджета GroundMeteoParams — то есть работа с байтами на линии
// RS-485 жила внутри страницы интерфейса с таблицей значений. Здесь этих
// зависимостей нет: только QByteArray, числа и строки.
//
// Функции перенесены дословно. Изменилось ровно то, что обязано было
// измениться при выносе из класса:
//   * адрес устройства приходит параметром, а не берётся из поля виджета;
//   * причина ошибки возвращается через errorMsg, а сигнал errorOccurred()
//     эмитит вызывающий виджет — сигналов у свободных функций нет;
//   * запоминание последних запрошенных регистров осталось в виджете:
//     это его состояние, а не часть протокола.
// Тексты сообщений в журнале сохранены буква в букву.
// ─────────────────────────────────────────────────────────────────────────────
namespace IwsProtocolCodec {

// ── Запросы ─────────────────────────────────────────────────────────────────

/** Modbus RTU: чтение входных регистров (0x04) сплошным диапазоном. */
QByteArray createModbusReadRequest(quint8 deviceAddress,
                                   const QList<quint16>& parameters);

/** UMB: чтение перечисленных параметров (cmd 0x2F). */
QByteArray createUmbReadRequest(const QList<quint16>& parameters);

// ── Разбор ответов ──────────────────────────────────────────────────────────

/** Ранний вариант разбора Modbus: три float подряд (температура/влажность/давление). */
bool parseModbusResponse(const QByteArray& response, QMap<QString, double>& values);

/**
 * UMB. При неуспехе errorMsg содержит причину — вызывающая сторона решает,
 * показывать её или нет.
 */
bool parseUmbResponse(const QByteArray& response, QMap<QString, double>& values,
                      QString& errorMsg);

/**
 * Modbus RTU с картой регистров: значения сопоставляются запрошенным адресам.
 * deviceAddress — ожидаемый адрес устройства (сверяется с ответом).
 * При неуспехе errorMsg содержит причину; она может остаться пустой —
 * так было и раньше для случая «список запрошенных регистров пуст».
 */
bool parseModbusResponseWithMapping(
    quint8 deviceAddress,
    const QByteArray& response,
    const QList<quint16>& requestedRegisters,
    QMap<QString, double>& values,
    QString& errorMsg);

/** Один регистр карты ИВС → имя параметра и значение в физических единицах. */
bool convertModbusRegisterToValue(
    quint16 regAddr,
    quint16 rawValue,
    QString& paramName,
    double& scaledValue);

// ── Вспомогательное ─────────────────────────────────────────────────────────

/** CRC-16 протокола UMB (полином 0x8408, отражённый). */
quint16 calculateCRC16(const QByteArray& data);

/** CRC-16 протокола Modbus (полином 0xA001). */
quint16 calculateModbusCRC16(const QByteArray& data);

/** Код параметра UMB → имя ("Temperature", "Wind Speed", …). */
QString parameterCodeToName(quint16 code);

} // namespace IwsProtocolCodec

#endif // IWSPROTOCOLCODEC_H
