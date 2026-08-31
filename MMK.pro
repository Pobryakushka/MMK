PATH_TO_REFERENCES = $$absolute_path($$PWD/../../references)

QT       += core gui quick quickwidgets qml positioning location network serialport sql printsupport core-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

THIRDPARTY = $$PWD/3rdparty

# ─── Пути поиска заголовков собственного кода ───────────────────────────────
# Весь наш код лежит в src/ и подключается ЕДИНООБРАЗНО — путём от корня src,
# в котором первым элементом идёт слой: "ui/...", "core/...", "data/...",
# "devices/...", "map/...", "utils/...". Благодаря этому слой видно прямо в
# строке #include, а прежний разнобой ("ScreenTheme.h" / "ui/ScreenTheme.h" /
# "../ui/ScreenTheme.h" для одного и того же файла) стал невозможен.
#
# 3rdparty в путях поиска — ради вендорных библиотек, которые подключаются
# как "qwtzoom/..." (QwtChartZoom).
INCLUDEPATH += \
    $$PWD/src \
    $$THIRDPARTY

# ─── Plow (PlowAlgoritm) — расчёт фактического и среднего ветра ───
PLOW_DIR = $$THIRDPARTY/plow

# Пути к заголовкам оставляем прежними, чтобы основной проект видел инклюды
INCLUDEPATH += \
    $$PLOW_DIR \
    $$PLOW_DIR/InData \
    $$PLOW_DIR/Profile \
    $$PLOW_DIR/mhn

# Подключаем Plow как разделяемую библиотеку (.so)
# -L указывает директорию, где искать либу, -l задает имя (без префикса lib и расширения .so)
# ПРИМЕЧАНИЕ: Если файл называется libPlow.so (с большой буквы), замените -lplow на -lPlow
LIBS += -L$$PLOW_DIR -lPlowAlgoritm

# Опционально: добавляем rpath, чтобы исполняемый файл искал .so прямо в папке plow при запуске
QMAKE_LFLAGS += -Wl,-rpath,$$PLOW_DIR

# Исходники (.cpp) отсюда убраны, так как они уже скомпилированы в .so.
# Оставляем только заголовочные файлы для корректного отображения структуры в дереве Qt Creator.
PLOW_HEADERS = $$files($$PLOW_DIR/*.h,   true)
HEADERS *= $$PLOW_HEADERS

# ─── ClimatData — климатические данные по широте/долготе/месяцу ───
CLIMAT_DIR = $$THIRDPARTY/climatData
INCLUDEPATH += $$CLIMAT_DIR

SOURCES *= \
    $$CLIMAT_DIR/climatdata.cpp \
    $$CLIMAT_DIR/climatdataprivate.cpp

HEADERS *= \
    $$CLIMAT_DIR/climatdata.h \
    $$CLIMAT_DIR/climatdata_global.h \
    $$CLIMAT_DIR/climatdataprivate.h

# ─── QwtChartZoom — сторонний компонент масштабирования графиков Qwt ───
# (В. 1.5.2, Мельников С. А., 2012. Свободное использование, упоминание
# автора обязательно — см. шапки файлов.)
QWTZOOM_DIR = $$THIRDPARTY/qwtzoom

SOURCES *= \
    $$QWTZOOM_DIR/qaxiszoomsvc.cpp \
    $$QWTZOOM_DIR/qwheelzoomsvc.cpp \
    $$QWTZOOM_DIR/qwtchartzoom.cpp \
    $$QWTZOOM_DIR/zoomscontainer.cpp

HEADERS *= \
    $$QWTZOOM_DIR/qaxiszoomsvc.h \
    $$QWTZOOM_DIR/qwheelzoomsvc.h \
    $$QWTZOOM_DIR/qwtchartzoom.h \
    $$QWTZOOM_DIR/zoomscontainer.h

# Warn on deprecated Qt API usage (does not break the build, just emits warnings)
DEFINES += QT_DEPRECATED_WARNINGS


# ═══════════════════════════════════════════════════════════════════════════
#  Собственный код — по слоям (см. INCLUDEPATH выше)
# ═══════════════════════════════════════════════════════════════════════════

# ─── Точка входа ───────────────────────────────────────────────────────────
SOURCES += \
    src/app/main.cpp

# ─── UI: главное окно ──────────────────────────────────────────────────────
# Один класс MainWindow, реализация которого разложена по нескольким единицам
# трансляции по темам (было 4990 строк в одном файле). Публичный интерфейс
# по-прежнему один — mainwindow.h; общие для всех частей include и константы
# лежат в приватном mainwindow_internal.h.
SOURCES += \
    src/ui/mainwindow/mainwindow.cpp \
    src/ui/mainwindow/mainwindow_coords.cpp \
    src/ui/mainwindow/mainwindow_map.cpp \
    src/ui/mainwindow/mainwindow_measurement.cpp \
    src/ui/mainwindow/mainwindow_overlays.cpp \
    src/ui/mainwindow/mainwindow_sensors.cpp \
    src/ui/mainwindow/mainwindow_status.cpp

HEADERS += \
    src/ui/mainwindow/mainwindow.h \
    src/ui/mainwindow/mainwindow_internal.h

# ─── UI: архив измерений ───────────────────────────────────────────────────
# MeasurementResults — тоже один класс, разложенный по темам (было 3741 строка).
SOURCES += \
    src/ui/archive/MeasurementResults.cpp \
    src/ui/archive/MeasurementResults_db.cpp \
    src/ui/archive/MeasurementResults_export.cpp \
    src/ui/archive/MeasurementResults_meteo11.cpp \
    src/ui/archive/MeasurementResults_plots.cpp \
    src/ui/archive/MeasurementResults_style.cpp \
    src/ui/archive/ArchiveDatePopup.cpp \
    src/ui/archive/ArchiveExportView.cpp \
    src/ui/archive/ExportDialog.cpp

HEADERS += \
    src/ui/archive/MeasurementResults.h \
    src/ui/archive/MeasurementResults_internal.h \
    src/ui/archive/ArchiveDatePopup.h \
    src/ui/archive/ArchiveExportView.h \
    src/ui/archive/ExportDialog.h

# ─── UI: экраны и диалоги ──────────────────────────────────────────────────
SOURCES += \
    src/ui/pages/AlgorithmsCalc.cpp \
    src/ui/pages/GroundMeteoParams.cpp \
    src/ui/pages/LandingCalculation.cpp \
    src/ui/pages/Meteo11.cpp \
    src/ui/pages/SourceData.cpp \
    src/ui/pages/anglecheckpage.cpp \
    src/ui/pages/functionalcontroldialog.cpp \
    src/ui/pages/inspectionpage.cpp \
    src/ui/pages/sensorsettings.cpp \
    src/ui/pages/workregulationhubpage.cpp

HEADERS += \
    src/ui/pages/AlgorithmsCalc.h \
    src/ui/pages/GroundMeteoParams.h \
    src/ui/pages/LandingCalculation.h \
    src/ui/pages/Meteo11.h \
    src/ui/pages/SourceData.h \
    src/ui/pages/anglecheckpage.h \
    src/ui/pages/functionalcontroldialog.h \
    src/ui/pages/inspectionpage.h \
    src/ui/pages/sensorsettings.h \
    src/ui/pages/workregulationhubpage.h

# ─── UI: переиспользуемые виджеты ──────────────────────────────────────────
SOURCES += \
    src/ui/widgets/ClickableFrame.cpp \
    src/ui/widgets/ClickableLabel.cpp \
    src/ui/widgets/FlowLayout.cpp \
    src/ui/widgets/RpvIndicator.cpp \
    src/ui/widgets/VirtualKeyboard.cpp \
    src/ui/widgets/notificationtoast.cpp

HEADERS += \
    src/ui/widgets/ClickableFrame.h \
    src/ui/widgets/ClickableLabel.h \
    src/ui/widgets/FlowLayout.h \
    src/ui/widgets/RpvIndicator.h \
    src/ui/widgets/VirtualKeyboard.h \
    src/ui/widgets/notificationtoast.h

# ─── UI: оформление (общий вид экранов, кнопка «Назад») ────────────────────
HEADERS += \
    src/ui/theme/ScreenTheme.h

# ─── Логика: расчёт профиля ветра ──────────────────────────────────────────
SOURCES += \
    src/core/windprofile/WindShearCalculator.cpp \
    src/core/windprofile/windprofilecalculator.cpp

HEADERS += \
    src/core/windprofile/WindShearCalculator.h \
    src/core/windprofile/windprofilecalculator.h

# ─── Логика: GRIB-конвейер Метео-11 ────────────────────────────────────────
SOURCES += \
    src/core/grib/GfsDownloadRunner.cpp \
    src/core/grib/GribMeteo11Pipeline.cpp \
    src/core/grib/GribWindProfileAdapter.cpp \
    src/core/grib/Meteo11Calculator.cpp \
    src/core/grib/Meteo11ProfileBuilder.cpp \
    src/core/grib/MushroomResultParser.cpp \
    src/core/grib/MushroomRunner.cpp \
    src/core/grib/ProcessRunner.cpp

HEADERS += \
    src/core/grib/GfsDownloadRunner.h \
    src/core/grib/GribConfig.h \
    src/core/grib/GribMeteo11Pipeline.h \
    src/core/grib/GribWindProfileAdapter.h \
    src/core/grib/Meteo11Calculator.h \
    src/core/grib/Meteo11ProfileBuilder.h \
    src/core/grib/Meteo11Types.h \
    src/core/grib/MushroomMessage.h \
    src/core/grib/MushroomResultParser.h \
    src/core/grib/MushroomRunner.h \
    src/core/grib/ProcessRunner.h

# ─── Логика: бюллетень Метео-11 (кодирование по протоколу) ─────────────────
# Чистые вычисления без QtWidgets; раньше жили приватными статическими
# методами внутри виджета архива измерений.
SOURCES += \
    src/core/meteo11/Meteo11Codec.cpp

HEADERS += \
    src/core/meteo11/Meteo11Codec.h \
    src/core/meteo11/Meteo11Data.h

# ─── Логика: выгрузка результатов измерения (TXT/CSV/JSON/PDF/XLSX) ────────
SOURCES += \
    src/core/export/MeasurementExporter.cpp

HEADERS += \
    src/core/export/MeasurementExporter.h

# ─── Логика: модели данных ─────────────────────────────────────────────────
HEADERS += \
    src/core/model/InitialParameters.h \
    src/core/model/LandingCalculationState.h

# ─── Хранение (БД) ─────────────────────────────────────────────────────────
SOURCES += \
    src/data/databasemanager.cpp \
    src/data/surfacemeteosaver.cpp

HEADERS += \
    src/data/databasemanager.h \
    src/data/surfacemeteosaver.h

# ─── Датчики: АМС / ГНСС / БИНС / ИВС + автопоиск ──────────────────────────
SOURCES += \
    src/devices/autoconnector.cpp \
    src/devices/ams/amshandler.cpp \
    src/devices/ams/amsprotocol.cpp \
    src/devices/bins/binshandler.cpp \
    src/devices/gnss/gnsshandler.cpp \
    src/devices/gnss/zedf9preceiver.cpp \
    src/devices/iws/customprotocol.cpp

HEADERS += \
    src/devices/autoconnector.h \
    src/devices/ams/amshandler.h \
    src/devices/ams/amsprotocol.h \
    src/devices/bins/binshandler.h \
    src/devices/gnss/gnsshandler.h \
    src/devices/gnss/zedf9preceiver.h \
    src/devices/iws/customprotocol.h

# ─── Карта ─────────────────────────────────────────────────────────────────
SOURCES += \
    src/map/FormMapView.cpp \
    src/map/LocalTileServer.cpp

HEADERS += \
    src/map/FormMapView.h \
    src/map/LocalTileServer.h \
    src/map/qmlcoordinateproxy.h

# ─── Утилиты ───────────────────────────────────────────────────────────────
HEADERS += \
    src/utils/CoordHelper.h

# ─── Формы Qt Designer (лежат рядом со своими классами) ────────────────────
FORMS += \
    src/ui/mainwindow/mainwindow.ui \
    src/ui/archive/MeasurementResults.ui \
    src/ui/archive/ExportDialog.ui \
    src/ui/pages/AlgorithmsCalc.ui \
    src/ui/pages/GroundMeteoParams.ui \
    src/ui/pages/LandingCalculation.ui \
    src/ui/pages/Meteo11.ui \
    src/ui/pages/SourceData.ui \
    src/ui/pages/anglecheckpage.ui \
    src/ui/pages/functionalcontroldialog.ui \
    src/ui/pages/inspectionpage.ui \
    src/ui/pages/sensorsettings.ui \
    src/ui/pages/workregulationhubpage.ui \
    src/map/FormMapView.ui

RESOURCES += \
    Resources.qrc


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(qwt.pri)
include(3rdparty/QXlsx/QXlsx.pri)


# ─── Копирование климатической базы рядом с исполняемым файлом ───
# WindProfileCalculator ищет базу относительно applicationDirPath().
# Это правило копирует 3rdparty/climatData в build-папку при сборке,
# поэтому climatData/climat/warm0405.out оказывается рядом с бинарником.
climat_db.files = $$PWD/3rdparty/climatData
climat_db.path  = $$OUT_PWD
COPIES += climat_db
