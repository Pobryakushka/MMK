#include "ExportDialog.h"
#include "qpushbutton.h"
#include "ui_ExportDialog.h"

ExportDialog::ExportDialog(const MeasurementSnapshot &snap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog)
    , m_snap(snap)
{
    ui->setupUi(this);

    populateRecordInfo();
    disableEmptySections();

    if (QPushButton *btn = ui->buttonBox->button(QDialogButtonBox::Save))
        btn->setText(tr("Сохранить..."));

    auto connectFmt = [this](QRadioButton *rb) {
        connect(rb, &QRadioButton::toggled, this, &ExportDialog::onFormatToggled);
    };
    connectFmt(ui->rbTxt);
    connectFmt(ui->rbCsv);
    connectFmt(ui->rbJson);
    connectFmt(ui->rbPdf);
    connectFmt(ui->rbXlsx);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    ui->stackedOptions->setCurrentIndex(0);
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::populateRecordInfo()
{
    if (m_snap.recordId > 0) {
        ui->lblRecordId->setText(QString::number(m_snap.recordId));
        ui->lblDateTime->setText(
            m_snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
    } else {
        ui->lblRecordId->setText(tr("нет данных"));
        ui->lblDateTime->setText("-");
    }
}

void ExportDialog::disableEmptySections()
{
    auto disable = [](QCheckBox *cb) {
        cb->setChecked(false);
        cb->setEnabled(false);
    };

    if (!m_snap.coordinatesValid) disable(ui->cbCoordinates);
    if (!m_snap.surfaceMeteoValid) disable(ui->cbSurfaceMeteo);
    if (m_snap.avgWind.isEmpty()) disable(ui->cbAvgWind);
    if (m_snap.actualWind.isEmpty()) disable(ui->cbActualWind);
    if (m_snap.measuredWind.isEmpty()) disable(ui->cbMeasuredWind);
    if (m_snap.windShear.isEmpty()) disable(ui->cbWindShear);
    if (!m_snap.meteo11Updated.valid) disable(ui->cbMeteo11Updated);
    if (!m_snap.meteo11Approximate.valid) disable(ui->cbMeteo11Approx);
    if (!m_snap.meteo11FromStation.valid) disable(ui->cbMeteo11Station);
}

void ExportDialog::onFormatToggled()
{
    if (ui->rbCsv->isChecked()) ui->stackedOptions->setCurrentIndex(1);
    else if (ui->rbPdf->isChecked()) ui->stackedOptions->setCurrentIndex(2);
    else ui->stackedOptions->setCurrentIndex(0);
}

ExportOptions ExportDialog::getOptions() const
{
    ExportOptions opts;

    if (ui->rbCsv->isChecked()) opts.format = ExportOptions::CSV;
    else if (ui->rbJson->isChecked()) opts.format = ExportOptions::JSON;
    else if (ui->rbPdf->isChecked()) opts.format = ExportOptions::PDF;
    else if (ui->rbXlsx->isChecked()) opts.format = ExportOptions::XLSX;
    else opts.format = ExportOptions::TXT;

    switch (ui->cmbCsvSeparator->currentIndex()) {
    case 1: opts.csvSeparator = ','; break;
    case 2: opts.csvSeparator = '\t'; break;
    default: opts.csvSeparator = ';'; break;
    }

    static const QPageSize::PageSizeId kSizes[] = {
        QPageSize::A4, QPageSize::A3, QPageSize::Letter
    };
    opts.pdfPageSize = kSizes[qBound(0, ui->cmbPageSize->currentIndex(), 2)];
    opts.pdfLandscape = (ui->cmbOrientation->currentIndex() == 1);
    opts.pdfCharts = ui->cbPdfCharts->isChecked();

    opts.includeCoordinates = ui->cbCoordinates->isChecked();
    opts.includeSurfaceMeteo = ui->cbSurfaceMeteo->isChecked();
    opts.includeAvgWind = ui->cbAvgWind->isChecked();
    opts.includeActualWind = ui->cbActualWind->isChecked();
    opts.includeMeasuredWind = ui->cbMeasuredWind->isChecked();
    opts.includeWindShear = ui->cbWindShear->isChecked();
    opts.includeMeteo11Updated = ui->cbMeteo11Updated->isChecked();
    opts.includeMeteo11Approx = ui->cbMeteo11Approx->isChecked();
    opts.includeMeteo11Station = ui->cbMeteo11Station->isChecked();

    return opts;
}