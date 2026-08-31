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
    // Правила ниже покрывают и "служебные" части виджетов — полосы прокрутки,
    // индикаторы чекбоксов, стрелку комбобокса, шапки таблиц. Если их не
    // покрыть, именно через них и проступает вид Fusion.
    //
    // Здесь задаются только статические правила. Переключаемые состояния
    // (выбранный тип бюллетеня Метео-11, активная вкладка, наличие записей
    // за дату) выражены динамическими свойствами и селекторами вида
    // [state="..."] — так состояние не требует setStyleSheet() на конкретном
    // виджете, который стирал бы остальные правила для него.
    setStyleSheet(
        // ── палитра макета ────────────────────────────────────────────────
        // green #0F6B4F · green-dark #0B5A41 · green-soft #E4F1EC
        // bg #EFF1F1 · card #FFFFFF · border #DDE1E3
        // text #1B211F · mute #6E7876 · amber #F9A825
        "QWidget#MeasurementResults, QWidget#archivePage, QStackedWidget#rootStack {"
        "  background-color: #EFF1F1;"
        "}"
        "QWidget { font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; color: #1B211F; }"

        // ── левая панель ─────────────────────────────────────────────────
        "QFrame#titleWidget { background: #FFFFFF; border: none; border-right: 1px solid #DDE1E3; }"
        "QFrame#railHead, QFrame#sectionDateTime, QFrame#sectionParams, QFrame#railFoot {"
        "  background: transparent; border: none;"
        "}"
        "QFrame#sectionDateTime, QFrame#sectionParams, QFrame#railFoot {"
        "  border-top: 1px solid #DDE1E3;"
        "}"
        "QLabel#lblTitle { color: #1B211F; font-size: 15px; font-weight: 700; }"
        "QLabel#lblSubtitle { color: #6E7876; font-size: 11px; }"
        "QLabel#capDateTime, QLabel#capParams {"
        "  color: #6E7876; font-size: 11px; font-weight: 600; padding-bottom: 2px;"
        "}"
        "QPushButton#btnPrevDate, QPushButton#btnNextDate {"
        "  background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 6px;"
        "  color: #0B5A41; font-size: 13px; padding: 0px;"
        "}"
        "QPushButton#btnPrevDate:hover, QPushButton#btnNextDate:hover { background: #E4F1EC; }"
        "QPushButton#btnPrevDate:disabled, QPushButton#btnNextDate:disabled {"
        "  color: #B7BEBB; background: #F7F8F8;"
        "}"
        "QPushButton#btnSelectDate {"
        "  background: #E4F1EC; border: 2px solid #0F6B4F; border-radius: 6px;"
        "  color: #0B5A41; font-weight: 700; font-size: 13px;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "}"
        "QPushButton#btnSelectDate:hover { background: #D8ECE3; }"
        "QPushButton#btnSelectDate[state=\"narrow\"] { font-size: 12px; padding: 0px 1px; }"
        "QLabel#lblAvailableRecords { color: #6E7876; font-size: 11px; font-style: italic; }"
        "QLabel#lblAvailableRecords[state=\"empty\"] { color: #B03A2E; }"

        // Строки параметров станции: подпись слева, значение справа моноширинным,
        // разделитель пунктиром — как param-row в макете. У последней строки
        // разделителя нет.
        "QFrame#rowLatitude, QFrame#rowLongitude, QFrame#rowAltitude {"
        "  background: transparent; border: none; border-bottom: 1px dashed #DDE1E3;"
        "}"
        "QFrame#rowDirectionAngle { background: transparent; border: none; }"
        "QLabel#lblLatitude, QLabel#lblLongitude, QLabel#lblAltitude, QLabel#lblDirectionAngle {"
        "  color: #6E7876; font-size: 12px;"
        "}"
        "QLabel#valLatitude, QLabel#valLongitude, QLabel#valAltitude, QLabel#valDirectionAngle {"
        "  color: #1B211F; font-size: 12px; font-weight: 600;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "}"

        "QPushButton#btnExport, QPushButton#btnClose {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; background: #FFFFFF;"
        "  font-weight: 600; font-size: 13px; color: #1B211F;"
        "}"
        "QPushButton#btnExport:hover { background: #F3F5F4; }"
        "QPushButton#btnClose { background: #0F6B4F; border-color: #0F6B4F; color: #FFFFFF; }"
        "QPushButton#btnClose:hover { background: #0B5A41; border-color: #0B5A41; }"

        // ── статусная строка над вкладками ────────────────────────────────
        "QLabel#lblDataStatus {"
        "  background: #FFFFFF; border-bottom: 1px solid #DDE1E3; color: #6E7876;"
        "  font-weight: 600; font-size: 10px; padding: 6px 14px;"
        "}"
        "QLabel#lblDataStatus[state=\"empty\"] { color: #B03A2E; }"

        // ── самодельная строка вкладок (нативная QTabBar скрыта) ──────────
        "QWidget#customTabBar { background: #EFF1F1; }"
        "QPushButton#archiveTabButton {"
        "  background: #E4E7E6; color: #6E7876; font-weight: 600; font-size: 10px;"
        "  padding: 5px 9px; border: none;"
        "  border-top-left-radius: 8px; border-top-right-radius: 8px;"
        "  border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;"
        "}"
        "QPushButton#archiveTabButton:hover { color: #0B5A41; }"
        "QPushButton#archiveTabButton[active=\"true\"] {"
        "  background: #FFFFFF; color: #0B5A41; border-bottom: 2px solid #0F6B4F;"
        "}"
        "QFrame#dataGroup { background: #EFF1F1; border: none; }"
        "QTabWidget::pane { border: none; background: #FFFFFF; }"
        "QTabWidget > QWidget { background: #FFFFFF; }"

        // ── карточки графиков (chart-card из макета) ──────────────────────
        "QFrame#cardFrame_avgWindSpeed, QFrame#cardFrame_avgWindDir,"
        "QFrame#cardFrame_realWindSpeed, QFrame#cardFrame_realWindDir,"
        "QFrame#cardFrame_measWindSpeed, QFrame#cardFrame_measWindDir,"
        "QFrame#cardFrame_shearSpeed, QFrame#cardFrame_shearDir {"
        "  border: 1px solid #DDE1E3; border-radius: 10px; background: #FFFFFF;"
        "}"
        "QwtPlot { border: none; background: transparent; }"
        "QLabel#label_avgWindSpeed, QLabel#label_avgWindDirection,"
        "QLabel#label_realWindSpeed, QLabel#label_realWindDirection,"
        "QLabel#label_measuredWindSpeed, QLabel#label_measuredWindDirection,"
        "QLabel#label_shearSpeed, QLabel#label_shearDirection {"
        "  background: #E4F1EC; color: #0B5A41; font-weight: 600; font-size: 12px;"
        "  border-top-left-radius: 9px; border-top-right-radius: 9px;"
        "  border-bottom: 1px solid #DDE1E3; padding: 9px 12px;"
        "}"

        // ── таблицы ──────────────────────────────────────────────────────
        // Эти же правила вынесены в общий стиль приложения
        // ui/table-theme.qss и действуют на все таблицы программы. Здесь
        // они продублированы намеренно: как стиль конкретного виджета они
        // перекрывают общий и гарантируют, что «Архив измерений»
        // выглядит ровно так же, даже если общий стиль позже поправят.
        "QTableWidget, QTableView {"
        // border-radius убран (был 8px): Qt не обрезает содержимое
        // QAbstractScrollArea по скруглённой рамке — только красит саму
        // рамку скруглённой, а прямоугольные ячейки (особенно чередующиеся
        // серые строки) всё равно рисуются поверх без обрезки и торчат
        // острыми углами за пределы скругления, особенно заметно в нижних
        // углах и при прокрутке. Проще и надёжнее просто не скруглять.
        "  border: 1px solid #DDE1E3; gridline-color: #EEF0EF;"
        "  background: #FFFFFF; alternate-background-color: #F7F8F8; font-size: 12px;"
        "  selection-background-color: #E4F1EC; selection-color: #0B5A41;"
        "}"
        "QTableWidget::item, QTableView::item { padding: 5px 8px; border: none; }"
        "QTableWidget::item:selected, QTableView::item:selected { background: #E4F1EC; color: #0B5A41; }"
        "QHeaderView { background: transparent; border: none; }"
        "QHeaderView::section {"
        "  background: #E4F1EC; color: #0B5A41; font-weight: 600; font-size: 12px;"
        "  border: none; border-bottom: 1px solid #DDE1E3; border-right: 1px solid #EEF0EF;"
        "  padding: 7px 8px;"
        "}"
        "QHeaderView::section:last { border-right: none; }"
        // Скруглённые верхние углы шапки. Заливка секций (#E4F1EC)
        // прямоугольная, а рамка таблицы скруглена (border-radius приходит из
        // общего ui/table-theme.qss), из-за чего зелёный фон шапки торчал за
        // рамку в левом и правом верхних углах. Скругляем углы крайних секций
        // под ту же кривизну.
        "QHeaderView::section:first { border-top-left-radius: 7px; }"
        "QHeaderView::section:last { border-top-right-radius: 7px; }"
        "QHeaderView::section:only-one { border-top-left-radius: 7px; border-top-right-radius: 7px; }"
        "QTableCornerButton::section { background: #E4F1EC; border: none; border-bottom: 1px solid #DDE1E3; }"
        // В «Наземных условиях» подпись параметра живёт в вертикальном
        // заголовке, но по макету это обычная ячейка, а не шапка таблицы.
        "QTableWidget#tableWidget_parm1b65 QHeaderView::section {"
        "  background: transparent; color: #1B211F; font-weight: 400;"
        "  border: none; border-bottom: 1px solid #EEF0EF; padding: 7px 10px;"
        "}"
        "QTableWidget#tableWidget_parm1b65[state=\"narrow\"] QHeaderView::section {"
        "  font-size: 11px; padding: 7px 6px;"
        "}"

        // ── поля ввода и выпадающие списки ───────────────────────────────
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; padding: 4px 6px; background: #FFFFFF;"
        "  selection-background-color: #E4F1EC; selection-color: #0B5A41;"
        "}"
        "QLineEdit:read-only { background: #F7F8F8; }"
        "QComboBox {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; padding: 4px 8px; background: #FFFFFF;"
        "}"
        "QComboBox:disabled { background: #F7F8F8; color: #9AA3A0; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox QAbstractItemView {"
        "  border: 1px solid #DDE1E3; border-radius: 6px; background: #FFFFFF;"
        "  selection-background-color: #E4F1EC; selection-color: #0B5A41; outline: none;"
        "}"

        // ── кнопки по умолчанию ──────────────────────────────────────────
        "QPushButton {"
        "  background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 6px;"
        "  padding: 7px 13px; color: #1B211F; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #F3F5F4; border-color: #0F6B4F; }"
        "QPushButton:disabled { color: #B7BEBB; background: #F1F3F2; border-color: #E6E9E8; }"

        // Тулбар Метео-11 — плоские контейнеры без рамки и заголовка,
        // сами кнопки-пилюли переключаются через свойство [pressed].
        "QGroupBox#groupBox_bulletenType, QGroupBox#groupBox_bulletenFormat {"
        "  border: none; background: transparent; margin: 0; padding: 0;"
        "}"
        "QGroupBox { border: 1px solid #DDE1E3; border-radius: 10px; background: #FFFFFF;"
        "  margin-top: 10px; padding-top: 8px; }"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 6px;"
        "  color: #6E7876; font-size: 11px; font-weight: 600; background: transparent;"
        "}"
        "QPushButton[pill=\"true\"] {"
        "  background: #FFFFFF; border: 1px solid #DDE1E3; border-radius: 8px;"
        "  color: #6E7876; font-weight: 600; font-size: 12px; padding: 8px 13px;"
        "}"
        "QPushButton[pill=\"true\"]:hover { border-color: #0F6B4F; color: #0B5A41; }"
        "QPushButton[pill=\"true\"][pressed=\"true\"] {"
        "  background: #0F6B4F; border-color: #0F6B4F; color: #FFFFFF;"
        "}"
        "QPushButton[pill=\"true\"]:disabled { background: #F1F3F2; border-color: #E6E9E8; color: #B7BEBB; }"
        // Бюллетень старше 12 ч — янтарная пилюля-предупреждение
        "QPushButton[pill=\"true\"][state=\"stale\"] {"
        "  background: #FFF4DC; border-color: #F9A825; color: #8A6100;"
        "}"
        "QPushButton[pill=\"true\"][state=\"stale\"][pressed=\"true\"] {"
        "  background: #E65100; border-color: #E65100; color: #FFFFFF;"
        "}"

        // Бейдж годности бюллетеня и время составления
        "QLineEdit#lineEdit_bulleten {"
        "  border: none; border-radius: 10px; padding: 5px 12px; font-weight: 700;"
        "  font-size: 11px; background: #E4F1EC; color: #0B5A41;"
        "}"
        "QLineEdit#lineEdit_bulleten[state=\"warn\"] { background: #FFF4DC; color: #8A6100; }"
        "QLineEdit#lineEdit_bulleten[state=\"none\"] { background: #F1F3F2; color: #9AA3A0; }"
        "QLineEdit#lineEdit_bulleten[state=\"bad\"]  { background: #FBE4E4; color: #B3261E; }"
        "QLineEdit#lineEdit_bulletenTime {"
        "  border: none; background: transparent; color: #6E7876; font-size: 11px;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "}"
        "QLineEdit#lineEdit_bulletenTime[state=\"stale\"] {"
        "  background: #FFF4DC; color: #8A6100; border-radius: 6px; padding: 2px 8px; font-weight: 700;"
        "}"
        // Табличный вид Метео-11: компактные подписи и узкие поля значений —
        // без этого подписи наследуют крупный шрифт из .ui и строки расползаются.
        "QWidget#page_meteo11_table QLabel { font-size: 10px; color: #6E7876; font-weight: 400; }"
        "QWidget#page_meteo11_table QLineEdit {"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "  font-size: 11px; font-weight: 600; color: #1B211F; max-width: 130px; padding: 1px 4px;"
        "}"
        "QWidget#page_meteo11_approximate QLabel { font-size: 10px; color: #6E7876; }"
        "QWidget#page_meteo11_approximate QLineEdit {"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace;"
        "  font-size: 11px; color: #1B211F; padding: 1px 4px;"
        "}"
        "QTextEdit#textEdit_meteo11, QTextEdit#textEdit_meteo11_updated {"
        "  background: #F7F8F8; border: 1px solid #DDE1E3; border-radius: 8px; padding: 12px 14px;"
        "  font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace; font-size: 13px;"
        "}"

        // ── чекбоксы ─────────────────────────────────────────────────────
        "QCheckBox { spacing: 8px; font-size: 12px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px;"
        "  border: 1px solid #C7CDCB; background: #FFFFFF; }"
        "QCheckBox::indicator:hover { border-color: #0F6B4F; }"
        // Галочку QSS сам не рисует, если фон индикатора задан стилшитом,
        // поэтому подкладываем её изображением из ресурсов.
        "QCheckBox::indicator:checked { background: #0F6B4F; border-color: #0F6B4F;"
        "  image: url(:/icons/checkmark_white.svg); }"
        "QCheckBox::indicator:disabled { background: #F1F3F2; border-color: #E6E9E8; }"

        // ── полосы прокрутки (иначе через них проступает Fusion) ─────────
        "QScrollBar:vertical { background: transparent; width: 11px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: #C7CDCB; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #A9B2AF; }"
        "QScrollBar:horizontal { background: transparent; height: 11px; margin: 0px; }"
        "QScrollBar::handle:horizontal { background: #C7CDCB; border-radius: 5px; min-width: 30px; }"
        "QScrollBar::handle:horizontal:hover { background: #A9B2AF; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; border: none; background: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: none; }"
        "QAbstractScrollArea::corner { background: transparent; border: none; }"

        "QToolTip { background: #1B211F; color: #FFFFFF; border: none; padding: 5px 8px; }"
        );
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
