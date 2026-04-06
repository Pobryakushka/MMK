#include "Meteo11.h"
#include "ui_Meteo11.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDebug>

// Коды высот строк таблицы (19 стандартных уровней Метео-11)
const QStringList Meteo11::kHeightCodes = {
    "02", "04", "08", "12", "16", "20", "24", "30",   // до 3000 м
    "40", "50", "60", "80",                            // 4000–8000 м
    "10", "12", "14", "18", "22", "26", "30"           // 10–30 км
};

Meteo11::Meteo11(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::Meteo11)
    , m_applied(false)
{
    ui->setupUi(this);
    setWindowTitle("Метео-11 — ввод бюллетеня от метеостанции");
    adjustSize();
    setMinimumSize(sizeHint());
    setSizeGripEnabled(true);

    // Настраиваем таблицу: 3 колонки — ПП, НН, СС
    ui->tableWidget_meteo11->setColumnCount(3);
    ui->tableWidget_meteo11->setColumnWidth(0, 60);   // ПП — код высоты
    ui->tableWidget_meteo11->setColumnWidth(1, 100);  // НН — направление
    ui->tableWidget_meteo11->setColumnWidth(2, 100);  // СС — скорость
    ui->tableWidget_meteo11->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_meteo11->setEditTriggers(QAbstractItemView::AllEditTriggers);

    // Заполняем колонку ПП кодами высот (только для отображения, не редактируется)
    for (int r = 0; r < kHeightCodes.size() && r < ui->tableWidget_meteo11->rowCount(); ++r) {
        auto *item = new QTableWidgetItem(kHeightCodes[r]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setBackground(QColor("#F5F5F5"));
        item->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget_meteo11->setItem(r, 0, item);
    }

    connect(ui->btnMet11Apply,  &QPushButton::clicked, this, &Meteo11::onApplyClicked);
    connect(ui->btnMet11Parse,  &QPushButton::clicked, this, &Meteo11::onParseClicked);
    connect(ui->btnMet11Clear,  &QPushButton::clicked, this, &Meteo11::onClearClicked);
    connect(ui->btnMet11Close,  &QPushButton::clicked, this, [this]{ close(); });
}

Meteo11::~Meteo11()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Применить»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onApplyClicked()
{
    const QString datetimeStr = ui->lineEdit_Met11DateTime->text().trimmed();

    if (datetimeStr.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода",
                             "Укажите дату и время бюллетеня (поле «Дата и время»).");
        ui->lineEdit_Met11DateTime->setFocus();
        return;
    }

    // Пробуем несколько форматов даты/времени
    QDateTime dt;
    for (const QString &fmt : {
                               "dd.MM.yyyy HH:mm",
                               "dd.MM.yy HH:mm",
                               "ddHHmm",
                               "d HH mm"}) {
        dt = QDateTime::fromString(datetimeStr, fmt);
        if (dt.isValid()) break;
    }
    if (!dt.isValid()) {
        // Поле заполнено как ДДЧЧМ — берём текущий год/месяц
        if (datetimeStr.length() == 5 && datetimeStr.toInt() > 0) {
            int day  = datetimeStr.left(2).toInt();
            int hour = datetimeStr.mid(2, 2).toInt();
            int tenM = datetimeStr.right(1).toInt();
            QDate today = QDate::currentDate();
            dt = QDateTime(QDate(today.year(), today.month(), day),
                           QTime(hour, tenM * 10, 0));
        }
    }
    if (!dt.isValid()) {
        dt = QDateTime::currentDateTime();
        qDebug() << "Meteo11: не удалось разобрать дату, берём текущую";
    }
    m_bulletinTime = dt;

    // Считываем поля заголовка
    QJsonObject json;
    json["station_num"]          = ui->lineEdit_Met11StationNum->text().trimmed();
    json["station_height"]       = ui->lineEdit_Met11StationHeight->text().trimmed();
    json["datetime"]             = datetimeStr;
    json["ground_pres_dev"]      = ui->lineEdit_Met11GroundPresDev->text().trimmed();
    json["ground_virt_temp_dev"] = ui->lineEdit_Met11GroundVertTempDev->text().trimmed();
    json["achieved_wind_height"] = ui->lineEdit_Met11AchievedSensHeight->text().trimmed();
    json["raw_string"]           = ui->plainEdit_rawBulletin->toPlainText().trimmed();

    // Считываем слои из таблицы (колонки: 0=ПП, 1=НН, 2=СС)
    QJsonArray layers;
    for (int r = 0; r < ui->tableWidget_meteo11->rowCount(); ++r) {
        const QString hCode = kHeightCodes.value(r);
        QTableWidgetItem *itNN = ui->tableWidget_meteo11->item(r, 1);
        QTableWidgetItem *itSS = ui->tableWidget_meteo11->item(r, 2);
        const QString nn = itNN ? itNN->text().trimmed() : QString();
        const QString ss = itSS ? itSS->text().trimmed() : QString();
        if (!nn.isEmpty() || !ss.isEmpty()) {
            QJsonObject layer;
            layer["height_code"] = hCode;
            layer["nn"]          = nn;
            layer["ss"]          = ss;
            layers.append(layer);
        }
    }
    json["layers"] = layers;

    m_bulletinJson   = json;
    m_validityPeriod = "12h";
    m_applied        = true;

    // Показываем статус в метке
    ui->lblStatus->setText(QString("✓ Применён: %1, слоёв: %2")
                               .arg(dt.toString("dd.MM.yyyy HH:mm"))
                               .arg(layers.size()));

    qDebug() << "Meteo11: бюллетень применён, время:" << dt
             << "слоёв:" << layers.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Разобрать строку»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onParseClicked()
{
    const QString raw = ui->plainEdit_rawBulletin->toPlainText().trimmed();
    if (raw.isEmpty()) {
        QMessageBox::information(this, "Разбор строки",
                                 "Вставьте строку бюллетеня в поле выше.");
        return;
    }

    // Нормализуем разделители: тире, дефисы, пробелы → одиночный пробел
    QString normalized = raw;
    normalized.replace(QRegularExpression("[—–\\-]+"), " ");
    normalized.replace(QRegularExpression("\\s+"), " ").trimmed();

    QStringList parts = normalized.split(' ', Qt::SkipEmptyParts);

    // Ожидаемая структура Метео-11:
    // Метео 11NNNNN ДДЧЧМ BBBB БББТ0Т0 [слои...] BтBтBвBв
    // Пример: Метео 1101 15011 0100 51258 0256 ...
    // parts[0]="Метео" parts[1]="11NNNNN" или parts[1]="1101" и т.д.

    int idx = 0;
    // Пропускаем слово "Метео" если есть
    if (idx < parts.size() && parts[idx].toLower() == "метео")
        ++idx;

    // 11NNNNN — тип "11" + номер станции
    if (idx < parts.size() && parts[idx].startsWith("11")) {
        QString stationPart = parts[idx].mid(2); // убираем "11"
        ui->lineEdit_Met11StationNum->setText(stationPart);
        ++idx;
    }

    // ДДЧЧМ — дата/время
    if (idx < parts.size()) {
        ui->lineEdit_Met11DateTime->setText(parts[idx]);
        ++idx;
    }

    // BBBB — высота станции
    if (idx < parts.size()) {
        ui->lineEdit_Met11StationHeight->setText(parts[idx]);
        ++idx;
    }

    // БББТ0Т0 — давление+температура (5 цифр: 3 давление + 2 температура)
    if (idx < parts.size() && parts[idx].length() >= 5) {
        const QString pt = parts[idx];
        ui->lineEdit_Met11GroundPresDev->setText(pt.left(3));
        ui->lineEdit_Met11GroundVertTempDev->setText(pt.mid(3, 2));
        ++idx;
    }

    // Слои ветра: каждая группа = ППНН-СС (4 символа код высоты + 2 направление - 2 скорость)
    // или ППТТННСС — до и после 10 км разный формат, парсим упрощённо
    // Группы слоёв: XXXX-TTNNSS
    int layerRow = 0;
    while (idx < parts.size() && layerRow < ui->tableWidget_meteo11->rowCount()) {
        const QString grp = parts[idx];

        // Пропускаем итоговые группы BтBт и BвBв (2-значные в конце)
        if (idx == parts.size() - 1 && grp.length() == 4 && grp.toInt() > 0) {
            ui->lineEdit_Met11AchievedSensHeight->setText(
                QString::number(grp.right(2).toInt()));
            ++idx;
            break;
        }

        // Группа слоя: первые 2-4 символа — код высоты+доп, последние 6 — ТТННСС
        if (grp.length() >= 6) {
            const QString ttnnss = grp.right(6);
            // НН — символы 2-3 (индексы 2,3)
            const QString nn = ttnnss.mid(2, 2);
            // СС — символы 4-5 (индексы 4,5)
            const QString ss = ttnnss.mid(4, 2);

            if (layerRow < ui->tableWidget_meteo11->rowCount()) {
                ui->tableWidget_meteo11->setItem(layerRow, 1,
                                                 new QTableWidgetItem(nn));
                ui->tableWidget_meteo11->setItem(layerRow, 2,
                                                 new QTableWidgetItem(ss));
                ++layerRow;
            }
        }
        ++idx;
    }

    ui->lblStatus->setText("Строка разобрана — проверьте поля и нажмите «Применить»");
    ui->lblStatus->setStyleSheet("color: #E65100; font-weight: bold;");
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Очистить»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onClearClicked()
{
    ui->plainEdit_rawBulletin->clear();
    ui->lineEdit_Met11StationNum->clear();
    ui->lineEdit_Met11DateTime->clear();
    ui->lineEdit_Met11StationHeight->clear();
    ui->lineEdit_Met11GroundPresDev->clear();
    ui->lineEdit_Met11GroundVertTempDev->clear();
    ui->lineEdit_Met11AchievedSensHeight->clear();
    ui->lblStatus->clear();
    ui->lblStatus->setStyleSheet("color: #2E7D32; font-weight: bold;");

    // Очищаем только редактируемые колонки (НН и СС), оставляем ПП
    for (int r = 0; r < ui->tableWidget_meteo11->rowCount(); ++r) {
        ui->tableWidget_meteo11->setItem(r, 1, new QTableWidgetItem(""));
        ui->tableWidget_meteo11->setItem(r, 2, new QTableWidgetItem(""));
    }

    m_applied      = false;
    m_bulletinJson = QJsonObject();
    m_bulletinTime = QDateTime();
}