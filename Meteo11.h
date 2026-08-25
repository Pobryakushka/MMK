#ifndef METEO11_H
#define METEO11_H

#include <QWidget>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QStyledItemDelegate>

class QLineEdit;
class QLabel;
class QWidget;

namespace Ui {
class Meteo11;
}

// Делегат ячеек таблицы "Слои ветра": редактор ячейки — обычный QLineEdit
// (как и создаёт QStyledItemDelegate по умолчанию для строковых данных),
// но с привязанной экранной клавиатурой (VirtualKeyboard) — цифровая
// раскладка для всех трёх колонок (ПП, Напр., СС).
class Meteo11LayerCellDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit Meteo11LayerCellDelegate(QObject *parent = nullptr);
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
};

// Страница "Метео-11" — встраивается в общий стек MainWindow (QStackedWidget),
// как и SourceData/GroundMeteoParams, а не открывается отдельным окном.
// Наружу отдаёт сигнал навигации backRequested(); переключение страниц
// делает MainWindow.
class Meteo11 : public QWidget {
    Q_OBJECT

public:
    explicit Meteo11(QWidget *parent = nullptr);
    ~Meteo11();

    // ── Данные бюллетеня (доступны после нажатия «Применить») ────────────────

    /** Данные введены и подтверждены кнопкой «Применить». */
    bool isApplied() const { return m_applied; }

    /** Время составления бюллетеня. */
    QDateTime bulletinTime() const { return m_bulletinTime; }

    /**
     * Данные бюллетеня в JSON для записи в meteo_11_bulletin.bulletin_data.
     * Формат:
     * {
     *   "station_num":          "NNNNN",
     *   "station_height":       "BBBB",
     *   "datetime":             "ДДЧЧМ",
     *   "ground_pres_dev":      "ДДД",
     *   "ground_virt_temp_dev": "T0T0",
     *   "achieved_wind_height": "BвBв",
     *   "raw_string":           "Метео 1101 — ...",
     *   "layers": [
     *     { "height_code": "02", "nn": "25", "ss": "06" },
     *     ...
     *   ]
     * }
     */
    QJsonObject bulletinJson() const { return m_bulletinJson; }

    /** Период действия ("12h"). */
    QString validityPeriod() const { return m_validityPeriod; }

    /** Сбросить флаг «применён» после сохранения в БД. */
    void resetApplied() { m_applied = false; }

signals:
    void backRequested();

private slots:
    void onApplyClicked();
    void onParseClicked();   // разобрать сырую строку → заполнить поля
    void onClearClicked();

private:
    void updateStatusPill();

    // ── Валидация полей ввода (подсветка незаполненных) ─────────────────────
    struct RequiredField {
        QLineEdit *edit;
        QLabel    *hint;
        QString    label;   // человекочитаемое имя поля для подсказки
    };
    QList<RequiredField> requiredFields() const;
    void setupValidation();                              // подключает сигналы очистки подсветки
    bool validateRequiredFields(bool focusFirst = true);  // true, если все обязательные поля заполнены
    void setFieldInvalid(QLineEdit *edit, QLabel *hint, bool invalid,
                          const QString &fieldLabel = QString());
    void shakeWidget(QWidget *w);                         // короткая анимация "встряски"
    void setupVirtualKeyboard();                          // привязка экранной клавиатуры к полям

    Ui::Meteo11 *ui;

    bool        m_applied;
    QDateTime   m_bulletinTime;
    QJsonObject m_bulletinJson;
    QString     m_validityPeriod;

    // Коды высот для строк таблицы (19 стандартных уровней):
    // 02 04 08 12 16 20 24 30 40 50 60 80 10 12 14 18 22 26 30
    static const QStringList kHeightCodes;
};

#endif // METEO11_H
