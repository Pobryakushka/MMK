#ifndef SOURCEDATA_H
#define SOURCEDATA_H
#include <QWidget>
#include <QDialog>
#include <QButtonGroup>
#include <QCloseEvent>

// Forward declaration
class GroundMeteoParams;

namespace Ui {
class SourceData;
}

class SourceData : public QDialog {
    Q_OBJECT

public:
    explicit SourceData(QWidget *parent = nullptr);
    ~SourceData();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::SourceData *ui;
    GroundMeteoParams *groundMeteoParams; // Постоянный экземпляр
};

#endif // SOURCEDATA_H
