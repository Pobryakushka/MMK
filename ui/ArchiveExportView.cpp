#include "ArchiveExportView.h"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
        "QLabel { color: #1B211F; font-family: 'Segoe UI','Inter',sans-serif; }"
        "QLabel#expTitle { font-size: 15px; font-weight: 700; }"
        "QLabel#expRecInfo { color: #6E7876; font-size: 11px; }"
        "QLabel#expRecInfo b { color: #1B211F; }"
        "QLabel[class=\"sectionTitle\"] { color: #6E7876; font-size: 11px; font-weight: 600; }"
        "QPushButton#expBackBtn { background: none; border: none; color: #0F6B4F; font-weight: 600; font-size: 13px; text-align: left; }"
        "QPushButton#expBackBtn:hover { text-decoration: underline; }"
        "QPushButton[class=\"fmtCard\"] { border: 1.5px solid #DDE1E3; border-radius: 12px; background: #FFFFFF; padding: 10px; font-family: 'JetBrains Mono','Consolas','Courier New',monospace; font-weight: 800; font-size: 15px; color: #6E7876; }"
        "QPushButton[class=\"fmtCard\"]:hover { border-color: #0F6B4F; }"
        "QPushButton[class=\"fmtCard\"][sel=\"true\"] { border-color: #0F6B4F; background: #E4F1EC; color: #0B5A41; }"
        "QWidget#expOptionsPanel { background: #F7F8F8; border: 1px solid #DDE1E3; border-radius: 12px; }"
        "QPushButton[class=\"segBtn\"] { border: none; background: #FFFFFF; padding: 7px 14px; font-size: 12px; font-weight: 600; color: #6E7876; }"
        "QPushButton[class=\"segBtn\"][sel=\"true\"] { background: #0F6B4F; color: #FFFFFF; }"
        "QWidget[class=\"segGroup\"] { border: 1px solid #DDE1E3; border-radius: 8px; }"
        "QCheckBox { font-size: 12.5px; color: #1B211F; spacing: 8px; }"
        "QCheckBox:disabled { color: #A9AFAD; }"
        // Плоский индикатор чекбокса (без нативной галочки системного QStyle) —
        // залитый зелёный квадрат со скруглением вместо неё, как toggle в макете.
        "QCheckBox::indicator { width: 16px; height: 16px; border: 1.5px solid #DDE1E3; border-radius: 4px; background: #FFFFFF; }"
        "QCheckBox::indicator:hover { border-color: #0F6B4F; }"
        "QCheckBox::indicator:checked { background: #0F6B4F; border-color: #0F6B4F; }"
        "QCheckBox::indicator:disabled { background: #F1F3F2; border-color: #DDE1E3; }"
        "QPushButton#expCancelBtn, QPushButton#expSaveBtn { border-radius: 6px; font-weight: 600; font-size: 13px; padding: 8px 20px; }"
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
    head->setStyleSheet("background: #FFFFFF; border-bottom: 1px solid #DDE1E3;");
    auto *headLayout = new QHBoxLayout(head);
    headLayout->setContentsMargins(20, 14, 20, 14);
    headLayout->setSpacing(14);

    auto *backBtn = new QPushButton("‹ Назад к архиву", head);
    backBtn->setObjectName("expBackBtn");
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, &ArchiveExportView::backRequested);

    auto *title = new QLabel("Экспорт результатов измерений", head);
    title->setObjectName("expTitle");

    m_recordInfo = new QLabel(head);
    m_recordInfo->setObjectName("expRecInfo");
    m_recordInfo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headLayout->addWidget(backBtn);
    headLayout->addWidget(title, 1);
    headLayout->addWidget(m_recordInfo);
    root->addWidget(head);

    // ── Тело ─────────────────────────────────────────────────────────────
    auto *body = new QWidget(this);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(20, 20, 20, 20);
    bodyLayout->setSpacing(6);
    bodyLayout->setAlignment(Qt::AlignTop);

    auto *fmtTitle = new QLabel("Формат файла", body);
    fmtTitle->setProperty("class", "sectionTitle");
    bodyLayout->addWidget(fmtTitle);
    bodyLayout->addSpacing(4);

    auto *fmtRow = new QHBoxLayout();
    fmtRow->setSpacing(10);
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
        fmtRow->addWidget(card, 1);
    }
    bodyLayout->addLayout(fmtRow);
    bodyLayout->addSpacing(18);

    auto *optTitle = new QLabel("Параметры формата", body);
    optTitle->setProperty("class", "sectionTitle");
    bodyLayout->addWidget(optTitle);
    bodyLayout->addSpacing(4);

    auto *optionsPanel = new QWidget(body);
    optionsPanel->setObjectName("expOptionsPanel");
    optionsPanel->setMaximumWidth(760);
    auto *optionsLayout = new QVBoxLayout(optionsPanel);
    optionsLayout->setContentsMargins(16, 14, 16, 14);
    optionsLayout->setSpacing(10);

    auto makeOptRow = [&](const QString &labelText, QWidget *control) -> QWidget* {
        auto *row = new QWidget(optionsPanel);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(14);
        auto *lbl = new QLabel(labelText, row);
        lbl->setFixedWidth(150);
        lbl->setStyleSheet("color: #6E7876; font-size: 12.5px;");
        rowLayout->addWidget(lbl);
        rowLayout->addWidget(control, 1);
        optionsLayout->addWidget(row);
        return row;
    };

    // CSV separator
    auto *csvSegGroup = new QWidget(optionsPanel);
    csvSegGroup->setProperty("class", "segGroup");
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

    // No options placeholder
    auto *noOptLabel = new QLabel("Дополнительных параметров для этого формата нет", optionsPanel);
    noOptLabel->setStyleSheet("color: #6E7876; font-style: italic; font-size: 12.5px;");
    m_noOptionsRow = makeOptRow(QString(), noOptLabel);

    bodyLayout->addWidget(optionsPanel);
    bodyLayout->addSpacing(18);

    auto *secTitle = new QLabel("Разделы для экспорта", body);
    secTitle->setProperty("class", "sectionTitle");
    bodyLayout->addWidget(secTitle);
    bodyLayout->addSpacing(4);

    auto *secGrid = new QGridLayout();
    secGrid->setHorizontalSpacing(20);
    secGrid->setVerticalSpacing(4);
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
        secGrid->addWidget(secBoxes[i], i / 2, i % 2);
    }
    bodyLayout->addLayout(secGrid);
    bodyLayout->addStretch(1);

    root->addWidget(body, 1);

    // ── Футер ────────────────────────────────────────────────────────────
    auto *foot = new QWidget(this);
    foot->setStyleSheet("background: #FFFFFF; border-top: 1px solid #DDE1E3;");
    auto *footLayout = new QHBoxLayout(foot);
    footLayout->setContentsMargins(20, 14, 20, 14);
    footLayout->setSpacing(10);
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
    root->addWidget(foot);

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
    card->setMinimumHeight(64);
    card->setText(ext + "\n" + label);
    return card;
}

QPushButton *ArchiveExportView::makeSegButton(const QString &text)
{
    auto *btn = new QPushButton(text, this);
    btn->setProperty("class", "segBtn");
    btn->setCursor(Qt::PointingHandCursor);
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
    auto apply = [](QCheckBox *cb, bool available) {
        cb->setEnabled(available);
        cb->setChecked(available);
    };
    apply(m_secCoordinates,    m_snap.coordinatesValid);
    apply(m_secSurfaceMeteo,   m_snap.surfaceMeteoValid);
    apply(m_secAvgWind,        !m_snap.avgWind.isEmpty());
    apply(m_secActualWind,     !m_snap.actualWind.isEmpty());
    apply(m_secMeasuredWind,   !m_snap.measuredWind.isEmpty());
    apply(m_secWindShear,      !m_snap.windShear.isEmpty());
    apply(m_secMeteo11Updated, m_snap.meteo11Updated.valid);
    apply(m_secMeteo11Approx,  m_snap.meteo11Approximate.valid);
    apply(m_secMeteo11Station, m_snap.meteo11FromStation.valid);
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
