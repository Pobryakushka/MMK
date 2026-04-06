#ifndef METEO11_H
#define METEO11_H

#include <QDialog>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace Ui {
class Meteo11;
}

class Meteo11 : public QDialog {
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

private slots:
    void onApplyClicked();
    void onParseClicked();   // разобрать сырую строку → заполнить поля
    void onClearClicked();

private:
    Ui::Meteo11 *ui;

    bool        m_applied;
    QDateTime   m_bulletinTime;
    QJsonObject m_bulletinJson;
    QString     m_validityPeriod;

    // Коды высот для строк таблицы: 02, 04, 08, 12, 16, 20, 24, 30
    static const QStringList kHeightCodes;
};

#endif // METEO11_H