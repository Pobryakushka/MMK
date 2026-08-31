// ─────────────────────────────────────────────────────────────────────────
// Поля координат и ориентации: чтение и запись, проверка заполненности,
// режим ручного ввода и разрешение конфликта источников (карта/ГНСС/БИНС).
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

// =================================================
// Методы работы с координатами
// =================================================

void MainWindow::setCoordField(QLineEdit *edit, double dec_deg)
{
    if (!edit) return;

    // Конвертируем десятичные градусы
    QString dmsString = CoordHelper::toDisplayDMS(dec_deg);
    edit->setText(dmsString);
}

double MainWindow::getCoordField(QLineEdit *edit, bool &ok) const
{
    ok = false;
    if (!edit) return 0.0;

    QString text = edit->text().trimmed();
    if (text.isEmpty()) return 0.0;

    double degrees = 0.0;
    ok = CoordHelper::parseDMS(text, degrees);

    return ok ? degrees : 0.0;
}

// ── Готовность положения (ГНСС) / ориентации (БИНС) ─────────────────────
// ВАЖНО: это флаги "данные реально получены", а НЕ парсинг текста полей —
// поля editLatitude/../editPitchAngle в .ui изначально содержат непустые
// демонстрационные значения (Designer), поэтому "текст непустой" не значит
// "данные реальные". Флаги выставляются только настоящими источниками —
// см. onGnssDataReceived/updateCoordinatesFromMap/onBinsDataReceived/
// updateManualHighlightAfterManualInput.
bool MainWindow::hasPositionData() const
{
    return m_hasGnssPosition;
}

bool MainWindow::hasOrientationData() const
{
    return m_hasBinsOrientation;
}

// "Сырая" проверка полей — используется ТОЛЬКО для решения о жёлтой
// подсветке при выходе из ручного режима (см. updateManualHighlightAfterManualInput).
bool MainWindow::fieldsLookLikePosition() const
{
    bool ok1 = false, ok2 = false;
    getCoordField(ui->editLatitude, ok1);
    getCoordField(ui->editLongitude, ok2);
    const bool altOk = !ui->editAltitude->text().trimmed().isEmpty();
    return ok1 && ok2 && altOk;
}

bool MainWindow::fieldsLookLikeOrientation() const
{
    bool ok = false;
    ui->editDirectionAngle->text().toDouble(&ok);
    if (!ok) return false;
    ui->editRollAngle->text().toDouble(&ok);
    if (!ok) return false;
    ui->editPitchAngle->text().toDouble(&ok);
    return ok;
}

void MainWindow::onCoordTextEdited(QLineEdit *edit)
{
    if (!edit) return;

    static bool isProcessing = false;
    if (isProcessing) return;
    isProcessing = true;

    QString rawText = edit->text();
    int cursorPos = edit->cursorPosition();

    QString formatted = CoordHelper::formatInput(rawText, cursorPos);

    if (formatted != rawText) {
        edit->setText(formatted);
        edit->setCursorPosition(cursorPos);
    }

    isProcessing = false;
}

// Выравнивает checkboxGnssPos (дубликат-чип на странице "Положение") по
// текущему checked-состоянию "настоящего" checkboxGnss (на карте).
// blockSignals — чтобы не спровоцировать повторный вызов пересылки на
// checkboxGnss (см. лямбду в setupUi/конструкторе) и не зациклиться.
void MainWindow::syncGnssPosCheckbox()
{
    if (!ui->checkboxGnssPos) return;
    const bool checked = ui->checkboxGnss->isChecked();
    if (ui->checkboxGnssPos->isChecked() == checked) return;
    ui->checkboxGnssPos->blockSignals(true);
    ui->checkboxGnssPos->setChecked(checked);
    ui->checkboxGnssPos->blockSignals(false);
}

void MainWindow::checkAndDisableConflictingSources(const QString &activeSource)
{
    if (activeSource == "map") {
        // Отключаем GNSS и ручной ввод
        if (m_gnssEnabled) {
            ui->checkboxGnss->setChecked(false);
        }
        m_manualInputEnabled = false;
        // Координаты теперь с карты — предыдущая подсветка "введено вручную"
        // (если была) больше не актуальна.
        m_gnssManualHighlight = false;
        updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
        updateOverallReadiness();
    } else if (activeSource == "gnss") {
        // Отключаем карту и ручной ввод
        if (m_mapCoordinatesEnabled) {
            ui->btnMapCoordinates->setChecked(false);
            m_mapCoordinatesEnabled = false;
            updateMapCoordinatesButtonStyle();
        }
        m_manualInputEnabled = false;
        m_gnssManualHighlight = false;
        updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
        updateOverallReadiness();
    } else if (activeSource == "manual") {
        // Отключаем карту и GNSS
        if (m_mapCoordinatesEnabled) {
            ui->btnMapCoordinates->setChecked(false);
            m_mapCoordinatesEnabled = false;
            updateMapCoordinatesButtonStyle();
        }
        if (m_gnssEnabled) {
            ui->checkboxGnss->setChecked(false);
        }
    }
}

void MainWindow::updateCoordinateSource(const QString &source)
{
    QString message = "Источник координат: " + source;
    statusBar()->showMessage(message, 3000);
    qDebug() << message;
}

void MainWindow::updateFieldsEditability()
{
    // Поля редактируемы только если все источники выключены (ручной ввод)
    bool fieldsEditable = !m_mapCoordinatesEnabled && !m_gnssEnabled;

    ui->editLatitude->setReadOnly(!fieldsEditable);
    ui->editLongitude->setReadOnly(!fieldsEditable);
    ui->editAltitude->setReadOnly(!fieldsEditable);

    // Раньше здесь стоял точечный setStyleSheet(...) с серым фоном "не
    // редактируется" — он перебивал общий стиль карточки положения
    // (QSS страницы page_position, см. .ui). Визуальное отличие
    // редактируемо/нередактируемо теперь достаточно даёт enabled/disabled
    // самих полей (см. onManualInputClicked) — readOnly используется здесь
    // только для источников "карта"/"ГНСС", когда поля остаются enabled,
    // но не должны принимать ручной ввод.
}

// Вызывается сразу после выхода из режима ручного ввода (onManualInputClicked,
// переход enabled→disabled). Смотрит на итоговое содержимое полей — какая
// из групп (положение/ориентация) реально заполнена — и подсвечивает
// соответствующую плашку датчика жёлтым. Группы независимы (вариант B).
void MainWindow::updateManualHighlightAfterManualInput()
{
    const bool posOk = fieldsLookLikePosition();
    const bool oriOk = fieldsLookLikeOrientation();

    m_gnssManualHighlight = posOk;
    m_binsManualHighlight = oriOk;

    // Ручной ввод подтверждён (кнопка "Ручной ввод" выключена) — если поля
    // валидны, это ТЕПЕРЬ реальные данные положения/ориентации, а не просто
    // текст в поле. Не сбрасываем в false при !posOk/!oriOk — оператор мог
    // выключить ручной режим, не тронув эту группу полей вовсе, тогда как
    // они были данные с датчика ранее.
    if (posOk) m_hasGnssPosition = true;
    if (oriOk) m_hasBinsOrientation = true;

    updateGnssStatusLabel(m_gnssHandler && m_gnssHandler->isConnected());
    updateBinsStatusLabel(m_binsHandler && m_binsHandler->isConnected());
    updateOverallReadiness();
}
