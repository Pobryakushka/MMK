// ─────────────────────────────────────────────────────────────────────────
// Страница «Карта»: создание QML-компонента, выбор типа карты и MBTiles,
// маркеры, плавающие элементы управления поверх карты.
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

void MainWindow::setupMapCoordinatesButton()
{
    // Кнопка теперь в UI файле, просто настраиваем иконку и подключаем сигнал
    QIcon markerIcon(":/dat/images/marker.png");
    ui->btnMapCoordinates->setIcon(markerIcon);
    ui->btnMapCoordinates->setIconSize(QSize(20, 20));

    connect(ui->btnMapCoordinates, &QPushButton::clicked, this, &MainWindow::onMapCoordinatesToggled);
    // Дубликат-чип на странице "Положение" — тот же обработчик (он не
    // принимает параметров, просто флипает m_mapCoordinatesEnabled и красит
    // оба виджета через updateMapCoordinatesButtonStyle()).
    connect(ui->btnMapCoordinatesPos, &QPushButton::clicked, this, &MainWindow::onMapCoordinatesToggled);
}

void MainWindow::updateGnssMarkerOnMap(double latitude, double longitude)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
        QMetaObject::invokeMethod(main, "updateGnssMarker", Qt::DirectConnection,
                                  Q_ARG(QVariant, latitude),
                                  Q_ARG(QVariant, longitude),
                                  Q_ARG(QVariant, m_gnssEnabled));
    }
}

void MainWindow::updateMapCoordinatesButtonStyle()
{
    QIcon markerIcon(":/dat/images/marker.png");

    // Есть ДВА виджета этой кнопки — оригинал на карте (плавающий маркер,
    // по которому и правда тапают) и чип-дубликат на странице "Положение"
    // (см. .ui). Одно состояние m_mapCoordinatesEnabled — оба отражают его
    // одинаково, каждый в своём стиле.
    ui->btnMapCoordinates->setIcon(markerIcon);
    ui->btnMapCoordinates->setIconSize(QSize(20, 20));
    ui->btnMapCoordinates->setChecked(m_mapCoordinatesEnabled);
    // Чипу на странице "Положение" достаточно setChecked: его вид (белый /
    // залитый зелёным) целиком описан ролью [toggle] в ui/screen-theme.qss,
    // селектором :checked. Точечный setStyleSheet здесь раньше перебивал эту
    // стилизацию — и оформление жило прямо в коде обработчика.
    if (ui->btnMapCoordinatesPos)
        ui->btnMapCoordinatesPos->setChecked(m_mapCoordinatesEnabled);

    // Пока режим выбора не активен, карта не должна двигать маркер по тапу
    // (координаты в форме всё равно не обновятся — см. лямбду на
    // coordinateFromChanged в конструкторе, — а «прыгающий» маркер сбивал
    // оператора с толку).
    qcp.setPickingEnabled(m_mapCoordinatesEnabled);

    if (m_mapCoordinatesEnabled) {
        ui->btnMapCoordinates->setText("Тапните карту");
        ui->btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: #0F6B4F;"
            "   border: 2px solid #0B5A41;"
            "   border-radius: 12px;"
            "   padding: 4px 14px 4px 10px;"
            "   font-size: 9pt; font-weight: 600; color: #FFFFFF;"
            "}"
            "QPushButton:hover { background-color: #0B5A41; }"
            );
        ui->btnMapCoordinates->setToolTip("Режим координат с карты активен — тапните точку на карте");
    } else {
        ui->btnMapCoordinates->setText("Указать точку");
        ui->btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(255,255,255,235);"
            "   border: none;"
            "   border-radius: 12px;"
            "   padding: 4px 14px 4px 10px;"
            "   font-size: 9pt; font-weight: 600; color: #1C1F22;"
            "}"
            "QPushButton:hover { background-color: #f0f0f0; }"
            );
        ui->btnMapCoordinates->setToolTip("Использовать координаты с карты (нажмите, затем тапните точку на карте)");
    }
}

void MainWindow::onMapCoordinatesToggled()
{
    m_mapCoordinatesEnabled = !m_mapCoordinatesEnabled;
    updateMapCoordinatesButtonStyle();

    if (m_mapCoordinatesEnabled) {
        checkAndDisableConflictingSources("map");
        updateCoordinateSource("Карта");
    } else {
        updateCoordinateSource("Нет");
    }
    updateFieldsEditability();

    emit mapCoordinatesModeChanged(m_mapCoordinatesEnabled);

    // Выводим сообщение о смене режима
    QString status = m_mapCoordinatesEnabled ?
                         "Режим координат с карты ВКЛЮЧЕН" :
                         "Режим координат с карты ВЫКЛЮЧЕН";
    statusBar()->showMessage(status, 3000);
}

void MainWindow::updateCoordinatesFromMap(double latitude, double longitude)
{
    setCoordField(ui->editLatitude, latitude);
    setCoordField(ui->editLongitude, longitude);

    // Координаты выбраны на карте — это реальные данные положения (высота
    // от карты не приходит, но широта/долгота — основа "положения").
    m_hasGnssPosition = true;
    updateMapCoordDisplay("С карты");
    // Маркер на карте (MapComponent.qml) скрыт, пока оператор не выбрал
    // точку хотя бы раз — иначе он бы показывал захардкоженные координаты
    // по умолчанию из QmlCoordinateProxy как будто уже выбранную точку.
    qcp.setHasSelection(true);
    updateOverallReadiness();

    // Передаем сигнал другим окнам
    emit coordinatesUpdatedFromMap(latitude, longitude);
}

void MainWindow::repositionMapFloatingControls()
{
    if (!ui->mapCanvas) return;

    const int margin = 16;
    const int gap = 8;
    const int canvasWidth = ui->mapCanvas->width();

    // Строка 1 слева: текущие выбранные координаты — подсказка при выборе
    // точки маркером (см. updateMapCoordDisplay()).
    if (ui->lblMapCoordDisplay) {
        ui->lblMapCoordDisplay->adjustSize();
        ui->lblMapCoordDisplay->move(margin, margin);
    }

    // Строка 1 справа: маркер (выбор координат с карты) + GNSS справа от него
    const int markerSize = ui->btnMapCoordinates->width();
    const int gnssWidth = ui->checkboxGnss->width();
    const int row1Height = ui->btnMapCoordinates->height();

    const int gnssX = canvasWidth - gnssWidth - margin;
    const int markerX = gnssX - gap - markerSize;
    ui->checkboxGnss->move(gnssX, margin);
    ui->btnMapCoordinates->move(markerX, margin);

    // Строка 2: выбор типа карты — под строкой 1, прижат к правому краю
    const int comboWidth = ui->comboBox_mapTypes->width();
    const int y2 = margin + row1Height + gap;
    ui->comboBox_mapTypes->move(canvasWidth - comboWidth - margin, y2);

    // Поднимаем плавающие элементы над картой в порядке отрисовки
    if (ui->lblMapCoordDisplay)
        ui->lblMapCoordDisplay->raise();
    ui->btnMapCoordinates->raise();
    ui->checkboxGnss->raise();
    ui->comboBox_mapTypes->raise();
}

// Обновляет текст плавающей подсказки над картой (lblMapCoordDisplay) в
// соответствии с текущими editLatitude/editLongitude — они уже хранят
// последнее выбранное значение в отображаемом DMS-формате (см. setCoordField,
// вызывается и из onGnssDataReceived, и из updateCoordinatesFromMap).
// m_hasGnssPosition отличает "реальные данные когда-либо получены" от
// демо-значений полей из Designer (см. комментарий у hasPositionData()).
void MainWindow::updateMapCoordDisplay(const QString &sourceLabel)
{
    if (!ui->lblMapCoordDisplay) return;

    m_lastCoordSourceLabel = sourceLabel;

    if (!m_hasGnssPosition) {
        ui->lblMapCoordDisplay->setText("Координаты не выбраны");
    } else {
        // Источник в подписи — иначе непонятно, какой из двух разных
        // маркеров на карте (красный пин "с карты" / синяя точка GNSS)
        // сейчас показывает актуальную точку.
        ui->lblMapCoordDisplay->setText(
            sourceLabel + QString(": Ш: %1   Д: %2")
                .arg(ui->editLatitude->text(), ui->editLongitude->text()));
    }

    repositionMapFloatingControls();
}

void MainWindow::createMapComponent(const QString &pluginName)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
        qcp.setMapTypes(QStringList());
        QMetaObject::invokeMethod(main, "createMapComponent", Qt::DirectConnection,
                                  Q_ARG(QVariant, pluginName));
        setupMapItems(main);
    }
}

void MainWindow::setupMapItems(QQuickItem *item)
{
    if (item) {
        item->update();
    }
}

// ── MBTiles / LocalTileServer методы ─────────────────────────────────────────

void MainWindow::writeProvidersJson(const QString &providersDir, const QString &urlTemplate)
{
    // Строим JSON через replace, чтобы %z/%x/%y в urlTemplate не мешал QString::arg()
    QString tmpl =
        "{\n"
        "    \"UrlTemplate\":      \"TILE_URL\",\n"
        "    \"ImageFormat\":      \"png\",\n"
        "    \"MapCopyRight\":     \"<a href='https://www.openstreetmap.org/copyright'>OpenStreetMap</a>\",\n"
        "    \"DataCopyRight\":    \"<a href='https://www.openstreetmap.org/copyright'>OpenStreetMap contributors</a>\",\n"
        "    \"MinimumZoomLevel\": 0,\n"
        "    \"MaximumZoomLevel\": 19\n"
        "}";
    QByteArray json = tmpl.replace("TILE_URL", urlTemplate).toUtf8();

    static const QStringList types = {
        "street", "terrain", "street-hires", "terrain-hires",
        "satellite", "satellite-hires", "cycle", "cycle-hires",
        "transit", "transit-hires", "night-transit", "night-transit-hires",
        "hiking", "hiking-hires"
    };
    for (const QString &type : types) {
        QFile f(providersDir + "/" + type);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
            f.write(json);
    }
}

void MainWindow::refreshMapCombo()
{
    QComboBox *combo = ui->comboBox_mapTypes;
    QSignalBlocker blocker(combo);

    // Запоминаем текущий выбор, чтобы восстановить его после перестройки
    QString savedMbtiles = m_currentMbtilesPath;
    int     savedOsmIdx  = m_osmCurrentIndex;

    combo->clear();

    // 1. OSM-типы
    for (const QString &name : m_osmMapTypeNames)
        combo->addItem(name);

    // 2. .mbtiles-файлы из MapCache
    QDir dir(m_mapCacheDir);
    QStringList files = dir.entryList({"*.mbtiles"}, QDir::Files, QDir::Name);
    if (!files.isEmpty()) {
        combo->insertSeparator(combo->count());
        for (const QString &file : files) {
            QString label = "[офлайн] " + QFileInfo(file).completeBaseName();
            combo->addItem(label, dir.absoluteFilePath(file));
        }
    }

    // 3. Восстанавливаем выбор
    if (!savedMbtiles.isEmpty()) {
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemData(i).toString() == savedMbtiles) {
                combo->setCurrentIndex(i);
                return;
            }
        }
    }
    if (savedOsmIdx >= 0 && savedOsmIdx < m_osmMapTypeNames.size())
        combo->setCurrentIndex(savedOsmIdx);
}

void MainWindow::onMapComboChanged(int index)
{
    if (index < 0) return;

    QString mbtilesPath = ui->comboBox_mapTypes->itemData(index).toString();

    if (!mbtilesPath.isEmpty()) {
        // Офлайн-режим: тайлы только из MBTiles-файла
        m_currentMbtilesPath = mbtilesPath;
        applyMbtilesFile(mbtilesPath);
    } else if (index < m_osmMapTypeNames.size()) {
        // Онлайн-режим: OSM с кэшированием в MBTiles
        m_currentMbtilesPath.clear();
        m_osmCurrentIndex = index;
        applyOnlineMapType(index);
    }
}

void MainWindow::applyOnlineMapType(int osmIndex)
{
    // Только переключаем БД в сервере — карту не пересоздаём (порт не меняется)
    QString mapName     = m_osmMapTypeNames.value(osmIndex, "Street Map");
    QString mbtilesPath = m_mapCacheDir + "/" + mapName + ".mbtiles";

    if (m_tileServer)
        m_tileServer->switchTo(mbtilesPath,
                               "https://a.tile.openstreetmap.org/%1/%2/%3.png");

    // Переключаем активный тип карты в QML (сетка тайлов та же, данные из нашего сервера)
    qcp.setCurrentMapType(osmIndex);
}

void MainWindow::applyMbtilesFile(const QString &mbtilesPath)
{
    // Только переключаем БД в сервере — карту не пересоздаём (порт не меняется)
    if (m_tileServer)
        m_tileServer->switchTo(mbtilesPath, QString()); // офлайн: нет upstream
}
