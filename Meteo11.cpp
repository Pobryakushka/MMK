#include "Meteo11.h"
#include "ui_Meteo11.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDebug>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QStyle>

// Коды высот строк таблицы (19 стандартных уровней Метео-11)
const QStringList Meteo11::kHeightCodes = {
    "02", "04", "08", "12", "16", "20", "24", "30",   // до 3000 м
    "40", "50", "60", "80",                            // 4000–8000 м
    "10", "12", "14", "18", "22", "26", "30"           // 10–30 км
};

Meteo11::Meteo11(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Meteo11)
    , m_applied(false)
{
    ui->setupUi(this);

    // Настраиваем таблицу: 3 колонки — ПП, НН, СС
    ui->tableWidget_meteo11->setColumnCount(3);
    ui->tableWidget_meteo11->setColumnWidth(0, 60);   // ПП — код высоты
    ui->tableWidget_meteo11->setColumnWidth(1, 100);  // НН — направление
    ui->tableWidget_meteo11->setColumnWidth(2, 100);  // СС — скорость
    ui->tableWidget_meteo11->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_meteo11->setEditTriggers(QAbstractItemView::AllEditTriggers);

    // Колонка ПП — поправка за плотность воздуха.
    // По умолчанию "//" (не измерялась); оператор может вписать значение вручную.
    for (int r = 0; r < ui->tableWidget_meteo11->rowCount(); ++r) {
        auto *item = new QTableWidgetItem("//");
        item->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget_meteo11->setItem(r, 0, item);
    }

    connect(ui->btnMet11Apply,  &QPushButton::clicked, this, &Meteo11::onApplyClicked);
    connect(ui->btnMet11Parse,  &QPushButton::clicked, this, &Meteo11::onParseClicked);
    connect(ui->btnMet11Clear,  &QPushButton::clicked, this, &Meteo11::onClearClicked);
    connect(ui->btnMet11Back,   &QPushButton::clicked, this, [this]{ emit backRequested(); });

    // ── Экранная клавиатура: подсказки типа ввода для каждого поля ──────────
    // Поля, где вводятся только цифры, — цифровая клавиатура.
    ui->lineEdit_Met11StationNum->setInputMethodHints(Qt::ImhDigitsOnly);
    ui->lineEdit_Met11DateTime->setInputMethodHints(Qt::ImhDigitsOnly);
    ui->lineEdit_Met11StationHeight->setInputMethodHints(Qt::ImhDigitsOnly);
    ui->lineEdit_Met11AchievedSensHeight->setInputMethodHints(Qt::ImhDigitsOnly);
    // Поля со знаком (±) — цифровая клавиатура с поддержкой знака/точки.
    ui->lineEdit_Met11GroundPresDev->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
    ui->lineEdit_Met11GroundVertTempDev->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
    // Строка бюллетеня содержит буквы ("Метео") — обычная клавиатура, без ограничений.
    ui->plainEdit_rawBulletin->setInputMethodHints(Qt::ImhNone);

    setupValidation();
    updateStatusPill();
}

Meteo11::~Meteo11()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
// Бейдж-пилюля состояния бюллетеня (аналог lblStatusPill в GroundMeteoParams)
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::updateStatusPill()
{
    if (m_applied) {
        ui->lblMet11StatusPill->setText(QString::fromUtf8("● БЮЛЛЕТЕНЬ ЗАГРУЖЕН"));
        ui->lblMet11StatusPill->setStyleSheet(
            "padding: 6px 18px; border-radius: 17px; font-weight: bold; "
            "background-color: #E8F5E9; color: #0F6B4F;");
    } else {
        ui->lblMet11StatusPill->setText(QString::fromUtf8("● БЮЛЛЕТЕНЬ НЕ ЗАГРУЖЕН"));
        ui->lblMet11StatusPill->setStyleSheet(
            "padding: 6px 18px; border-radius: 17px; font-weight: bold; "
            "background-color: #FFEBEE; color: #C62828;");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Применить»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onApplyClicked()
{
    // Подсвечиваем красной рамкой все незаполненные обязательные поля разом
    // и уводим фокус на первое из них — вместо единичного QMessageBox.
    if (!validateRequiredFields(/*focusFirst=*/true)) {
        ui->lblStatus->setText("Заполните обязательные поля, отмеченные красным");
        ui->lblStatus->setStyleSheet("color: #C62828; font-weight: bold;");
        return;
    }

    const QString datetimeStr = ui->lineEdit_Met11DateTime->text().trimmed();

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
    // BтBтBвBв: 4-символьный код → разбиваем на темп. и ветровую высоты
    // Если введено 4-значное число (как сохраняет «Разобрать») — берём по 2 символа.
    // Иначе трактуем как ветровую высоту (старый формат ввода).
    {
        const QString ach = ui->lineEdit_Met11AchievedSensHeight->text().trimmed();
        bool ok = false;
        ach.toInt(&ok);
        if (ok && ach.length() == 4) {
            json["achieved_temp_height"] = ach.left(2);
            json["achieved_wind_height"] = ach.right(2);
        } else {
            json["achieved_temp_height"] = "0";
            json["achieved_wind_height"] = ach;
        }
    }
    json["raw_string"]           = ui->plainEdit_rawBulletin->toPlainText().trimmed();

    // Считываем слои из таблицы (колонки: 0=ПП, 1=НН, 2=СС)
    // Код высоты берётся по позиции строки из kHeightCodes
    QJsonArray layers;
    for (int r = 0; r < ui->tableWidget_meteo11->rowCount(); ++r) {
        const QString hCode = kHeightCodes.value(r);
        QTableWidgetItem *itPP = ui->tableWidget_meteo11->item(r, 0);
        QTableWidgetItem *itNN = ui->tableWidget_meteo11->item(r, 1);
        QTableWidgetItem *itSS = ui->tableWidget_meteo11->item(r, 2);
        const QString pp = itPP ? itPP->text().trimmed() : "//";
        const QString nn = itNN ? itNN->text().trimmed() : QString();
        const QString ss = itSS ? itSS->text().trimmed() : QString();
        if (!nn.isEmpty() || !ss.isEmpty()) {
            QJsonObject layer;
            layer["height_code"] = hCode;
            layer["pp"]          = pp;
            layer["nn"]          = nn;
            layer["ss"]          = ss;
            layers.append(layer);
        }
    }
    json["layers"] = layers;

    m_bulletinJson   = json;
    m_validityPeriod = "12h";
    m_applied        = true;

    // Показываем статус в метке и обновляем пилюлю в шапке
    ui->lblStatus->setText(QString("✓ Применён: %1, слоёв: %2")
                               .arg(dt.toString("dd.MM.yyyy HH:mm"))
                               .arg(layers.size()));
    ui->lblStatus->setStyleSheet("color: #0F6B4F; font-weight: bold;");
    updateStatusPill();

    qDebug() << "Meteo11: бюллетень применён, время:" << dt
             << "слоёв:" << layers.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Заполнить поля из строки»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onParseClicked()
{
    const QString raw = ui->plainEdit_rawBulletin->toPlainText().trimmed();
    if (raw.isEmpty()) {
        setRawBulletinInvalid(true);
        ui->plainEdit_rawBulletin->setFocus();
        return;
    }
    setRawBulletinInvalid(false);

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

    // Разбираем слои по парам токенов:
    //   ниже 10 км: ВВПП (4 симв.) + ТТННСС (6 симв.)
    //   выше 10 км: ВВ   (2 симв.) + ТТННСС (6 симв.)
    // Последний 4-символьный токен — BтBтBвBв (достигнутые высоты), не слой.
    int layerRow = 0;
    QString pendingPP;          // ПП из предыдущего ВВПП-токена
    bool    hasPendingPP = false;

    while (idx < parts.size() && layerRow < ui->tableWidget_meteo11->rowCount()) {
        const QString grp = parts[idx];

        if (grp.length() == 4) {
            // ВВПП: первые 2 — высота, последние 2 — поправка за плотность
            pendingPP    = grp.right(2);
            hasPendingPP = true;
        } else if (grp.length() == 2) {
            // ВВ (высота в км, >10 км): ПП отсутствует
            pendingPP    = "//";
            hasPendingPP = true;
        } else if (grp.length() == 6) {
            // ТТННСС: заполняем строку таблицы
            const QString nn = grp.mid(2, 2);
            const QString ss = grp.mid(4, 2);
            const QString pp = hasPendingPP ? pendingPP : "//";

            auto *ppItem = new QTableWidgetItem(pp);
            ppItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget_meteo11->setItem(layerRow, 0, ppItem);
            ui->tableWidget_meteo11->setItem(layerRow, 1, new QTableWidgetItem(nn));
            ui->tableWidget_meteo11->setItem(layerRow, 2, new QTableWidgetItem(ss));
            ++layerRow;
            hasPendingPP = false;
        }
        ++idx;
    }

    // BтBтBвBв — достигнутые высоты зондирования (следующий токен после слоёв)
    // Сохраняем полный 4-символьный код: левые 2 — темп., правые 2 — ветровое
    if (idx < parts.size() && parts[idx].length() == 4) {
        ui->lineEdit_Met11AchievedSensHeight->setText(parts[idx]);
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

    // Сбрасываем все три колонки: ПП → "//" (default), НН и СС → ""
    for (int r = 0; r < ui->tableWidget_meteo11->rowCount(); ++r) {
        auto *pp = new QTableWidgetItem("//");
        pp->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget_meteo11->setItem(r, 0, pp);
        ui->tableWidget_meteo11->setItem(r, 1, new QTableWidgetItem(""));
        ui->tableWidget_meteo11->setItem(r, 2, new QTableWidgetItem(""));
    }

    m_applied      = false;
    m_bulletinJson = QJsonObject();
    m_bulletinTime = QDateTime();
    updateStatusPill();

    // Сбрасываем всю подсветку "поле не заполнено"
    for (const auto &f : requiredFields())
        setFieldInvalid(f.edit, f.hint, false);
    setRawBulletinInvalid(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Валидация полей ввода — подсветка незаполненных (вариант «рамка + подпись»)
// ─────────────────────────────────────────────────────────────────────────────
QList<Meteo11::RequiredField> Meteo11::requiredFields() const
{
    return {
        { ui->lineEdit_Met11StationNum,          ui->lblStationNumHint,   "Номер МС" },
        { ui->lineEdit_Met11DateTime,             ui->lblDateTimeHint,     "Дата и время" },
        { ui->lineEdit_Met11StationHeight,        ui->lblStationHeightHint,"Высота МС" },
        { ui->lineEdit_Met11GroundPresDev,        ui->lblPresDevHint,      "Откл. давления" },
        { ui->lineEdit_Met11GroundVertTempDev,    ui->lblTempDevHint,      "Откл. вирт. темп." },
    };
}

void Meteo11::setupValidation()
{
    // Как только оператор начинает печатать — красная рамка/подпись сразу снимается
    // с этого конкретного поля (не дожидаясь повторного нажатия «Применить»).
    for (const auto &f : requiredFields()) {
        connect(f.edit, &QLineEdit::textChanged, this, [this, f](const QString &text) {
            if (!text.trimmed().isEmpty())
                setFieldInvalid(f.edit, f.hint, false);
        });
    }
    connect(ui->plainEdit_rawBulletin, &QPlainTextEdit::textChanged, this, [this]() {
        if (!ui->plainEdit_rawBulletin->toPlainText().trimmed().isEmpty())
            setRawBulletinInvalid(false);
    });
}

bool Meteo11::validateRequiredFields(bool focusFirst)
{
    bool allValid = true;
    QLineEdit *firstInvalid = nullptr;

    for (const auto &f : requiredFields()) {
        const bool empty = f.edit->text().trimmed().isEmpty();
        setFieldInvalid(f.edit, f.hint, empty, f.label);
        if (empty) {
            allValid = false;
            if (!firstInvalid) firstInvalid = f.edit;
        }
    }

    if (!allValid && focusFirst && firstInvalid)
        firstInvalid->setFocus();

    return allValid;
}

void Meteo11::setFieldInvalid(QLineEdit *edit, QLabel *hint, bool invalid, const QString &fieldLabel)
{
    if (!edit) return;

    const bool wasInvalid = edit->property("invalid").toBool();
    edit->setProperty("invalid", invalid);
    edit->style()->unpolish(edit);
    edit->style()->polish(edit);
    edit->update();

    if (hint)
        hint->setText(invalid ? "Поле не заполнено" : QString());

    // Анимируем "встряску" только в момент появления ошибки, не на каждый вызов
    if (invalid && !wasInvalid)
        shakeWidget(edit);

    Q_UNUSED(fieldLabel);
}

void Meteo11::setRawBulletinInvalid(bool invalid)
{
    const bool wasInvalid = ui->plainEdit_rawBulletin->property("invalid").toBool();
    ui->plainEdit_rawBulletin->setProperty("invalid", invalid);
    ui->plainEdit_rawBulletin->style()->unpolish(ui->plainEdit_rawBulletin);
    ui->plainEdit_rawBulletin->style()->polish(ui->plainEdit_rawBulletin);
    ui->plainEdit_rawBulletin->update();

    ui->lblRawBulletinError->setVisible(invalid);

    if (invalid && !wasInvalid)
        shakeWidget(ui->plainEdit_rawBulletin);
}

void Meteo11::shakeWidget(QWidget *w)
{
    if (!w) return;
    const QPoint basePos = w->pos();

    auto *anim = new QPropertyAnimation(w, "pos", w);
    anim->setDuration(280);
    anim->setKeyValueAt(0.0,  basePos);
    anim->setKeyValueAt(0.2,  basePos + QPoint(-6, 0));
    anim->setKeyValueAt(0.4,  basePos + QPoint(6, 0));
    anim->setKeyValueAt(0.6,  basePos + QPoint(-4, 0));
    anim->setKeyValueAt(0.8,  basePos + QPoint(2, 0));
    anim->setKeyValueAt(1.0,  basePos);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
