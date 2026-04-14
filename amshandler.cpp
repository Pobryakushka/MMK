#include "amshandler.h"
#include "databasemanager.h"
#include <QDebug>

AMSHandler::AMSHandler(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_protocol(new AMSProtocol(this))
    , m_responseTimer(new QTimer(this))
    , m_exchangeDataTimer(new QTimer(this))
    , m_waitingForResponse(false)
    , m_isConnecting(false)
    , m_lastCommand(static_cast<AMSCommand>(0x00))
    , m_currentRecordId(-1)
    , m_measurementStatus(STATUS_IDLE)
    , m_currentStage(STAGE_IDLE)
    , m_currentMode(MODE_WORKING)
    , m_currentAveragingTime(AVERAGING_3_MIN)
    , m_currentLitera(LITERA_1)
    , m_lastProgress(0)
    , m_lastAngle(0.0f)
    , m_intermediateDataSent(false)
    , m_funcControlAfterFailure(false)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &AMSHandler::onDataReceived);

    m_responseTimer->setSingleShot(true);
    m_responseTimer->setInterval(5000); // 5 секунд таймаут
    connect(m_responseTimer, &QTimer::timeout, this, &AMSHandler::onResponseTimeout);

    // Таймер для периодического опроса процесса измерения (0xA4)
    m_exchangeDataTimer->setInterval(500); // Опрос каждые 2 секунды
    connect(m_exchangeDataTimer, &QTimer::timeout, this, &AMSHandler::onExchangeDataTimer);

    // Таймер опроса статуса открытия/закрытия антенны (0xAD) — каждые 1 с
    m_antennaPollTimer = new QTimer(this);
    m_antennaPollTimer->setInterval(1000);
    connect(m_antennaPollTimer, &QTimer::timeout, this, &AMSHandler::onAntennaPollTimer);

    // Таймер опроса статуса поворота антенны (0xAF) — каждые 1 с
    m_rotatePollTimer = new QTimer(this);
    m_rotatePollTimer->setInterval(1000);
    connect(m_rotatePollTimer, &QTimer::timeout, this, &AMSHandler::onRotatePollTimer);
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
        m_isConnecting = false;
        return false;
    }

    qInfo() << "AMSHandler: Порт открыт:" << portName << "со скоростью" << baudRate;
    qInfo() << "AMSHandler: Выполняется тест линии...";
    m_isConnecting = true;
    emit statusMessage("Тест линии...");

    // Выполняем тест линии
    QTimer::singleShot(500, this, [this]() {
        QByteArray testPacket = m_protocol->createLineTestPacket();
        qDebug() << "TestPacket: " << testPacket;
        if (sendPacket(testPacket, CMD_LINE_TEST)) {
            qInfo() << "AMSHandler: Отправлен тест линии";
        } else {
            qWarning() << "AMSHandler: Не удалось отправить тест линии";
            m_isConnecting = false;
            emit errorOccurred("Не удалось отправить тест линии");
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
    m_isConnecting = false;
    m_responseTimer->stop();
    m_exchangeDataTimer->stop();
    m_antennaPollTimer->stop();
    m_rotatePollTimer->stop();
    m_receiveBuffer.clear();

    // Сброс состояния измерения
    setMeasurementStatus(STATUS_IDLE);
    setMeasurementStage(STAGE_IDLE);
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

// ===== УПРАВЛЕНИЕ ПРОЦЕССОМ ИЗМЕРЕНИЯ СОГЛАСНО ДИАГРАММЕ =====

bool AMSHandler::startMeasurementSequence(WorkMode mode, AveragingTime avgTime, Litera litera,
                                          const StationCoordinates &coords,
                                          const QDateTime &dateTime)
{
    m_intermediateDataSent = false;
    if (!isConnected()) {
        emit errorOccurred("Нет подключения к АМС");
        return false;
    }

    if (m_measurementStatus == STATUS_RUNNING) {
        emit errorOccurred("Измерение уже выполняется");
        return false;
    }

    // Сохраняем параметры для использования в процессе
    m_currentMode = mode;
    m_currentAveragingTime = avgTime;
    m_currentLitera = litera;
    m_currentCoords = coords;
    m_currentDateTime = dateTime;

    // Создаём запись в БД
    m_currentRecordId = createMainArchiveRecord("Автоматическое измерение");
    if (m_currentRecordId < 0) {
        emit errorOccurred("Не удалось создать запись в БД");
        return false;
    }

    // Сохраняем координаты
    saveStationCoordinates(m_currentRecordId, coords);

    // Начинаем последовательность согласно диаграмме
    setMeasurementStatus(STATUS_RUNNING);
    setMeasurementStage(STAGE_SEND_MODE);

    qInfo() << "AMSHandler: Начата последовательность измерения";
    emit statusMessage("Начало измерения...");

    // Начинаем с отправки режима работы (0xA1)
    advanceMeasurementStage();

    return true;
}

void AMSHandler::setMeasurementStage(MeasurementStage stage)
{
    if (m_currentStage == stage) return;

    m_currentStage = stage;

    QString stageDesc;
    switch (stage) {
    case STAGE_IDLE: stageDesc = "Ожидание"; break;
    case STAGE_SEND_SOURCE_DATA: stageDesc = "Отправка исходных данных"; break;
    case STAGE_SEND_MODE: stageDesc = "Передача режима работы"; break;
    case STAGE_SEND_COORDS: stageDesc = "Передача координат"; break;
    case STAGE_SEND_DATETIME: stageDesc = "Установка даты и времени"; break;
    case STAGE_START_MEASUREMENT: stageDesc = "Старт измерений"; break;
    case STAGE_EXCHANGE_DATA: stageDesc = "Обмен данными о процессе"; break;
    case STAGE_REQUEST_RESULTS: stageDesc = "Запрос результатов"; break;
    case STAGE_CALCULATE_PROFILE: stageDesc = "Расчёт профиля ветра"; break;
    case STAGE_COMPLETED: stageDesc = "Завершено"; break;
    }

    qInfo() << "AMSHandler: Этап измерения:" << stageDesc;
    emit measurementStageChanged(stage, stageDesc);
    emit statusMessage(stageDesc);
}

void AMSHandler::setMeasurementStatus(MeasurementStatus status)
{
    if (m_measurementStatus == status) return;

    m_measurementStatus = status;

    QString statusDesc;
    switch (status) {
    case STATUS_IDLE: statusDesc = "Нет активных измерений"; break;
    case STATUS_RUNNING: statusDesc = "Измерение в процессе"; break;
    case STATUS_READY: statusDesc = "Измерение завершено успешно"; break;
    case STATUS_FAILURE: statusDesc = "Ошибка измерения"; break;
    }

    qInfo() << "AMSHandler: Статус измерения:" << statusDesc;
    emit measurementStatusChanged(status);
}

void AMSHandler::advanceMeasurementStage()
{
    switch (m_currentStage) {
    case STAGE_SEND_MODE: {
        // Передача признаков режимов работы (0xA1)
        setMeasurementStage(STAGE_SEND_MODE);
        QByteArray packet = m_protocol->createModeTransferPacket(m_currentMode, m_currentAveragingTime, m_currentLitera);
        sendPacket(packet, CMD_MODE_TRANSFER);
        break;
    }

    case STAGE_SEND_COORDS: {
        // Передача координат (0xA2)
        setMeasurementStage(STAGE_SEND_COORDS);
        QByteArray packet = m_protocol->createCoordsTransferPacket(m_currentCoords);
        sendPacket(packet, CMD_COORDS_TRANSFER);
        break;
    }

    case STAGE_SEND_DATETIME: {
        // Установка даты и времени (0xAE)
        setMeasurementStage(STAGE_SEND_DATETIME);
        QByteArray packet = m_protocol->createSetDateTimePacket(m_currentDateTime);
        sendPacket(packet, CMD_SET_DATETIME);
        break;
    }

    case STAGE_START_MEASUREMENT: {
        // Старт измерений (0xA3)
        setMeasurementStage(STAGE_START_MEASUREMENT);
        QByteArray packet = m_protocol->createStartMeasurementPacket();
        sendPacket(packet, CMD_START_MEASUREMENT);
        break;
    }

    case STAGE_EXCHANGE_DATA: {
        // Начинаем периодический опрос процесса (0xA4)
        setMeasurementStage(STAGE_EXCHANGE_DATA);
        m_exchangeDataTimer->start();
        onExchangeDataTimer(); // Сразу делаем первый запрос
        break;
    }

    case STAGE_REQUEST_RESULTS: {
        // Запрос результатов измерения
        setMeasurementStage(STAGE_REQUEST_RESULTS);
        requestMeasurementResults();
        break;
    }

    case STAGE_CALCULATE_PROFILE: {
        // Расчёт профиля ветра (может быть выполнен внешним модулем)
        setMeasurementStage(STAGE_CALCULATE_PROFILE);
        completeMeasurement();
        break;
    }

    default:
        qWarning() << "AMSHandler: Неизвестный этап измерения";
        break;
    }
}

void AMSHandler::onExchangeDataTimer()
{
    if (m_currentStage != STAGE_EXCHANGE_DATA) {
        m_exchangeDataTimer->stop();
        return;
    }

    // Отправляем команду обмена данными (0xA4)
    // continueProcess = true для продолжения измерения
    QByteArray packet = m_protocol->createDataExchangePacket(true);
    sendPacket(packet, CMD_DATA_EXCHANGE);
}

void AMSHandler::processExchangeDataResponse(int progress, float angle)
{
    m_lastProgress = progress;
    m_lastAngle = angle;

    emit measurementProgressUpdated(progress, angle);

    // Switch по progress согласно диаграмме
    if (progress == -1) {
        // Case: -1 → Измерение завершено
        qInfo() << "AMSHandler: Измерение завершено успешно (progress = -1)";
        m_exchangeDataTimer->stop();
        setMeasurementStatus(STATUS_READY);

        // Переходим к запросу результатов
        setMeasurementStage(STAGE_REQUEST_RESULTS);
        advanceMeasurementStage();

    } else if (progress == -2) {
        // Case: -2 → АМС сообщил об ошибке измерения.
        // Останавливаем таймер опроса и запрашиваем функциональный контроль (0xA7),
        // чтобы выяснить причину отказа. failMeasurement() будет вызван после
        // получения ответа на 0xA7 — это гарантирует, что m_currentRecordId
        // ещё валиден и детали неисправностей попадут в БД.
        qWarning() << "AMSHandler: Ошибка измерения (progress = -2), запрашиваем функциональный контроль";

        m_exchangeDataTimer->stop();
        m_funcControlAfterFailure = true;  // Флаг: ответ 0xA7 завершит failMeasurement
        requestFunctionalControl();

    } else if (progress == 80 || progress == 72) {
        if (!m_intermediateDataSent){
            m_intermediateDataSent = true;
            // Case: 80 или 72 → Требуются промежуточные данные
            qInfo() << "AMSHandler: Запрос промежуточных данных (progress =" << progress << ")";
            m_exchangeDataTimer->stop(); // Временно останавливаем

            // Сигнализируем о необходимости предоставить данные
            emit needIntermediateData(progress);

            // После получения данных через sendSourceDataDuringMeasurement()
            // измерение продолжится
        }
    } else {
        // Default: 0-100 → Обновление данных, продолжение цикла
        QString msg = QString("Процесс измерения: %1%, угол: %2°").arg(progress).arg(angle, 0, 'f', 1);
        emit statusMessage(msg);

        // Таймер продолжит опрос автоматически
    }
}

bool AMSHandler::sendSourceDataDuringMeasurement(int day, int hour, int tenMinutes,
                                                 float stationAltitude,
                                                 const QVector<float> &avgWindDir,
                                                 const QVector<float> &avgWindSpeed,
                                                 float reachedHeight,
                                                 float surfaceWindDir, float surfaceWindSpeed,
                                                 const QDateTime &currentDateTime)
{
    if (m_currentStage != STAGE_EXCHANGE_DATA) {
        qWarning() << "AMSHandler: Отправка исходных данных возможна только в режиме EXCHANGE_DATA";
        return false;
    }

    QByteArray packet = m_protocol->createSourceDataPacket(day, hour, tenMinutes,
                                                           stationAltitude,
                                                           avgWindDir, avgWindSpeed,
                                                           reachedHeight,
                                                           surfaceWindDir, surfaceWindSpeed,
                                                           currentDateTime);

    bool success = sendPacket(packet, CMD_SOURCE_DATA);

    if (success) {
        qInfo() << "AMSHandler: Отправлены исходные данные в процессе измерения";
        // После отправки данных возобновляем опрос
        QTimer::singleShot(1000, this, [this]() {
            if (m_currentStage == STAGE_EXCHANGE_DATA) {
                m_exchangeDataTimer->start();
            }
        });
    }

    return success;
}

void AMSHandler::requestMeasurementResults()
{
    // Запрашиваем все профили ветра
    // 0xAC - измеренный ветер, 0xA9 - средний, 0xAA - действительный

    qInfo() << "AMSHandler: Запрос измеренного ветра (0xAC)";
    QByteArray packet = m_protocol->createMeasuredWindRequestPacket();
    sendPacket(packet, CMD_MEASURED_WIND_REQUEST);

    // После получения ответа будут запрошены остальные профили
}

void AMSHandler::completeMeasurement()
{
    m_intermediateDataSent = false;
    setMeasurementStage(STAGE_COMPLETED);
    setMeasurementStatus(STATUS_READY);

    qInfo() << "AMSHandler: Измерение успешно завершено, ID записи:" << m_currentRecordId;

    // Оба сигнала эмитируем пока m_currentRecordId ещё валиден
    emit dataWrittenToDatabase(m_currentRecordId);
    emit measurementCompleted(m_currentRecordId);
    emit statusMessage("Измерение завершено успешно");

    // Сброс состояния — ПОСЛЕ emit
    m_currentRecordId = -1;
}

void AMSHandler::failMeasurement(const QString &reason)
{
    m_intermediateDataSent = false;
    m_funcControlAfterFailure = false;  // На случай нестандартного пути завершения
    setMeasurementStatus(STATUS_FAILURE);

    if (m_currentRecordId > 0) {
        saveCriticalMessage(m_currentRecordId, reason, "ERROR");
    }

    qCritical() << "AMSHandler: Измерение не удалось:" << reason;
    emit measurementFailed(reason);
    emit errorOccurred("Ошибка измерения: " + reason);

    // Сброс состояния
    m_currentRecordId = -1;
    m_exchangeDataTimer->stop();
}

bool AMSHandler::stopMeasurement()
{
    if (m_measurementStatus != STATUS_RUNNING) {
        qWarning() << "AMSHandler: Нет активного измерения для остановки";
        return false;
    }

    // Отправляем команду остановки (0xA4 с параметром continueProcess = false)
    QByteArray packet = m_protocol->createDataExchangePacket(false);
    bool success = sendPacket(packet, CMD_DATA_EXCHANGE);

    if (success) {
        m_exchangeDataTimer->stop();
        failMeasurement("Измерение остановлено пользователем");
    }

    return success;
}

// ===== ОТДЕЛЬНЫЕ МЕТОДЫ ДЛЯ СОВМЕСТИМОСТИ =====

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

bool AMSHandler::setWorkMode(WorkMode mode, AveragingTime avgTime, Litera litera)
{
    QByteArray packet = m_protocol->createModeTransferPacket(mode, avgTime, litera);
    return sendPacket(packet, CMD_MODE_TRANSFER);
}

bool AMSHandler::setStationCoordinates(const StationCoordinates &coords)
{
    QByteArray packet = m_protocol->createCoordsTransferPacket(coords);
    return sendPacket(packet, CMD_COORDS_TRANSFER);
}

bool AMSHandler::setDateTime(const QDateTime &dateTime)
{
    QByteArray packet = m_protocol->createSetDateTimePacket(dateTime);
    return sendPacket(packet, CMD_SET_DATETIME);
}

// ===== УПРАВЛЕНИЕ АНТЕННОЙ =====

bool AMSHandler::openAntenna()
{
    m_rotatePollTimer->stop();
    QByteArray packet = m_protocol->createAntennaControlPacket(0x00);
    bool ok = sendPacket(packet, CMD_ANTENNA_CONTROL);
    if (ok)
        m_antennaPollTimer->start();
    return ok;
}

bool AMSHandler::closeAntenna()
{
    m_rotatePollTimer->stop();
    QByteArray packet = m_protocol->createAntennaControlPacket(0x01);
    bool ok = sendPacket(packet, CMD_ANTENNA_CONTROL);
    if (ok)
        m_antennaPollTimer->start();
    return ok;
}

bool AMSHandler::getAntennaStatus()
{
    QByteArray packet = m_protocol->createAntennaControlPacket(0x02);
    return sendPacket(packet, CMD_ANTENNA_CONTROL);
}

bool AMSHandler::rotateAntenna(float angle)
{
    m_antennaPollTimer->stop();
    QByteArray packet = m_protocol->createRotateAntennaPacket(0x00, angle);
    bool ok = sendPacket(packet, CMD_ROTATE_ANTENNA);
    if (ok)
        m_rotatePollTimer->start();
    return ok;
}

bool AMSHandler::stopAntennaRotation()
{
    m_rotatePollTimer->stop();
    QByteArray packet = m_protocol->createRotateAntennaPacket(0x02, 0.0f);
    return sendPacket(packet, CMD_ROTATE_ANTENNA);
}

// ===== ОТПРАВКА ПАКЕТОВ =====

bool AMSHandler::sendPacket(const QByteArray &packet, AMSCommand command)
{
    if (!m_serialPort->isOpen()) {
        qWarning() << "AMSHandler: Попытка отправки без подключения";
        return false;
    }

    if (m_waitingForResponse &&
        command != CMD_DATA_EXCHANGE &&
        command != CMD_ANTENNA_CONTROL &&
        command != CMD_ROTATE_ANTENNA) {
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

    // Обрабатываем все пакеты в буфере
    while (true) {
        // Ищем начало пакета
        int startIndex = -1;
        for (int i = 0; i < m_receiveBuffer.size(); i++) {
            quint8 byte = static_cast<quint8>(m_receiveBuffer[i]);
            if (byte >= 0xA0 && byte <= 0xAF) { // Расширенный диапазон команд
                startIndex = i;
                break;
            }
        }

        if (startIndex == -1) {
            m_receiveBuffer.clear();
            break;
        }

        if (startIndex > 0) {
            m_receiveBuffer.remove(0, startIndex);
        }

        if (m_receiveBuffer.size() < 3) {
            break;
        }

        quint8 lastByte = static_cast<quint8>(m_receiveBuffer.back());
        if (lastByte != 0xFF) {
            break;
        }

        int stopIndex = m_receiveBuffer.size() - 1;
        QByteArray packet = m_receiveBuffer.left(stopIndex + 1);
        m_receiveBuffer.remove(0, stopIndex + 1);

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
        qWarning() << "AMSHandler: Неверная контрольная сумма пакета";
        emit errorOccurred("Ошибка: неверная контрольная сумма");
        return;
    }

    AMSCommand command = m_protocol->getPacketCommand(packet);
    qDebug() << "AMSHandler: Обработка ответа на команду" << Qt::hex << static_cast<int>(command);

    switch (command) {
    case CMD_LINE_TEST: {
        bool ok = m_protocol->parseLineTestResponse(packet);
        if (ok) {
            qInfo() << "AMSHandler: Тест линии пройден успешно";
            m_isConnecting = false;
            emit connected();
            emit statusMessage("Подключено к АМС");
        } else {
            qWarning() << "AMSHandler: Тест линии не пройден";
            m_isConnecting = false;
            emit errorOccurred("Тест линии не пройден");
        }
        break;
    }

    case CMD_MODE_TRANSFER: {
        bool ok = m_protocol->parseModeTransferResponse(packet);
        if (ok && m_currentStage == STAGE_SEND_MODE) {
            qInfo() << "AMSHandler: Режим работы установлен";
            // Переходим к следующему этапу
            setMeasurementStage(STAGE_SEND_COORDS);
            advanceMeasurementStage();
        }
        break;
    }

    case CMD_COORDS_TRANSFER: {
        bool ok = m_protocol->parseCoordsTransferResponse(packet);
        if (ok && m_currentStage == STAGE_SEND_COORDS) {
            qInfo() << "AMSHandler: Координаты установлены";
            // Переходим к следующему этапу
            setMeasurementStage(STAGE_SEND_DATETIME);
            advanceMeasurementStage();
        }
        break;
    }

    case CMD_SET_DATETIME: {
        bool ok = m_protocol->parseSetDateTimeResponse(packet);
        if (ok && m_currentStage == STAGE_SEND_DATETIME) {
            qInfo() << "AMSHandler: Дата и время установлены";
            // Переходим к старту измерений
            setMeasurementStage(STAGE_START_MEASUREMENT);
            advanceMeasurementStage();
        }
        break;
    }

    case CMD_START_MEASUREMENT: {
        bool ok;
        WorkMode mode = m_protocol->parseStartMeasurementResponse(packet, ok);
        if (ok) {
            qInfo() << "AMSHandler: Измерение запущено, режим:" << mode;
            // Переходим к циклу обмена данными
            setMeasurementStage(STAGE_EXCHANGE_DATA);
            advanceMeasurementStage();
        }
        break;
    }

    case CMD_DATA_EXCHANGE: {
        bool ok;
        MeasurementProgress progress = m_protocol->parseDataExchangeResponse(packet, ok);
        if (ok) {
            processExchangeDataResponse(progress.percentComplete, progress.currentAngle);
        }
        break;
    }

    case CMD_SOURCE_DATA: {
        bool ok = m_protocol->parseSourceDataResponse(packet);
        if (ok) {
            qInfo() << "AMSHandler: Исходные данные приняты АМС";
        }
        break;
    }

    case CMD_MEASURED_WIND_REQUEST: {
        bool ok;
        QVector<MeasuredWindData> data = m_protocol->parseMeasuredWindResponse(packet, ok);
        if (ok) {
            qInfo() << "AMSHandler: Получен профиль измеренного ветра," << data.size() << "точек";
            saveMeasuredWindProfile(m_currentRecordId, data);
            emit measuredWindDataReceived(data);

            // Запрашиваем средний ветер
            QTimer::singleShot(500, this, &AMSHandler::requestAvgWind);
        }
        break;
    }

    case CMD_AVG_WIND_REQUEST: {
        bool ok;
        QVector<WindProfileData> data = m_protocol->parseAvgWindResponse(packet, ok);
        if (ok) {
            qInfo() << "AMSHandler: Получен профиль среднего ветра," << data.size() << "точек";
            saveAvgWindProfile(m_currentRecordId, data);
            emit avgWindDataReceived(data);

            // Запрашиваем действительный ветер
            QTimer::singleShot(500, this, &AMSHandler::requestActualWind);
        }
        break;
    }

    case CMD_ACTUAL_WIND_REQUEST: {
        bool ok;
        QVector<WindProfileData> data = m_protocol->parseActualWindResponse(packet, ok);
        if (ok) {
            qInfo() << "AMSHandler: Получен профиль действительного ветра," << data.size() << "точек";
            saveActualWindProfile(m_currentRecordId, data);
            emit actualWindDataReceived(data);

            // Все данные получены, завершаем процесс
            QTimer::singleShot(500, this, [this]() {
                setMeasurementStage(STAGE_CALCULATE_PROFILE);
                advanceMeasurementStage();
            });
        }
        break;
    }

    case CMD_FUNC_CONTROL: {
        quint32 bitMask, powerOnCount;
        bool ok = m_protocol->parseFuncControlResponse(packet, bitMask, powerOnCount);
        if (ok) {
            FuncControlResult fc = AMSProtocol::funcControlDetails(bitMask);

            if (m_funcControlAfterFailure) {
                // Ответ пришёл как реакция на ошибку измерения (progress == -2).
                // Составляем подробное сообщение о неисправностях и завершаем процесс.
                m_funcControlAfterFailure = false;

                QString reason;
                if (fc.allOk()) {
                    // Битовая маска чистая — оборудование исправно, причина в другом
                    reason = "АМС сообщил об ошибке измерения (функциональный контроль: всё исправно)";
                    qWarning() << "AMSHandler: progress=-2, но ФК не выявил неисправностей";
                } else {
                    // Бит=1 → устройство неисправно; формируем список
                    QStringList parts;
                    if (!fc.faults.isEmpty())
                        parts << "Неисправности: " + fc.faults.join("; ");
                    if (!fc.errors.isEmpty())
                        parts << "Ошибки: " + fc.errors.join("; ");
                    reason = "АМС: ошибка измерения — " + parts.join(" | ");
                    qCritical() << "AMSHandler: Функциональный контроль после ошибки:" << reason;
                }

                // Сохраняем детали в БД (пока m_currentRecordId ещё валиден)
                if (m_currentRecordId > 0) {
                    saveCriticalMessage(m_currentRecordId, reason, "ERROR");
                }

                emit functionalControlDataReceived(bitMask, powerOnCount);
                failMeasurement(reason);

            } else {
                // Плановый запрос функционального контроля (не связан с ошибкой)
                if (fc.allOk()) {
                    emit statusMessage("Функциональный контроль: всё оборудование исправно");
                } else {
                    QStringList all = fc.faults + fc.errors;
                    emit errorOccurred("Функциональный контроль АМС: " + all.join("; "));
                }
                emit functionalControlDataReceived(bitMask, powerOnCount);
            }
        }
        break;
    }

    case CMD_ANTENNA_CONTROL: {
        bool ok;
        quint8 status = m_protocol->parseAntennaControlResponse(packet, ok);
        if (ok) {
            qInfo() << "AMSHandler: Антенна —" << AMSProtocol::antennaStatusString(status);
            // Останавливаем опрос при финальном статусе
            if (status == ANTENNA_SUCCESS || status == ANTENNA_FAULT) {
                m_antennaPollTimer->stop();
                if (status == ANTENNA_FAULT)
                    emit errorOccurred("Антенна: аварийная остановка открытия/закрытия");
            }
            emit antennaStatusReceived(status);
        }
        break;
    }

    case CMD_ROTATE_ANTENNA: {
        quint8 status;
        float currentAngle;
        bool ok = m_protocol->parseRotateAntennaResponse(packet, status, currentAngle);
        if (ok) {
            qInfo() << "AMSHandler: Поворот —" << AMSProtocol::rotateStatusString(status)
                << "угол:" << currentAngle;
            // Останавливаем опрос при финальном статусе
            if (status == ROTATE_IDLE_OK || status == ROTATE_FAULT) {
                m_rotatePollTimer->stop();
                if (status == ROTATE_FAULT)
                    emit errorOccurred("Поворот антенны: аварийная остановка");
            }
            emit rotateStatusReceived(status, currentAngle);
        }
        break;
    }

    default:
        qWarning() << "AMSHandler: Неизвестная команда в ответе:" << Qt::hex << static_cast<int>(command);
        break;
    }
}

void AMSHandler::onAntennaPollTimer()
{
    if (!isConnected()) {
        m_antennaPollTimer->stop();
        return;
    }
    qDebug() << "AMSHandler: Опрос статуса антенны (0xAD, cmd=0x02)";
    getAntennaStatus();
}

void AMSHandler::onRotatePollTimer()
{
    if (!isConnected()) {
        m_rotatePollTimer->stop();
        return;
    }
    // Запрос текущего статуса поворота (0xAF, cmd=0x01 — запрос состояния)
    qDebug() << "AMSHandler: Опрос статуса поворота антенны (0xAF, cmd=0x01)";
    QByteArray packet = m_protocol->createRotateAntennaPacket(0x01, 0.0f);
    sendPacket(packet, CMD_ROTATE_ANTENNA);
}

void AMSHandler::onResponseTimeout()
{
    m_waitingForResponse = false;

    QString error = QString("Таймаут ответа на команду 0x%1")
                        .arg(static_cast<int>(m_lastCommand), 2, 16, QChar('0'));

    qWarning() << "AMSHandler:" << error;

    // Если это критично для процесса измерения
    if (m_measurementStatus == STATUS_RUNNING) {
        failMeasurement("Таймаут ответа от АМС");
    } else {
        emit errorOccurred(error);
    }
}

void AMSHandler::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    QString errorMsg = QString("Ошибка порта: %1").arg(m_serialPort->errorString());
    qCritical() << "AMSHandler:" << errorMsg;
    emit errorOccurred(errorMsg);

    if (m_measurementStatus == STATUS_RUNNING) {
        failMeasurement("Ошибка связи с АМС");
    }
}

// ===== МЕТОДЫ ЗАПИСИ В БД =====

bool AMSHandler::saveMeteo11Bulletin(const QJsonObject &bulletinJson,
                                     const QDateTime   &bulletinTime,
                                     const QString     &validityPeriod)
{
    if (m_currentRecordId <= 0) {
        qWarning() << "AMSHandler::saveMeteo11Bulletin: нет активного record_id, пропускаем";
        return false;
    }
    if (!DatabaseManager::instance()->connect()) {
        qWarning() << "AMSHandler::saveMeteo11Bulletin: нет подключения к БД";
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO meteo_11_bulletin "
        "  (record_id, bulletin_data, bulletin_time, validity_period) "
        "VALUES (:record_id, :data::jsonb, :time, :period) "
        "ON CONFLICT (record_id) DO UPDATE SET "
        "  bulletin_data   = EXCLUDED.bulletin_data, "
        "  bulletin_time   = EXCLUDED.bulletin_time, "
        "  validity_period = EXCLUDED.validity_period"
        );
    query.bindValue(":record_id", m_currentRecordId);
    query.bindValue(":data",      QString::fromUtf8(
                                 QJsonDocument(bulletinJson).toJson(QJsonDocument::Compact)));
    query.bindValue(":time",      bulletinTime);
    query.bindValue(":period",    validityPeriod);

    if (!query.exec()) {
        qWarning() << "AMSHandler::saveMeteo11Bulletin: ошибка БД:"
                   << query.lastError().text();
        return false;
    }

    qInfo() << "AMSHandler: Метео-11 сохранён для record_id=" << m_currentRecordId
            << "время:" << bulletinTime.toString("dd.MM.yyyy HH:mm");
    return true;
}

int AMSHandler::createMainArchiveRecord(const QString &notes)
{
    if (!DatabaseManager::instance()->connect()) return -1;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);

    // ИСПРАВЛЕНО: используем completion_time (название поля в реальной БД)
    query.prepare("INSERT INTO main_archive (completion_time, notes) "
                  "VALUES (NOW(), :notes) RETURNING record_id");
    query.bindValue(":notes", notes.isEmpty() ? QVariant(QVariant::String) : notes);

    if (!query.exec() || !query.next()) {
        QString error = QString("Ошибка создания записи в main_archive: %1")
                            .arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return -1;
    }

    int recordId = query.value(0).toInt();
    qInfo() << "AMSHandler: Создана запись в main_archive с ID:" << recordId;
    return recordId;
}

bool AMSHandler::saveAvgWindProfile(int recordId, const QVector<WindProfileData> &data)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    QDateTime measurementTime = QDateTime::currentDateTime();

    db.transaction();

    // ШАГ 1: Генерируем ОДИН profile_id для всего профиля
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT nextval('avg_wind_profile_profile_id_seq')")) {
        db.rollback();
        QString error = QString("Ошибка генерации profile_id: %1")
                            .arg(idQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    if (!idQuery.next()) {
        db.rollback();
        qCritical() << "AMSHandler: Не удалось получить profile_id";
        return false;
    }

    int profileId = idQuery.value(0).toInt();
    qDebug() << "AMSHandler: Сгенерирован profile_id =" << profileId << "для avg_wind_profile";

    // ШАГ 2: Вставляем ВСЕ точки с ОДНИМ И ТЕМ ЖЕ profile_id
    QSqlQuery query(db);
    query.prepare("INSERT INTO avg_wind_profile "
                  "(profile_id, height, wind_speed, wind_direction, measurement_time) "
                  "VALUES (:profile_id, :height, :speed, :direction, :time)");

    for (int i = 0; i < data.size(); i++) {
        const WindProfileData &point = data[i];

        qDebug() << "AMSHandler: avg_wind точка" << i
                 << "height=" << point.height
                 << "speed=" << point.windSpeed
                 << "direction=" << point.windDirection;

        query.bindValue(":profile_id", profileId);
        query.bindValue(":height", point.height);
        query.bindValue(":speed", point.windSpeed);
        query.bindValue(":direction", point.windDirection);
        query.bindValue(":time", measurementTime);

        if (!query.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки среднего ветра: %1")
                                .arg(query.lastError().text());
            qCritical() << "AMSHandler:" << error;
            emit databaseError(error);
            return false;
        }
    }

    db.commit();

    qInfo() << "AMSHandler: Сохранён профиль среднего ветра:"
            << data.size() << "точек с profile_id =" << profileId;

    // ШАГ 3: Обновляем ссылку в wind_profiles_references (upsert по record_id)
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "INSERT INTO wind_profiles_references (record_id, avg_wind_profile_id) "
        "VALUES (:record_id, :profile_id) "
        "ON CONFLICT (record_id) DO UPDATE SET avg_wind_profile_id = EXCLUDED.avg_wind_profile_id"
        );
    refQuery.bindValue(":record_id", recordId);
    refQuery.bindValue(":profile_id", profileId);
    if (!refQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка upsert avg_wind_profile_id:" << refQuery.lastError().text();
    }

    return true;
}

bool AMSHandler::saveActualWindProfile(int recordId, const QVector<WindProfileData> &data)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    QDateTime measurementTime = QDateTime::currentDateTime();

    db.transaction();

    // ШАГ 1: Генерируем ОДИН profile_id для всего профиля
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT nextval('actual_wind_profile_profile_id_seq')")) {
        db.rollback();
        QString error = QString("Ошибка генерации profile_id: %1")
                            .arg(idQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    if (!idQuery.next()) {
        db.rollback();
        qCritical() << "AMSHandler: Не удалось получить profile_id";
        return false;
    }

    int profileId = idQuery.value(0).toInt();
    qDebug() << "AMSHandler: Сгенерирован profile_id =" << profileId << "для actual_wind_profile";

    // ШАГ 2: Вставляем ВСЕ точки с ОДНИМ И ТЕМ ЖЕ profile_id
    QSqlQuery query(db);
    query.prepare("INSERT INTO actual_wind_profile "
                  "(profile_id, height, wind_speed, wind_direction, measurement_time) "
                  "VALUES (:profile_id, :height, :speed, :direction, :time)");

    for (int i = 0; i < data.size(); i++) {
        const WindProfileData &point = data[i];

        qDebug() << "AMSHandler: actual_wind точка" << i
                 << "height=" << point.height
                 << "speed=" << point.windSpeed
                 << "direction=" << point.windDirection;

        query.bindValue(":profile_id", profileId);
        query.bindValue(":height", point.height);
        query.bindValue(":speed", point.windSpeed);
        query.bindValue(":direction", point.windDirection);
        query.bindValue(":time", measurementTime);

        if (!query.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки действительного ветра: %1")
                                .arg(query.lastError().text());
            qCritical() << "AMSHandler:" << error;
            emit databaseError(error);
            return false;
        }
    }

    db.commit();

    qInfo() << "AMSHandler: Сохранён профиль действительного ветра:"
            << data.size() << "точек с profile_id =" << profileId;

    // ШАГ 3: Обновляем ссылку в wind_profiles_references (upsert по record_id)
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "INSERT INTO wind_profiles_references (record_id, actual_wind_profile_id) "
        "VALUES (:record_id, :profile_id) "
        "ON CONFLICT (record_id) DO UPDATE SET actual_wind_profile_id = EXCLUDED.actual_wind_profile_id"
        );
    refQuery.bindValue(":record_id", recordId);
    refQuery.bindValue(":profile_id", profileId);
    if (!refQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка upsert actual_wind_profile_id:" << refQuery.lastError().text();
    }

    return true;
}

bool AMSHandler::saveMeasuredWindProfile(int recordId, const QVector<MeasuredWindData> &data)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    QDateTime measurementTime = QDateTime::currentDateTime();

    db.transaction();

    // ШАГ 1: Генерируем ОДИН profile_id для всего профиля
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT nextval('measured_wind_profile_profile_id_seq')")) {
        db.rollback();
        QString error = QString("Ошибка генерации profile_id: %1")
                            .arg(idQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        emit databaseError(error);
        return false;
    }

    if (!idQuery.next()) {
        db.rollback();
        qCritical() << "AMSHandler: Не удалось получить profile_id";
        return false;
    }

    int profileId = idQuery.value(0).toInt();
    qDebug() << "AMSHandler: Сгенерирован profile_id =" << profileId << "для measured_wind_profile";

    // ШАГ 2: Вставляем ВСЕ точки с ОДНИМ И ТЕМ ЖЕ profile_id
    QSqlQuery query(db);
    query.prepare("INSERT INTO measured_wind_profile "
                  "(profile_id, height, wind_speed, wind_direction, measurement_time) "
                  "VALUES (:profile_id, :height, :speed, :direction, :time)");

    for (const MeasuredWindData &point : data) {
        query.bindValue(":profile_id", profileId);  // ОДИН И ТОТ ЖЕ!
        query.bindValue(":height", point.height);
        query.bindValue(":speed", point.windSpeed);
        query.bindValue(":direction", point.windDirection);
        query.bindValue(":time", measurementTime);

        if (!query.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки измеренного ветра: %1")
                                .arg(query.lastError().text());
            qCritical() << "AMSHandler:" << error;
            emit databaseError(error);
            return false;
        }
    }

    db.commit();

    qInfo() << "AMSHandler: Сохранён профиль измеренного ветра:"
            << data.size() << "точек с profile_id =" << profileId;

    // ШАГ 3: Обновляем ссылку в wind_profiles_references (upsert по record_id)
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "INSERT INTO wind_profiles_references (record_id, measured_wind_profile_id) "
        "VALUES (:record_id, :profile_id) "
        "ON CONFLICT (record_id) DO UPDATE SET measured_wind_profile_id = EXCLUDED.measured_wind_profile_id"
        );
    refQuery.bindValue(":record_id", recordId);
    refQuery.bindValue(":profile_id", profileId);
    if (!refQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка upsert measured_wind_profile_id:" << refQuery.lastError().text();
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