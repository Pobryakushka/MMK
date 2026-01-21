#include "amshandler.h"
#include "databasemanager.h"
#include <QDebug>

AMSHandler::AMSHandler(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_protocol(new AMSProtocol(this))
    , m_responseTimer(new QTimer(this))
    , m_waitingForResponse(false)
    , m_lastCommand(static_cast<AMSCommand>(0x00))
    , m_currentRecordId(-1)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &AMSHandler::onDataReceived);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &AMSHandler::onSerialError);

    m_responseTimer->setSingleShot(true);
    m_responseTimer->setInterval(5000); // 5 секунд таймаут
    connect(m_responseTimer, &QTimer::timeout, this, &AMSHandler::onResponseTimeout);
}

AMSHandler::~AMSHandler()
{
    disconnectFromAMS();
}

// ===== УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ =====

bool AMSHandler::connectToAMS(const QString &portName, qint32 baudRate,
                              QSerialPort::DataBits dataBits,
                              QSerialPort::Parity parity,
                              QSerialPort::StopBits stopBits)
{
    if (m_serialPort->isOpen()) {
        qWarning() << "AMSHandler: Порт уже открыт";
        return false;
    }

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QString error = QString("Не удалось открыть порт: %1").arg(m_serialPort->errorString());
        qCritical() << "AMSHandler:" << error;
        emit errorOccurred(error);
        return false;
    }

    qInfo() << "AMSHandler: Подключено к" << portName << "со скоростью" << baudRate;
    emit connected();
    emit statusMessage("Подключено к АМС");

    // Выполняем тест линии
    QTimer::singleShot(500, this, [this]() {
        QByteArray testPacket = m_protocol->createLineTestPacket();
        if (sendPacket(testPacket, CMD_LINE_TEST)) {
            qInfo() << "AMSHandler: Отправлен тест линии";
        }
    });

    return true;
}

void AMSHandler::disconnectFromAMS()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        qInfo() << "AMSHandler: Отключено от АМС";
        emit disconnected();
        emit statusMessage("Отключено от АМС");
    }

    m_waitingForResponse = false;
    m_responseTimer->stop();
    m_receiveBuffer.clear();
}

bool AMSHandler::isConnected() const
{
    return m_serialPort->isOpen();
}

// ===== НАСТРОЙКА БД =====

void AMSHandler::setDatabase(const QString &host, int port, const QString &dbName,
                             const QString &user, const QString &password)
{
    DatabaseManager::instance()->configure(host, port, dbName, user, password);
    DatabaseManager::instance()->connect();
}

// ===== ОТПРАВКА ПАКЕТОВ =====

bool AMSHandler::sendPacket(const QByteArray &packet, AMSCommand command)
{
    if (!m_serialPort->isOpen()) {
        qWarning() << "AMSHandler: Попытка отправки без подключения";
        return false;
    }

    if (m_waitingForResponse) {
        qWarning() << "AMSHandler: Ожидается ответ на предыдущую команду";
        return false;
    }

    qint64 written = m_serialPort->write(packet);
    if (written == -1) {
        QString error = QString("Ошибка записи: %1").arg(m_serialPort->errorString());
        qWarning() << "AMSHandler:" << error;
        emit errorOccurred(error);
        return false;
    }

    m_serialPort->flush();
    m_waitingForResponse = true;
    m_lastCommand = command;
    m_responseTimer->start();

    qDebug() << "AMSHandler: Отправлена команда" << Qt::hex << static_cast<int>(command)
             << "размер" << packet.size() << "байт";

    return true;
}

// ===== ПРИЕМ ДАННЫХ =====

void AMSHandler::onDataReceived()
{
    QByteArray data = m_serialPort->readAll();
    m_receiveBuffer.append(data);

    qDebug() << "AMSHandler: Получено" << data.size() << "байт, буфер:" << m_receiveBuffer.size();

    // Ищем стоповый байт 0xFF
    while (true) {
        int stopIndex = m_receiveBuffer.indexOf(static_cast<char>(0xFF));
        if (stopIndex == -1) {
            break; // Пакет не полный
        }

        // Извлекаем пакет
        QByteArray packet = m_receiveBuffer.left(stopIndex + 1);
        m_receiveBuffer.remove(0, stopIndex + 1);

        // Обрабатываем пакет
        if (packet.size() >= 3) {
            processReceivedPacket(packet);
        }
    }
}

void AMSHandler::processReceivedPacket(const QByteArray &packet)
{
    m_responseTimer->stop();
    m_waitingForResponse = false;

    if (!m_protocol->isPacketValid(packet)) {
        qWarning() << "AMSHandler: Получен невалидный пакет";
        emit errorOccurred("Ошибка контрольной суммы");
        return;
    }

    AMSCommand cmd = m_protocol->getPacketCommand(packet);
    qDebug() << "AMSHandler: Обработка команды" << Qt::hex << static_cast<int>(cmd);

    switch (cmd) {
        case CMD_LINE_TEST: {
            if (m_protocol->parseLineTestResponse(packet)) {
                qInfo() << "AMSHandler: Тест линии успешен";
                emit statusMessage("Тест линии: OK");
            } else {
                qWarning() << "AMSHandler: Тест линии не пройден";
                emit errorOccurred("Тест линии не пройден");
            }
            break;
        }

        case CMD_MODE_TRANSFER: {
            if (m_protocol->parseModeTransferResponse(packet)) {
                qInfo() << "AMSHandler: Режим работы установлен";
                emit statusMessage("Режим работы установлен");
            }
            break;
        }

        case CMD_COORDS_TRANSFER: {
            if (m_protocol->parseCoordsTransferResponse(packet)) {
                qInfo() << "AMSHandler: Координаты переданы";
                emit statusMessage("Координаты установлены");
            }
            break;
        }

        case CMD_START_MEASUREMENT: {
            bool ok;
            WorkMode mode = m_protocol->parseStartMeasurementResponse(packet, ok);
            if (ok) {
                QString modeStr = (mode == MODE_WORKING) ? "РАБОЧИЙ" : "ДЕЖУРНЫЙ";
                qInfo() << "AMSHandler: Измерения запущены в режиме" << modeStr;
                emit statusMessage(QString("Измерения запущены: %1").arg(modeStr));

                // Создаем запись в архиве
                m_currentRecordId = createMainArchiveRecord("Запуск измерений");
            }
            break;
        }

        case CMD_DATA_EXCHANGE: {
            bool ok;
            MeasurementProgress progress = m_protocol->parseDataExchangeResponse(packet, ok);
            if (ok) {
                emit measurementProgressUpdated(progress.percentComplete, progress.currentAngle);

                if (progress.percentComplete == -1) {
                    qInfo() << "AMSHandler: Измерения завершены успешно";
                    emit statusMessage("Измерения завершены");
                } else if (progress.percentComplete == -2) {
                    qWarning() << "AMSHandler: Ошибка при измерении";
                    emit errorOccurred("Ошибка при измерении");

                    if (m_currentRecordId > 0) {
                        saveCriticalMessage(m_currentRecordId, "Ошибка при измерении", "ERROR");
                    }
                } else {
                    qDebug() << "AMSHandler: Прогресс" << progress.percentComplete
                             << "% угол" << progress.currentAngle << "°";
                }
            }
            break;
        }

        case CMD_SOURCE_DATA: {
            if (m_protocol->parseSourceDataResponse(packet)) {
                qInfo() << "AMSHandler: Исходные данные приняты";
                emit statusMessage("Исходные данные отправлены");
            }
            break;
        }

        case CMD_FUNC_CONTROL: {
            quint32 bitMask, powerOnCount;
            if (m_protocol->parseFuncControlResponse(packet, bitMask, powerOnCount)) {
                qInfo() << "AMSHandler: Функциональный контроль - маска:"
                        << Qt::hex << bitMask << "включений:" << powerOnCount;
                emit functionalControlDataReceived(bitMask, powerOnCount);

                // Проверяем биты ошибок
                if (bitMask != 0x1FF) { // Все биты должны быть 1 для исправности
                    QString errors;
                    if (!(bitMask & (1 << 0))) errors += "Превышено время вращения; ";
                    if (!(bitMask & (1 << 1))) errors += "Аварийная остановка антенны; ";
                    if (!(bitMask & (1 << 2))) errors += "Превышено время открытия антенны; ";
                    if (!(bitMask & (1 << 3))) errors += "Превышено время закрытия антенны; ";
                    if (!(bitMask & (1 << 4))) errors += "Нет сбора данных; ";
                    if (!(bitMask & (1 << 5))) errors += "СЧ не прошёл контроль; ";
                    if (!(bitMask & (1 << 6))) errors += "Не готов передатчик; ";
                    if (!(bitMask & (1 << 7))) errors += "Ошибка ПО; ";
                    if (!(bitMask & (1 << 8))) errors += "Неверная дата/время; ";

                    qWarning() << "AMSHandler: Обнаружены ошибки:" << errors;
                    emit errorOccurred(errors);

                    if (m_currentRecordId > 0) {
                        saveCriticalMessage(m_currentRecordId, errors, "WARNING");
                    }
                }
            }
            break;
        }

        case CMD_AVG_WIND_REQUEST: {
            bool ok;
            QVector<WindProfileData> data = m_protocol->parseAvgWindResponse(packet, ok);
            if (ok) {
                qInfo() << "AMSHandler: Получены данные среднего ветра:" << data.size() << "точек";
                emit avgWindDataReceived(data);

                if (m_currentRecordId > 0 || (m_currentRecordId = createMainArchiveRecord("Средний ветер")) > 0) {
                    saveAvgWindProfile(m_currentRecordId, data);
                }
            }
            break;
        }

        case CMD_ACTUAL_WIND_REQUEST: {
            bool ok;
            QVector<WindProfileData> data = m_protocol->parseActualWindResponse(packet, ok);
            if (ok) {
                qInfo() << "AMSHandler: Получены данные действительного ветра:" << data.size() << "точек";
                emit actualWindDataReceived(data);

                if (m_currentRecordId > 0 || (m_currentRecordId = createMainArchiveRecord("Действительный ветер")) > 0) {
                    saveActualWindProfile(m_currentRecordId, data);
                }
            }
            break;
        }

        case CMD_MEASURED_WIND_REQUEST: {
            bool ok;
            QVector<MeasuredWindData> data = m_protocol->parseMeasuredWindResponse(packet, ok);
            if (ok) {
                qInfo() << "AMSHandler: Получены данные измеренного ветра:" << data.size() << "точек";
                emit measuredWindDataReceived(data);

                if (m_currentRecordId > 0 || (m_currentRecordId = createMainArchiveRecord("Измеренный ветер")) > 0) {
                    saveMeasuredWindProfile(m_currentRecordId, data);
                }
            }
            break;
        }

        case CMD_ANTENNA_CONTROL: {
            bool ok;
            quint8 status = m_protocol->parseAntennaControlResponse(packet, ok);
            if (ok) {
                QString statusStr;
                switch (status) {
                    case 0x00: statusStr = "В процессе"; break;
                    case 0x01: statusStr = "Завершено успешно"; break;
                    case 0x02: statusStr = "Аварийная остановка"; break;
                    default: statusStr = "Неизвестно";
                }
                qInfo() << "AMSHandler: Статус антенны:" << statusStr;
                emit antennaStatusReceived(status);
            }
            break;
        }

        case CMD_SET_DATETIME: {
            if (m_protocol->parseSetDateTimeResponse(packet)) {
                qInfo() << "AMSHandler: Дата/время установлены";
                emit statusMessage("Дата/время установлены");
            }
            break;
        }

        case CMD_ROTATE_ANTENNA: {
            quint8 status;
            float angle;
            if (m_protocol->parseRotateAntennaResponse(packet, status, angle)) {
                qDebug() << "AMSHandler: Антенна - статус:" << status << "угол:" << angle;
            }
            break;
        }

        default:
            qWarning() << "AMSHandler: Неизвестная команда" << Qt::hex << static_cast<int>(cmd);
            break;
    }
}

void AMSHandler::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    QString errorStr = m_serialPort->errorString();
    qCritical() << "AMSHandler: Ошибка порта:" << errorStr;
    emit errorOccurred(errorStr);
}

void AMSHandler::onResponseTimeout()
{
    m_waitingForResponse = false;
    qWarning() << "AMSHandler: Таймаут ответа на команду" << Qt::hex << static_cast<int>(m_lastCommand);
    emit errorOccurred("Таймаут ответа от АМС");
}

// ===== УПРАВЛЕНИЕ ИЗМЕРЕНИЯМИ =====

bool AMSHandler::startMeasurement()
{
    QByteArray packet = m_protocol->createStartMeasurementPacket();
    return sendPacket(packet, CMD_START_MEASUREMENT);
}

bool AMSHandler::stopMeasurement()
{
    QByteArray packet = m_protocol->createDataExchangePacket(false);
    return sendPacket(packet, CMD_DATA_EXCHANGE);
}

bool AMSHandler::continueMeasurement()
{
    QByteArray packet = m_protocol->createDataExchangePacket(true);
    return sendPacket(packet, CMD_DATA_EXCHANGE);
}

// ===== ЗАПРОС ДАННЫХ =====

bool AMSHandler::requestAvgWind()
{
    QByteArray packet = m_protocol->createAvgWindRequestPacket();
    return sendPacket(packet, CMD_AVG_WIND_REQUEST);
}

bool AMSHandler::requestActualWind()
{
    QByteArray packet = m_protocol->createActualWindRequestPacket();
    return sendPacket(packet, CMD_ACTUAL_WIND_REQUEST);
}

bool AMSHandler::requestMeasuredWind()
{
    QByteArray packet = m_protocol->createMeasuredWindRequestPacket();
    return sendPacket(packet, CMD_MEASURED_WIND_REQUEST);
}

bool AMSHandler::requestFunctionalControl()
{
    QByteArray packet = m_protocol->createFuncControlPacket();
    return sendPacket(packet, CMD_FUNC_CONTROL);
}

// ===== УСТАНОВКА ПАРАМЕТРОВ =====

bool AMSHandler::setWorkMode(WorkMode mode, Litera litera)
{
    QByteArray packet = m_protocol->createModeTransferPacket(mode, litera);
    return sendPacket(packet, CMD_MODE_TRANSFER);
}

bool AMSHandler::setStationCoordinates(const StationCoordinates &coords)
{
    QByteArray packet = m_protocol->createCoordsTransferPacket(coords);
    bool result = sendPacket(packet, CMD_COORDS_TRANSFER);

    // Сохраняем координаты в БД
    if (result && m_currentRecordId > 0) {
        saveStationCoordinates(m_currentRecordId, coords);
    }

    return result;
}

bool AMSHandler::setDateTime(const QDateTime &dateTime)
{
    QByteArray packet = m_protocol->createSetDateTimePacket(dateTime);
    return sendPacket(packet, CMD_SET_DATETIME);
}

bool AMSHandler::sendSourceData(int day, int hour, int tenMinutes,
                                float stationAltitude,
                                const QVector<float> &avgWindDir,
                                const QVector<float> &avgWindSpeed,
                                float reachedHeight,
                                float surfaceWindDir, float surfaceWindSpeed,
                                const QDateTime &currentDateTime)
{
    QByteArray packet = m_protocol->createSourceDataPacket(day, hour, tenMinutes,
                                                           stationAltitude,
                                                           avgWindDir, avgWindSpeed,
                                                           reachedHeight,
                                                           surfaceWindDir, surfaceWindSpeed,
                                                           currentDateTime);
    return sendPacket(packet, CMD_SOURCE_DATA);
}

// ===== УПРАВЛЕНИЕ АНТЕННОЙ =====

bool AMSHandler::openAntenna()
{
    QByteArray packet = m_protocol->createAntennaControlPacket(0x00);
    return sendPacket(packet, CMD_ANTENNA_CONTROL);
}

bool AMSHandler::closeAntenna()
{
    QByteArray packet = m_protocol->createAntennaControlPacket(0x01);
    return sendPacket(packet, CMD_ANTENNA_CONTROL);
}

bool AMSHandler::getAntennaStatus()
{
    QByteArray packet = m_protocol->createAntennaControlPacket(0x02);
    return sendPacket(packet, CMD_ANTENNA_CONTROL);
}

bool AMSHandler::rotateAntenna(float angle)
{
    QByteArray packet = m_protocol->createRotateAntennaPacket(0x00, angle);
    return sendPacket(packet, CMD_ROTATE_ANTENNA);
}

bool AMSHandler::stopAntennaRotation()
{
    QByteArray packet = m_protocol->createRotateAntennaPacket(0x02, 0.0f);
    return sendPacket(packet, CMD_ROTATE_ANTENNA);
}

// ===== РАБОТА С БД =====

int AMSHandler::createMainArchiveRecord(const QString &notes)
{
    if (!DatabaseManager::instance()->connect()) return -1;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare("INSERT INTO main_archive (completion_time, notes) VALUES (NOW(), :notes) RETURNING record_id");
    query.bindValue(":notes", notes.isEmpty() ? QVariant(QVariant::String) : notes);

    if (!query.exec() || !query.next()) {
        QString error = QString("Ошибка создания записи в архиве: %1").arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return -1;
    }

    int recordId = query.value(0).toInt();
    qInfo() << "AMSHandler: Создана запись в архиве, ID:" << recordId;
    emit dataWrittenToDatabase(recordId);

    return recordId;
}

bool AMSHandler::saveAvgWindProfile(int recordId, const QVector<WindProfileData> &data)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // ИСПРАВЛЕНИЕ: Получаем один profile_id для ВСЕГО профиля
    QSqlQuery getIdQuery(db);
    if (!getIdQuery.exec("SELECT nextval('avg_wind_profile_profile_id_seq')") || !getIdQuery.next()) {
        QString error = QString("Ошибка получения profile_id: %1")
            .arg(getIdQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    int profileId = getIdQuery.value(0).toInt();
    qDebug() << "AMSHandler: Получен profile_id =" << profileId << "для среднего ветра";

    // Теперь записываем ВСЕ точки профиля с ОДНИМ profile_id
    QSqlQuery queryPoint(db);
    queryPoint.prepare("INSERT INTO avg_wind_profile (profile_id, height, wind_speed, wind_direction, measurement_time) "
                      "VALUES (:profile_id, :height, :speed, :direction, NOW())");

    db.transaction();

    for (const WindProfileData &point : data) {
        queryPoint.bindValue(":profile_id", profileId);  // ОДИНАКОВЫЙ для всех точек!
        queryPoint.bindValue(":height", point.height);
        queryPoint.bindValue(":speed", point.windSpeed);
        queryPoint.bindValue(":direction", point.windDirection);

        if (!queryPoint.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки среднего ветра: %1")
                .arg(queryPoint.lastError().text());
            qCritical() << "AMSHandler:" << error;
            emit databaseError(error);
            return false;
        }
    }

    db.commit();
    qInfo() << "AMSHandler: Сохранён профиль среднего ветра, profile_id =" << profileId
            << "," << data.size() << "точек";

    // Теперь сохраняем ссылку в wind_profiles_references
    // Сначала проверяем, есть ли уже запись для этого record_id
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT record_id FROM wind_profiles_references WHERE record_id = :record_id");
    checkQuery.bindValue(":record_id", recordId);

    if (!checkQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка проверки wind_profiles_references:" << checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next()) {
        // Запись существует - обновляем
        qDebug() << "AMSHandler: Обновляем существующую запись wind_profiles_references для record_id" << recordId;
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE wind_profiles_references SET avg_wind_profile_id = :profile_id "
                          "WHERE record_id = :record_id");
        updateQuery.bindValue(":profile_id", profileId);
        updateQuery.bindValue(":record_id", recordId);

        if (!updateQuery.exec()) {
            qCritical() << "AMSHandler: Ошибка обновления avg_wind_profile_id:" << updateQuery.lastError().text();
            return false;
        }
        qDebug() << "AMSHandler: Обновлено avg_wind_profile_id =" << profileId << "для record_id" << recordId;
    } else {
        // Записи нет - создаем новую (другие profile_id установим позже)
        qDebug() << "AMSHandler: Создаем новую запись wind_profiles_references для record_id" << recordId;
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO wind_profiles_references "
                          "(record_id, avg_wind_profile_id, actual_wind_profile_id, measured_wind_profile_id, wind_shear_profile_id) "
                          "VALUES (:record_id, :avg_id, NULL, NULL, NULL)");
        insertQuery.bindValue(":record_id", recordId);
        insertQuery.bindValue(":avg_id", profileId);

        if (!insertQuery.exec()) {
            qCritical() << "AMSHandler: Ошибка создания wind_profiles_references:" << insertQuery.lastError().text();
            return false;
        }
        qDebug() << "AMSHandler: Создана запись wind_profiles_references: record_id" << recordId << ", avg_wind_profile_id" << profileId;
    }

    return true;
}

bool AMSHandler::saveActualWindProfile(int recordId, const QVector<WindProfileData> &data)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // ИСПРАВЛЕНИЕ: Получаем один profile_id для ВСЕГО профиля
    QSqlQuery getIdQuery(db);
    if (!getIdQuery.exec("SELECT nextval('actual_wind_profile_profile_id_seq')") || !getIdQuery.next()) {
        QString error = QString("Ошибка получения profile_id: %1")
            .arg(getIdQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    int profileId = getIdQuery.value(0).toInt();
    qDebug() << "AMSHandler: Получен profile_id =" << profileId << "для действительного ветра";

    // Записываем ВСЕ точки с ОДНИМ profile_id
    QSqlQuery queryPoint(db);
    queryPoint.prepare("INSERT INTO actual_wind_profile (profile_id, height, wind_speed, wind_direction, measurement_time) "
                      "VALUES (:profile_id, :height, :speed, :direction, NOW())");

    db.transaction();

    for (const WindProfileData &point : data) {
        queryPoint.bindValue(":profile_id", profileId);
        queryPoint.bindValue(":height", point.height);
        queryPoint.bindValue(":speed", point.windSpeed);
        queryPoint.bindValue(":direction", point.windDirection);

        if (!queryPoint.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки действительного ветра: %1")
                .arg(queryPoint.lastError().text());
            qCritical() << "AMSHandler:" << error;
            emit databaseError(error);
            return false;
        }
    }

    db.commit();
    qInfo() << "AMSHandler: Сохранён профиль действительного ветра, profile_id =" << profileId
            << "," << data.size() << "точек";

    // Обновляем wind_profiles_references
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT record_id FROM wind_profiles_references WHERE record_id = :record_id");
    checkQuery.bindValue(":record_id", recordId);

    if (!checkQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка проверки wind_profiles_references:" << checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next()) {
        qDebug() << "AMSHandler: Обновляем actual_wind_profile_id для record_id" << recordId;
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE wind_profiles_references SET actual_wind_profile_id = :profile_id "
                          "WHERE record_id = :record_id");
        updateQuery.bindValue(":profile_id", profileId);
        updateQuery.bindValue(":record_id", recordId);

        if (!updateQuery.exec()) {
            qCritical() << "AMSHandler: Ошибка обновления actual_wind_profile_id:" << updateQuery.lastError().text();
            return false;
        }
        qDebug() << "AMSHandler: Обновлено actual_wind_profile_id =" << profileId;
    } else {
        qDebug() << "AMSHandler: Создаем новую запись с actual_wind_profile_id для record_id" << recordId;
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO wind_profiles_references "
                          "(record_id, avg_wind_profile_id, actual_wind_profile_id, measured_wind_profile_id, wind_shear_profile_id) "
                          "VALUES (:record_id, NULL, :actual_id, NULL, NULL)");
        insertQuery.bindValue(":record_id", recordId);
        insertQuery.bindValue(":actual_id", profileId);

        if (!insertQuery.exec()) {
            qCritical() << "AMSHandler: Ошибка создания wind_profiles_references:" << insertQuery.lastError().text();
            return false;
        }
        qDebug() << "AMSHandler: Создана запись с actual_wind_profile_id" << profileId;
    }

    return true;
}

bool AMSHandler::saveMeasuredWindProfile(int recordId, const QVector<MeasuredWindData> &data)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // ИСПРАВЛЕНИЕ: Получаем один profile_id для ВСЕГО профиля
    QSqlQuery getIdQuery(db);
    if (!getIdQuery.exec("SELECT nextval('measured_wind_profile_profile_id_seq')") || !getIdQuery.next()) {
        QString error = QString("Ошибка получения profile_id: %1")
            .arg(getIdQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    int profileId = getIdQuery.value(0).toInt();
    qDebug() << "AMSHandler: Получен profile_id =" << profileId << "для измеренного ветра";

    // Записываем ВСЕ точки с ОДНИМ profile_id
    QSqlQuery queryPoint(db);
    queryPoint.prepare("INSERT INTO measured_wind_profile (profile_id, height, wind_speed, wind_direction, measurement_time) "
                      "VALUES (:profile_id, :height, :speed, :direction, NOW())");

    db.transaction();

    int validCount = 0;
    int invalidCount = 0;

    for (const MeasuredWindData &point : data) {
        // Диагностика: выводим первые 3 точки
        if (validCount + invalidCount < 3) {
            qDebug() << "AMSHandler: Точка" << (validCount + invalidCount)
                     << "- height:" << point.height
                     << ", speed:" << point.windSpeed
                     << ", dir:" << point.windDirection
                     << ", reliability:" << point.reliability;
        }

        // Пропускаем недействительные точки (reliability != 2)
        if (point.reliability != 2) {
            invalidCount++;
            continue;
        }

        validCount++;

        queryPoint.bindValue(":profile_id", profileId);
        queryPoint.bindValue(":height", point.height);
        queryPoint.bindValue(":speed", point.windSpeed);
        queryPoint.bindValue(":direction", point.windDirection);

        if (!queryPoint.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки измеренного ветра: %1")
                .arg(queryPoint.lastError().text());
            qCritical() << "AMSHandler:" << error;
            emit databaseError(error);
            return false;
        }
    }

    db.commit();
    qInfo() << "AMSHandler: Сохранён профиль измеренного ветра, profile_id =" << profileId
            << ", всего точек:" << data.size()
            << ", записано:" << validCount
            << ", пропущено (недействительных):" << invalidCount;

    // Обновляем wind_profiles_references
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT record_id FROM wind_profiles_references WHERE record_id = :record_id");
    checkQuery.bindValue(":record_id", recordId);

    if (!checkQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка проверки wind_profiles_references:" << checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next()) {
        qDebug() << "AMSHandler: Обновляем measured_wind_profile_id для record_id" << recordId;
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE wind_profiles_references SET measured_wind_profile_id = :profile_id "
                          "WHERE record_id = :record_id");
        updateQuery.bindValue(":profile_id", profileId);
        updateQuery.bindValue(":record_id", recordId);

        if (!updateQuery.exec()) {
            qCritical() << "AMSHandler: Ошибка обновления measured_wind_profile_id:" << updateQuery.lastError().text();
            return false;
        }
        qDebug() << "AMSHandler: Обновлено measured_wind_profile_id =" << profileId;
    } else {
        qDebug() << "AMSHandler: Создаем новую запись с measured_wind_profile_id для record_id" << recordId;
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO wind_profiles_references "
                          "(record_id, avg_wind_profile_id, actual_wind_profile_id, measured_wind_profile_id, wind_shear_profile_id) "
                          "VALUES (:record_id, NULL, NULL, :measured_id, NULL)");
        insertQuery.bindValue(":record_id", recordId);
        insertQuery.bindValue(":measured_id", profileId);

        if (!insertQuery.exec()) {
            qCritical() << "AMSHandler: Ошибка создания wind_profiles_references:" << insertQuery.lastError().text();
            return false;
        }
        qDebug() << "AMSHandler: Создана запись с measured_wind_profile_id" << profileId;
    }

    return true;
}

bool AMSHandler::saveStationCoordinates(int recordId, const StationCoordinates &coords)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare("INSERT INTO station_coordinates (record_id, latitude, longitude, altitude) "
                 "VALUES (:record_id, :lat, :lon, :alt)");

    // Преобразуем секунды в градусы
    double latDeg = coords.latitude / 3600.0;
    double lonDeg = coords.longitude / 3600.0;

    query.bindValue(":record_id", recordId);
    query.bindValue(":lat", latDeg);
    query.bindValue(":lon", lonDeg);
    query.bindValue(":alt", coords.altitude);

    if (!query.exec()) {
        QString error = QString("Ошибка записи координат: %1").arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    qInfo() << "AMSHandler: Сохранены координаты станции";
    return true;
}

bool AMSHandler::saveCriticalMessage(int recordId, const QString &message, const QString &severity)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare("INSERT INTO critical_messages (record_id, message_text, message_time, severity_level) "
                 "VALUES (:record_id, :message, NOW(), :severity)");

    query.bindValue(":record_id", recordId);
    query.bindValue(":message", message);
    query.bindValue(":severity", severity);

    if (!query.exec()) {
        QString error = QString("Ошибка записи критического сообщения: %1").arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    qInfo() << "AMSHandler: Сохранено критическое сообщение:" << message;
    return true;
}
