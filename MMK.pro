PATH_TO_REFERENCES = ../../references

QT       += core gui quick quickwidgets qml positioning location network serialport sql printsupport core-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

THIRDPARTY = $$PWD/3rdparty

# Plow (PlowAlgoritm) — расчёт фактического и среднего ветра
PLOW_DIR = $$THIRDPARTY/plow

INCLUDEPATH += \
    $$PLOW_DIR \
    $$PLOW_DIR/InData \
    $$PLOW_DIR/Profile \
    $$PLOW_DIR/mhn

PLOW_SOURCES = $$files($$PLOW_DIR/*.cpp, true)
PLOW_HEADERS = $$files($$PLOW_DIR/*.h,   true)

SOURCES *= $$PLOW_SOURCES
HEADERS *= $$PLOW_HEADERS

# ClimatData — климатические данные по широте/долготе/месяцу
CLIMAT_DIR = $$THIRDPARTY/climatData
INCLUDEPATH += $$CLIMAT_DIR

SOURCES *= \
    $$CLIMAT_DIR/climatdata.cpp \
    $$CLIMAT_DIR/climatdataprivate.cpp

HEADERS *= \
    $$CLIMAT_DIR/climatdata.h \
    $$CLIMAT_DIR/climatdata_global.h \
    $$CLIMAT_DIR/climatdataprivate.h

# Warn on deprecated Qt API usage (does not break the build, just emits warnings)
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    LocalTileServer.cpp \
    AlgorithmsCalc.cpp \
    ExportDialog.cpp \
    GroundMeteoParams.cpp \
    LandingCalculation.cpp \
    Map/FormMapView.cpp \
    MeasurementExporter.cpp \
    MeasurementResults.cpp \
    Meteo11.cpp \
    SourceData.cpp \
    binshandler.cpp \
    databasemanager.cpp \
    functionalcontroldialog.cpp \
    gnsshandler.cpp \
    main.cpp \
    mainwindow.cpp \
    sensorsettings.cpp \
    zedf9preceiver.cpp \
    customprotocol.cpp \
    amshandler.cpp \
    amsprotocol.cpp \
    zoom/qaxiszoomsvc.cpp \
    zoom/qwheelzoomsvc.cpp \
    zoom/qwtchartzoom.cpp \
    zoom/zoomscontainer.cpp \
    autoconnector.cpp \
    WindShearCalculator.cpp \
    surfacemeteosaver.cpp \
    workregulationdialog.cpp \
    RpvIndicator.cpp \
    windprofilecalculator.cpp

HEADERS += \
    LocalTileServer.h \
    AlgorithmsCalc.h \
    CoordHelper.h \
    ExportDialog.h \
    GroundMeteoParams.h \
    LandingCalculation.h \
    LandingCalculationState.h \
    Map/FormMapView.h \
    Map/InitialParameters.h \
    MeasurementExporter.h \
    MeasurementResults.h \
    Meteo11.h \
    SourceData.h \
    binshandler.h \
    databasemanager.h \
    functionalcontroldialog.h \
    gnsshandler.h \
    mainwindow.h \
    qmlcoordinateproxy.h \
    sensorsettings.h \
    zedf9preceiver.h \
    customprotocol.h \
    amshandler.h \
    amsprotocol.h \
    zoom/qaxiszoomsvc.h \
    zoom/qwheelzoomsvc.h \
    zoom/qwtchartzoom.h \
    zoom/zoomscontainer.h \
    autoconnector.h \
    WindShearCalculator.h \
    surfacemeteosaver.h \
    workregulationdialog.h \
    RpvIndicator.h \
    windprofilecalculator.h


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
    functionalcontroldialog.ui \
    mainwindow.ui \
    sensorsettings.ui \
    ExportDialog.ui

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