#include "ArchiveExportView.h"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QStyle>
#include <QVariant>

ArchiveExportView::ArchiveExportView(QWidget *parent)
    : QWidget(parent)
    , m_currentFormat(ExportOptions::TXT)
    , m_csvSep(';')
    , m_pdfSize(QPageSize::A4)
    , m_pdfLandscape(false)
{
    setStyleSheet(
        "ArchiveExportView { background: #FFFFFF; }"
        "QWidget#expHead { background: #FFFFFF; border-bottom: 1px solid #DDE1E3; }"
        "QWidget#expFoot { background: #FFFFFF; border-top: 1px solid #DDE1E3; }"
        "QLabel { color: #1B211F; font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; }"
        "QLabel#expTitle { font-size: 14px; font-weight: 700; }"
        "QLabel#expRecInfo { color: #6E7876; font-size: 11px; }"
        "QLabel#expRecInfo b { color: #1B211F; }"
        "QLabel[class=\"sectionTitle\"] { color: #6E7876; font-size: 11px; font-weight: 600; }"
        "QPushButton#expBackBtn { background: none; border: none; border-radius: 6px; color: #0F6B4F; font-weight: 600; font-size: 13px; text-align: left; padding: 4px 8px; }"
        "QPushButton#expBackBtn:hover { background: #E4F1EC; }"
        "QPushButton[class=\"fmtCard\"] { border: 1.5px solid #DDE1E3; outline: none; border-radius: 9px; background: #FFFFFF; padding: 6px; font-family: 'JetBrains Mono','DejaVu Sans Mono','Consolas',monospace; font-weight: 800; font-size: 13px; color: #6E7876; }"
        "QPushButton[class=\"fmtCard\"]:hover { border-color: #0F6B4F; }"
        "QPushButton[class=\"fmtCard\"][sel=\"true\"] { border-color: #0F6B4F; background: #E4F1EC; color: #0B5A41; }"
        "QWidget#expOptionsPanel { background: #F7F8F8; border: 1px solid #DDE1E3; border-radius: 11px; }"
        "QPushButton[class=\"segBtn\"] { border: none; outline: none; background: #FFFFFF; padding: 6px 10px; font-size: 11px; font-weight: 600; color: #6E7876; }"
        "QPushButton[class=\"segBtn\"][sel=\"true\"] { background: #0F6B4F; color: #FFFFFF; }"
        "QWidget[class=\"segGroup\"] { border: 1px solid #DDE1E3; border-radius: 8px; }"
        "QCheckBox { font-size: 12px; color: #1B211F; spacing: 7px; }"
        "QCheckBox:disabled { color: #A9AFAD; }"
        "QWidget#expSecItem { border-bottom: 1px dashed #DDE1E3; }"
        "QLabel#expSecBadge {"
        "  background: #E1E6E4; color: #6E7876; font-size: 9px;"
        "  border-radius: 8px; padding: 2px 6px;"
        "}"
        // Плоский индикатор чекбокса (без нативной галочки системного QStyle) —
        // залитый зелёный квадрат со скруглением вместо неё, как toggle в макете.
        "QCheckBox::indicator { width: 15px; height: 15px; border: 1.5px solid #DDE1E3; border-radius: 4px; background: #FFFFFF; }"
        "QCheckBox::indicator:hover { border-color: #0F6B4F; }"
        "QCheckBox::indicator:checked { background: #0F6B4F; border-color: #0F6B4F;"
        "  image: url(:/icons/checkmark_white.svg); }"
        "QCheckBox::indicator:disabled { background: #F1F3F2; border-color: #DDE1E3; }"
        "QPushButton#expCancelBtn, QPushButton#expSaveBtn { border-radius: 6px; font-weight: 600; font-size: 13px; padding: 8px 18px; }"
        "QPushButton#expCancelBtn { background: #FFFFFF; border: 1px solid #DDE1E3; color: #1B211F; }"
        "QPushButton#expCancelBtn:hover { background: #F3F5F4; }"
        "QPushButton#expSaveBtn { background: #0F6B4F; border: none; color: #FFFFFF; }"
        "QPushButton#expSaveBtn:hover { background: #0B5A41; }"
        );

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Шапка ────────────────────────────────────────────────────────────
    auto *head = new QWidget(this);
    head->setObjectName("expHead");
    head->setAttribute(Qt::WA_StyledBackground, true);
    auto *headLayout = new QHBoxLayout(head);
    headLayout->setContentsMargins(16, 10, 16, 10);
    headLayout->setSpacing(12);

    auto *backBtn = new QPushButton("‹ Назад", head);
    backBtn->setObjectName("expBackBtn");
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, &ArchiveExportView::backRequested);

    auto *title = new QLabel("Экспорт результатов измерений", head);
    title->setObjectName("expTitle");
    title->setAlignment(Qt::AlignCenter);

    m_recordInfo = new QLabel(head);
    m_recordInfo->setObjectName("expRecInfo");
    m_recordInfo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headLayout->addWidget(backBtn);
    headLayout->addWidget(title, 1);
    headLayout->addWidget(m_recordInfo);
    // По умолчанию QHBoxLayout прижимает элементы с фиксированной по высоте
    // политикой размера (кнопка, подписи) к верху строки, а не по центру —
    // из-за этого "‹ Назад к архиву" выглядела заметно выше середины шапки,
    // особенно рядом с более крупным заголовком. Явно центрируем все три
    // элемента по вертикали в границах шапки.
    headLayout->setAlignment(backBtn, Qt::AlignVCenter);
    headLayout->setAlignment(title, Qt::AlignVCenter);
    headLayout->setAlignment(m_recordInfo, Qt::AlignVCenter);
    // Явно фиксируем шапку по высоте — она не должна расти вместе с
    // содержимым и обязана оставаться на месте при любых обстоятельствах.
    head->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    root->addWidget(head);

    // ── Тело ─────────────────────────────────────────────────────────────
    // Раньше body добавлялось в root напрямую (со stretch-фактором) — если
    // содержимое (формат + параметры + разделы) в сумме не помещалось по
    // высоте, слой не мог сжаться и футер с кнопками "Отмена"/"Сохранить"
    // просто выталкивался за пределы видимой области окна, как раньше было
    // с таблицами на других вкладках архива. Оборачиваем body в QScrollArea:
    // шапка и футер с кнопками остаются на месте всегда, а само тело
    // прокручивается при нехватке места.
    auto *bodyScroll = new QScrollArea(this);
    bodyScroll->setWidgetResizable(true);
    bodyScroll->setFrameShape(QFrame::NoFrame);
    bodyScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bodyScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *body = new QWidget(bodyScroll);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(20, 16, 20, 16);
    bodyLayout->setSpacing(5);
    bodyLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *fmtTitle = new QLabel("Формат файла", body);
    fmtTitle->setProperty("class", "sectionTitle");
    fmtTitle->setAlignment(Qt::AlignHCenter);
    bodyLayout->addWidget(fmtTitle);
    bodyLayout->setAlignment(fmtTitle, Qt::AlignHCenter);
    bodyLayout->addSpacing(4);

    auto *fmtRow = new QHBoxLayout();
    fmtRow->setSpacing(8);
    struct { ExportOptions::Format fmt; const char *ext; const char *label; } kFormats[] = {
        { ExportOptions::TXT,  "TXT",  "Текст"   },
        { ExportOptions::CSV,  "CSV",  "Таблица" },
        { ExportOptions::JSON, "JSON", "Данные"  },
        { ExportOptions::PDF,  "PDF",  "Отчёт"   },
        { ExportOptions::XLSX, "XLSX", "Excel"   },
    };
    for (const auto &kf : kFormats) {
        QPushButton *card = makeFormatCard(kf.ext, kf.label);
        m_fmtButtons[kf.fmt] = card;
        connect(card, &QPushButton::clicked, this, [this, fmt = kf.fmt] { selectFormat(fmt); });
        // Раньше все 5 карточек шли со stretch=1 в QHBoxLayout — при равном
        // stretch-факторе, но разной длине текста ("Таблица" длиннее "Excel"),
        // каждая карточка сначала получает свой sizeHint по ширине, и только
        // ОСТАВШЕЕСЯ место делится поровну, поэтому в сумме ширины выходили
        // разными. Явно фиксированная ширина — гарантированно одинаковый
        // размер независимо от текста.
        card->setFixedWidth(118);
        fmtRow->addWidget(card);
    }
    // В макете карточки форматов ограничены по ширине (max-width: 760px),
    // иначе на широком экране они растягиваются в непропорциональные плашки.
    // Раньше этот блок (как и панель параметров, и сетка разделов ниже)
    // просто прижимался к левому краю страницы — теперь центрируем каждый
    // такой блок по горизонтали через выравнивание элемента в bodyLayout.
    auto *fmtRowHolder = new QWidget(body);
    fmtRowHolder->setLayout(fmtRow);
    fmtRow->setContentsMargins(0, 0, 0, 0);
    bodyLayout->addWidget(fmtRowHolder);
    bodyLayout->setAlignment(fmtRowHolder, Qt::AlignHCenter);
    bodyLayout->addSpacing(10);

    auto *optTitle = new QLabel("Параметры формата", body);
    optTitle->setProperty("class", "sectionTitle");
    optTitle->setAlignment(Qt::AlignHCenter);
    bodyLayout->addWidget(optTitle);
    bodyLayout->setAlignment(optTitle, Qt::AlignHCenter);
    bodyLayout->addSpacing(4);

    auto *optionsPanel = new QWidget(body);
    optionsPanel->setObjectName("expOptionsPanel");
    optionsPanel->setAttribute(Qt::WA_StyledBackground, true);
    optionsPanel->setMaximumWidth(680);
    auto *optionsLayout = new QVBoxLayout(optionsPanel);
    optionsLayout->setContentsMargins(12, 10, 12, 10);
    optionsLayout->setSpacing(6);

    auto makeOptRow = [&](const QString &labelText, QWidget *control) -> QWidget* {
        auto *row = new QWidget(optionsPanel);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(12);
        auto *lbl = new QLabel(labelText, row);
        lbl->setFixedWidth(140);
        lbl->setStyleSheet("color: #6E7876; font-size: 12px;");
        rowLayout->addWidget(lbl);
        rowLayout->addWidget(control, 1);
        optionsLayout->addWidget(row);
        return row;
    };

    // CSV separator
    auto *csvSegGroup = new QWidget(optionsPanel);
    csvSegGroup->setProperty("class", "segGroup");
    csvSegGroup->setAttribute(Qt::WA_StyledBackground, true);
    auto *csvSegLayout = new QHBoxLayout(csvSegGroup);
    csvSegLayout->setContentsMargins(0, 0, 0, 0);
    csvSegLayout->setSpacing(0);
    const char *csvLabels[3] = { "; точка с запятой", ", запятая", "Tab" };
    for (int i = 0; i < 3; ++i) {
        m_csvSepButtons[i] = makeSegButton(csvLabels[i]);
        connect(m_csvSepButtons[i], &QPushButton::clicked, this, [this, i] { selectCsvSep(i); });
        csvSegLayout->addWidget(m_csvSepButtons[i]);
    }
    csvSegGroup->setMaximumWidth(320);
    m_csvOptRow = makeOptRow("Разделитель CSV", csvSegGroup);

    // PDF page size
    auto *pdfSizeGroup = new QWidget(optionsPanel);
    pdfSizeGroup->setProperty("class", "segGroup");
    pdfSizeGroup->setAttribute(Qt::WA_StyledBackground, true);
    auto *pdfSizeLayout = new QHBoxLayout(pdfSizeGroup);
    pdfSizeLayout->setContentsMargins(0, 0, 0, 0);
    pdfSizeLayout->setSpacing(0);
    const char *pdfSizeLabels[3] = { "A4", "A3", "Letter" };
    for (int i = 0; i < 3; ++i) {
        m_pdfSizeButtons[i] = makeSegButton(pdfSizeLabels[i]);
        connect(m_pdfSizeButtons[i], &QPushButton::clicked, this, [this, i] { selectPdfSize(i); });
        pdfSizeLayout->addWidget(m_pdfSizeButtons[i]);
    }
    pdfSizeGroup->setMaximumWidth(220);
    m_pdfSizeRow = makeOptRow("Размер страницы PDF", pdfSizeGroup);

    // PDF orientation
    auto *pdfOrientGroup = new QWidget(optionsPanel);
    pdfOrientGroup->setProperty("class", "segGroup");
    pdfOrientGroup->setAttribute(Qt::WA_StyledBackground, true);
    auto *pdfOrientLayout = new QHBoxLayout(pdfOrientGroup);
    pdfOrientLayout->setContentsMargins(0, 0, 0, 0);
    pdfOrientLayout->setSpacing(0);
    const char *pdfOrientLabels[2] = { "Книжная", "Альбомная" };
    for (int i = 0; i < 2; ++i) {
        m_pdfOrientButtons[i] = makeSegButton(pdfOrientLabels[i]);
        connect(m_pdfOrientButtons[i], &QPushButton::clicked, this, [this, i] { selectPdfOrient(i); });
        pdfOrientLayout->addWidget(m_pdfOrientButtons[i]);
    }
    pdfOrientGroup->setMaximumWidth(220);
    m_pdfOrientRow = makeOptRow("Ориентация PDF", pdfOrientGroup);

    // PDF charts
    m_pdfChartsCheck = new QCheckBox("Включить изображения графиков", optionsPanel);
    m_pdfChartsCheck->setChecked(true);
    m_pdfChartsRow = makeOptRow("Графики в PDF", m_pdfChartsCheck);

    // Плейсхолдер "нет доп. параметров" — не обычная строка "подпись: поле"
    // (у него нет подписи слева), поэтому не через makeOptRow (там всегда
    // резервируется фиксированная колонка под подпись, и текст съезжал
    // ближе к левому краю панели, а не по центру).
    auto *noOptLabel = new QLabel("Дополнительных параметров для этого формата нет", optionsPanel);
    noOptLabel->setStyleSheet("color: #6E7876; font-style: italic; font-size: 12px;");
    noOptLabel->setAlignment(Qt::AlignCenter);
    m_noOptionsRow = noOptLabel;
    optionsLayout->addWidget(m_noOptionsRow);

    bodyLayout->addWidget(optionsPanel);
    bodyLayout->setAlignment(optionsPanel, Qt::AlignHCenter);
    bodyLayout->addSpacing(10);

    auto *secTitle = new QLabel("Разделы для экспорта", body);
    secTitle->setProperty("class", "sectionTitle");
    secTitle->setAlignment(Qt::AlignHCenter);
    bodyLayout->addWidget(secTitle);
    bodyLayout->setAlignment(secTitle, Qt::AlignHCenter);
    bodyLayout->addSpacing(4);

    auto *secGrid = new QGridLayout();
    secGrid->setHorizontalSpacing(16);
    secGrid->setVerticalSpacing(2);
    m_secCoordinates    = new QCheckBox("Координаты станции", body);
    m_secSurfaceMeteo   = new QCheckBox("Наземные метеоусловия", body);
    m_secAvgWind        = new QCheckBox("Ветер (осреднённый)", body);
    m_secActualWind     = new QCheckBox("Ветер (фактический)", body);
    m_secMeasuredWind   = new QCheckBox("Ветер (измеренный)", body);
    m_secWindShear      = new QCheckBox("Сдвиг ветра", body);
    m_secMeteo11Updated = new QCheckBox("Метео-11, уточнённый", body);
    m_secMeteo11Approx  = new QCheckBox("Метео-11, приближённый", body);
    m_secMeteo11Station = new QCheckBox("Метео-11, от станции", body);
    QCheckBox *secBoxes[9] = {
        m_secCoordinates, m_secSurfaceMeteo, m_secAvgWind, m_secActualWind,
        m_secMeasuredWind, m_secWindShear, m_secMeteo11Updated,
        m_secMeteo11Approx, m_secMeteo11Station
    };
    for (int i = 0; i < 9; ++i) {
        secBoxes[i]->setChecked(true);
        // Каждый раздел — строка с пунктирным разделителем снизу и местом под
        // бейдж "нет данных" справа (sec-item из макета).
        auto *row = new QWidget(body);
        row->setObjectName("expSecItem");
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(2, 3, 2, 3);
        rowLayout->setSpacing(7);
        rowLayout->addWidget(secBoxes[i], 1);
        auto *badge = new QLabel("нет данных", row);
        badge->setObjectName("expSecBadge");
        badge->setVisible(false);
        m_secBadges[i] = badge;
        rowLayout->addWidget(badge);
        secGrid->addWidget(row, i / 2, i % 2);
    }
    auto *secGridHolder = new QWidget(body);
    secGridHolder->setMaximumWidth(680);
    secGridHolder->setLayout(secGrid);
    secGrid->setContentsMargins(0, 0, 0, 0);
    bodyLayout->addWidget(secGridHolder);
    bodyLayout->setAlignment(secGridHolder, Qt::AlignHCenter);
    bodyLayout->addSpacing(10);

    // ── Футер ────────────────────────────────────────────────────────────
    // "Отмена"/"Сохранить" были отдельным виджетом-панелью, закреплённой
    // ВНЕ прокручиваемой области (в root, а не в bodyLayout) — по идее это
    // должно было гарантированно держать их на месте, но по факту кнопки
    // всё равно продолжали обрезаться снизу окна на реальном экране, и
    // разобраться в точной причине без сборки и запуска не вышло. Более
    // надёжный вариант — сделать кнопки частью самого прокручиваемого
    // содержимого (в конце, после списка разделов): тогда они физически не
    // могут "потеряться" за пределами окна — до них всегда можно долистать
    // прокруткой этой же страницы, как до любого другого элемента формы.
    auto *foot = new QWidget(body);
    foot->setObjectName("expFoot");
    foot->setAttribute(Qt::WA_StyledBackground, true);
    auto *footLayout = new QHBoxLayout(foot);
    footLayout->setContentsMargins(0, 14, 0, 4);
    footLayout->setSpacing(8);
    footLayout->addStretch(1);
    auto *cancelBtn = new QPushButton("Отмена", foot);
    cancelBtn->setObjectName("expCancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &ArchiveExportView::backRequested);
    auto *saveBtn = new QPushButton("Сохранить...", foot);
    saveBtn->setObjectName("expSaveBtn");
    connect(saveBtn, &QPushButton::clicked, this, [this] {
        emit exportRequested(m_snap, buildOptions());
    });
    footLayout->addWidget(cancelBtn);
    footLayout->addWidget(saveBtn);
    bodyLayout->addWidget(foot);
    bodyLayout->setAlignment(foot, Qt::AlignHCenter);

    bodyScroll->setWidget(body);
    root->addWidget(bodyScroll, 1);

    selectFormat(ExportOptions::TXT);
    selectCsvSep(0);
    selectPdfSize(0);
    selectPdfOrient(0);
    updateOptionRows();
}

QPushButton *ArchiveExportView::makeFormatCard(const QString &ext, const QString &label)
{
    auto *card = new QPushButton(this);
    card->setProperty("class", "fmtCard");
    card->setCursor(Qt::PointingHandCursor);
    // Без этого нажатая кнопка сохраняет клавиатурный фокус, а Qt по
    // умолчанию рисует поверх неё пунктирную рамку фокуса — на карточках
    // формата это не так заметно, но на сегментированных кнопках рядом
    // (CSV-разделитель, размер/ориентация PDF) она выглядела как лишняя
    // горизонтальная линия сверху/снизу ровно на стыке кнопок.
    card->setFocusPolicy(Qt::NoFocus);
    card->setMinimumHeight(52);
    card->setText(ext + "\n" + label);
    return card;
}

QPushButton *ArchiveExportView::makeSegButton(const QString &text)
{
    auto *btn = new QPushButton(text, this);
    btn->setProperty("class", "segBtn");
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    return btn;
}

void ArchiveExportView::setSnapshot(const MeasurementSnapshot &snap)
{
    m_snap = snap;

    if (snap.recordId > 0) {
        m_recordInfo->setText(QString("Запись № <b>%1</b><br/>%2")
                                   .arg(snap.recordId)
                                   .arg(snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss")));
    } else {
        m_recordInfo->setText("нет данных");
    }

    selectFormat(ExportOptions::TXT);
    updateSectionsAvailability();
}

void ArchiveExportView::selectFormat(ExportOptions::Format fmt)
{
    m_currentFormat = fmt;
    for (int i = 0; i < 5; ++i) {
        QPushButton *btn = m_fmtButtons[i];
        btn->setProperty("sel", i == fmt);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
    updateOptionRows();
}

void ArchiveExportView::selectCsvSep(int idx)
{
    static const QChar seps[3] = { ';', ',', '\t' };
    m_csvSep = seps[qBound(0, idx, 2)];
    for (int i = 0; i < 3; ++i) {
        m_csvSepButtons[i]->setProperty("sel", i == idx);
        m_csvSepButtons[i]->style()->unpolish(m_csvSepButtons[i]);
        m_csvSepButtons[i]->style()->polish(m_csvSepButtons[i]);
    }
}

void ArchiveExportView::selectPdfSize(int idx)
{
    static const QPageSize::PageSizeId sizes[3] = {
        QPageSize::A4, QPageSize::A3, QPageSize::Letter
    };
    m_pdfSize = sizes[qBound(0, idx, 2)];
    for (int i = 0; i < 3; ++i) {
        m_pdfSizeButtons[i]->setProperty("sel", i == idx);
        m_pdfSizeButtons[i]->style()->unpolish(m_pdfSizeButtons[i]);
        m_pdfSizeButtons[i]->style()->polish(m_pdfSizeButtons[i]);
    }
}

void ArchiveExportView::selectPdfOrient(int idx)
{
    m_pdfLandscape = (idx == 1);
    for (int i = 0; i < 2; ++i) {
        m_pdfOrientButtons[i]->setProperty("sel", i == idx);
        m_pdfOrientButtons[i]->style()->unpolish(m_pdfOrientButtons[i]);
        m_pdfOrientButtons[i]->style()->polish(m_pdfOrientButtons[i]);
    }
}

void ArchiveExportView::updateOptionRows()
{
    m_csvOptRow->setVisible(m_currentFormat == ExportOptions::CSV);
    m_pdfSizeRow->setVisible(m_currentFormat == ExportOptions::PDF);
    m_pdfOrientRow->setVisible(m_currentFormat == ExportOptions::PDF);
    m_pdfChartsRow->setVisible(m_currentFormat == ExportOptions::PDF);
    m_noOptionsRow->setVisible(m_currentFormat == ExportOptions::TXT ||
                                m_currentFormat == ExportOptions::JSON ||
                                m_currentFormat == ExportOptions::XLSX);
}

void ArchiveExportView::updateSectionsAvailability()
{
    // Порядок должен совпадать с порядком создания чекбоксов в конструкторе —
    // бейдж "нет данных" ищется по тому же индексу.
    const bool availability[9] = {
        m_snap.coordinatesValid,
        m_snap.surfaceMeteoValid,
        !m_snap.avgWind.isEmpty(),
        !m_snap.actualWind.isEmpty(),
        !m_snap.measuredWind.isEmpty(),
        !m_snap.windShear.isEmpty(),
        m_snap.meteo11Updated.valid,
        m_snap.meteo11Approximate.valid,
        m_snap.meteo11FromStation.valid,
    };
    QCheckBox *boxes[9] = {
        m_secCoordinates, m_secSurfaceMeteo, m_secAvgWind, m_secActualWind,
        m_secMeasuredWind, m_secWindShear, m_secMeteo11Updated,
        m_secMeteo11Approx, m_secMeteo11Station
    };
    for (int i = 0; i < 9; ++i) {
        boxes[i]->setEnabled(availability[i]);
        boxes[i]->setChecked(availability[i]);
        if (m_secBadges[i])
            m_secBadges[i]->setVisible(!availability[i]);
    }
}

ExportOptions ArchiveExportView::buildOptions() const
{
    ExportOptions opts;
    opts.format = m_currentFormat;
    opts.csvSeparator = m_csvSep;
    opts.pdfPageSize = m_pdfSize;
    opts.pdfLandscape = m_pdfLandscape;
    opts.pdfCharts = m_pdfChartsCheck->isChecked();

    opts.includeCoordinates   = m_secCoordinates->isChecked();
    opts.includeSurfaceMeteo  = m_secSurfaceMeteo->isChecked();
    opts.includeAvgWind       = m_secAvgWind->isChecked();
    opts.includeActualWind    = m_secActualWind->isChecked();
    opts.includeMeasuredWind  = m_secMeasuredWind->isChecked();
    opts.includeWindShear     = m_secWindShear->isChecked();
    opts.includeMeteo11Updated = m_secMeteo11Updated->isChecked();
    opts.includeMeteo11Approx  = m_secMeteo11Approx->isChecked();
    opts.includeMeteo11Station = m_secMeteo11Station->isChecked();

    return opts;
}
