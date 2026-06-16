

    _PATH = ..
    _PATH_INC = $${_PATH}/_include/ClimatData
    CONFIG(debug,debug|release) {
        DEBUG_SFX =
    } else {
        DEBUG_SFX =
    }

    _LIB = ClimatData$${DEBUG_SFX}

INCLUDEPATH += $${_PATH_INC}
LIBS += -L$$OUT_PWD/../_lib -l$${_LIB}
