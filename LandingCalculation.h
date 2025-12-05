#ifndef LANDINGCALCULATION_H
#define LANDINGCALCULATION_H

#include <QDialog>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTableWidgetItem>

namespace Ui {
class LandingCalculation;
}

class LandingCalculation : public QDialog
{
    Q_OBJECT

public:
    explicit LandingCalculation(QWidget *parent = nullptr);
    ~LandingCalculation();

private slots:
    void onEditButtonClicked();
    void onPasswordSubmit();
    void onPasswordCancel();
    void onEditClose();

    void onTableAdd();
    void onTableDelete();
    void onTableUp();
    void onTableDown();
    void onTableApply();
    void onTableEdit();

    void onTableResultsCellClicked(int row, int column);
    void onTableResultsCellChanged(int row, int column);
    void onTableEditParamsItemChanged(QTableWidgetItem *item);

private:
    Ui::LandingCalculation *ui;

    // Хеш пароля (SHA-256)
    // Пароль по умолчанию: "admin" -> хеш ниже
    const QString PASSWORD_HASH = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";

    QString previousCellValue;
    int previousCellRow;
    int previousCellColumn;

    QString hashPassword(const QString &password);
    bool verifyPassword(const QString &password);
    void loadPasswordHash();
    void savePasswordHash(const QString &hash);
    QString getConfigFilePath();

    void setupEditTable();
    void swapTableRows(int row1, int row2);
};

#endif // LANDINGCALCULATION_H
