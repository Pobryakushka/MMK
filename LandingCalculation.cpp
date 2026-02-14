#include "LandingCalculation.h"
#include "ui_LandingCalculation.h"
#include <QTextStream>
#include <QDebug>

static const double KRASOVSKY_A = 6378245.0;

LandingCalculation::LandingCalculation(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LandingCalculation),
    m_B_M(0.0),
    m_L_M(0.0),
    m_H_M(0.0),
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

    connect(ui->btnCalculate, &QPushButton::clicked,
            this, &LandingCalculation::onCalculateClicked);

    connect(ui->cbManualInput, &QCheckBox::toggled,
            this, &LandingCalculation::onManualWindInputToggled);

    ui->editDistance->setReadOnly(true);
    ui->editDirection->setReadOnly(true);

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

// =================================================
// Установка координат метеокомплекса из MainWindow
// =================================================

void LandingCalculation::setMeteoStationCoords(double lat_deg, double lon_deg, double alt_m)
{
    m_B_M = toRad(lat_deg);
    m_L_M = toRad(lon_deg);
    m_H_M = alt_m;
}

// =================================================
// Переключение ручного ввода параметров ветра
// =================================================

void LandingCalculation::onManualWindInputToggled(bool checked)
{
    ui->editDistance->setReadOnly(!checked);
    ui->editDistance->setReadOnly(!checked);

    if (!checked){
        // При снятии галочки сбрасваем поля в read-only стиль
        ui->editDistance->setStyleSheet("");
        ui->editDirection->setStyleSheet("");
    } else {
        // Показ, что поля редактируемы
        ui->editDistance->setStyleSheet("background-color: #FFFACD;");
        ui->editDistance->setStyleSheet("background-color: #FFFACD;");
    }
}

// =================================================
// Парсинг строки ГГ ММ СС
// =================================================

bool LandingCalculation::parseDMS(const QString &text, double &degrees)
{
    QString cleaned = text.trimmed();
    if (cleaned.isEmpty()) return false;

    cleaned.replace(QRegExp("[°'\"°']+"), " ");
    cleaned = cleaned.simplified();

    QStringList parts = cleaned.split(' ', Qt::SkipEmptyParts);

    if (parts.size() == 1) {
        // Пробуем как десятичное число
        bool ok = false;
        degrees = parts[0].replace(',', '.').toDouble(&ok);
        return ok;
    }
    if (parts.size() >= 2) {
        bool ok1, ok2, ok3 = true;
        double deg = parts[0].replace(',', '.').toDouble(&ok1);
        double min = parts[1].replace(',', '.').toDouble(&ok2);
        double sec = 0.0;
        if (parts.size() >= 3) {
            sec = parts[2].replace(',','.').toDouble(&ok3);
        }
        if (!ok1 || !ok2 || !ok3) return false;

        degrees = deg + min / 60.0 + sec / 3600.0;
        return true;
    }

    return false;
}

// =================================================
// Форматирование координат в строку
// =================================================

QString LandingCalculation::formatDMS(double dec_deg, bool isLat)
{
    bool negative = (dec_deg < 0.0);
    dec_deg = qAbs(dec_deg);

    int d = static_cast<int>(dec_deg);
    double rem = (dec_deg - d) * 60.0;
    int m = static_cast<int>(rem);
    double s = (rem - m) * 60.0;

    QString suffix;
    if (isLat)
        suffix = negative ? "S" : "N";
    else
        suffix = negative ? "W" : "E";

    return QString("%1°%2'%3\" %4")
            .arg(d, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 5, 'f', 2, QChar('0'))
            .arg(suffix);
}

// =================================================
// Валидация полей ввода
// =================================================

bool LandingCalculation::validateInputs()
{
    QStringList errors;

    // Проверка координат ТПП
    if (ui->editLatCUP->text().trimmed().isEmpty())
        errors << "Не заполнена широта ТПП";
    if (ui->editLonCUP->text().trimmed().isEmpty())
        errors << "Не заполнена долгота ТПП";
    if (ui->editHeightUM->text().trimmed().isEmpty())
        errors << "Не заполнена высота ТПП";

    // Проверка параметров парашютной системы
    if (ui->editTimeVP->text().trimmed().isEmpty())
        errors << "Не заполнено время падения (tпад)";
    if (ui->editLossPV->text().trimmed().isEmpty())
        errors << "Не заполнена потеря высоты (hнап)";
    if (ui->editSpeedDescent->text().trimmed().isEmpty())
        errors << "Не заполнена скорость снижения (Vсн)";
    if (ui->editStaffTime->text().trimmed().isEmpty())
        errors << "Не заполнен штилевой относ (A0)";

    // Проверка высота дисантирования
    if (ui->editHvhod->text().trimmed().isEmpty())
        errors << "Не заполнена высота десантирования (Hдес)";

    // Проверка параметров ветра
    if (ui->editDistance->text().trimmed().isEmpty())
        errors << "Не заполнена скорость ветра (Uср)";
    if (ui->editDirection->text().trimmed().isEmpty())
        errors << "Не заполнено направление ветра δ(ср)";

    // Проверка угла площадки
    if (ui->editTrueBearing->text().trimmed().isEmpty())
        errors << "Не заполнен истинный путевой угол площадки";

    if (!errors.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода",
                             "Обнаружены ошибки:\n\n" + errors.join("\n"));
        return false;
    }

    return true;
}

// =================================================
// Основной расчет по алгоритму
// =================================================

LandingCalculationResult LandingCalculation::calculate()
{
    LandingCalculationResult res;
    res.valid = false;

    // Считываем и парсим координаты ТПП
    double B_tpp_deg = 0.0, L_tpp_deg = 0.0;

    if (!parseDMS(ui->editLatCUP->text(), B_tpp_deg)) {
        QMessageBox::warning(this, "Ошибка", "Некорректный формат широты ТПП.\nВведите: ГГ ММ СС");
        return res;
    }
    if (!parseDMS(ui->editLonCUP->text(), L_tpp_deg)) {
        QMessageBox::warning(this, "Ошибка", "Некорректный формат долготы ТПП.\nВведите: ГГ ММ СС");
        return res;
    }

    // Применяем знак (N/S, E/W)
    if (ui->comboLatCUP->currentText() == "S") B_tpp_deg = -B_tpp_deg;
    if (ui->comboLonCUP->currentText() == "W") L_tpp_deg = -L_tpp_deg;

    // Высота ТПП (м)
    bool ok;
    double H_tpp = ui->editHeightUM->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Ошибка", "Некорректное значение высоты ТПП");
        return res;
    }

    // Параметры парашютной системы
    double t_pad = ui->editTimeVP->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение tпад"); return res; }

    double h_nap = ui->editLossPV->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение hнап"); return res; }

    double V_sn = ui->editSpeedDescent->text().replace(',', '.').toDouble(&ok);
    if (!ok || qFuzzyIsNull(V_sn)) {
        QMessageBox::warning(this, "Ошибка", "Некорректное значение Vсн (не может быть 0)");
        return res;
    }

    double A0 = ui->editStaffTime->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение A0"); return res; }

    // Высота десантирования
    double H_des = ui->editHvhod->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение Hдес"); return res; }

    // Истинный путевой угол
    double alpha_pl_deg = ui->editTrueBearing->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение угла площадки"); return res; }
    double alpha_pl = toRad(alpha_pl_deg);

    // Параметры ветра
    double U_sr = ui->editDistance->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение скорости ветра Uср"); return res; }

    double delta_sr_deg = ui->editDirection->text().replace(',', '.').toDouble(&ok);
    if (!ok) { QMessageBox::warning(this, "Ошибка", "Некорректное значение направления ветра δср"); return res; }
    double delta_sr = toRad(delta_sr_deg);

    // ========= 1. Время снижения парашютиста =========================
    double T_sn = (H_des - h_nap) / V_sn + t_pad;

    // ========= 2. Ветровой снос ======================================
    double Z_sn = T_sn * U_sr;

    // ========= 3. Приращение координат (от ТПП к ТНВ) ================
    double delta_x = Z_sn * qCos(delta_sr) + A0 * qCos(alpha_pl + M_PI);
    double delta_y = Z_sn * qSin(delta_sr) + A0 * qSin(alpha_pl + M_PI);

    // ========= 4. Координаты ТНВ =====================================
    double B_tpp = toRad(B_tpp_deg);
    double L_tpp = toRad(L_tpp_deg);

    double aH = KRASOVSKY_A + H_des;  // a + H_дес

    // Аргумент arccos для B
    double argB = 1.0 - (delta_x * delta_x) / (2.0 * aH * aH);
    argB = qBound(-1.0, argB, 1.0);
    double dB = qAcos(argB);

    double B_tnv;
    if (delta_x >= 0.0)
        B_tnv = B_tpp + dB;
    else
        B_tnv = B_tpp - dB;

    // Аргумент arccos для L
    double cos2B = qCos(B_tpp) * qCos(B_tpp);
    double argL;
    if (qFuzzyIsNull(cos2B)) {
        argL = 1.0;
    } else {
        argL = 1.0 - (delta_y * delta_y) / (2.0 * aH * aH * cos2B);
    }
    argL = qBound(-1.0, argL, 1.0);
    double dL = qAcos(argL);

    double L_tnv;
    if (delta_y >= 0.0)
        L_tnv = L_tpp + dL;
    else
        L_tnv = L_tpp - dL;

    double H_tnv = H_tpp + H_des;

    // Переводим в десятичные градусы
    double B_tnv_deg = toDeg(B_tnv);
    double L_tnv_deg = toDeg(L_tnv);

    // ========= 5. Расстояние и дирекционный угол от М до ТПП =========
    auto calcDistAngle = [&](double B_T_rad, double L_T_rad,
                              double &D_T, double &A_T_deg)
    {
        // Приращения от М до точки Т
        double dxT, dyT;

        if (m_B_M >= B_T_rad)
            dxT = -qSqrt(2.0 * KRASOVSKY_A * KRASOVSKY_A * (1.0 - qCos(m_B_M - B_T_rad)));
        else
            dxT =  qSqrt(2.0 * KRASOVSKY_A * KRASOVSKY_A * (1.0 - qCos(m_B_M - B_T_rad)));

        double cos2BT = qCos(B_T_rad) * qCos(B_T_rad);
        if (m_L_M >= L_T_rad)
            dyT = -qSqrt(2.0 * KRASOVSKY_A * KRASOVSKY_A * cos2BT * (1.0 - qCos(m_L_M - L_T_rad)));
        else
            dyT =  qSqrt(2.0 * KRASOVSKY_A * KRASOVSKY_A * cos2BT * (1.0 - qCos(m_L_M - L_T_rad)));

        // Расстояние
        D_T = qSqrt(dxT * dxT + dyT * dyT);

        // Дирекционный угол
        double A_T_rad;
        if (D_T < 1e-9) {
            A_T_rad = 0.0;
        } else if (dyT >= 0.0) {
            A_T_rad = qAsin(dxT / D_T);
        } else {
            A_T_rad = M_PI - qAsin(dxT / D_T);
        }

        // Приводим к [0, 2π)
        if (A_T_rad < 0.0) A_T_rad += 2.0 * M_PI;
        A_T_deg = toDeg(A_T_rad);
    };

    double D_tpp, A_tpp_deg;
    calcDistAngle(B_tpp, L_tpp, D_tpp, A_tpp_deg);

    double D_tnv, A_tnv_deg;
    calcDistAngle(B_tnv, L_tnv, D_tnv, A_tnv_deg);

    qDebug() << "B_tpp_deg:" << B_tpp_deg << "L_tpp_deg:" << L_tpp_deg;
    qDebug() << "T_sn:" << T_sn << "Z_sn:" << Z_sn;
    qDebug() << "dx:" << delta_x << "dy:" << delta_y;

    // =================================================
    // Сохранение результата
    // =================================================

    res.B_tnv_deg  = B_tnv_deg;
    res.L_tnv_deg  = L_tnv_deg;
    res.H_tnv      = H_tnv;
    res.T_sn       = T_sn;
    res.Z_sn       = Z_sn;
    res.delta_x    = delta_x;
    res.delta_y    = delta_y;
    res.D_tpp      = D_tpp;
    res.A_tpp_deg  = A_tpp_deg;
    res.D_tnv      = D_tnv;
    res.A_tnv_deg  = A_tnv_deg;
    res.valid      = true;

    return res;
}

// =================================================
// Вывод результатов в UI
// =================================================
void LandingCalculation::displayResults(const LandingCalculationResult &res)
{
    if (!res.valid) return;

    // Координаты ТНВ
    ui->editLatTNV->setText(formatDMS(res.B_tnv_deg, true));
    ui->editLonTNV->setText(formatDMS(res.L_tnv_deg, false));
    ui->editHeightTNV->setText(QString("%1 м").arg(res.H_tnv, 0, 'f', 0));

    // Устанавливаем комбобоксы (N/S, E/W)
    ui->comboBox_2->setCurrentIndex(res.B_tnv_deg >= 0.0 ? 0 : 1); // Северная/Южная
    ui->comboBox_3->setCurrentIndex(res.L_tnv_deg >= 0.0 ? 0 : 1); // Восточная/Западная

    // Положение метеопоста относительно ТНВ
    ui->editDistanceTNV->setText(QString("%1 м").arg(res.D_tnv, 0, 'f', 0));
    ui->editAngleTNV->setText(QString("%1°").arg(res.A_tnv_deg, 0, 'f', 1));

    // Положение метеопоста относительно ТПП
    ui->editDistanceTPP->setText(QString("%1 м").arg(res.D_tpp, 0, 'f', 0));
    ui->editAngleTPP->setText(QString("%1°").arg(res.A_tpp_deg, 0, 'f', 1));

    // Переключаемся на вкладку результатов
    ui->tabWidget->setCurrentWidget(ui->tabResults);
}

void LandingCalculation::onCalculateClicked()
{
    if (!validateInputs()) return;

    LandingCalculationResult res = calculate();

    if (res.valid) {
        displayResults(res);
    }
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
