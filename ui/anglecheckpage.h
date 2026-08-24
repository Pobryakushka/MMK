#ifndef ANGLECHECKPAGE_H
#define ANGLECHECKPAGE_H

#include <QWidget>
#include "sensors/amshandler.h"

class QLabel;

namespace Ui {
class AngleCheckPage;
}

// ============================================================
// Страница «Проверка правильности установки углов (ТО-2)» —
// поворот антенны (команда 0xAF) на заданный угол + остановка.
// Положение РПВ отображается компасом RpvIndicator — тем же
// виджетом, что и на экране «Пуск измерения».
// Встроенная страница stackedWidget, доступна из WorkRegulationHubPage.
// ============================================================
class AngleCheckPage : public QWidget
{
    Q_OBJECT
public:
    explicit AngleCheckPage(AMSHandler *amsHandler, QWidget *parent = nullptr);
    ~AngleCheckPage() override;

signals:
    void backRequested();

private slots:
    void onStart();
    void onStop();
    void onRotateStatus(quint8 status, float angle);
    void onAmsConnected();
    void onAmsDisconnected();

private:
    Ui::AngleCheckPage *ui;
    AMSHandler *m_amsHandler;
    bool m_rotating = false;

    void setControlsEnabled(bool enabled);
    void showStatus(const QString &text, bool error = false);
    void updateAmsBanner();
};

#endif // ANGLECHECKPAGE_H
