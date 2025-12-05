#include "GroundMeteoParams.h"
#include "ui_GroundMeteoParams.h"
#include <QDebug>
#include <QtMath>

GroundMeteoParams::GroundMeteoParams(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint)
    , ui(new Ui::GroundMeteoParams)
    , m_serialPort(nullptr)
    , m_protocol(UMB_PROTOCOL)
    , m_deviceAddress(0x70)
{
    ui->setupUi(this);

    connect(ui->btnGroundParamsClose, &QPushButton::clicked, this, [this](){
        close();
    });
    connect(ui->btnGroundParamsClear, &QPushButton::clicked, this, &GroundMeteoParams::deleteDataFromTable);
}

GroundMeteoParams::~GroundMeteoParams()
{
    delete ui;
}

void GroundMeteoParams::deleteDataFromTable()
{
    int columnToClear = 1;
    int rowCount = ui->tableWidget_GroundParams->rowCount();

    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem *item = ui->tableWidget_GroundParams->item(row, columnToClear);
        if (item) {
            item->setText("");
        }
    }
}

void GroundMeteoParams::setSerialPort(QSerialPort* port)
{
    m_serialPort = port;
}

void GroundMeteoParams::setProtocol(RS485Protocol protocol)
{
    m_protocol = protocol;
}

void GroundMeteoParams::setDeviceAddress(quint8 address)
{
    m_deviceAddress = address;
}

void GroundMeteoParams::requestData(const QList<quint16>& parameters)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        emit errorOccurred("Serial port is not open");
        return;
    }

    QByteArray request;

    if (m_protocol == MODBUS_RTU) {
        request = createModbusReadRequest(parameters);
    } else if (m_protocol == UMB_PROTOCOL) {
        request = createUmbReadRequest(parameters);
    }

    if (!request.isEmpty()) {
        qint64 written = m_serialPort->write(request);
        if (written == -1) {
            emit errorOccurred("Failed to write to serial port");
        } else {
            qDebug() << "Request sent:" << request.toHex(' ');
        }
    }
}

void GroundMeteoParams::onDataReceived(const QByteArray& data)
{
    m_receiveBuffer.append(data);

    QMap<QString, double> values;
    bool parseSuccess = false;

    if (m_protocol == MODBUS_RTU) {
        // Для MODBUS проверяем минимальный размер пакета
        if (m_receiveBuffer.size() >= 5) {
            parseSuccess = parseModbusResponse(m_receiveBuffer, values);
            if (parseSuccess) {
                m_receiveBuffer.clear();
            }
        }
    } else if (m_protocol == UMB_PROTOCOL) {
        // Для UMB ищем полный пакет (от SOH до EOT)
        int sohPos = m_receiveBuffer.indexOf(0x01);
        int eotPos = m_receiveBuffer.indexOf(0x04, sohPos);

        if (sohPos >= 0 && eotPos > sohPos) {
            QByteArray packet = m_receiveBuffer.mid(sohPos, eotPos - sohPos + 1);
            parseSuccess = parseUmbResponse(packet, values);

            if (parseSuccess) {
                m_receiveBuffer.remove(0, eotPos + 1);
            }
        }

        // Очищаем буфер если он слишком большой
        if (m_receiveBuffer.size() > 1024) {
            m_receiveBuffer.clear();
        }
    }

    if (parseSuccess && !values.isEmpty()) {
        updateTableWithData(values);
        emit dataUpdated(values);
    }
}

void GroundMeteoParams::updateTableWithData(const QMap<QString, double>& values)
{
    for (auto it = values.begin(); it != values.end(); ++it) {
        QString rowName = mapParameterToTableRow(it.key());

        if (rowName.isEmpty()) {
            continue;
        }

        // Ищем строку в таблице
        int rowCount = ui->tableWidget_GroundParams->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            QTableWidgetItem* paramItem = ui->tableWidget_GroundParams->item(row, 0);
            if (paramItem && paramItem->text().contains(rowName, Qt::CaseInsensitive)) {
                // Обновляем значение
                QTableWidgetItem* valueItem = ui->tableWidget_GroundParams->item(row, 1);
                if (!valueItem) {
                    valueItem = new QTableWidgetItem();
                    ui->tableWidget_GroundParams->setItem(row, 1, valueItem);
                }
                valueItem->setText(QString::number(it.value(), 'f', 2));
                break;
            }
        }
    }

    qDebug() << "Table updated with" << values.size() << "parameters";
}

QString GroundMeteoParams::mapParameterToTableRow(const QString& paramName)
{
    // Маппинг названий параметров на строки таблицы
    if (paramName == "Wind Speed") return "скорость ветра";
    if (paramName == "Wind Direction") return "направление ветра";
    if (paramName == "Pressure") return "давление";
    if (paramName == "Humidity") return "влажность";
    if (paramName == "Temperature") return "температура";

    return QString();
}

QByteArray GroundMeteoParams::createModbusReadRequest(const QList<quint16>& parameters)
{
    QByteArray request;
    request.append(m_deviceAddress);
    request.append(0x03); // Read holding registers

    quint16 startAddr = parameters.first();
    request.append(static_cast<char>((startAddr >> 8) & 0xFF));
    request.append(static_cast<char>(startAddr & 0xFF));

    quint16 regCount = parameters.size();
    request.append(static_cast<char>((regCount >> 8) & 0xFF));
    request.append(static_cast<char>(regCount & 0xFF));

    quint16 crc = calculateModbusCRC16(request);
    request.append(static_cast<char>(crc & 0xFF));
    request.append(static_cast<char>((crc >> 8) & 0xFF));

    return request;
}

QByteArray GroundMeteoParams::createUmbReadRequest(const QList<quint16>& parameters)
{
    QByteArray request;

    request.append(0x01); // SOH
    request.append(0x10); // ver
    request.append(0x01); // to (high byte)
    request.append(0x70); // to (low byte) - device address
    request.append(0x01); // from (high byte)
    request.append(0xF0); // from (low byte) - data collector address

    int dataLength = 3 + (parameters.size() * 2);
    request.append(static_cast<char>(dataLength));

    request.append(0x02); // STX
    request.append(0x2F); // cmd (read parameters)
    request.append(0x10); // verc
    request.append(static_cast<char>(parameters.size()));

    for (int i = 0; i < parameters.size(); ++i) {
        quint16 param = parameters[i];
        request.append(static_cast<char>(param & 0xFF));
        request.append(static_cast<char>((param >> 8) & 0xFF));
    }

    request.append(0x03); // ETX

    quint16 crc = calculateCRC16(request);
    request.append(static_cast<char>(crc & 0xFF));
    request.append(static_cast<char>((crc >> 8) & 0xFF));

    request.append(0x04); // EOT

    qDebug() << "UMB Request created:" << request.toHex(' ');

    return request;
}

bool GroundMeteoParams::parseModbusResponse(const QByteArray& response, QMap<QString, double>& values)
{
    if (response.size() < 5) return false;

    quint8 addr = static_cast<quint8>(response[0]);
    quint8 func = static_cast<quint8>(response[1]);
    quint8 byteCount = static_cast<quint8>(response[2]);

    if (func != 0x03) return false;

    for (int i = 0; i < byteCount; i += 4) {
        if (i + 3 < byteCount) {
            union {
                float f;
                quint32 i;
            } converter;

            converter.i = (static_cast<quint32>(response[3 + i]) << 24) |
                         (static_cast<quint32>(response[4 + i]) << 16) |
                         (static_cast<quint32>(response[5 + i]) << 8) |
                         static_cast<quint32>(response[6 + i]);

            switch (i / 4) {
                case 0: values["Temperature"] = converter.f; break;
                case 1: values["Humidity"] = converter.f; break;
                case 2: values["Pressure"] = converter.f; break;
                default: break;
            }
        }
    }

    return !values.isEmpty();
}

bool GroundMeteoParams::parseUmbResponse(const QByteArray& response, QMap<QString, double>& values)
{
    if (response.size() < 12) {
        emit errorOccurred("UMB response too short");
        return false;
    }

    if (response[0] != 0x01 || response[1] != 0x10 ||
        response[response.size()-1] != 0x04) {
        emit errorOccurred("Invalid UMB packet structure");
        return false;
    }

    qDebug() << "UMB Response hex:" << response.toHex(' ');

    quint8 dataLength = static_cast<quint8>(response[6]);
    int stxPos = 7;

    if (stxPos >= response.size() || response[stxPos] != 0x02) {
        emit errorOccurred("STX not found at expected position");
        return false;
    }

    int etxPos = stxPos + 1 + dataLength;
    if (etxPos >= response.size() || response[etxPos] != 0x03) {
        emit errorOccurred("ETX not found at expected position");
        return false;
    }

    // Проверка CRC
    QByteArray crcData = response.left(etxPos + 1);
    quint16 expectedCrc = calculateCRC16(crcData);

    int crcPos = etxPos + 1;
    if (crcPos + 1 >= response.size()) {
        emit errorOccurred("CRC data not found");
        return false;
    }

    quint16 receivedCrc = static_cast<quint8>(response[crcPos]) |
            (static_cast<quint8>(response[crcPos + 1]) << 8);

    qDebug() << "Expected CRC:" << QString("0x%1").arg(expectedCrc, 4, 16, QChar('0'));
    qDebug() << "Received CRC:" << QString("0x%1").arg(receivedCrc, 4, 16, QChar('0'));

    if (expectedCrc != receivedCrc) {
        emit errorOccurred(QString("CRC mismatch: expected 0x%1, got 0x%2")
                           .arg(expectedCrc, 4, 16, QChar('0'))
                           .arg(receivedCrc, 4, 16, QChar('0')));
        return false;
    }

    // Парсинг данных
    int dataPos = stxPos + 1;

    if (dataPos + 3 >= etxPos) {
        emit errorOccurred("Invalid data section in UMB response");
        return false;
    }

    quint8 cmd = static_cast<quint8>(response[dataPos++]);
    quint8 verc = static_cast<quint8>(response[dataPos++]);
    quint8 status = static_cast<quint8>(response[dataPos++]);
    quint8 paramCount = static_cast<quint8>(response[dataPos++]);

    qDebug() << "CMD:" << cmd << "VERC:" << verc << "Status:" << status
             << "Parameter count:" << paramCount;

    for (int i = 0; i < paramCount && dataPos < etxPos; i++) {
        if (dataPos + 4 >= etxPos) {
            qDebug() << "Not enough data for parameter" << i;
            break;
        }

        quint8 paramLength = static_cast<quint8>(response[dataPos]);
        quint8 errorCode = static_cast<quint8>(response[dataPos + 1]);

        quint16 paramCode = static_cast<quint8>(response[dataPos + 2]) |
                (static_cast<quint8>(response[dataPos + 3]) << 8);

        qDebug() << "Parameter" << i << ": length=" << paramLength
                 << "error=" << errorCode << "code=" << paramCode;

        if (errorCode == 0 && paramLength >= 5 && dataPos + paramLength < etxPos) {
            quint8 dataType = static_cast<quint8>(response[dataPos+4]);

            if (dataType == 0x16) { // float
                if(paramLength >= 8) {
                    union {
                        float f;
                        quint32 i;
                    } converter;

                    converter.i = static_cast<quint32>(static_cast<quint8>(response[dataPos + 5])) |
                            (static_cast<quint32>(static_cast<quint8>(response[dataPos + 6])) << 8) |
                            (static_cast<quint32>(static_cast<quint8>(response[dataPos + 7])) << 16) |
                            (static_cast<quint32>(static_cast<quint8>(response[dataPos + 8])) << 24);

                    if (qIsFinite(converter.f) && !qIsNaN(converter.f)) {
                        QString paramName = parameterCodeToName(paramCode);
                        if (!paramName.isEmpty()) {
                            values[paramName] = static_cast<double>(converter.f);
                            qDebug() << "Added float parameter:" << paramName << "=" << converter.f;
                        }
                    }
                }
            } else if (dataType == 0x10) { // uint8_t
                if (paramLength >= 5) {
                    quint8 value = static_cast<quint8>(response[dataPos + 5]);
                    QString paramName = parameterCodeToName(paramCode);
                    if (!paramName.isEmpty()) {
                        values[paramName] = static_cast<double>(value);
                        qDebug() << "Added uint8 parameter:" << paramName << "=" << value;
                    }
                }
            }
        }

        dataPos += paramLength + 1;
    }

    qDebug() << "Total parsed parameters:" << values.size();

    return !values.isEmpty();
}

quint16 GroundMeteoParams::calculateCRC16(const QByteArray& data)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < data.size(); i++) {
        quint8 byte = static_cast<quint8>(data[i]);

        for (int j = 0; j < 8; j++) {
            quint16 x16;
            if ((crc & 0x0001) ^ (byte & 0x01)) {
                x16 = 0x8408;
            } else {
                x16 = 0x0000;
            }
            crc = crc >> 1;
            crc ^= x16;
            byte = byte >> 1;
        }
    }

    return crc;
}

quint16 GroundMeteoParams::calculateModbusCRC16(const QByteArray& data)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < data.size(); i++) {
        crc ^= static_cast<quint8>(data[i]);

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

QString GroundMeteoParams::parameterCodeToName(quint16 code)
{
    switch (code) {
        case 0x0064: return "Temperature";
        case 0x00C8: return "Humidity";
        case 0x012C: return "Pressure";
        case 0x0190: return "Wind Speed";
        case 0x01F4: return "Wind Direction";
        case 0x0258: return "Precipitation";
        case 0x02BC: return "Precipitation Type";
        case 0x0334: return "CO2";
        case 0x038E: return "Acceleration Std Dev";
        case 0x0393: return "Magnetic Induction";
        case 0x0398: return "Yaw Angle";
        case 0x0481: return "Precipitation Intensity";
        case 0x2712: return "Supply Voltage";
        case 0x4E20: return "Status";
        default:
            qDebug() << "Unknown parameter code:" << code;
            return QString("Parameter_%1").arg(code);
    }
}
