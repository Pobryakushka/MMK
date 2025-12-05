#-------------------------------------------------
#
# Qwt Library configuration file
#
#-------------------------------------------------


win32 {
#    win32-x-g++ {
#        QWT_PATH = /usr/qwt$${VER_SFX}-win
#    } else {
        _PATH = $$PATH_TO_REFERENCES/libcommanager
#    }
    _PATH_INC = $${_PATH}/include
    CONFIG(debug,debug|release) {
        DEBUG_SFX =
    } else {
        DEBUG_SFX =
    }
    QWT_LIB = commanager$${DEBUG_SFX}
}

_PATH = $$PATH_TO_REFERENCES/libcommanager

_PATH_INC = $${_PATH}/include

QWT_LIB = commanager

INCLUDEPATH += $${_PATH_INC}
LIBS += -L$${_PATH}/lib -l$${QWT_LIB}
