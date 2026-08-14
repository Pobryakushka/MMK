#ifndef GROUNDMETEOPARAMS_H
#define GROUNDMETEOPARAMS_H

#include <QWidget>
#include <QMap>
#include <QSerialPort>
#include <QCloseEvent>
#include <QTimer>
#include <QTableWidget>
#include <QStyledItemDelegate>

namespace Ui {
class GroundMeteoParams;
}

// Делегат ячейки таблицы "Значение": создаёт QLineEdit-редактор с
// ограничениями конкретной строки (диапазон/точность/знак) и привязывает
// к нему VirtualKeyboard, чтобы клавиатура появлялась автоматически и не
// перекрывала таблицу.
class GroundParamValueDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit GroundParamValueDelegate(QObject *parent = nullptr);
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void destroyEditor(QWidget *editor, const QModelIndex &index) const override;
};

class GroundMeteoParams : public QWidget {
    Q_OBJECT

public:
    enum RS485Protocol {
        MODBUS_RTU,
        UMB_PROTOCOL
    };

    // Состояние приземных данных. Единая точка правды для всей программы.
    //   NoData — НЕ ВСЕ 5 строк применены кнопкой "Применить" / получены от IWS.
    //   Fresh  — все 5 применены, прошло < 30 минут с последнего применения.
    //   Stale  — все 5 применены, прошло > 30 минут (данные считаются устаревшими,
    //            но НЕ блокируют запуск измерения — только уведомление).
    enum SurfaceState {
        NoData,
        Fresh,
        Stale
    };
    Q_ENUM(SurfaceState)

    // Длительность "свежести" данных, мс. Перезапускается при каждом применении.
    static constexpr int kStaleTimeoutMs = 30 * 60 * 1000; // 30 минут

    explicit GroundMeteoParams(QWidget *parent = nullptr);
    ~GroundMeteoParams();

    void deleteDataFromTable();

    // Статический метод для получения экземпляра (синглтон паттерн)
    static GroundMeteoParams* instance();

    // RS485 функционал
    void setProtocol(RS485Protocol protocol);
    void setDeviceAddress(quint8 address);

    // Геттеры для последних полученных значений приземного ветра
    double lastWindSpeed() const     { return m_lastWindSpeed; }
    double lastWindDirection() const { return m_lastWindDirection; }
    bool   hasLastData() const       { return m_hasLastData; }

    // ── Состояние приземных данных (единая точка правды) ────────────────────
    SurfaceState surfaceState() const { return m_surfaceState; }

    // Есть ли в таблице правки, которые ещё не применены кнопкой "Применить".
    // Используется в closeEvent для подтверждающего диалога.
    bool hasUnappliedChanges() const { return m_dirty; }

    // Создание запросов (публичные методы)
    QByteArray createModbusReadRequest(const QList<quint16>& parameters);
    QByteArray createUmbReadRequest(const QList<quint16>& parameters);
    QByteArray createRequest(const QList<quint16>& parameters);

public slots:
    // Обработка входящих данных
    void onDataReceived(const QByteArray& data);

private slots:
    void updateTableWithData(const QMap<QString, double>& values);
    void applyManualInput();
    void onTableItemChanged(QTableWidgetItem *item);   // отслеживание m_dirty
    void onStaleTimerTimeout();                         // 30 мин истекли
    void updateStatusPill(GroundMeteoParams::SurfaceState state); // обновление пилюли статуса

private:
    Ui::GroundMeteoParams *ui;
    RS485Protocol m_protocol;
    quint8 m_deviceAddress;
    QByteArray m_receiveBuffer;

    // Статическая переменная для синглтона
    static GroundMeteoParams* s_instance;

    // Кеш последних полученных значений приземного ветра
    double m_lastWindSpeed    = 0.0;
    double m_lastWindDirection = 0.0;
    bool   m_hasLastData      = false;

    // Для отслеживания запрошенных регистров (Modbus)
    QList<quint16> m_lastRequestedRegisters;

    // ── Готовность приземных данных ──────────────────────────────────────────
    // Флаги "это значение применено" — по каждой из 5 строк таблицы.
    // Применением считается:
    //   • нажатие кнопки "Применить" с валидным значением в строке;
    //   • получение значения от IWS (любой опрос).
    // Сброс — кнопка "Очистить" / при ручном вводе пустой строки + Применить.
    bool m_hasSpeed       = false;
    bool m_hasDirection   = false;
    bool m_hasPressure    = false;
    bool m_hasHumidity    = false;
    bool m_hasTemperature = false;

    SurfaceState m_surfaceState = NoData;

    QTimer *m_staleTimer = nullptr;   // singleShot на 30 мин

    // Признак "в таблице есть правки, не применённые кнопкой Применить".
    // Сбрасывается при applyManualInput / deleteDataFromTable / onDataReceived
    // (после программной записи). Выставляется только при РУЧНОМ редактировании
    // ячеек оператором (через сигнал itemChanged).
    bool m_dirty = false;
    bool m_suppressDirty = false;     // подавление itemChanged при программной записи

    // Парсинг ответов (приватные методы)
    bool parseModbusResponse(const QByteArray& response, QMap<QString, double>& values);
    bool parseUmbResponse(const QByteArray& response, QMap<QString, double>& values);

    // НОВЫЕ методы для Modbus RTU с маппингом регистров
    bool parseModbusResponseWithMapping(
        const QByteArray& response,
        const QList<quint16>& requestedRegisters,
        QMap<QString, double>& values);

    bool convertModbusRegisterToValue(
        quint16 regAddr,
        quint16 rawValue,
        QString& paramName,
        double& scaledValue);

    // Вспомогательные функции
    quint16 calculateCRC16(const QByteArray& data);
    quint16 calculateModbusCRC16(const QByteArray& data);
    QString parameterCodeToName(quint16 code);
    QString mapParameterToTableRow(const QString& paramName);

    // Пересчёт m_surfaceState из 5 флагов + эмит сигнала при изменении.
    void recomputeSurfaceState();
    // Перезапустить таймер 30 мин (вызывается при любом применении/приёме).
    void restartStaleTimer();

    // Применяет визуальный стиль (карточка/пилюля/таблица/кнопки) — вызывается
    // один раз из конструктора.
    void applyVisualStyle();

    // Если в таблице есть неприменённые правки (m_dirty) — спрашивает
    // подтверждение (применить/отменить/не уходить). Возвращает true, если
    // можно продолжать переход "назад" (правки применены или сброшены),
    // false — если оператор нажал "Отмена" и остаётся на странице.
    // Общий код для кнопки "Закрыть" и для closeEvent (на случай, если
    // что-то всё же вызовет close() программно).
    bool confirmDiscardIfDirty();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void errorOccurred(const QString& error);
    void dataUpdated(const QMap<QString, double>& values);

    // Состояние приземных данных изменилось. MainWindow слушает этот сигнал
    // и обновляет lblStatus + доступность кнопки старта.
    void surfaceStateChanged(GroundMeteoParams::SurfaceState newState);

    // Запрос вернуться на предыдущую страницу (аналог
    // AlgorithmsCalculation::backRequested()) — MainWindow переключает
    // stackedWidget обратно на страницу "Исходные данные".
    void backRequested();
};

#endif // GROUNDMETEOPARAMS_H
