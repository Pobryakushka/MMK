#ifndef COORDHELPER_H
#define COORDHELPER_H

// Файл содержит общие методы для работы с координатами
// Используется в MainWindow и LandingCalculation

#include <QString>
#include <QStringList>
#include <QRegExp>
#include <cmath>

namespace CoordHelper {

// Десятичные градусы -> строка Градусы Минуты Секунды
inline QString toDisplayDMS(double dec_deg)
{
    bool neg = dec_deg < 0.0;
    dec_deg = std::abs(dec_deg);

    int d = static_cast<int>(dec_deg);
    double rem = (dec_deg - d) * 60.0;
    int m = static_cast<int>(rem);
    int s = static_cast<int>(std::round((rem - m) * 60.0));

    if (s >= 60) { s -= 60; m += 1; }
    if (m >= 60) { m -= 60; d += 1; }

    QString result = QString ("%1°%2'%3\"")
            .arg(d, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));

    if (neg) result.prepend("-");
    return result;
}

// Строка -> десятичные градусы
inline bool parseDMS(const QString &text, double &degrees)
{
    QString s = text.trimmed();
    if (s.isEmpty()) return false;

    s.replace(QRegExp("[°'\"]+"), " ");
    s = s.simplified();

    QStringList parts = s.split(' ', Qt::SkipEmptyParts);

    if (parts.size() == 1) {
        QString p = parts[0].replace(',', '.');

        // Слитный формат
        static QRegExp onlyDigits("^\\d+$");
        if (onlyDigits.exactMatch(p) && p.length() >= 5 && p.length() <= 7) {
            int secLen = 2;
            int minLen = 2;
            int degLen = p.length() - secLen - minLen;
            if (degLen >= 1 && degLen <= 3) {
                double d = p.left(degLen).toDouble();
                double m = p.mid(degLen, minLen).toDouble();
                double sec = p.mid(degLen + minLen, secLen).toDouble();
                degrees = d + m / 60.0 + sec / 3600.0;
                return true;
            }
        }

        // Обычное десятичное число
        bool ok = false;
        degrees = p.toDouble(&ok);
        return ok;
    }

    if (parts.size() >= 2) {
        bool ok1, ok2, ok3 = true;
        double d = parts[0].replace(',', '.').toDouble(&ok1);
        double m = parts[1].replace(',', '.').toDouble(&ok2);
        double sec = 0.0;
        if (parts.size() >= 3)
            sec = parts[2].replace(',', '.').toDouble(&ok3);
        if (!ok1 || !ok2 || !ok3) return false;
        degrees = d + m / 60.0 + sec / 3600.0;
        return true;
    }

    return false;
}

// Форматирование при вводе ( ° ' " )
inline QString formatInput(const QString &raw, int &cursorPos)
{
    // Извлекаем только цифры
    QString digits;
    for (QChar c : raw) {
        if (c.isDigit()) digits.append(c);
    }

    if (digits.length() > 6) digits = digits.left(6);

    QString result;
    int digitCount = digits.length();

    if (digitCount == 0) {
        result = "";
    } else if (digitCount <= 2) {
        result = digits;
    } else if (digitCount <= 4) {
        result = digits.left(2) + "°" + digits.mid(2);
    } else {
        result = digits.left(2) + "°" + digits.mid(2, 2) + "'" + digits.mid(4);
        if (digitCount == 6) result += "\"";
    }

    cursorPos = result.length();
    return result;
}

}

#endif // COORDHELPER_H
