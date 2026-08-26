PATH_TO_REFERENCES = ../../references

QT       += core gui quick quickwidgets qml positioning location network serialport sql printsupport core-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

THIRDPARTY = $$PWD/3rdparty

# ─── Plow (PlowAlgoritm) — расчёт фактического и среднего ветра ───
PLOW_DIR = $$THIRDPARTY/plow
rr
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
HEADERS *= $$PLOW_HEADERS \
    ui/ClickableFrame.h \
    ui/anglecheckpage.h \
    ui/inspectionpage.h \
    ui/workregulationhubpage.h \
    ui/notificationtoast.h \

# ─── ClimatData — климатические данные по широте/долготе/месяцу ───
CLIMAT_DIR = $$THIRDPARTY/climatData
INCLUDEPATH += $$CLIMAT_DIR

SOURCES *= \
    $$CLIMAT_DIR/climatdata.cpp \
    $$CLIMAT_DIR/climatdataprivate.cpp \
    ui/ClickableFrame.cpp \
    ui/anglecheckpage.cpp \
    ui/inspectionpage.cpp \
    ui/workregulationhubpage.cpp \
    ui/notificationtoast.cpp \

HEADERS *= \
    $$CLIMAT_DIR/climatdata.h \
    $$CLIMAT_DIR/climatdata_global.h \
    $$CLIMAT_DIR/climatdataprivate.h

# Warn on deprecated Qt API usage (does not break the build, just emits warnings)
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    LocalTileServer.cpp \
    calculationAlgorithms/AlgorithmsCalc.cpp \
    ui/ExportDialog.cpp \
    sensors/GroundMeteoParams.cpp \
    calculationAlgorithms/LandingCalculation.cpp \
    Map/FormMapView.cpp \
    ui/MeasurementExporter.cpp \
    MeasurementResults.cpp \
    Meteo11.cpp \
    SourceData.cpp \
    sensors/binshandler.cpp \
    databasemanager.cpp \
    ui/functionalcontroldialog.cpp \
    sensors/gnsshandler.cpp \
    main.cpp \
    ui/mainwindow.cpp \
    ui/sensorsettings.cpp \
    sensors/zedf9preceiver.cpp \
    customprotocol.cpp \
    sensors/amshandler.cpp \
    sensors/amsprotocol.cpp \
    zoom/qaxiszoomsvc.cpp \
    zoom/qwheelzoomsvc.cpp \
    zoom/qwtchartzoom.cpp \
    zoom/zoomscontainer.cpp \
    autoconnector.cpp \
    calculationAlgorithms/WindShearCalculator.cpp \
    surfacemeteosaver.cpp \
    ui/RpvIndicator.cpp \
    calculationAlgorithms/windprofilecalculator.cpp \
    VirtualKeyboard.cpp \
    Meteo11Grib/ProcessRunner.cpp \
    Meteo11Grib/GfsDownloadRunner.cpp \
    Meteo11Grib/MushroomRunner.cpp \
    Meteo11Grib/MushroomResultParser.cpp \
    Meteo11Grib/Meteo11Calculator.cpp \
    Meteo11Grib/Meteo11ProfileBuilder.cpp \
    Meteo11Grib/GribWindProfileAdapter.cpp \
    Meteo11Grib/GribMeteo11Pipeline.cpp \
    ui/ClickableLabel.cpp \

HEADERS += \
    LocalTileServer.h \
    calculationAlgorithms/AlgorithmsCalc.h \
    CoordHelper.h \
    ui/ExportDialog.h \
    sensors/GroundMeteoParams.h \
    calculationAlgorithms/LandingCalculation.h \
    LandingCalculationState.h \
    Map/FormMapView.h \
    Map/InitialParameters.h \
    ui/MeasurementExporter.h \
    MeasurementResults.h \
    Meteo11.h \
    SourceData.h \
    sensors/binshandler.h \
    databasemanager.h \
    ui/functionalcontroldialog.h \
    sensors/gnsshandler.h \
    ui/mainwindow.h \
    qmlcoordinateproxy.h \
    ui/sensorsettings.h \
    sensors/zedf9preceiver.h \
    customprotocol.h \
    sensors/amshandler.h \
    sensors/amsprotocol.h \
    zoom/qaxiszoomsvc.h \
    zoom/qwheelzoomsvc.h \
    zoom/qwtchartzoom.h \
    zoom/zoomscontainer.h \
    autoconnector.h \
    calculationAlgorithms/WindShearCalculator.h \
    surfacemeteosaver.h \
    ui/RpvIndicator.h \
    calculationAlgorithms/windprofilecalculator.h \
    VirtualKeyboard.h \
    Meteo11Grib/ProcessRunner.h \
    Meteo11Grib/GfsDownloadRunner.h \
    Meteo11Grib/MushroomRunner.h \
    Meteo11Grib/MushroomMessage.h \
    Meteo11Grib/MushroomResultParser.h \
    Meteo11Grib/Meteo11Types.h \
    Meteo11Grib/Meteo11Calculator.h \
    Meteo11Grib/Meteo11ProfileBuilder.h \
    Meteo11Grib/GribWindProfileAdapter.h \
    Meteo11Grib/GribMeteo11Pipeline.h \
    Meteo11Grib/GribConfig.h \
    ui/ClickableLabel.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    AlgorithmsCalc.ui \
    GroundMeteoParams.ui \
    LandingCalculation.ui \
    Map/FormMapView.ui \
    MeasurementResults.ui \
    Meteo11.ui \
    SourceData.ui \
    anglecheckpage.ui \
    functionalcontroldialog.ui \
    inspectionpage.ui \
    mainwindow.ui \
    sensorsettings.ui \
    ExportDialog.ui \
    workregulationhubpage.ui \

RESOURCES += \
    Resources.qrc

include(qwt.pri)
include(QXlsx/QXlsx.pri)


# ─── Копирование климатической базы рядом с исполняемым файлом ───
# WindProfileCalculator ищет базу относительно applicationDirPath().
# Это правило копирует 3rdparty/climatData в build-папку при сборке,
# поэтому climatData/climat/warm0405.out оказывается рядом с бинарником.
climat_db.files = $$PWD/3rdparty/climatData
climat_db.path  = $$OUT_PWD
COPIES += climat_db