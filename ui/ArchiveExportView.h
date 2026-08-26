#ifndef ARCHIVEEXPORTVIEW_H
#define ARCHIVEEXPORTVIEW_H

#include <QWidget>
#include "MeasurementExporter.h"
// MeasurementSnapshot хранится здесь ПО ЗНАЧЕНИЮ (m_snap) — для этого нужны
// полные определения типов её QVector-полей уже в заголовке (а не только
// в .cpp), иначе падает компиляция moc-файла, который включает только этот
// заголовок.
#include "sensors/amsprotocol.h"
#include "calculationAlgorithms/WindShearCalculator.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QWidget;

// Встроенный экран экспорта результатов измерений — заменяет модальный
// ExportDialog, подменяя содержимое окна архива (см. #exportView в макете
// "archive v1 docked sidebar.html"). Сама логика экспорта (сборка ExportOptions,
// генерация файла) остаётся в MeasurementExporter — этот класс только
// отвечает за представление формы выбора формата/опций/разделов.
class ArchiveExportView : public QWidget
{
    Q_OBJECT
public:
    explicit ArchiveExportView(QWidget *parent = nullptr);

    void setSnapshot(const MeasurementSnapshot &snap);

signals:
    void backRequested();
    void exportRequested(const MeasurementSnapshot &snap, const ExportOptions &opts);

private:
    MeasurementSnapshot m_snap;
    ExportOptions::Format m_currentFormat;

    QLabel *m_recordInfo;

    QPushButton *m_fmtButtons[5]; // индекс = ExportOptions::Format

    QWidget *m_csvOptRow;
    QPushButton *m_csvSepButtons[3];
    QChar m_csvSep;

    QWidget *m_pdfSizeRow;
    QWidget *m_pdfOrientRow;
    QWidget *m_pdfChartsRow;
    QPushButton *m_pdfSizeButtons[3];
    QPageSize::PageSizeId m_pdfSize;
    QPushButton *m_pdfOrientButtons[2];
    bool m_pdfLandscape;
    QCheckBox *m_pdfChartsCheck;

    QWidget *m_noOptionsRow;

    QCheckBox *m_secCoordinates;
    QCheckBox *m_secSurfaceMeteo;
    QCheckBox *m_secAvgWind;
    QCheckBox *m_secActualWind;
    QCheckBox *m_secMeasuredWind;
    QCheckBox *m_secWindShear;
    QCheckBox *m_secMeteo11Updated;
    QCheckBox *m_secMeteo11Approx;
    QCheckBox *m_secMeteo11Station;

    void selectFormat(ExportOptions::Format fmt);
    void selectCsvSep(int idx);
    void selectPdfSize(int idx);
    void selectPdfOrient(int idx);
    void updateOptionRows();
    void updateSectionsAvailability();
    ExportOptions buildOptions() const;
    QPushButton *makeFormatCard(const QString &ext, const QString &label);
    QPushButton *makeSegButton(const QString &text);
};

#endif // ARCHIVEEXPORTVIEW_H
