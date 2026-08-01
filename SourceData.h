#ifndef SOURCEDATA_H
#define SOURCEDATA_H
#include <QWidget>
#include <QDialog>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QJsonObject>
#include <QDateTime>

// Forward declaration
class GroundMeteoParams;
class Meteo11;

namespace Ui {
class SourceData;
}

class SourceData : public QDialog {
    Q_OBJECT

public:
    explicit SourceData(QWidget *parent = nullptr);
    ~SourceData();

    // Доступ к данным бюллетеня Метео-11
    bool        hasMeteo11Bulletin() const;
    QJsonObject meteo11BulletinJson() const;
    QDateTime   meteo11BulletinTime() const;
    QString     meteo11ValidityPeriod() const;
    void        resetMeteo11Applied();   // вызвать после сохранения в БД

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::SourceData *ui;
    GroundMeteoParams *groundMeteoParams;
    Meteo11           *m_meteo11Dialog;  // постоянный экземпляр — данные не теряются
};

#endif // SOURCEDATA_H