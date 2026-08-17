#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include "MeasurementExporter.h"

namespace Ui {
class ExportDialog;
}

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(const MeasurementSnapshot &snap, QWidget *parent = nullptr);
    ~ExportDialog() override;

    ExportOptions getOptions() const;

private slots:
    void onFormatToggled();

private:
    void populateRecordInfo();
    void disableEmptySections();

    Ui::ExportDialog *ui;
    const MeasurementSnapshot &m_snap;
};

#endif // EXPORTDIALOG_H
