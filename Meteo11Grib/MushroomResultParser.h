#pragma once
#include <QVector>
#include <QString>
#include "MushroomMessage.h"

// Разбирает текстовый вывод программы Mushroom (формат строки,
// который печатают ProcessMessage() и main()) в структурированный
// список MushroomMessage. Не зависит от Qt Widgets и от способа
// запуска процесса — можно переиспользовать где угодно, где есть
// сырой текст вывода Mushroom.
class MushroomResultParser {
public:
    static QVector<MushroomMessage> parse(const QString &rawOutput);
};
