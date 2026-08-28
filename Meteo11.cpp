#include "Meteo11.h"
#include "ui_Meteo11.h"
#include "VirtualKeyboard.h"
#include "ui/ScreenTheme.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDebug>
#include <QLineEdit>
#include <QLabel>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QGuiApplication>
#include <QClipboard>

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
    setAttribute(Qt::WA_StyledBackground, true);

    // Вид экрана в стиле «Архива измерений». Роли кнопок помечаем свойствами
    // ДО применения темы, чтобы селекторы [primary]/[nav] сразу сработали.
    ui->btnMet11Apply->setProperty("primary", true);
    setupArchiveBackButton(ui->btnMet11Back);
    applyArchiveScreenTheme(this);

    // Настраиваем таблицу: 3 колонки — ПП, НН, СС
    ui->tableWidget_meteo11->setColumnCount(3);
    ui->tableWidget_meteo11->setColumnWidth(0, 60);   // ПП — код высоты
    ui->tableWidget_meteo11->setColumnWidth(1, 100);  // НН — направление
    ui->tableWidget_meteo11->setColumnWidth(2, 100);  // СС — скорость
    ui->tableWidget_meteo11->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_meteo11->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->tableWidget_meteo11->setItemDelegate(new Meteo11LayerCellDelegate(ui->tableWidget_meteo11));

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

    // На планшете нет физической клавиатуры с Ctrl+V — вставка из буфера
    // нужна отдельной кнопкой рядом со строкой бюллетеня.
    connect(ui->btnMet11Paste, &QPushButton::clicked, this, [this] {
        ui->lineEdit_rawBulletin->setText(QGuiApplication::clipboard()->text().trimmed());
        setFieldInvalid(ui->lineEdit_rawBulletin, ui->lblRawBulletinHint, false);
    });

    // ── Экранная клавиатура: для каждого поля — своя раскладка/ограничения ──
    setupVirtualKeyboard();

    setupValidation();
    updateStatusPill();
}

void Meteo11::setupVirtualKeyboard()
{
    using VK = VirtualKeyboard;

    // Номер МС — только цифры, 5 знаков
    VK::Constraints stationNum;
    stationNum.allowNegative   = false;
    stationNum.allowDecimal    = false;
    stationNum.maxLength       = 5;
    stationNum.allowModeSwitch = false;
    VK::attach(ui->lineEdit_Met11StationNum, VK::Mode::Numeric, stationNum);

    // Дата/время ДДЧЧМ — только цифры, 5 знаков
    VK::Constraints dateTime;
    dateTime.allowNegative   = false;
    dateTime.allowDecimal    = false;
    dateTime.maxLength       = 5;
    dateTime.allowModeSwitch = false;
    VK::attach(ui->lineEdit_Met11DateTime, VK::Mode::Numeric, dateTime);

    // Высота МС — только цифры
    VK::Constraints stationHeight;
    stationHeight.allowNegative   = false;
    stationHeight.allowDecimal    = false;
    stationHeight.maxLength       = 4;
    stationHeight.allowModeSwitch = false;
    VK::attach(ui->lineEdit_Met11StationHeight, VK::Mode::Numeric, stationHeight);

    // Отклонение давления (ДДД) — код со знаком, без дробной части
    VK::Constraints presDev;
    presDev.allowNegative   = true;
    presDev.allowDecimal    = false;
    presDev.maxLength       = 4;
    presDev.allowModeSwitch = false;
    VK::attach(ui->lineEdit_Met11GroundPresDev, VK::Mode::Numeric, presDev);

    // Отклонение вирт. температуры (T0T0) — код со знаком, без дробной части
    VK::Constraints tempDev;
    tempDev.allowNegative   = true;
    tempDev.allowDecimal    = false;
    tempDev.maxLength       = 3;
    tempDev.allowModeSwitch = false;
    VK::attach(ui->lineEdit_Met11GroundVertTempDev, VK::Mode::Numeric, tempDev);

    // Достигнутая высота зондирования (BтBтBвBв) — только цифры, 4 знака
    VK::Constraints achieved;
    achieved.allowNegative   = false;
    achieved.allowDecimal    = false;
    achieved.maxLength       = 4;
    achieved.allowModeSwitch = false;
    VK::attach(ui->lineEdit_Met11AchievedSensHeight, VK::Mode::Numeric, achieved);

    // Строка бюллетеня — содержит буквы ("Метео"), поэтому полная
    // буквенная раскладка. Переключение на цифровую раскладку не нужно —
    // цифры и так есть на верхнем ряду буквенной клавиатуры.
    VK::Constraints rawBulletin;
    rawBulletin.allowModeSwitch = false;
    VK::attach(ui->lineEdit_rawBulletin, VK::Mode::Text, rawBulletin);
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
    // Цвета пилюли — из палитры «Архива измерений» (green-soft / red).
    if (m_applied) {
        ui->lblMet11StatusPill->setText(QString::fromUtf8("● БЮЛЛЕТЕНЬ ЗАГРУЖЕН"));
        ui->lblMet11StatusPill->setStyleSheet(
            "padding: 4px 14px; border-radius: 13px; font-weight: bold; font-size: 9pt; "
            "background-color: #E4F1EC; color: #0B5A41;");
    } else {
        ui->lblMet11StatusPill->setText(QString::fromUtf8("● БЮЛЛЕТЕНЬ НЕ ЗАГРУЖЕН"));
        ui->lblMet11StatusPill->setStyleSheet(
            "padding: 4px 14px; border-radius: 13px; font-weight: bold; font-size: 9pt; "
            "background-color: #FBE4E4; color: #B3261E;");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Автозаполнение "//" в строках после последней реально введённой —
// зонд не долетел до этих высот, как и в архивных бюллетенях.
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::fillMissingTrailingLayers()
{
    auto *table = ui->tableWidget_meteo11;

    int lastFilled = -1;
    for (int r = 0; r < table->rowCount(); ++r) {
        QTableWidgetItem *itNN = table->item(r, 1);
        QTableWidgetItem *itSS = table->item(r, 2);
        const bool hasNN = itNN && !itNN->text().trimmed().isEmpty();
        const bool hasSS = itSS && !itSS->text().trimmed().isEmpty();
        if (hasNN || hasSS)
            lastFilled = r;
    }

    for (int r = lastFilled + 1; r < table->rowCount(); ++r) {
        auto *pp = new QTableWidgetItem("//");
        pp->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 0, pp);
        table->setItem(r, 1, new QTableWidgetItem("//"));
        table->setItem(r, 2, new QTableWidgetItem("//"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Применить»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onApplyClicked()
{
    // Подсвечиваем красной рамкой все незаполненные обязательные поля разом
    // и уводим фокус на первое из них — вместо единичного QMessageBox.
    // Подсвечиваем красной рамкой все незаполненные обязательные поля разом —
    // БЕЗ передачи фокуса (focusFirst=false), иначе setFocus() на QLineEdit
    // сам откроет экранную клавиатуру поверх страницы, хотя нужна только
    // видимая индикация "поле не заполнено".
    if (!validateRequiredFields(/*focusFirst=*/false)) {
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
    json["raw_string"]           = ui->lineEdit_rawBulletin->text().trimmed();

    // Строки после последней реально введённой заполняем "//" — зонд
    // не достиг этих высот (та же логика, что и в архивных бюллетенях).
    fillMissingTrailingLayers();

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
    const QString raw = ui->lineEdit_rawBulletin->text().trimmed();
    if (raw.isEmpty()) {
        // Только видимая индикация (рамка + подпись), без setFocus() —
        // иначе фокус на поле сам вызовет экранную клавиатуру.
        setFieldInvalid(ui->lineEdit_rawBulletin, ui->lblRawBulletinHint, true, "Строка бюллетеня");
        return;
    }
    setFieldInvalid(ui->lineEdit_rawBulletin, ui->lblRawBulletinHint, false);

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

    // Высоты, до которых зонд не долетел (нет данных в строке), сразу
    // помечаем "//" в таблице — наглядно показывает достигнутый предел.
    fillMissingTrailingLayers();

    ui->lblStatus->setText("Строка разобрана — проверьте поля и нажмите «Применить»");
    ui->lblStatus->setStyleSheet("color: #E65100; font-weight: bold;");
}

// ─────────────────────────────────────────────────────────────────────────────
// Кнопка «Очистить»
// ─────────────────────────────────────────────────────────────────────────────
void Meteo11::onClearClicked()
{
    ui->lineEdit_rawBulletin->clear();
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
    setFieldInvalid(ui->lineEdit_rawBulletin, ui->lblRawBulletinHint, false);
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
    connect(ui->lineEdit_rawBulletin, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!text.trimmed().isEmpty())
            setFieldInvalid(ui->lineEdit_rawBulletin, ui->lblRawBulletinHint, false);
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

// ─────────────────────────────────────────────────────────────────────────
// Meteo11LayerCellDelegate — редактор ячеек таблицы "Слои ветра" с
// привязанной экранной клавиатурой (цифровая раскладка для всех колонок:
// ПП — поправка за плотность, Напр. — направление 0-360, СС — скорость).
// ─────────────────────────────────────────────────────────────────────────

Meteo11LayerCellDelegate::Meteo11LayerCellDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* Meteo11LayerCellDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                                 const QModelIndex &index) const
{
    QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
    auto *lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit)
        return editor;

    VirtualKeyboard::Constraints c;
    c.allowNegative   = false;
    c.allowDecimal    = false;
    c.allowModeSwitch = false;

    switch (index.column()) {
    case 0: // ПП — двузначный код поправки за плотность воздуха; по
            // умолчанию "//" (не измерялось) — разрешаем "/" вместе с цифрами.
        c.maxLength  = 2;
        c.allowSlash = true;
        break;
    case 1: // Напр. — направление ветра, 0–360°
        c.maxLength = 3;
        break;
    case 2: // СС — скорость, м/с
        c.maxLength = 2;
        break;
    default:
        break;
    }

    VirtualKeyboard::attach(lineEdit, VirtualKeyboard::Mode::Numeric, c);
    return editor;
}
