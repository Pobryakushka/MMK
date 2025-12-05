#ifndef SOURCEDATA_H
#define SOURCEDATA_H
#include <QWidget>
#include <QDialog>
#include <QButtonGroup>

namespace Ui {
class SourceData;
}

class SourceData : public QDialog {
    Q_OBJECT

public:
    explicit SourceData(QWidget *parent = nullptr);
    ~SourceData();

private:
    Ui::SourceData *ui;


};

#endif // SOURCEDATA_H
