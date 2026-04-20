#-------------------------------------------------
#
# Qwt Library configuration file
#
#-------------------------------------------------

# QWT_VER = 5.2.3
QWT_VER = 6.1.5

contains(QWT_VER,^5\\..*\\..*) {
    VER_SFX     = 5
    UNIX_SFX    = -qt4
} else {
    VER_SFX     =
    UNIX_SFX    =
}

unix {
    CONFIG += qwt
    LIBS += -lqwt-qt5
}

win32 {
    win32-x-g++ {
        QWT_PATH = /usr/qwt$${VER_SFX}-win
    } else {
        QWT_PATH = $$PATH_TO_REFERENCES/qwt/6.1.5/x64 #path relative to build directory
    }
    QWT_INC_PATH = $${QWT_PATH}/include
    CONFIG(debug,debug|release) {
        DEBUG_SFX =
    } else {
        DEBUG_SFX =
    }
    QWT_LIB = qwt$${DEBUG_SFX}$${VER_SFX}
    INCLUDEPATH += $${QWT_INC_PATH}
    LIBS += -L$${QWT_PATH}/lib -l$${QWT_LIB}
}
