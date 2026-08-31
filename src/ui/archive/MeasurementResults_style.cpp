// ─────────────────────────────────────────────────────────────────────────
// Внешний вид страницы архива: общий QSS, таблицы, оформление графиков
// Qwt, самодельная строка вкладок и адаптивная (планшетная) компоновка.
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

// Переключение визуального состояния виджета через динамическое свойство.
// Именно так (а не через setStyleSheet() на самом виджете) состояние можно
// менять, не стирая остальные правила из общего стилшита диалога: setStyleSheet()
// на конкретном виджете имеет приоритет над стилшитом предка и заменяет собой
// весь его набор правил для этого виджета.
void MeasurementResults::setWidgetState(QWidget *w, const QString &state)
{
    if (!w) return;
    if (w->property("state").toString() == state) return;
    w->setProperty("state", state);
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

// Бейдж годности бюллетеня — пилюля шириной по тексту, цвет задаётся
// состоянием (ok/warn/bad/none), правила для которых лежат в applyArchiveStyle().
void MeasurementResults::setBulletinBadge(const QString &text, const QString &state)
{
    QLineEdit *badge = ui->lineEdit_bulleten;
    if (!badge) return;
    badge->setText(text);
    setWidgetState(badge, state);
    // QLineEdit по умолчанию просит ширину «под 17 символов» независимо от
    // содержимого — для пилюли считаем ширину по фактическому тексту.
    badge->setFixedWidth(badge->fontMetrics().horizontalAdvance(text) + 30);
}

// Блок бюллетеня в макете — карточка по высоте текста, а не поле во всю
// панель. QTextEdit сам по себе всегда занимает всё доступное место, поэтому
// после каждой смены содержимого подгоняем высоту под документ.
void MeasurementResults::fitMeteo11TextHeight()
{
    QTextEdit *view = ui->textEdit_meteo11;
    if (!view) return;

    view->document()->setTextWidth(view->viewport()->width());
    const int docHeight = qCeil(view->document()->size().height());
    const int frame     = 2 * view->frameWidth() + 28; // рамка + вертикальные отступы из QSS
    view->setFixedHeight(qBound(90, docHeight + frame, 460));

    // Иначе единственный элемент сетки центрируется по вертикали и карточка
    // с бюллетенем «висит» посреди пустой вкладки вместо верха, как в макете.
    if (ui->gridLayout_meteo11_string)
        ui->gridLayout_meteo11_string->setAlignment(view, Qt::AlignTop);
}

// Табличный вид Метео-11: слева компактная сетка расшифрованных полей,
// справа таблица ПП/ТТДДСС во всю оставшуюся ширину. В .ui у сетки крупные
// шрифты и растянутые строки, из-за чего вкладка выглядела разреженной.
void MeasurementResults::setupMeteo11TableLayout()
{
    if (QGridLayout *g = ui->gridLayout_Meteo11Params) {
        g->setVerticalSpacing(6);
        g->setHorizontalSpacing(12);
        g->setContentsMargins(0, 0, 16, 0);
        g->setColumnStretch(0, 1);
        g->setColumnStretch(1, 0);
        g->setRowStretch(g->rowCount(), 1);   // прижать поля к верху

        // Подписи вроде "Отклонение наземной виртуальной температуры, °С
        // (Т0Т0):" — однострочные QLabel без переноса, их естественная
        // ширина легко превышает всю доступную ширину вкладки на планшете и
        // раздвигала весь grid (а вместе с ним и соседнюю таблицу ПП/ТТДДСС)
        // далеко вправо за видимую область. Включаем перенос по словам и
        // даём подписям потолок ширины, чтобы они переносились на 2-3 строки
        // вместо того, чтобы тянуть колонку в одну линию.
        for (int i = 0; i < g->count(); ++i) {
            if (QLabel *lbl = qobject_cast<QLabel *>(g->itemAt(i)->widget())) {
                lbl->setWordWrap(true);
                lbl->setMaximumWidth(190);
            }
        }
    }
    // Нижний отступ вкладки: в .ui у verticalLayout_meteo11 заданы только
    // левый/верхний/правый (16/14/16), а нижний остаётся стилевым (9), из-за
    // чего содержимое подходит к низу вкладки ближе, чем к её бокам. Берём его
    // равным боковому, чтобы поля вкладки были симметричными.
    int pad = 16;
    if (QVBoxLayout *tab = ui->verticalLayout_meteo11) {
        const QMargins m = tab->contentsMargins();
        pad = m.left();
        tab->setContentsMargins(m.left(), m.top(), m.right(), pad);
    }
    // Тот же отступ внутри вкладочной QScrollArea: докрутив содержимое до
    // конца, пользователь видит нижнюю скруглённую рамку таблицы с полем под
    // ней, а не срезанной краем вьюпорта.
    if (QGridLayout *g = ui->gridLayout_4)
        g->setContentsMargins(0, 0, 0, pad);
    if (QTableWidget *t = ui->tableWidget_meteo11Formalize) {
        t->setMinimumWidth(280);
        // Высоту таблицы определяет свободное место во вкладке, а не её
        // собственный sizeHint. Иначе вкладочная QScrollArea не даёт содержимому
        // сжаться: подписи параметров переносятся по словам, поэтому у раскладки
        // содержимого есть heightForWidth, а QScrollArea в этом случае держит
        // содержимое не ниже heightForWidth — а тот считается по sizeHint строк,
        // то есть по 192 точкам стандартного sizeHint QTableWidget, независимо
        // от того, что таблице хватило бы и меньшего. Пол высоты остаётся за
        // минимумом из updateMeteo11TableHeight(), который QSizePolicy::Ignored
        // не отменяет.
        QSizePolicy sp = t->sizePolicy();
        sp.setVerticalPolicy(QSizePolicy::Ignored);
        t->setSizePolicy(sp);
    }
    updateMeteo11TableHeight();
}

// Нижний предел высоты таблицы ПП/ТТДДСС — шапка и две строки, по метрикам
// самой таблицы, а не пиксельной константой, чтобы следовать за системным
// шрифтом.
//
// Предел намеренно низкий: вместе с панелью параметров он обязан помещаться в
// самое низкое окно, которое вообще допускает приложение (минимум страницы —
// 572 точки по высоте). Иначе содержимое вкладки перестаёт влезать во вьюпорт,
// у вкладочной QScrollArea включается прокрутка — и нижний край таблицы
// срезается её границей. Именно так это и проявлялось: при сужении окна тулбар
// вкладки встаёт в два ряда (см. applyResponsiveLayout) и забирает ещё около
// 50 точек по высоте, поэтому на окне высотой 600-640 таблица обрезалась
// снизу, хотя при широком окне той же высоты помещалась целиком. Строки,
// которые не влезли, листает собственная прокрутка таблицы, и рамка остаётся
// целой при любой высоте окна.
//
// Потолка высоты здесь нет намеренно: таблица занимает всю оставшуюся высоту
// вкладки (растяжка её строки в setMeteo11TableStacked), а строки, которые не
// поместились, листаются её собственной прокруткой — так же, как в макете, где
// карточка таблицы продолжается ниже последней строки. Пробовал ограничить
// высоту содержимым: с setFixedHeight содержимое вкладки становится выше её
// вьюпорта на невысоком окне (замер на 986x866 — реальный размер окна на
// планшете), вкладка начинает прокручиваться и нижний край таблицы уезжает под
// границу экрана; с setMaximumHeight раскладка прижимает таблицу к низу
// растянутой ячейки, отрывая её от панели параметров.
//
// От нижнего края таблицу держат поля раскладки (см. setupMeteo11TableLayout):
// её рамка стоит в 32 точках от низа окна, а не в 21, как было.
void MeasurementResults::updateMeteo11TableHeight()
{
    QTableWidget *table = ui->tableWidget_meteo11Formalize;
    if (!table)
        return;

    const int headerH = table->horizontalHeader()->sizeHint().height();
    const int rowH    = table->verticalHeader()->defaultSectionSize();

    table->setMinimumHeight(headerH + 2 * rowH + 2 * table->frameWidth());
}

// Табличный вид Метео-11: расшифрованные поля и таблица ПП/ТТДДСС стоят рядом
// в широком окне и друг под другом в узком. Бок о бок на планшете каждой
// колонке достаётся около 270 точек — подписи полей обрезаются, а значения
// сжимаются до одного символа.
void MeasurementResults::setMeteo11TableStacked(bool stacked)
{
    QGridLayout  *outer  = ui->gridLayout_4;
    QGridLayout  *params = ui->gridLayout_Meteo11Params;
    QTableWidget *table  = ui->tableWidget_meteo11Formalize;
    if (!outer || !params || !table) return;

    outer->removeItem(params);
    outer->removeWidget(table);

    if (stacked) {
        outer->addLayout(params, 0, 0);
        outer->addWidget(table,  1, 0);
        outer->setColumnStretch(0, 1);
        outer->setColumnStretch(2, 0);
        // Панель параметров сверху по своей высоте, таблица забирает всю
        // оставшуюся высоту вкладки, а строки, которые не поместились,
        // листаются её собственной прокруткой. От нижнего края экрана её
        // держат поля раскладки, заданные в setupMeteo11TableLayout().
        outer->setRowStretch(0, 0);
        outer->setRowStretch(1, 1);
        outer->setRowStretch(2, 0);
    } else {
        outer->addLayout(params, 0, 0);
        outer->addWidget(table,  0, 2);
        outer->setColumnStretch(0, 3);
        outer->setColumnStretch(2, 2);
        outer->setRowStretch(0, 1);
        outer->setRowStretch(1, 0);
        outer->setRowStretch(2, 0);
    }
    outer->invalidate();
}

// Меняет готовую QHBoxLayout из формы на FlowLayout с теми же виджетами.
// Проще, чем переносить ряды кнопок в .ui: Qt Designer не умеет пользовательские
// раскладки, а поведение нужно только одно — перенос по ширине.
void MeasurementResults::replaceWithFlowLayout(QLayout *source, int spacing)
{
    if (!source) return;
    QWidget *host = source->parentWidget();
    if (!host) return;

    const QMargins margins = source->contentsMargins();

    QVector<QWidget *> widgets;
    while (QLayoutItem *item = source->takeAt(0)) {
        if (QWidget *w = item->widget())
            widgets.append(w);
        delete item;   // распорки из ряда кнопок больше не нужны
    }

    // Раскладку нельзя просто заменить у виджета, пока старая жива:
    // QWidget::setLayout() ругается, если layout уже установлен.
    QLayout *parentLayout = nullptr;
    if (host->layout() != source) {
        // вложенная раскладка — ищем её владельца, чтобы вставить новую на то же место
        parentLayout = host->layout();
    }

    if (parentLayout) {
        auto *flow = new FlowLayout(nullptr, 0, spacing, spacing);
        flow->setContentsMargins(margins);
        for (QWidget *w : qAsConst(widgets))
            flow->addWidget(w);
        if (auto *box = qobject_cast<QBoxLayout *>(parentLayout)) {
            const int index = box->indexOf(source);
            box->removeItem(source);
            delete source;
            box->insertLayout(qMax(0, index), flow);
        } else {
            delete flow;
        }
        return;
    }

    delete source;
    auto *flow = new FlowLayout(host, 0, spacing, spacing);
    flow->setContentsMargins(margins);
    for (QWidget *w : qAsConst(widgets))
        flow->addWidget(w);
}

// Планшетная (узкая) компоновка. На 1200x1920 при масштабе 150% окну достаётся
// 800 логических точек по ширине: два графика рядом превращаются в две
// нечитаемые полоски, а боковая панель съедает треть экрана. Ниже эти места
// переключаются по фактической ширине окна, а не по признаку устройства —
// так один и тот же код работает и в портретной, и в альбомной ориентации.
void MeasurementResults::applyResponsiveLayout(int width)
{
    const bool narrow = (width < kNarrowWidthThreshold);
    if (m_narrowLayout == narrow && m_responsiveApplied) return;
    m_narrowLayout = narrow;
    m_responsiveApplied = true;

    // Боковая панель: в узком окне отдаём основной области больше места, но не
    // настолько, чтобы в кнопку выбора даты перестала помещаться строка
    // "22.08.2026 11:54" — вместе с уменьшенным шрифтом 250 точек хватает.
    // Ширину панели задаём жёстко: одного максимума мало — при нехватке места
    // раскладка сжимает её до минимума, и в кнопку выбора даты перестаёт
    // помещаться строка вида "06.04.2026 15:10".
    if (ui->titleWidget) {
        const int railWidth = narrow ? 250 : 270;
        ui->titleWidget->setMinimumWidth(railWidth);
        ui->titleWidget->setMaximumWidth(railWidth);
    }
    setWidgetState(ui->btnSelectDate, narrow ? "narrow" : "");

    // Тулбар Метео-11: в узком окне группы «тип бюллетеня» и «формат вывода»
    // встают друг под друга — иначе группе типа достаётся половина ширины и
    // её четыре кнопки вытягиваются в столбец по одной. Распорка между
    // группами при этом схлопывается: в вертикальном ряду она превращается в
    // пустой промежуток в десятки точек.
    if (QHBoxLayout *toolbar = ui->horizontalLayout_meteo11_buttons) {
        toolbar->setDirection(narrow ? QBoxLayout::TopToBottom
                                     : QBoxLayout::LeftToRight);
        for (int i = 0; i < toolbar->count(); ++i) {
            if (QSpacerItem *sp = toolbar->itemAt(i)->spacerItem()) {
                sp->changeSize(narrow ? 0 : 40, narrow ? 8 : 20,
                               narrow ? QSizePolicy::Minimum : QSizePolicy::Expanding,
                               QSizePolicy::Minimum);
            }
        }
        toolbar->invalidate();
    }

    // Графики: всегда рядом по горизонтали, в любой ширине окна. Данные —
    // это профиль по высотам, значение (скорость/направление) вторично, а
    // высота — по оси Y; поставленные друг под другом графики в узком окне
    // раньше отдавали всю ширину под малоинформативную ось X и вдвое ужимали
    // высоту, на которой как раз и нужно читать показания. Бок о бок каждому
    // графику достаётся уже вертикаль вкладки почти целиком.
    const QList<QHBoxLayout *> chartRows = {
        ui->chartsRow_avgWind, ui->chartsRow_realWind,
        ui->chartsRow_measWind, ui->chartsRow_shear
    };
    for (QHBoxLayout *row : chartRows)
        if (row)
            row->setDirection(QBoxLayout::LeftToRight);

    // Немного сузили ещё раз по просьбе пользователя: графики и так уже
    // всегда рядом по горизонтали (см. выше), но 380/440 всё же оставляли
    // таблице по высотам маловато места на экране — снизили, чтобы таблица
    // под графиками открывалась пошире, до появления собственной прокрутки.
    const QList<QFrame *> cards = {
        ui->cardFrame_avgWindSpeed, ui->cardFrame_avgWindDir,
        ui->cardFrame_realWindSpeed, ui->cardFrame_realWindDir,
        ui->cardFrame_measWindSpeed, ui->cardFrame_measWindDir,
        ui->cardFrame_shearSpeed, ui->cardFrame_shearDir
    };
    for (QFrame *card : cards)
        if (card)
            // Ещё раз чуть ниже (было 240/270) — по отзыву "почти всё
            // хорошо, но ещё чуть-чуть".
            card->setMaximumHeight(narrow ? 210 : 235);

    // Таблица под графиками (и таблица Метео-11) теперь внутри своей
    // QScrollArea и раньше ничем не ограничивалась по высоте — росла вместе
    // со всеми строками, поэтому при прокрутке страницы не было видно, где
    // заканчивается сама таблица (просто обрез по краю внешней прокрутки).
    // Даём таблице собственный потолок высоты — тогда виден её нижний край
    // (рамка), а лишние строки листаются её родной прокруткой.
    const QList<QTableWidget *> chartTables = {
        ui->tableWidget_AverageWind, ui->tableWidget_realWind,
        ui->tableWidget_izmWind_2, ui->table_windShear
    };
    for (QTableWidget *table : chartTables)
        if (table)
            table->setMaximumHeight(narrow ? 200 : 230);

    // Таблице бюллетеня Метео-11 высоту здесь не трогаем: она задана по её
    // собственному содержимому в updateMeteo11TableHeight() и от ширины окна
    // не зависит.

    // Метео-11: раньше переключалось между "рядом" (широкий экран) и "друг
    // над другом" (узкий) по общему порогу kNarrowWidthThreshold — но этот
    // порог считается по ширине всей страницы архива (с учётом левой
    // панели), а не по фактической ширине, доступной именно этой вкладке,
    // и таблица бюллетеня всё равно норовила уехать вправо за экран в
    // варианте "рядом". Раз в полный рост это не помогло даже после переноса
    // подписей параметров — переводим таблицу Метео-11 в режим "друг над
    // другом" всегда, независимо от ширины: так параметры и таблица по
    // очереди получают всю ширину вкладки целиком, и делить её пополам
    // никогда не нужно.
    setMeteo11TableStacked(true);

    // «Наземные условия»: подпись параметра живёт в вертикальном заголовке, и
    // его ширину QHeaderView берёт по самой длинной подписи. На планшете это
    // съедало почти всю панель, и от колонки значения оставалась полоска у
    // правого края — поэтому в узком окне ширина жёстко ограничивается, а
    // шрифт подписи уменьшается, чтобы текст в неё помещался.
    if (QTableWidget *t = ui->tableWidget_parm1b65) {
        QHeaderView *vh = t->verticalHeader();
        if (narrow) {
            vh->setFixedWidth(340);
        } else {
            vh->setMaximumWidth(QWIDGETSIZE_MAX);
            vh->setMinimumWidth(360);
        }
        setWidgetState(t, narrow ? "narrow" : "");
    }

    // В таблице сдвига ветра четыре колонки; на планшете полные заголовки в
    // них не помещаются и обрезаются на середине слова.
    if (QTableWidget *t = ui->table_windShear) {
        const QStringList wide   = { "Высота, м", "Скорость, м/с/30м",
                                     "Изменение направления, °", "Уровень" };
        const QStringList compact = { "Высота, м", "Скор., м/с/30м",
                                      "Δ напр., °", "Уровень" };
        t->setHorizontalHeaderLabels(narrow ? compact : wide);
    }

    // Статусная строка над вкладками в узком окне переносится на две строки
    if (ui->lblDataStatus)
        ui->lblDataStatus->setWordWrap(narrow);
}

// ─────────────────────────────────────────────────────────────────────────────
// Визуальный стиль архива — зелёная палитра/типографика по макету
// "archive v1 docked sidebar.html". Здесь задаются только статические
// (не зависящие от состояния) правила; переключаемые состояния (выбранная
// вкладка бюллетеня Метео-11, активный формат экспорта и т.п.) по-прежнему
// переключаются точечно через setStyleSheet() конкретных виджетов — как это
// уже было принято в остальном коде этого класса.
// ─────────────────────────────────────────────────────────────────────────────
void MeasurementResults::applyArchiveStyle()
{
    // Стиль архива — QSS поверх Fusion (фиксируется глобально в main.cpp).
    // Fusion выбран потому, что ничего не дорисовывает там, где оформление
    // задано стилшитом; системные темы (например cleanlooks на Astra/Fly)
    // игнорируют часть правил и рисуют поверх свой объёмный chrome.
    //
    // Сами правила лежат в ui/theme/archive-style.qss — отдельным файлом
    // оформления, как screen-theme.qss и table-theme.qss, а не строковой
    // константой посреди кода. Они покрывают и "служебные" части виджетов
    // (полосы прокрутки, индикаторы чекбоксов, стрелку комбобокса, шапки
    // таблиц): если их не покрыть, именно через них и проступает вид Fusion.
    //
    // В файле только статические правила. Переключаемые состояния (выбранный
    // тип бюллетеня Метео-11, активная вкладка, наличие записей за дату)
    // выражены динамическими свойствами и селекторами вида [state="..."] —
    // так состояние не требует setStyleSheet() на конкретном виджете,
    // который стирал бы остальные правила для него.
    QFile qss(QStringLiteral(":/ui/archive-style.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        setStyleSheet(QString::fromUtf8(qss.readAll()));
    else
        qWarning("applyArchiveStyle: не загружен ui/archive-style.qss");
}

// Единая настройка таблиц архива. В макете таблица занимает всю ширину
// панели, строки равной высоты чередуются полосами, вертикального заголовка
// нет (высота вынесена в обычную колонку), выделение — светло-зелёное.
void MeasurementResults::setupArchiveTables()
{
    const QList<QTableWidget *> tables = {
        ui->tableWidget_AverageWind, ui->tableWidget_realWind,
        ui->tableWidget_izmWind_2,   ui->table_windShear,
        ui->tableWidget_meteo11Formalize
    };

    for (QTableWidget *t : tables) {
        if (!t) continue;
        t->verticalHeader()->setVisible(false);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        t->setFocusPolicy(Qt::NoFocus);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setSelectionMode(QAbstractItemView::SingleSelection);
        t->setWordWrap(false);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->horizontalHeader()->setHighlightSections(false);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        // Явный маленький минимум секции — без этого Stretch всё равно не
        // даёт колонке сжаться меньше рассчитанного по шрифту заголовка
        // минимума, и на узкой странице (особенно у таблицы Метео-11 внутри
        // своей QScrollArea) таблица могла вылезать вправо за видимую область.
        t->horizontalHeader()->setMinimumSectionSize(28);
        t->verticalHeader()->setDefaultSectionSize(32);
        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    // Наземные условия: подпись параметра остаётся в вертикальном заголовке,
    // поэтому он виден и занимает основную ширину, а колонка значения узкая.
    if (QTableWidget *t = ui->tableWidget_parm1b65) {
        t->verticalHeader()->setVisible(true);
        t->horizontalHeader()->setVisible(false);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        t->setFocusPolicy(Qt::NoFocus);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionMode(QAbstractItemView::NoSelection);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        t->verticalHeader()->setDefaultSectionSize(36);
        t->verticalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->verticalHeader()->setMinimumWidth(360);
        // Таблица наземных условий короткая и фиксированной длины — прижимаем
        // её к верху вкладки по фактической высоте строк, как в макете, вместо
        // растягивания пустой рамки на всю панель.
        t->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        t->setFixedHeight(t->rowCount() * 36 + 2 * t->frameWidth() + 2);
        t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // Иначе единственный элемент сетки центрируется по вертикали и
        // таблица «висит» посреди пустой вкладки.
        if (ui->gridLayout_12)
            ui->gridLayout_12->setAlignment(t, Qt::AlignTop);
    }
}

// Оформление графиков Qwt под макет: карточка рисуется рамкой QFrame вокруг,
// поэтому сам график остаётся плоским и прозрачным. Ось высот вертикальная —
// привычный вид профиля; из макета взяты только цвета, пунктирная сетка и
// отсутствие подписей осей (название вынесено в зелёную шапку карточки).
void MeasurementResults::styleArchivePlot(QwtPlot *plot)
{
    if (!plot) return;

    plot->setAutoFillBackground(false);
    plot->setCanvasBackground(Qt::white);
    plot->setContentsMargins(0, 0, 0, 0);
    plot->plotLayout()->setCanvasMargin(0);
    plot->plotLayout()->setAlignCanvasToScales(true);

    if (auto *canvas = qobject_cast<QwtPlotCanvas *>(plot->canvas())) {
        canvas->setFrameStyle(QFrame::NoFrame);
        canvas->setBorderRadius(0);
        canvas->setPalette(QColor(Qt::white));
    }

    // Заголовок и подписи осей убраны — их роль играет шапка карточки
    plot->setTitle(QwtText());
    plot->setAxisTitle(QwtPlot::xBottom, QwtText());
    plot->setAxisTitle(QwtPlot::yLeft,   QwtText());

    QFont tickFont = plot->font();
    tickFont.setPixelSize(10);
    for (int axis : {QwtPlot::xBottom, QwtPlot::yLeft}) {
        plot->setAxisFont(axis, tickFont);
        if (QwtScaleWidget *sw = plot->axisWidget(axis)) {
            QPalette pal = sw->palette();
            pal.setColor(QPalette::WindowText, QColor("#C7CDCB")); // линия оси и засечки
            pal.setColor(QPalette::Text,       QColor("#6E7876")); // цифры на оси
            sw->setPalette(pal);
            sw->setMargin(0);
            sw->setSpacing(4);
        }
    }
}

// Единая пунктирная сетка макета (#EEF0EF) — заменяет чёрно-серую по умолчанию.
QwtPlotGrid *MeasurementResults::makeArchiveGrid()
{
    auto *grid = new QwtPlotGrid();
    grid->setMajorPen(QPen(QColor("#E2E6E4"), 0, Qt::DashLine));
    grid->enableXMin(false);
    grid->enableYMin(false);
    return grid;
}

// Линия профиля: тонкая цветная кривая с белыми точками-кружками, как в макете.
void MeasurementResults::styleArchiveCurve(QwtPlotCurve *curve, const QColor &color)
{
    if (!curve) return;
    curve->setPen(QPen(color, 2));
    curve->setStyle(QwtPlotCurve::Lines);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                   QBrush(Qt::white), QPen(color, 2), QSize(6, 6)));
}

// Строит самодельную строку вкладок поверх скрытой нативной QTabBar (см.
// комментарий у объявления m_customTabBar в .h) — единственный способ
// гарантированно получить плоские скруглённые сверху вкладки макета
// независимо от системной темы/QStyle.
void MeasurementResults::setupCustomTabBar()
{
    QTabWidget *tabs = ui->tabWidget;
    if (!tabs) return;

    // Одного hide() недостаточно: QTabWidget при перекладке (setUpLayout)
    // возвращает своей QTabBar видимость, и системные вкладки снова
    // проступают поверх самодельной строки. Дополнительно зажимаем её
    // высоту в ноль, чтобы она не занимала места ни в каком случае.
    tabs->tabBar()->hide();
    tabs->tabBar()->setFixedHeight(0);

    m_customTabBar = new QWidget(this);
    m_customTabBar->setObjectName("customTabBar");
    // Раньше здесь был FlowLayout, переносивший лишние вкладки на вторую
    // строку — из-за этого высота строки вкладок "прыгала" (1 или 2 строки)
    // и съедала место у контента под ней. Вкладки должны быть в один ряд
    // всегда; если все шесть подряд не помещаются по ширине — строка вкладок
    // скроллится по горизонтали (обёрнута в scrollArea_tabBar ниже), а не
    // переносится вниз.
    auto *layout = new QHBoxLayout(m_customTabBar);
    layout->setSpacing(2);
    layout->setContentsMargins(16, 8, 16, 0);

    m_tabButtons.clear();
    for (int i = 0; i < tabs->count(); ++i) {
        auto *btn = new QPushButton(tabs->tabText(i), m_customTabBar);
        btn->setObjectName("archiveTabButton");
        btn->setProperty("active", i == tabs->currentIndex());
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(btn, &QPushButton::clicked, this, [tabs, i] { tabs->setCurrentIndex(i); });
        layout->addWidget(btn);
        m_tabButtons.append(btn);
    }
    layout->addStretch(1);

    connect(tabs, &QTabWidget::currentChanged, this, &MeasurementResults::updateCustomTabBarHighlight);

    // Строка вкладок — сама по себе узкая полоска фиксированной высоты
    // внутри горизонтального scrollArea без рамки и без вертикальной
    // прокрутки: обёртка нужна только на случай, если сумма ширин вкладок
    // всё же превысит ширину экрана.
    auto *tabBarScroll = new QScrollArea(this);
    tabBarScroll->setObjectName("scrollArea_tabBar");
    tabBarScroll->setWidget(m_customTabBar);
    tabBarScroll->setWidgetResizable(false);
    tabBarScroll->setFrameShape(QFrame::NoFrame);
    tabBarScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabBarScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tabBarScroll->setFixedHeight(36);
    tabBarScroll->setStyleSheet("QScrollArea#scrollArea_tabBar { background: #EFF1F1; border: none; }");

    // Вставляем строку кнопок между статусной строкой (row 0) и самим
    // QTabWidget (row 1) в gridLayout_2, сдвигая tabWidget на row 2.
    QGridLayout *grid = ui->gridLayout_2;
    if (grid) {
        grid->removeWidget(tabs);
        grid->addWidget(tabBarScroll, 1, 0);
        grid->addWidget(tabs, 2, 0);
    }
}

void MeasurementResults::updateCustomTabBarHighlight(int currentIndex)
{
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        QPushButton *btn = m_tabButtons[i];
        if (!btn) continue;
        btn->setProperty("active", i == currentIndex);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

// Дополнительные поля АМС/зонда (координаты, поправки, даты) на странице
// "приближённый" бюллетень Метео-11 отсутствуют в новом макете архива —
// оставляем функциональность, но по умолчанию сворачиваем блок за кнопкой.
void MeasurementResults::setupAmsProbeCollapse()
{
    m_amsProbeWidgets = {
        ui->label_amsDegrees1, ui->lineEdit_AMSDegrees1, ui->label_amsMinutes1, ui->lineEdit_AMSMinutes1,
        ui->label_amsSeconds1, ui->lineEdit_AMSSeconds1, ui->label_amsLongitude, ui->lineEdit_AMSLongtitude,
        ui->label_amsDegrees2, ui->lineEdit_AMSDegrees2, ui->label_amsMinutes2, ui->lineEdit_AMSMinutes2,
        ui->label_amsSeconds2, ui->lineEdit_AMSSeconds2, ui->label_amsLatitude, ui->lineEdit_AMSLatitude,
        ui->label_ams_title,
        ui->label_minutesFromProbe, ui->lineEdit_AMSMinutesFromProbe, ui->label_probeDistance, ui->lineEdit_ProbeDistance,
        ui->label_bulletinType, ui->lineEdit_AMSTypeBl, ui->label_bulletinDate, ui->lineEdit_AMSDateBl,
        ui->label_amsBias, ui->lineEdit_AMSBias,
        ui->label_lastProbeDate, ui->lineEdit_DateLastUTC, ui->label_biasUTC, ui->lineEdit_BiasUTC,
        ui->label_avgTime, ui->lineEdit_avgTime,
        ui->label_probeDegrees1, ui->lineEdit_probeDegrees1, ui->label_probeMinutes1, ui->lineEdit_probeMinutes1,
        ui->label_probeSeconds1, ui->lineEdit_probeSeconds1, ui->label_probeLongitude, ui->lineEdit_probeLongtitude,
        ui->label_probeDegrees2, ui->lineEdit_probeDegrees2, ui->label_probeMinutes2, ui->lineEdit_probeMinutes2,
        ui->label_probeSeconds2, ui->lineEdit_probeSeconds2, ui->label_probeLatitude, ui->lineEdit_probeLatitude,
        ui->label_probe_title,
    };

    for (QWidget *w : qAsConst(m_amsProbeWidgets))
        if (w) w->setVisible(false);

    auto *toggleBtn = new QPushButton("Показать доп. поля АМС/зонда ▾", ui->page_meteo11_approximate);
    toggleBtn->setStyleSheet(
        "QPushButton { border: 1px solid #DDE1E3; border-radius: 8px; background: #F7F8F8;"
        " color: #6E7876; font-size: 11px; font-weight: 600; padding: 6px 12px; }"
        "QPushButton:hover { border-color: #0F6B4F; color: #0B5A41; }");
    connect(toggleBtn, &QPushButton::clicked, this, [this, toggleBtn] {
        m_amsProbeFieldsVisible = !m_amsProbeFieldsVisible;
        for (QWidget *w : qAsConst(m_amsProbeWidgets))
            if (w) w->setVisible(m_amsProbeFieldsVisible);
        toggleBtn->setText(m_amsProbeFieldsVisible
                                ? "Скрыть доп. поля АМС/зонда ▴"
                                : "Показать доп. поля АМС/зонда ▾");
    });

    // Добавляем новой строкой в конец существующей сетки — не задевает
    // расположение уже имеющихся полей.
    if (ui->gridLayout_approximate)
        ui->gridLayout_approximate->addWidget(toggleBtn, ui->gridLayout_approximate->rowCount(), 0, 1, -1);
}
