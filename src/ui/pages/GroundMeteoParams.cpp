#include "ui/pages/GroundMeteoParams.h"
#include "ui_GroundMeteoParams.h"
#include "ui/widgets/VirtualKeyboard.h"
#include "ui/theme/ScreenTheme.h"
#include "devices/iws/IwsProtocolCodec.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QHeaderView>
#include <QPointer>

GroundMeteoParams* GroundMeteoParams::s_instance = nullptr;

static constexpr double kHpaToMmHg = 0.750064;

// ── Формат ввода для 5 строк таблицы (индекс = номер строки) ──────────────
// Только формат (знак/десятичные разряды) — БЕЗ ограничения диапазона
// значений: специально не ограничиваем min/max, чтобы не мешать вводу
// реальных показаний.
namespace {
struct RowFormat { int decimals; bool allowNegative; };
constexpr RowFormat kRowFormat[5] = {
    { 1, false },  // 0: скорость ветра, м/с
    { 0, false },  // 1: направление ветра, град
    { 1, false },  // 2: давление, мм рт. ст.
    { 0, false },  // 3: относительная влажность, %
    { 1, true  },  // 4: температура, °C
};
}

// ─────────────────────────────────────────────────────────────────────────
// GroundParamValueDelegate
// ─────────────────────────────────────────────────────────────────────────

GroundParamValueDelegate::GroundParamValueDelegate(QTableWidget *table, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_table(table)
{
}

QWidget* GroundParamValueDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                                 const QModelIndex &index) const
{
    auto *editor = new QLineEdit(parent);
    // Выравнивание по левому краю: вводимый текст растёт вправо, внутрь ячейки,
    // а не прижимается к правой границе таблицы (на узком экране планшета
    // правое выравнивание выталкивало значение за границу). Выравнивание
    // редактора и закрытой ячейки обязано совпадать — см. конструктор
    // GroundMeteoParams, где то же выравнивание ставится самим item'ам.
    editor->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    editor->setFont(option.font);

    editor->setAutoFillBackground(true);
    editor->setStyleSheet("background-color: #FFFFFF; border: none; padding: 0 4px;");

    const int row = index.row();
    if (row >= 0 && row < 5) {
        const RowFormat &fmt = kRowFormat[row];
        VirtualKeyboard::Constraints c;
        c.allowNegative   = fmt.allowNegative;
        c.allowDecimal    = fmt.decimals > 0;
        c.maxDecimals     = fmt.decimals;
        c.maxLength       = 7;
        c.allowModeSwitch = false;   // строго числовые поля — переключение на буквы не нужно
        // Диапазон значений (minValue/maxValue) намеренно не ограничиваем.
        VirtualKeyboard::attach(editor, VirtualKeyboard::Mode::Numeric, c);
    }

    // Запоминаем, какая ячейка сейчас редактируется — см. paint().
    m_editingIndex = index;

    return editor;
}

void GroundParamValueDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QStyledItemDelegate::setEditorData(editor, index);

    if (auto *le = qobject_cast<QLineEdit*>(editor))
        le->selectAll();
}

void GroundParamValueDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
    if (auto *le = qobject_cast<QLineEdit*>(editor))
        VirtualKeyboard::detach(le);
    if (m_editingIndex == index)
        m_editingIndex = QPersistentModelIndex();
    QStyledItemDelegate::destroyEditor(editor, index);
    // Ячейка снова "закрыта" — её собственный текст опять должен рисоваться
    // обычным порядком (см. paint()). Таблица маленькая (5 строк) — просто
    // просим перерисовать весь viewport целиком, не вычисляя rect ячейки.
    if (m_table)
        m_table->viewport()->update();
}

void GroundParamValueDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                                     const QModelIndex &index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);

    // Принудительно перерисовываем область ячейки сразу после того, как
    // редактор занял её геометрию. Без этого при повторном редактировании
    // уже введённого значения на миг остаётся видна прежняя (закрытая)
    // отрисовка ячейки узкой полоской из-под нового редактора — старое
    // значение "просвечивает" слева до следующего перерисовывания.
    if (m_table)
        m_table->viewport()->update(option.rect);
}

void GroundParamValueDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    // Пока для этой ячейки открыт редактор, он и так полностью лежит поверх
    // и рисует актуальный текст сам — если вдобавок отрисовать здесь ещё и
    // ПРЕЖНИЙ текст самого item'а (обычно Qt не должен, но на некоторых
    // платформах/стилях всё же успевает), у краёв редактора остаётся видна
    // узкая полоска старого значения из-под нового. Поэтому во время
    // редактирования текст item'а просто не рисуем.
    if (index != m_editingIndex)
        QStyledItemDelegate::paint(painter, option, index);

    if (index.data(GroundMeteoParams::kInvalidRole).toBool()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPen pen(QColor("#C62828"));
        pen.setWidthF(1.6);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        const QRectF r = QRectF(option.rect).adjusted(2, 2, -2, -2);
        painter->drawRoundedRect(r, 4, 4);
        painter->restore();
    }
}

GroundMeteoParams::GroundMeteoParams(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GroundMeteoParams)
    , m_protocol(MODBUS_RTU)
    , m_deviceAddress(0x70)
    , m_lastWindSpeed(0.0)
    , m_lastWindDirection(0.0)
    , m_hasLastData(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);

    s_instance = this;

    qDebug() << "GroundMeteoParams initialized with Modbus RTU protocol, device address 0x01";

    connect(ui->btnGroundParamsClose, &QPushButton::clicked, this, [this](){
        if (!confirmDiscardIfDirty())
            return; // оператор нажал "Отмена" — остаёмся на странице
        VirtualKeyboard::hideKeyboard(); // на случай, если ячейка ещё редактировалась
        emit backRequested();
    });
    connect(ui->btnGroundParamsClear, &QPushButton::clicked, this, &GroundMeteoParams::deleteDataFromTable);
    connect(ui->btnGroundParamsApply, &QPushButton::clicked, this, &GroundMeteoParams::applyManualInput);

    // Делаем колонку значений редактируемой
    QTableWidget *table = ui->tableWidget_GroundParams;
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *item = table->item(row, 1);
        if (!item) {
            item = new QTableWidgetItem("");
            table->setItem(row, 1, item);
        }
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        // Выравнивание по левому краю — обязательно ТАКОЕ ЖЕ, как у редактора
        // (см. GroundParamValueDelegate::createEditor). Если выравнивание
        // ячейки и редактора расходятся, на некоторых платформах старое
        // значение на миг остаётся видно из-под нового редактора, пока тот не
        // перерисуется поверх. По правому краю не выравниваем: на узком экране
        // планшета значение прижималось к правой границе таблицы и выходило
        // за неё.
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    // Ширину колонок раскладывает сам QHeaderView, без абсолютных пикселей:
    //   "Параметр" — ResizeToContents: ровно под текст подписей;
    //   "Значение" — Stretch: занимает всё оставшееся место.
    // Так таблица одинаково корректно раскладывается и на широком экране
    // ноутбука, и на узком экране планшета при любом системном масштабе.
    // Оба режима пересчитываются от ширины viewport на каждом проходе
    // раскладки и не хранят "пользовательской" ширины секции — поэтому
    // "сдвинуть" колонку и оставить так делегат или экранная клавиатура
    // не могут в принципе, и сумма ширин колонок всегда равна ширине
    // viewport (горизонтальной прокрутке взяться неоткуда).
    QHeaderView *hHeader = table->horizontalHeader();
    hHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(1, QHeaderView::Stretch);

    // Валидированный редактор + автопривязка экранной клавиатуры для ячеек
    // столбца "Значение" (см. GroundParamValueDelegate выше).
    table->setItemDelegateForColumn(1, new GroundParamValueDelegate(table, table));

    // Отслеживаем ручные правки ячеек — для m_dirty (подтверждение при закрытии).
    // Программные записи в таблицу обёрнуты в m_suppressDirty, чтобы не путать.
    connect(table, &QTableWidget::itemChanged,
            this, &GroundMeteoParams::onTableItemChanged);

    // Таймер устаревания. SingleShot — перезапускаем restartStaleTimer() при
    // каждом применении (и ручном, и от IWS). По истечении → состояние Stale.
    m_staleTimer = new QTimer(this);
    m_staleTimer->setSingleShot(true);
    connect(m_staleTimer, &QTimer::timeout,
            this, &GroundMeteoParams::onStaleTimerTimeout);

    applyVisualStyle();
}

GroundMeteoParams::~GroundMeteoParams()
{
    qDebug() << "GroundMeteoParams destructor";
    if (s_instance == this) {
        s_instance = nullptr;
    }
    delete ui;
}

// Общая логика подтверждения при уходе со страницы с неприменёнными правками
// (используется и кнопкой "Закрыть", и closeEvent как защитный запасной путь).
bool GroundMeteoParams::confirmDiscardIfDirty()
{
    if (!m_dirty)
        return true;

    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Несохранённые изменения");
    box.setText("В таблице есть изменения, не применённые кнопкой «Применить».");
    box.setInformativeText("Применить их перед закрытием?");
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Yes);
    box.button(QMessageBox::Yes)->setText("Применить");
    box.button(QMessageBox::No)->setText("Не применять");
    box.button(QMessageBox::Cancel)->setText("Отмена");

    const int ret = box.exec();
    if (ret == QMessageBox::Cancel)
        return false;

    if (ret == QMessageBox::Yes) {
        applyManualInput();   // применяем — m_dirty сбросится внутри
    } else {
        // "Не применять" — просто отмечаем как не-dirty, изменения теряются
        m_dirty = false;
    }
    return true;
}

// closeEvent оставлен как защитный запасной путь на случай, если что-то
// всё же вызовет close() программно — в обычной работе страницы этот путь
// не используется, уход "назад" идёт через кнопку "Закрыть" (см. конструктор).
void GroundMeteoParams::closeEvent(QCloseEvent *event)
{
    if (!confirmDiscardIfDirty()) {
        event->ignore();
        return;
    }

    VirtualKeyboard::hideKeyboard();
    QWidget::closeEvent(event);
}

GroundMeteoParams* GroundMeteoParams::instance()
{
    return s_instance;
}

// ── Протокол ИВС ────────────────────────────────────────────────────────────
// Сборка запросов и разбор ответов Modbus/UMB переехали в
// devices/iws/IwsProtocolCodec: работа с байтами на линии RS-485 — не дело
// страницы интерфейса. Здесь остались тонкие обёртки. Они подставляют то,
// что принадлежит именно виджету, — адрес устройства и список последних
// запрошенных регистров, — и эмитят errorOccurred(), которого у свободных
// функций кодека быть не может.

QByteArray GroundMeteoParams::createModbusReadRequest(const QList<quint16>& parameters)
{
    // СОХРАНЯЕМ запрошенные регистры для парсинга
    if (!parameters.isEmpty())
        m_lastRequestedRegisters = parameters;

    return IwsProtocolCodec::createModbusReadRequest(m_deviceAddress, parameters);
}

QByteArray GroundMeteoParams::createUmbReadRequest(const QList<quint16>& parameters)
{
    return IwsProtocolCodec::createUmbReadRequest(parameters);
}

bool GroundMeteoParams::parseUmbResponse(const QByteArray& response,
                                         QMap<QString, double>& values)
{
    QString errorMsg;
    const bool ok = IwsProtocolCodec::parseUmbResponse(response, values, errorMsg);
    if (!errorMsg.isEmpty())
        emit errorOccurred(errorMsg);
    return ok;
}

bool GroundMeteoParams::parseModbusResponseWithMapping(
    const QByteArray& response,
    const QList<quint16>& requestedRegisters,
    QMap<QString, double>& values)
{
    QString errorMsg;
    const bool ok = IwsProtocolCodec::parseModbusResponseWithMapping(
        m_deviceAddress, response, requestedRegisters, values, errorMsg);
    if (!errorMsg.isEmpty())
        emit errorOccurred(errorMsg);
    return ok;
}



void GroundMeteoParams::deleteDataFromTable()
{
    int columnToClear = 1;
    int rowCount = ui->tableWidget_GroundParams->rowCount();

    // Программная очистка ячеек — itemChanged не должен пометить m_dirty
    m_suppressDirty = true;
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem *item = ui->tableWidget_GroundParams->item(row, columnToClear);
        if (item) item->setText("");
    }
    m_suppressDirty = false;

    // Сбрасываем кеш ветра
    m_lastWindSpeed    = 0.0;
    m_lastWindDirection = 0.0;
    m_hasLastData      = false;

    // Сбрасываем все 5 флагов готовности
    m_hasSpeed = m_hasDirection = m_hasPressure = m_hasHumidity = m_hasTemperature = false;

    // Останавливаем таймер устаревания
    if (m_staleTimer) m_staleTimer->stop();

    // Сбрасываем dirty (после очистки правок "нет")
    m_dirty = false;
    m_lastUpdateWasManual = false;

    // Снимаем подсветку "поле не заполнено" со всех строк
    for (int row = 0; row < rowCount; ++row)
        setRowInvalid(row, false);
    if (m_rowHintLabel) m_rowHintLabel->hide();

    // Пересчёт состояния → NoData → сигнал
    recomputeSurfaceState();
}

void GroundMeteoParams::applyManualInput()
{
    QTableWidget *table = ui->tableWidget_GroundParams;

    // Универсальный парсер ячейки: пусто → невалидно; "12,3"/"12.3" → 12.3.
    auto readRow = [&](int row) -> std::pair<bool, double> {
        QTableWidgetItem *item = table->item(row, 1);
        if (!item || item->text().trimmed().isEmpty())
            return {false, 0.0};
        bool ok = false;
        double val = item->text().trimmed().replace(',', '.').toDouble(&ok);
        return {ok, val};
    };

    auto [speedOk, speed] = readRow(0);
    auto [dirOk,   dir]   = readRow(1);
    auto [presOk,  pres]  = readRow(2);   // ВРУЧНУЮ — уже мм рт.ст.
    auto [humOk,   hum]   = readRow(3);
    auto [tempOk,  temp]  = readRow(4);

    // ── Видимая подсветка незаполненных строк ────────────────────────────
    // Раньше при неполном вводе ничего не происходило видимо для оператора
    // (только qWarning в консоль). Теперь каждая незаполненная строка сразу
    // получает красную рамку в колонке "Значение" и подсвеченное красным имя
    // параметра; у первой из них дополнительно всплывает подпись-подсказка.
    const bool rowOk[5] = { speedOk, dirOk, presOk, humOk, tempOk };
    int firstInvalidRow = -1;
    for (int row = 0; row < 5; ++row) {
        setRowInvalid(row, !rowOk[row]);
        if (!rowOk[row] && firstInvalidRow < 0)
            firstInvalidRow = row;
    }
    if (firstInvalidRow >= 0) {
        showRowHint(firstInvalidRow, "Поле не заполнено");
        table->setCurrentCell(firstInvalidRow, 1);
        table->scrollToItem(table->item(firstInvalidRow, 1));
    }

    // Обновляем флаги готовности по каждому параметру независимо
    m_hasSpeed       = speedOk;
    m_hasDirection   = dirOk;
    m_hasPressure    = presOk;
    m_hasHumidity    = humOk;
    m_hasTemperature = tempOk;

    // Кеш ветра — для совместимости с существующим кодом, который читает
    // lastWindSpeed()/lastWindDirection() (например, runWindProfileCalculation).
    if (speedOk) m_lastWindSpeed     = speed;
    if (dirOk)   m_lastWindDirection = dir;
    m_hasLastData = speedOk || dirOk;

    qDebug() << "GroundMeteoParams::applyManualInput: speed=" << speed << "(" << speedOk << ")"
             << "dir=" << dir << "(" << dirOk << ")"
             << "pres=" << pres << "(" << presOk << ") [мм рт.ст.]"
             << "hum=" << hum << "(" << humOk << ")"
             << "temp=" << temp << "(" << tempOk << ")";

    // Собираем values с ключами, которые понимает остальной код
    // (SurfaceMeteoSaver, MainWindow и т.д.).
    QMap<QString, double> values;
    if (speedOk) values["Wind Speed Avg"]     = speed;
    if (dirOk)   values["Wind Direction Avg"] = dir;
    if (presOk)  values["Pressure Avg"]       = pres;     // уже мм рт.ст.
    if (humOk)   values["Humidity Avg"]       = hum;
    if (tempOk)  values["Temperature Avg"]    = temp;

    if (values.isEmpty()) {
        qWarning() << "GroundMeteoParams: нет корректных данных для применения";
        // Даже если ничего не введено — состояние пересчитать надо
        // (может стать NoData, если до этого было применено).
        recomputeSurfaceState();
        return;
    }

    // Кнопка "Применить" нажата → правки считаются применёнными
    m_dirty = false;

    // Это был РУЧНОЙ ввод (кнопка "Применить"), а не приём от ИВС — see
    // lastUpdateWasManual(). MainWindow использует это для жёлтой подсветки
    // плашки "ИВС".
    m_lastUpdateWasManual = true;

    // Перезапускаем таймер 30 мин — данные свежие
    restartStaleTimer();

    // Пересчитываем состояние и эмитим, если изменилось
    recomputeSurfaceState();

    // Оповещаем подписчиков (SurfaceMeteoSaver и т.д.)
    emit dataUpdated(values);
}

void GroundMeteoParams::setProtocol(RS485Protocol protocol)
{
    m_protocol = protocol;
    qDebug() << "Protocol set to:" << (protocol == UMB_PROTOCOL ? "UMB" : "MODBUS");
}

void GroundMeteoParams::setDeviceAddress(quint8 address)
{
    m_deviceAddress = address;
    qDebug() << "Device address set to:" << address << "(0x" << QString::number(address, 16) << ")";
}

void GroundMeteoParams::onDataReceived(const QByteArray& data)
{
    qDebug() << "=== GroundMeteoParams::onDataReceived ===";
    qDebug() << "Received" << data.size() << "bytes:" << data.toHex(' ');

    m_receiveBuffer.append(data);
    qDebug() << "Buffer now contains" << m_receiveBuffer.size() << "bytes";

    QMap<QString, double> values;
    bool parseSuccess = false;

    if (m_protocol == MODBUS_RTU) {
        qDebug() << "Parsing as MODBUS RTU";
        // Для MODBUS проверяем минимальный размер пакета
        if (m_receiveBuffer.size() >= 7) {
            quint8 byteCount = static_cast<quint8>(m_receiveBuffer[2]);
            int expectedSize = 3 + byteCount + 2;

            qDebug() << "Expected Modbus packet size:" << expectedSize;
            qDebug() << "Current buffer size:" << m_receiveBuffer.size();

            if (m_receiveBuffer.size() >= expectedSize) {
                QByteArray packet = m_receiveBuffer.left(expectedSize);
                qDebug() << "Extracted Modbus packet:" << packet.toHex(' ');

                // Используем метод с маппингом регистров
                parseSuccess = parseModbusResponseWithMapping(packet, m_lastRequestedRegisters, values);

                if (parseSuccess) {
                    qDebug() << "MODBUS parse successful:" << values.size() << "AVERAGE values";
                    m_receiveBuffer.remove(0, expectedSize);
                } else {
                    qDebug() << "MODBUS parse failed, clearing buffer";
                    m_receiveBuffer.clear();
                }
            } else {
                qDebug() << "Waiting for more data...";
            }
        } else {
            qDebug() << "Buffer too small for MODBUS (need 7, have" << m_receiveBuffer.size() << ")";
        }

        // Очистка переполненного буфера
        if (m_receiveBuffer.size() > 512) {
            qDebug() << "Buffer overflow, clearing";
            m_receiveBuffer.clear();
        }
    } else if (m_protocol == UMB_PROTOCOL) {
        qDebug() << "Parsing as UMB protocol";
        // Для UMB ищем полный пакет (от SOH до EOT)
        int sohPos = m_receiveBuffer.indexOf(0x01);
        int eotPos = m_receiveBuffer.indexOf(0x04, sohPos);

        qDebug() << "SOH position:" << sohPos << ", EOT position:" << eotPos;

        if (sohPos >= 0 && eotPos > sohPos) {
            QByteArray packet = m_receiveBuffer.mid(sohPos, eotPos - sohPos + 1);
            qDebug() << "Extracted packet:" << packet.toHex(' ');

            parseSuccess = parseUmbResponse(packet, values);

            if (parseSuccess) {
                qDebug() << "UMB parse successful, parsed" << values.size() << "values";
                m_receiveBuffer.remove(0, eotPos + 1);
            } else {
                qDebug() << "UMB parse failed";
            }
        } else {
            qDebug() << "Complete UMB packet not found yet";
        }

        // Очищаем буфер если он слишком большой
        if (m_receiveBuffer.size() > 1024) {
            qDebug() << "Buffer overflow, clearing";
            m_receiveBuffer.clear();
        }
    }

    if (parseSuccess && !values.isEmpty()) {
        qDebug() << "Calling updateTableWithData with" << values.size() << "values";
        for (auto it = values.begin(); it != values.end(); ++it) {
            qDebug() << "  " << it.key() << "=" << it.value();
        }

        // Перевод давления гПа → мм рт.ст. ПРЯМО В VALUES.
        // IWS отдаёт давление в гПа, а внутри программы везде используется
        // мм рт.ст. Конвертируем здесь, в одной точке — дальше во всех
        // подписчиках (SurfaceMeteoSaver, таблица, MainWindow) давление
        // приходит уже в мм рт.ст. Никаких *0.750064 в других местах быть
        // не должно.
        if (values.contains("Pressure Avg")) {
            values["Pressure Avg"] = values["Pressure Avg"] * kHpaToMmHg;
            qDebug() << "GroundMeteoParams: Pressure Avg переведён гПа→мм рт.ст. ="
                     << values["Pressure Avg"];
        }
        if (values.contains("Pressure")) {
            values["Pressure"] = values["Pressure"] * kHpaToMmHg;
            qDebug() << "GroundMeteoParams: Pressure переведён гПа→мм рт.ст. ="
                     << values["Pressure"];
        }

        // Обновляем флаги готовности для приземных параметров: данные пришли
        // от IWS — считаем их применёнными (грань ручной/IWS стёрта).
        if (values.contains("Wind Speed Avg") || values.contains("Wind Speed"))
            m_hasSpeed = true;
        if (values.contains("Wind Direction Avg") || values.contains("Wind Direction"))
            m_hasDirection = true;
        if (values.contains("Pressure Avg") || values.contains("Pressure"))
            m_hasPressure = true;
        if (values.contains("Humidity Avg") || values.contains("Humidity"))
            m_hasHumidity = true;
        if (values.contains("Temperature Avg") || values.contains("Temperature"))
            m_hasTemperature = true;

        // Данные пришли — таймер 30 мин обнуляем
        restartStaleTimer();
        // Состояние могло измениться (например, NoData → Fresh)
        recomputeSurfaceState();

        // Кешируем скорость и направление ветра (для передачи в АМС)
        // Поддерживаем оба протокола: UMB (текущие) и Modbus (средние)
        if (values.contains("Wind Speed Avg")) {
            m_lastWindSpeed = values["Wind Speed Avg"];
            m_hasLastData = true;
            qDebug() << "GroundMeteoParams: кеш Wind Speed Avg =" << m_lastWindSpeed;
        } else if (values.contains("Wind Speed")) {
            m_lastWindSpeed = values["Wind Speed"];
            m_hasLastData = true;
            qDebug() << "GroundMeteoParams: кеш Wind Speed =" << m_lastWindSpeed;
        }

        if (values.contains("Wind Direction Avg")) {
            m_lastWindDirection = values["Wind Direction Avg"];
            m_hasLastData = true;
            qDebug() << "GroundMeteoParams: кеш Wind Direction Avg =" << m_lastWindDirection;
        } else if (values.contains("Wind Direction")) {
            m_lastWindDirection = values["Wind Direction"];
            m_hasLastData = true;
            qDebug() << "GroundMeteoParams: кеш Wind Direction =" << m_lastWindDirection;
        }

        qDebug() << "GroundMeteoParams: m_hasLastData =" << m_hasLastData
                 << "speed =" << m_lastWindSpeed
                 << "dir =" << m_lastWindDirection;

        updateTableWithData(values);
        emit dataUpdated(values);
    } else {
        qDebug() << "No data to update (parseSuccess=" << parseSuccess << ", values.size=" << values.size() << ")";
    }

    qDebug() << "=== End onDataReceived ===";
}

void GroundMeteoParams::updateTableWithData(const QMap<QString, double>& values)
{
    qDebug() << "updateTableWithData called with" << values.size() << "values";

    // Данные пришли от ИВС, а не введены вручную — снимаем возможную
    // жёлтую подсветку "ручной ввод" на плашке ИВС.
    m_lastUpdateWasManual = false;

    // Программная запись — не помечаем как ручную правку
    m_suppressDirty = true;

    for (auto it = values.begin(); it != values.end(); ++it) {
        qDebug() << "Processing parameter:" << it.key() << "=" << it.value();

        QString rowName = mapParameterToTableRow(it.key());

        if (rowName.isEmpty()) {
            qDebug() << "No mapping found for parameter:" << it.key();
            continue;
        }

        qDebug() << "Mapped to row containing:" << rowName;

        int rowCount = ui->tableWidget_GroundParams->rowCount();
        bool found = false;

        for (int row = 0; row < rowCount; ++row) {
            QTableWidgetItem* paramItem = ui->tableWidget_GroundParams->item(row, 0);
            if (paramItem) {
                QString cellText = paramItem->text();

                if (cellText.contains(rowName, Qt::CaseInsensitive)) {
                    QTableWidgetItem* valueItem = ui->tableWidget_GroundParams->item(row, 1);
                    if (!valueItem) {
                        valueItem = new QTableWidgetItem();
                        ui->tableWidget_GroundParams->setItem(row, 1, valueItem);
                    }
                    // Давление в values теперь уже мм рт.ст. (перевод в onDataReceived).
                    // Никаких пересчётов здесь — пишем значение как есть.
                    valueItem->setText(QString::number(it.value(), 'f', 2));
                    qDebug() << "Updated row" << row << "with value:" << it.value();
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            qDebug() << "Row not found for:" << rowName;
        }
    }

    m_suppressDirty = false;
    qDebug() << "Table update completed";
}

QString GroundMeteoParams::mapParameterToTableRow(const QString& paramName)
{
    // Маппинг названий параметров на строки таблицы
    // Таблица содержит:
    // Row 0: "Наземная скорость ветра V, м/с"
    // Row 1: "Наземное направление ветра А, град"
    // Row 2: "Наземное давление P, мм рт. ст."
    // Row 3: "Наземная относительная влажность воздуха r, %"
    // Row 4: "Наземная температура T, C"

    qDebug() << "Mapping parameter:" << paramName;

    // UMB протокол (старые текущие значения)
    if (paramName == "Wind Speed") {
        qDebug() << "Mapped to: скорость ветра (UMB текущее)";
        return "скорость ветра";
    }
    if (paramName == "Wind Direction") {
        qDebug() << "Mapped to: направление ветра (UMB текущее)";
        return "направление ветра";
    }
    if (paramName == "Pressure") {
        qDebug() << "Mapped to: давление (UMB текущее)";
        return "давление";
    }
    if (paramName == "Humidity") {
        qDebug() << "Mapped to: влажность (UMB текущая)";
        return "влажность";
    }
    if (paramName == "Temperature") {
        qDebug() << "Mapped to: температура (UMB текущая)";
        return "температура";
    }

    // Modbus RTU (новые СРЕДНИЕ значения)
    if (paramName == "Wind Speed Avg") {
        qDebug() << "Mapped to: скорость ветра (Modbus СРЕДНЕЕ)";
        return "скорость ветра";
    }
    if (paramName == "Wind Direction Avg") {
        qDebug() << "Mapped to: направление ветра (Modbus СРЕДНЕЕ)";
        return "направление ветра";
    }
    if (paramName == "Pressure Avg") {
        qDebug() << "Mapped to: давление (Modbus СРЕДНЕЕ)";
        return "давление";
    }
    if (paramName == "Humidity Avg") {
        qDebug() << "Mapped to: влажность (Modbus СРЕДНЯЯ)";
        return "влажность";
    }
    if (paramName == "Temperature Avg") {
        qDebug() << "Mapped to: температура (Modbus СРЕДНЯЯ)";
        return "температура";
    }

    qDebug() << "No mapping found";
    return QString();
}

//QByteArray GroundMeteoParams::createModbusReadRequest(const QList<quint16>& parameters)
//{
//    QByteArray request;
//    request.append(m_deviceAddress);
//    request.append(0x03); // Read holding registers

//    quint16 startAddr = parameters.first();
//    request.append(static_cast<char>((startAddr >> 8) & 0xFF));
//    request.append(static_cast<char>(startAddr & 0xFF));

//    quint16 regCount = parameters.size();
//    request.append(static_cast<char>((regCount >> 8) & 0xFF));
//    request.append(static_cast<char>(regCount & 0xFF));

//    quint16 crc = calculateModbusCRC16(request);
//    request.append(static_cast<char>(crc & 0xFF));
//    request.append(static_cast<char>((crc >> 8) & 0xFF));

//    return request;
//}

//QByteArray GroundMeteoParams::createUmbReadRequest(const QList<quint16>& parameters)
//{
//    QByteArray request;

//    request.append(0x01); // SOH
//    request.append(0x10); // ver
//    request.append(0x01); // to (high byte)
//    request.append(0x70); // to (low byte) - device address
//    request.append(0x01); // from (high byte)
//    request.append(0xF0); // from (low byte) - data collector address

//    int dataLength = 3 + (parameters.size() * 2);
//    request.append(static_cast<char>(dataLength));

//    request.append(0x02); // STX
//    request.append(0x2F); // cmd (read parameters)
//    request.append(0x10); // verc
//    request.append(static_cast<char>(parameters.size()));

//    for (int i = 0; i < parameters.size(); ++i) {
//        quint16 param = parameters[i];
//        request.append(static_cast<char>(param & 0xFF));
//        request.append(static_cast<char>((param >> 8) & 0xFF));
//    }

//    request.append(0x03); // ETX

//    quint16 crc = calculateCRC16(request);
//    request.append(static_cast<char>(crc & 0xFF));
//    request.append(static_cast<char>((crc >> 8) & 0xFF));

//    request.append(0x04); // EOT

//    qDebug() << "UMB Request created:" << request.toHex(' ');

//    return request;
//}






// ===== НОВЫЕ МЕТОДЫ ДЛЯ MODBUS RTU =====



void GroundMeteoParams::onTableItemChanged(QTableWidgetItem *item)
{
    if (m_suppressDirty) return;   // программная запись — не считаем правкой
    m_dirty = true;

    // Как только оператор начал заполнять ранее подсвеченную строку —
    // снимаем с неё красную рамку и подпись, не дожидаясь "Применить".
    if (item && item->column() == 1 && !item->text().trimmed().isEmpty())
        setRowInvalid(item->row(), false);
}

void GroundMeteoParams::recomputeSurfaceState()
{
    SurfaceState newState;
    const bool allFive = m_hasSpeed && m_hasDirection
                         && m_hasPressure && m_hasHumidity && m_hasTemperature;

    if (!allFive) {
        newState = NoData;
    } else if (m_staleTimer && m_staleTimer->isActive()) {
        // Таймер ещё тикает — данные свежие
        newState = Fresh;
    } else {
        // Все 5 есть, но таймер не активен — значит уже истёк
        newState = Stale;
    }

    if (newState != m_surfaceState) {
        qDebug() << "GroundMeteoParams: surfaceState"
                 << m_surfaceState << "→" << newState
                 << "(hasSpeed=" << m_hasSpeed
                 << "hasDir=" << m_hasDirection
                 << "hasPres=" << m_hasPressure
                 << "hasHum=" << m_hasHumidity
                 << "hasTemp=" << m_hasTemperature << ")";
        m_surfaceState = newState;
        emit surfaceStateChanged(newState);
    }
}

void GroundMeteoParams::restartStaleTimer()
{
    if (m_staleTimer) m_staleTimer->start(kStaleTimeoutMs);
}

void GroundMeteoParams::onStaleTimerTimeout()
{
    qDebug() << "GroundMeteoParams: таймер 30 мин истёк — данные устарели";
    // Если данные есть — состояние станет Stale; если уже нет — останется NoData.
    recomputeSurfaceState();
}

// ─────────────────────────────────────────────────────────────────────────
// Подсветка незаполненных строк ("рамка + подпись", с анимацией встряски)
// ─────────────────────────────────────────────────────────────────────────

void GroundMeteoParams::setRowInvalid(int row, bool invalid)
{
    QTableWidget *table = ui->tableWidget_GroundParams;
    QTableWidgetItem *valueItem = table->item(row, 1);
    QTableWidgetItem *nameItem  = table->item(row, 0);

    if (valueItem) valueItem->setData(kInvalidRole, invalid);
    if (nameItem)  nameItem->setForeground(QColor(invalid ? "#C62828" : "#1C1F22"));

    table->viewport()->update();
}

void GroundMeteoParams::showRowHint(int row, const QString &text)
{
    QTableWidget *table = ui->tableWidget_GroundParams;
    const QModelIndex idx = table->model()->index(row, 1);
    const QRect cellRect = table->visualRect(idx);
    const QPoint topLeftInParent = table->mapTo(this, cellRect.topLeft());

    if (!m_rowHintLabel) {
        m_rowHintLabel = new QLabel(this);
        m_rowHintLabel->setStyleSheet(
            "background-color:#C62828; color:#FFFFFF; font-weight:bold; font-size:9pt;"
            "border-radius:5px; padding:4px 10px;");
        m_rowHintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    m_rowHintLabel->setText(text);
    m_rowHintLabel->adjustSize();

    const int x = topLeftInParent.x() + cellRect.width() - m_rowHintLabel->width() - 10;
    const int y = topLeftInParent.y() + (cellRect.height() - m_rowHintLabel->height()) / 2;
    m_rowHintLabel->move(x, y);
    m_rowHintLabel->raise();
    m_rowHintLabel->show();
    shakeWidget(m_rowHintLabel);

    QPointer<QLabel> hintGuard(m_rowHintLabel);
    QTimer::singleShot(2500, this, [hintGuard]() {
        if (hintGuard) hintGuard->hide();
    });
}

void GroundMeteoParams::shakeWidget(QWidget *w)
{
    if (!w) return;
    const QPoint basePos = w->pos();

    auto *anim = new QPropertyAnimation(w, "pos", w);
    anim->setDuration(280);
    anim->setKeyValueAt(0.0, basePos);
    anim->setKeyValueAt(0.2, basePos + QPoint(-6, 0));
    anim->setKeyValueAt(0.4, basePos + QPoint(6, 0));
    anim->setKeyValueAt(0.6, basePos + QPoint(-4, 0));
    anim->setKeyValueAt(0.8, basePos + QPoint(2, 0));
    anim->setKeyValueAt(1.0, basePos);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ─────────────────────────────────────────────────────────────────────────
// Визуальный стиль (единожды из конструктора) и обновление статус-пилюли
// ─────────────────────────────────────────────────────────────────────────

void GroundMeteoParams::applyVisualStyle()
{
    // Общий вид экрана («Архив измерений»): фон, поля, кнопки, группы.
    // Роли кнопок помечаем ДО темы — селекторы [primary]/[nav] сработают сразу.
    // Вид таблицы приходит отдельно, из стиля приложения ui/table-theme.qss.
    ui->btnGroundParamsApply->setProperty("primary", true);
    setupArchiveBackButton(ui->btnGroundParamsClose);
    applyArchiveScreenTheme(this);

    // Дописываем правила, специфичные только для этого экрана. Дописывать
    // МОЖНО: общая тема состоит из полноценных правил, и Qt разбирает
    // объединённый стилшит целиком (ломает разбор только голое объявление
    // в начале — см. ui/ScreenTheme.h).
    setStyleSheet(styleSheet() +
        "QLabel#lblGroundParams { color:#1B211F; }"
        "QLabel#lblStatusPill { border-radius:14px; padding:6px 18px; font-weight:700; font-size:9.5pt; }"
    );
}