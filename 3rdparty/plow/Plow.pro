#TEMPLATE = app
CONFIG += console plugin
CONFIG -= app_bundle
CONFIG -= qt
#QMAKE_CXXFLAGS += -std=c++98
QMAKE_CXXFLAGS += -std=c++11

#QMAKE_CXXFLAGS -= -std=gnu++11

CONFIG(debug,debug|release) {
    DEBUG_SFX =
} else {
    DEBUG_SFX =
}


TARGET = PlowAlgoritm$$DEBUG_SFX
VERSION = 0.1.11

TEMPLATE = lib
#CONFIG += staticlib

unix: OBJECTS_DIR = .obj


SOURCES += main.cpp \
    ParamsVdVsr_0_200m__3_4p/ParamsVdVsr_0_200m__3_4p.cpp \
    Subsidiary/fileprocessing.cpp \
    InData/indata.cpp \
    ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.cpp \
    FunctionsCalc/functionscalc.cpp \
    FunctionsCalc/mathfunc.cpp \
    FunctionsCalc/functionscalc_layer.cpp \
    InData/indataclimat.cpp \
    ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.cpp \
    FunctionsCalc/calc_bottomlayer.cpp \
    FunctionsCalc/calc_error.cpp \
    OutData/outdata.cpp \
    ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.cpp \
    ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.cpp \
    ParamsVd_Yl__4p/paramsvd_yl__4p.cpp \
    ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.cpp \
    ParamsVd_Yl__4p/ParamsVd_Yl__4_2p/paramsvd_yl__4_2p.cpp \
    ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.cpp \
    ParamsVd_Yl__4p/ParamsdelV__4_6p/paramsdelv__4_6p.cpp \
    ParamsVd_Yl__4p/ParamsVYl__4_7p/paramsvyl__4_7p.cpp \
    ParamsVd_Yl__4p/ParamsMatrix__4_4p/paramsmatrix__4_4p.cpp \
    ParamsVd_Yl__4p/SelectMod__4_5p/selectmod__4_5p.cpp \
    BottomEval__2p/BottomEval__2p.cpp \
    CastH__1p/casth__1p.cpp \
    Profile/profile.cpp \
    Profile/end.cpp \
    InData/constants.cpp \
    ParamsVsr_Yl__5p/paramsvsr_yl__5p.cpp \
    GetProfile__6p/getprofile__6p.cpp \
    WindShear/windshear.cpp \
    WindShear/constantswsh.cpp


HEADERS += \
    InData/InData.h \
    ParamsVdVsr_0_200m__3_4p/paramsvdvsr_0_200m__3_4p.h \
    Subsidiary/fileprocessing.h \
    InData/indata.h \
    ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.h \
    FunctionsCalc/functionscalc.h \
    FunctionsCalc/mathfunc.h \
    FunctionsCalc/functionscalc_layer.h \
    InData/indataclimat.h \
    ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h \
    FunctionsCalc/calc_bottomlayer.h \
    FunctionsCalc/calc_error.h \
    OutData/outdata.h \
    ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.h \
    ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.h \
    ParamsVd_Yl__4p/paramsvd_yl__4p.h \
    ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h \
    ParamsVd_Yl__4p/ParamsVd_Yl__4_2p/paramsvd_yl__4_2p.h \
    ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h \
    ParamsVd_Yl__4p/ParamsdelV__4_6p/paramsdelv__4_6p.h \
    ParamsVd_Yl__4p/ParamsVYl__4_7p/paramsvyl__4_7p.h \
    ParamsVd_Yl__4p/ParamsMatrix__4_4p/paramsmatrix__4_4p.h \
    ParamsVd_Yl__4p/SelectMod__4_5p/selectmod__4_5p.h \
    bottomeval__2p.h \
    casth__1p.h \
    BottomEval__2p/bottomeval__2p.h \
    CastH__1p/casth__1p.h \
    Profile/profile.h \
    Profile/end.h \
    InData/constants.h \
    ParamsVsr_Yl__5p/paramsvsr_yl__5p.h \
    GetProfile__6p/getprofile__6p.h \
    WindShear/windshear.h \
    WindShear/constantswsh.h \
    InData/Constants.h \
    mhn/structures.h \
    mhn/mbulletin.h

unix:DESTDIR = ../plow_linux/_lib
win32:DESTDIR = $$OUT_PWD/..

target.path = /home/admin1/tets
INSTALLS += target

headers1.files  = InData/constants.h
headers1.path   = ../plow_aarch64/_include/PlowAlgoritm/InData
INSTALLS += headers1

headers2.files  = Profile/profile.h
headers2.path   = ../plow_aarch64/_include/PlowAlgoritm/Profile
INSTALLS += headers2

headers3.files  = mhn/mbulletin.h
headers3.path   = ../plow_aarch64/_include/PlowAlgoritm/mhn
INSTALLS += headers3

#include(climatdata.pri)

unix:!macx {
    # Определяем архитектуру
    ARCH = $$system(uname -m)

    contains(ARCH, aarch64) {
        # ARM 64-bit (aarch64)
        LIBS += -L$$PWD/../plow_aarch64/_lib/ -lClimatData
        INCLUDEPATH += $$PWD/../plow_aarch64/_include/ClimatData
        DEPENDPATH  += $$PWD/../plow_aarch64/_include/ClimatData
    } else {
        # Linux x86_64 (и прочие x86)
        SOURCES += ../climatData/climatdata.cpp \
                    ../climatData/climatdataprivate.cpp
        HEADERS += ../climatData/climatData.h \
                    ../climatData/climatdata_global.h \
                    ../climatData/climatdataprivate.h
        INCLUDEPATH += ../climatData
    }
}

