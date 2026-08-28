#ifndef SCREENTHEME_H
#define SCREENTHEME_H

#include <QWidget>
#include <QFile>
#include <QIODevice>
#include <QDebug>
#include <QPushButton>
#include <QLayout>
#include <QLayoutItem>
#include <QList>
#include <QMargins>

// ─────────────────────────────────────────────────────────────────────────
// Единое место кнопки «‹ Назад» на всех экранах.
//
// Раньше каждый экран задавал отступы шапки и размер кнопки сам, и значения
// разошлись: 24/14, 12/10, 8/8, 6/0 и просто умолчание Qt (9/9), размер
// 96x48 или 80x32. При переходе между экранами кнопка заметно прыгала.
// Теперь позиция задана здесь одна на всё приложение; экран лишь вызывает
// setupArchiveBackButton(ui->btnНазад) вместо ручной пометки свойства.
// ─────────────────────────────────────────────────────────────────────────
static constexpr int kBackPadX  = 16;   // отступ шапки слева
static constexpr int kBackPadY  = 12;   // отступ шапки сверху
static constexpr int kBackWidth = 104;  // минимальная ширина кнопки
static constexpr int kBackHeight = 40;  // фиксированная высота кнопки

// Раскладка, непосредственно содержащая виджет (кнопка может лежать во
// вложенном layout'е шапки, а не в корневом layout'е родителя).
inline QLayout *archiveLayoutOf(QWidget *w)
{
    QWidget *parent = w ? w->parentWidget() : nullptr;
    if (!parent || !parent->layout())
        return nullptr;

    QList<QLayout *> queue{parent->layout()};
    while (!queue.isEmpty()) {
        QLayout *lay = queue.takeFirst();
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem *item = lay->itemAt(i);
            if (item->widget() == w)
                return lay;
            if (item->layout())
                queue.append(item->layout());
        }
    }
    return nullptr;
}

inline void setupArchiveBackButton(QPushButton *back)
{
    if (!back)
        return;

    back->setProperty("nav", true);
    back->setMinimumSize(kBackWidth, kBackHeight);
    back->setMaximumHeight(kBackHeight);

    // Выравниваем только левый и верхний отступы шапки — правый и нижний у
    // экранов осмысленно разные (там лежат заголовок, пилюли состояния и т.п.).
    if (QLayout *lay = archiveLayoutOf(back)) {
        const QMargins m = lay->contentsMargins();
        lay->setContentsMargins(kBackPadX, kBackPadY, m.right(), m.bottom());
    }
}

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
