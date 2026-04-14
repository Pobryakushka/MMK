#ifndef WINDSHEARCALCULATOR_H
#define WINDSHEARCALCULATOR_H

#include <QVector>
#include <QColor>
#include <QString>
#include "amsprotocol.h"

// Структура для результата расчета сдвига ветра
struct WindShearData {
    double height;          // Высота (средняя между двумя уровнями), м
    double shearMagnitude;  // Модуль сдвига ветра, м/с
    double shearDirection;  // Направление сдвига, градусы
    double shearPer30m;     // Сдвиг приведенный к 30м, м/с/30м
    int severityLevel;      // Уровень критичности: 0-слабый, 1-умеренный, 2-сильный, 3-очень сильный
    
    WindShearData() : height(0), shearMagnitude(0), shearDirection(0), 
                     shearPer30m(0), severityLevel(0) {}
};

/**
 * @brief Класс для вычисления вертикального сдвига ветра
 * 
 * Реализует алгоритм согласно ТЗ "Требования к алгоритму расчета сдвига ветра"
 * Поддерживает два способа расчета:
 * 1) Через скорость и направление ветра
 * 2) Через зональную и меридиональную составляющие
 */
class WindShearCalculator
{
public:
    WindShearCalculator();
    
    /**
     * @brief Вычисление сдвига ветра для профиля (способ 1)
     * @param profile Профиль ветра (скорость и направление на разных высотах)
     * @return Вектор данных сдвига ветра
     */
    static QVector<WindShearData> calculateShear(const QVector<WindProfileData> &profile);
    
    /**
     * @brief Вычисление сдвига ветра для измеренных данных
     * @param measuredData Измеренные данные ветра
     * @return Вектор данных сдвига ветра
     */
    static QVector<WindShearData> calculateShear(const QVector<MeasuredWindData> &measuredData);
    
    /**
     * @brief Определение уровня критичности сдвига ветра
     * @param shearPer30m Сдвиг ветра в м/с/30м
     * @return Уровень: 0-слабый (0-2), 1-умеренный (2-4), 2-сильный (4-6), 3-очень сильный (>6)
     */
    static int getSeverityLevel(double shearPer30m);
    
    /**
     * @brief Получение цвета для визуализации уровня критичности
     * @param severityLevel Уровень критичности (0-3)
     * @return QColor для отображения
     */
    static QColor getSeverityColor(int severityLevel);
    
    /**
     * @brief Получение текстового описания уровня критичности
     * @param severityLevel Уровень критичности (0-3)
     * @return Строка с описанием
     */
    static QString getSeverityText(int severityLevel);

    /**
     * @brief Самотестирование по эталонной таблице (Вариант 8 ТЗ).
     * Вызови один раз из main() или конструктора MainWindow.
     * Результаты печатаются через qDebug().
     * @return true если все тесты прошли
     */
    static bool runSelfTest();

private:
    /**
     * @brief Расчет сдвига ветра между двумя уровнями (Способ 1)
     * Использует скорость и направление ветра
     * 
     * @param V_i Скорость ветра на высоте H_i, м/с
     * @param alpha_i Направление ветра на высоте H_i, градусы
     * @param V_i_1 Скорость ветра на высоте H_{i-1}, м/с
     * @param alpha_i_1 Направление ветра на высоте H_{i-1}, градусы
     * @param H_i Высота верхнего уровня, м
     * @param H_i_1 Высота нижнего уровня, м
     * @param deltaV Выходной параметр: модуль сдвига ветра, м/с
     * @param deltaAlpha Выходной параметр: изменение направления, градусы
     */
    static void calculateShearMethod1(double V_i, double alpha_i,
                                     double V_i_1, double alpha_i_1,
                                     double H_i, double H_i_1,
                                     double &deltaV, double &deltaAlpha);
    
    /**
     * @brief Расчет сдвига ветра между двумя уровнями (Способ 2)
     * Использует зональную и меридиональную составляющие
     * 
     * @param V_i Скорость ветра на высоте H_i, м/с
     * @param alpha_i Направление ветра на высоте H_i, градусы
     * @param V_i_1 Скорость ветра на высоте H_{i-1}, м/с
     * @param alpha_i_1 Направление ветра на высоте H_{i-1}, градусы
     * @param H_i Высота верхнего уровня, м
     * @param H_i_1 Высота нижнего уровня, м
     * @param deltaV Выходной параметр: модуль сдвига ветра, м/с
     * @param deltaAlpha Выходной параметр: изменение направления, градусы
     */
    static void calculateShearMethod2(double V_i, double alpha_i,
                                     double V_i_1, double alpha_i_1,
                                     double H_i, double H_i_1,
                                     double &deltaV, double &deltaAlpha);
    
    /**
     * @brief Преобразование градусов в радианы
     */
    static double degToRad(double degrees);
    
    /**
     * @brief Преобразование радианов в градусы
     */
    static double radToDeg(double radians);
    
    /**
     * @brief Нормализация угла к диапазону [0, 360)
     */
    static double normalizeAngle(double angle);
    
    /**
     * @brief Функция sign(y) из формул
     * @return 1 если y >= 0, иначе -1
     */
    static int sign(double y);
};

#endif // WINDSHEARCALCULATOR_H
