# CMake generated Testfile for 
# Source directory: /home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests
# Build directory: /home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/catch2_unit_test_include-b12d07c.cmake")
include("/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/regression_unit_test_include-b12d07c.cmake")
include("/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/KrigingTest_include-b12d07c.cmake")
include("/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/CacheTest_include-b12d07c.cmake")
add_test([=[class_unit_test]=] "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/class_unit_test")
set_tests_properties([=[class_unit_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;31;add_test;/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;0;")
add_test([=[function_unit_test]=] "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/function_unit_test")
set_tests_properties([=[function_unit_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;36;add_test;/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;0;")
add_test([=[demo/armadillo_example]=] "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/armadillo_example")
set_tests_properties([=[demo/armadillo_example]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;41;add_test;/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;0;")
add_test([=[GetVersion]=] "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/GetVersion")
set_tests_properties([=[GetVersion]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;68;add_test;/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;0;")
add_test([=[JsonTest]=] "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/tests/JsonTest")
set_tests_properties([=[JsonTest]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;73;add_test;/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/tests/CMakeLists.txt;0;")
