#ifndef WORKREGULATIONDIALOG_H
#define WORKREGULATIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "amshandler.h"

// ============================================================
// Диалог «Контрольный осмотр (КО)»
// Управление антенной (команда 0xAD): открыть / закрыть
// ============================================================
class InspectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InspectionDialog(AMSHandler *amsHandler, QWidget *parent = nullptr);

private slots:
    void onOpenAntenna();
    void onCloseAntenna();
    void onAntennaStatus(quint8 status);

private:
    AMSHandler  *m_amsHandler;
    QPushButton *m_btnOpen;
    QPushButton *m_btnClose;
    QLabel      *m_lblStatus;

    void setControlsEnabled(bool enabled);
    void showStatus(const QString &text, bool error = false);
};

// ============================================================
// Диалог «Проверка правильности установки углов (ТО-2)»
// Поворот антенны (команда 0xAF) + остановка
// ============================================================
class AngleCheckDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AngleCheckDialog(AMSHandler *amsHandler, QWidget *parent = nullptr);

private slots:
    void onStart();
    void onStop();

private:
    AMSHandler  *m_amsHandler;
    QLineEdit   *m_editAngle;   // Поле ввода угла РПВ
    QPushButton *m_btnStart;
    QPushButton *m_btnStop;
    QLabel      *m_lblStatus;

    void showStatus(const QString &text, bool error = false);
};

// ============================================================
// Главный диалог «Регламентные работы»
// ============================================================
class WorkRegulationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WorkRegulationDialog(AMSHandler *amsHandler, QWidget *parent = nullptr);

private slots:
    void onInspectionClicked();
    void onAngleCheckClicked();

private:
    AMSHandler *m_amsHandler;
};

#endif // WORKREGULATIONDIALOG_H
