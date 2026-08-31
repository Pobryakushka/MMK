#ifndef INSPECTIONPAGE_H
#define INSPECTIONPAGE_H

#include <QWidget>
#include "devices/ams/amshandler.h"
#include "ui/widgets/notificationtoast.h"

namespace Ui {
class InspectionPage;
}

// ============================================================
// Страница «Контрольный осмотр (КО)» — управление антенной
// (команда 0xAD): открыть / закрыть. Встроенная страница
// stackedWidget (не QDialog — см. WorkRegulationPage/GroundMeteoParams
// для причины), доступна из WorkRegulationHubPage.
// ============================================================
class InspectionPage : public QWidget
{
    Q_OBJECT
public:
    explicit InspectionPage(AMSHandler *amsHandler, QWidget *parent = nullptr);
    ~InspectionPage() override;

signals:
    void backRequested();

private slots:
    void onOpenAntenna();
    void onCloseAntenna();
    void onAntennaStatus(quint8 status);
    void onAmsConnected();
    void onAmsDisconnected();

private:
    Ui::InspectionPage *ui;
    AMSHandler *m_amsHandler;
    NotificationToast *m_toast;

    void setControlsEnabled(bool enabled);
    void showStatus(const QString &text, NotificationToast::Kind kind = NotificationToast::Info);
    void updateAmsBanner();
};

#endif // INSPECTIONPAGE_H
