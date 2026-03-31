PATH_TO_REFERENCES = ../../references

QT       += core gui quick quickwidgets qml positioning serialport sql printsupport core-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
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
    workregulationdialog.cpp

HEADERS += \
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
    workregulationdialog.h


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


DISTFILES +=