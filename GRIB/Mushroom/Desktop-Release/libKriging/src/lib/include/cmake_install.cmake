# Install script for directory: /home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/installed")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/Bench.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/CacheFunction.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/Covariance.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/Kriging.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/KrigingException.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/KrigingLoader.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/LinearAlgebra.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/LinearRegression.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/LinearRegressionOptim.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/NoiseKriging.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/NuggetKriging.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/Optim.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/Random.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/Trend.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/demo" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/demo/DemoArmadilloClass.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/demo" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/demo/DemoClass.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/demo" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/demo/DemoFunction.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/demo" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/demo/demo.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/libKriging.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/ExplicitCopySpecifier.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/LinearHashStorage.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/cache_details.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/custom_hash_function.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/data_from_arma_vec.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/jsonutils.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/lk_armadillo.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/lkalloc.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils/nlohmann" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/nlohmann/json.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils/nlohmann" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/nlohmann/json_fwd.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging/utils" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/utils/utils.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libKriging" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/src/lib/include/libKriging/version.hpp")
endif()

