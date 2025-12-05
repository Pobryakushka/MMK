#include "LandingCalculation.h"
#include "ui_LandingCalculation.h"
#include <QTextStream>

LandingCalculation::LandingCalculation(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LandingCalculation),
    previousCellRow(-1),
    previousCellColumn(-1)
{
    ui->setupUi(this);

//    setFixedSize(size());

//    setupSizes();

    // Загружаем хеш пароля из файла (если есть)
    loadPasswordHash();

    // Устанавливаем начальную страницу - обычный режим
    ui->stackedWidget->setCurrentIndex(0);

    // Подключаем сигналы
    connect(ui->btnLandingCalcEdit, &QPushButton::clicked,
            this, &LandingCalculation::onEditButtonClicked);

    connect(ui->btnPasswordSubmit, &QPushButton::clicked,
            this, &LandingCalculation::onPasswordSubmit);

    connect(ui->btnPasswordCancel, &QPushButton::clicked,
            this, &LandingCalculation::onPasswordCancel);

    connect(ui->btnClose, &QPushButton::clicked,
            this, &QDialog::close);

    connect(ui->btnEditClose, &QPushButton::clicked,
            this, &LandingCalculation::onEditClose);

    // Enter в поле пароля
    connect(ui->editPassword, &QLineEdit::returnPressed,
            ui->btnPasswordSubmit, &QPushButton::click);

    connect(ui->btnTableAdd, &QPushButton::clicked,
            this, &LandingCalculation::onTableAdd);

    connect(ui->btnTableDelete, &QPushButton::clicked,
            this, &LandingCalculation::onTableDelete);

    connect(ui->btnTableUp, &QPushButton::clicked,
            this, &LandingCalculation::onTableUp);

    connect(ui->btnTableDown, &QPushButton::clicked,
            this, &LandingCalculation::onTableDown);

    connect(ui->btnTableApply, &QPushButton::clicked,
            this, &LandingCalculation::onTableApply);

    connect(ui->btnTableEdit, &QPushButton::clicked,
            this, &LandingCalculation::onTableEdit);

    connect(ui->tableResults, &QTableWidget::cellClicked,
            this, &LandingCalculation::onTableResultsCellClicked);

    connect(ui->tableResults, &QTableWidget::cellChanged,
            this, &LandingCalculation::onTableResultsCellChanged);

//    connect(ui->tabWidget, &QTabWidget::currentChanged,
//            this, &LandingCalculation::onTabChanged);

//    resize(initialDataSize);
//    setFixedSize(initialDataSize);

//    setupEditTable();
}

LandingCalculation::~LandingCalculation()
{
    delete ui;
}

QString LandingCalculation::hashPassword(const QString &password)
{
    // Вычисляем SHA-256 хеш пароля
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(),
        QCryptographicHash::Sha256
    );
    return QString(hash.toHex());
}

bool LandingCalculation::verifyPassword(const QString &password)
{
    QString inputHash = hashPassword(password);
    return inputHash == PASSWORD_HASH;
}

QString LandingCalculation::getConfigFilePath()
{
    // Получаем путь к директории конфигурации приложения
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(configPath);
    }
    return configPath + "/password.conf";
}

void LandingCalculation::loadPasswordHash()
{
    // Загружаем хеш пароля из файла конфигурации
    QString configFile = getConfigFilePath();
    QFile file(configFile);

    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString savedHash = in.readLine().trimmed();
        file.close();

        // Если хеш валидный (длина SHA-256 = 64 символа)
        if (savedHash.length() == 64) {
            // Можно загрузить сохраненный хеш
            // Используется константа из кода
        }
    }
}

void LandingCalculation::savePasswordHash(const QString &hash)
{
    // Сохраняем хеш пароля в файл конфигурации
    QString configFile = getConfigFilePath();
    QFile file(configFile);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << hash;
        file.close();
    }
}

void LandingCalculation::onEditButtonClicked()
{
    // Переключаемся на страницу ввода пароля
    ui->stackedWidget->setCurrentIndex(1);
    ui->editPassword->clear();
    ui->editPassword->setFocus();
}

void LandingCalculation::onPasswordSubmit()
{
    QString password = ui->editPassword->text();

    if (verifyPassword(password)) {
        // Пароль верный - переключаемся на режим редактирования
        ui->stackedWidget->setCurrentIndex(2);
        ui->editPassword->clear();
    } else {
        // Неверный пароль
        QMessageBox::warning(this,
                           "Ошибка",
                           "Неверный пароль!\n\nПопробуйте еще раз.",
                           QMessageBox::Ok);
        ui->editPassword->clear();
        ui->editPassword->setFocus();
    }
}

void LandingCalculation::onPasswordCancel()
{
    // Возвращаемся в обычный режим
    ui->stackedWidget->setCurrentIndex(0);
    ui->editPassword->clear();
}

void LandingCalculation::onEditClose()
{
    // Возвращаемся в обычный режим из режима редактирования
    ui->stackedWidget->setCurrentIndex(0);
}

void LandingCalculation::setupEditTable()
{
    for (int row = 0; row < ui->tableEditParams->rowCount(); ++row){
        for (int col = 0; col < ui->tableEditParams->columnCount(); ++col){
            QTableWidgetItem *item = ui->tableEditParams->item(row, col);
            if (!item){
                item = new QTableWidgetItem("");
                ui->tableEditParams->setItem(row, col, item);
            }

            if (col == 0) {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            } else {
                item->setFlags(item->flags() | Qt::ItemIsEditable);
            }
        }
    }
}

void LandingCalculation::onTableAdd()
{
    int currentRow = ui->tableEditParams->currentRow();

    if (currentRow < 0) {
        currentRow = ui->tableEditParams->rowCount() - 1;
    }

    ui->tableEditParams->insertRow(currentRow + 1);

    for (int col = 0; col < ui->tableEditParams->columnCount(); ++col){
        QTableWidgetItem *item = new QTableWidgetItem("");
        ui->tableEditParams->setItem(currentRow + 1, col, item);

        if (col == 0){
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }

    ui->tableEditParams->setCurrentCell(currentRow +1, 0);
}

void LandingCalculation::onTableDelete()
{
    int currentRow = ui->tableEditParams->currentRow();

    if (currentRow < 0){
        QMessageBox::warning(this, "Внимание", "Выберите строку для удаления.");
        return;
    }
    QTableWidgetItem *nameItem = ui->tableEditParams->item(currentRow, 0);
    if (nameItem && !nameItem->text().trimmed().isEmpty()){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Подтверждение", "Удалить выбранную строку", QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes){
            ui->tableEditParams->removeRow(currentRow);
        }
    } else {
        ui->tableEditParams->removeRow(currentRow);
    }
}

void LandingCalculation::onTableUp()
{
    int currentRow = ui->tableEditParams->currentRow();

    if (currentRow <= 0){
        QMessageBox::warning(this, "Внимание", "Невозможно переместить строку вверх.");
        return;
    }

    swapTableRows(currentRow, currentRow - 1);
    ui->tableEditParams->setCurrentCell(currentRow - 1, 0);
}

void LandingCalculation::onTableDown()
{
    int currentRow = ui->tableEditParams->currentRow();

    if (currentRow < 0 || currentRow >= ui->tableEditParams->rowCount() - 1){
        QMessageBox::warning(this, "Внимание", "Невозможно переместить строку вниз.");
        return;
    }

    swapTableRows(currentRow, currentRow + 1);
    ui->tableEditParams->setCurrentCell(currentRow + 1, 0);
}

void LandingCalculation::swapTableRows(int row1, int row2)
{
    for (int col = 0; col < ui->tableEditParams->columnCount(); ++col){
        QTableWidgetItem *item1 = ui->tableEditParams->takeItem(row1, col);
        QTableWidgetItem *item2 = ui->tableEditParams->takeItem(row2, col);

        ui->tableEditParams->setItem(row1, col, item2);
        ui->tableEditParams->setItem(row2, col, item1);
    }
}

void LandingCalculation::onTableApply()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Подтверждение", "Применить изменения параметров", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes){
        // Логика сохранения данных в БД или файл

        QMessageBox::information(this, "Успешно", "Параметры успешно применены");
    }
}

void LandingCalculation::onTableEdit()
{
    int currentRow = ui->tableEditParams->currentRow();
    int currentCol = ui->tableEditParams->currentColumn();

    if (currentRow < 0 || currentCol < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите ячейку для редактирования");
        return;
    }

    QTableWidgetItem *item = ui->tableEditParams->item(currentRow, currentCol);

    if (!item) {
        item = new QTableWidgetItem("");
        ui->tableEditParams->setItem(currentRow, currentCol, item);
    }
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->tableEditParams->editItem(item);
}

void LandingCalculation::onTableEditParamsItemChanged(QTableWidgetItem *item)
{
    if (!item) return;

    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
}

void LandingCalculation::onTableResultsCellClicked(int row, int column)
{
    if (column == 1) {
        QTableWidgetItem *item = ui->tableResults->item(row, column);
        if (item) {
            previousCellValue = item->text();
            previousCellRow = row;
            previousCellColumn = column;

            disconnect(ui->tableResults, &QTableWidget::cellChanged,
                       this, &LandingCalculation::onTableResultsCellChanged);

            item->setText("");

            connect(ui->tableResults, &QTableWidget::cellChanged,
                    this, &LandingCalculation::onTableResultsCellChanged);

            item->setFlags(item->flags() | Qt::ItemIsEditable);

            ui->tableResults->editItem(item);
        }
    }
}

void LandingCalculation::onTableResultsCellChanged(int row, int column)
{
    if (row == previousCellRow && column == previousCellColumn) {
        QTableWidgetItem *item = ui->tableResults->item(row, column);

        if (item) {
            QString newValue = item->text().trimmed();

            if (newValue.isEmpty()) {
                disconnect(ui->tableResults, &QTableWidget::cellChanged,
                           this, &LandingCalculation::onTableResultsCellChanged);
                item->setText(previousCellValue);

                connect(ui->tableResults, &QTableWidget::cellChanged,
                        this, &LandingCalculation::onTableResultsCellChanged);
            }
        }

        previousCellRow = -1;
        previousCellColumn = -1;
        previousCellValue.clear();
    }
}

//void LandingCalculation::setupSizes()
//{
//    initialDataSize = QSize(1372, 931);
//    resultsSize = QSize(1387, 935);
//}

//void LandingCalculation::onTabChanged()
//{
//    if (ui->tabWidget->currentWidget() == ui->tabResults){
//        setFixedSize(resultsSize);
//        resize(resultsSize);
//    } else if (ui->tabWidget->currentWidget() == ui->tabInitialData){
//        setFixedSize(initialDataSize);
//        resize(initialDataSize);
//    }
//}
