#ifndef LANDINGCALCULATION_H
#define LANDINGCALCULATION_H

#include <QDialog>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <cmath>
#include <QtMath>

namespace Ui {
class LandingCalculation;
}

struct LandingCalculationResult
{
    // Координаты ТНВ
    double B_tnv_deg; // Широта ТНВ, десятичные градусы
    double L_tnv_deg; // Долгота ТНВ, десятичные градусы
    double H_tnv; // Высота ТНВ, м

    // Промежуточные результаты
    double T_sn; // Время снижения, с
    double Z_sn; // Ветровой снос, м
    double delta_x; // Приращение X, м
    double delta_y; // Приращение Y, м

    // Положение метеокомплекса относительно ТНВ
    double D_tnv; // Расстояние от М до ТНВ, м
    double A_tnv_deg; // Дирекционный угол от М до ТНВ, град

    // Положение метеокомплекса относительно ТПП
    double D_tpp; // Расстояние от М до ТПП, м
    double A_tpp_deg; // Дирекционный угол от М до ТПП, град

    bool valid; // Успешный расчет
};

class LandingCalculation : public QDialog
{
    Q_OBJECT

public:
    explicit LandingCalculation(QWidget *parent = nullptr);
    ~LandingCalculation();

    // Установка координат метеокомплекса (вызов из Mainwindow)
    void setMeteoStationCoords(double lat_deg, double lon_deg, double alt_m);

private slots:
    void onEditButtonClicked();
    void onPasswordSubmit();
    void onPasswordCancel();
    void onEditClose();

    void onTableAdd();
    void onTableDelete();
    void onTableUp();
    void onTableDown();
    void onTableApply();
    void onTableEdit();

    void onTableResultsCellClicked(int row, int column);
    void onTableResultsCellChanged(int row, int column);
    void onTableEditParamsItemChanged(QTableWidgetItem *item);

    // Расчет по кнопке "Рассчитать"
    void onCalculateClicked();

    // Переключатель режима ручного ввода ветра
    void onManualWindInputToggled(bool checked);

private:
    Ui::LandingCalculation *ui;

    // Координаты метеокомплекса М (десятичные градусы и метры)
    double m_B_M; // Широта М, град
    double m_L_M; // Долгота М, рад
    double m_H_M; // Высота М, м

    // Хеш пароля (SHA-256)
    // Пароль по умолчанию: "admin" -> хеш ниже
    const QString PASSWORD_HASH = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";

    QString previousCellValue;
    int previousCellRow;
    int previousCellColumn;

    QString hashPassword(const QString &password);
    bool verifyPassword(const QString &password);
    void loadPasswordHash();
    void savePasswordHash(const QString &hash);
    QString getConfigFilePath();

    void setupEditTable();
    void swapTableRows(int row1, int row2);

    void saveState();
    void restoreState();

    void onCoordTextEdited(QLineEdit *edit);

    // ======= Методы расчета =======

    // Парсинг координат
    bool parseDMS(const QString &text, double &degrees);

    // Перевод десятичных градусов в радианы
    static double toRad(double deg) { return deg * M_PI / 180.0; }

    // Перевод радиан в десятичные градусы
    static double toDeg(double rad) { return rad * 180.0 / M_PI; }

    // Форматирование координат
    static QString formatDMS(double dec_deg, bool isLat);

    // Основной расчет
    LandingCalculationResult calculate();

    // Вывод результатов в UI
    void displayResults(const LandingCalculationResult &res);

    // Валидация полей ввода
    bool validateInputs();
};

#endif // LANDINGCALCULATION_H
