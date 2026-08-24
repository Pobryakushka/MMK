cmake_minimum_required(VERSION 3.16)
project(PlowAlgoritm VERSION 0.1.11 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up AUTOMOC and some sensible defaults for runtime execution
# When using Qt 6.3, you can replace the code block below with
# qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)

add_library(PlowAlgoritm SHARED
    BottomEval__2p/BottomEval__2p.cpp
    BottomEval__2p/bottomeval__2p.h
    CastH__1p/casth__1p.cpp CastH__1p/casth__1p.h
    FunctionsCalc/calc_bottomlayer.cpp FunctionsCalc/calc_bottomlayer.h
    FunctionsCalc/calc_error.cpp FunctionsCalc/calc_error.h
    FunctionsCalc/functionscalc.cpp FunctionsCalc/functionscalc.h
    FunctionsCalc/functionscalc_layer.cpp FunctionsCalc/functionscalc_layer.h
    FunctionsCalc/mathfunc.cpp FunctionsCalc/mathfunc.h
    GetProfile__6p/getprofile__6p.cpp GetProfile__6p/getprofile__6p.h
    InData/indata.h InData/indata.cpp
    InData/constants.cpp InData/constants.h
    InData/indataclimat.cpp InData/indataclimat.h
    OutData/outdata.cpp OutData/outdata.h
    ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.cpp ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h
    ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.cpp ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.h
    ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.cpp ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.h
    ParamsVdVsr_0_200m__3_4p/ParamsVdVsr_0_200m__3_4p.cpp
    ParamsVdVsr_0_200m__3_4p/paramsvdvsr_0_200m__3_4p.h
    ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.cpp ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.h
    ParamsVd_Yl__4p/ParamsMatrix__4_4p/paramsmatrix__4_4p.cpp ParamsVd_Yl__4p/ParamsMatrix__4_4p/paramsmatrix__4_4p.h
    ParamsVd_Yl__4p/ParamsVYl__4_7p/paramsvyl__4_7p.cpp ParamsVd_Yl__4p/ParamsVYl__4_7p/paramsvyl__4_7p.h
    ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.cpp ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h
    ParamsVd_Yl__4p/ParamsVd_Yl__4_2p/paramsvd_yl__4_2p.cpp ParamsVd_Yl__4p/ParamsVd_Yl__4_2p/paramsvd_yl__4_2p.h
    ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.cpp ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h
    ParamsVd_Yl__4p/ParamsdelV__4_6p/paramsdelv__4_6p.cpp ParamsVd_Yl__4p/ParamsdelV__4_6p/paramsdelv__4_6p.h
    ParamsVd_Yl__4p/SelectMod__4_5p/selectmod__4_5p.cpp ParamsVd_Yl__4p/SelectMod__4_5p/selectmod__4_5p.h
    ParamsVd_Yl__4p/paramsvd_yl__4p.cpp ParamsVd_Yl__4p/paramsvd_yl__4p.h
    ParamsVsr_Yl__5p/paramsvsr_yl__5p.cpp ParamsVsr_Yl__5p/paramsvsr_yl__5p.h
    Profile/end.cpp Profile/end.h
    Profile/profile.cpp Profile/profile.h
    Subsidiary/fileprocessing.cpp Subsidiary/fileprocessing.h
    WindShear/constantswsh.cpp WindShear/constantswsh.h
    WindShear/windshear.cpp WindShear/windshear.h
    main.cpp
    mhn/mbulletin.h
    mhn/structures.h
    main.cpp
    mhn/mbulletin.h
    mhn/structures.h
    ../climatData/climatdata.cpp
    ../climatData/climatdataprivate.cpp
)
target_include_directories(PlowAlgoritm PUBLIC
    ../climatData
)

target_compile_features(PlowAlgoritm PRIVATE
    cxx_std_11
)

# if(UNIX AND NOT MACOS)
#    target_link_libraries(PlowAlgoritm PRIVATE
#        ClimatData
#    )
#target_link_directories(PlowAlgoritm PRIVATE
#    ../plow_aarch64/_lib
#)
#endif()

# add_custom_command(TARGET PlowAlgoritm POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy
#            $<TARGET_FILE:PlowAlgoritm>
#            "${CMAKE_SOURCE_DIR}/../plow_aarch64/_lib/"
#    COMMENT "Copying binary to destination..."
# )
set(CMAKE_INSTALL_PREFIX "/data-flash/soft")
install(TARGETS PlowAlgoritm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
