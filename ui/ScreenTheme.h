#ifndef SCREENTHEME_H
#define SCREENTHEME_H

#include <QWidget>
#include <QFile>
#include <QIODevice>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────
// applyArchiveScreenTheme — приводит экран к виду «Архива измерений».
//
// Подключает общий QSS ui/screen-theme.qss (фон страницы, группы, поля
// ввода, кнопки, списки, чекбоксы, подсказки) ко всему поддереву виджета.
// Оформление лежит отдельным файлом-ресурсом и не смешано с кодом функций;
// экран лишь подключает его одной строкой после setupUi().
//
// Стиль ЗАМЕЩАЕТ собственный стилшит экрана, а не дописывается к нему. Это
// принципиально: Qt разбирает стилшит, начинающийся с голых объявлений
// ("background-color: #EFF1F1;" из .ui), как стиль одного виджета — и все
// правила, дописанные после них, молча отбрасывает. Именно поэтому фон
// страницы задан здесь правилом QWidget[archiveScreen="true"], а голое
// объявление фона из .ui у переводимых экранов удаляется.
//
// Роли виджетов помечаются свойствами, а не именами объектов:
//   btn->setProperty("primary", true);  // основное действие (зелёная заливка)
//   btn->setProperty("nav", true);      // навигация «‹ Назад»
// Свойства надо выставить ДО вызова этой функции.
// ─────────────────────────────────────────────────────────────────────────
inline void applyArchiveScreenTheme(QWidget *screen)
{
    if (!screen)
        return;

    QFile f(QStringLiteral(":/ui/screen-theme.qss"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("applyArchiveScreenTheme: не загружен ui/screen-theme.qss");
        return;
    }

    // Фон страницы рисуется правилом по этому свойству (см. screen-theme.qss).
    // WA_StyledBackground нужен, чтобы QWidget вообще применял фон из QSS.
    screen->setProperty("archiveScreen", true);
    screen->setAttribute(Qt::WA_StyledBackground, true);

    screen->setStyleSheet(QString::fromUtf8(f.readAll()));
}

#endif // SCREENTHEME_H
