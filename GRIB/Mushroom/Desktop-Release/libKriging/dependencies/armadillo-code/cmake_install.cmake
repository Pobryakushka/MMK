# Install script for directory: /home/otdel412/Desktop/AMS/Mushroom/mushroom/libKriging/dependencies/armadillo-code

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/tmp/include/" REGEX "/\\.svn$" EXCLUDE REGEX "/[^/]*\\.cmake$" EXCLUDE REGEX "/[^/]*\\~$" EXCLUDE REGEX "/[^/]*orig$" EXCLUDE)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so.11.4.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so.11"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/libarmadillo.so.11.4.2"
    "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/libarmadillo.so.11"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so.11.4.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so.11"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/libarmadillo.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libarmadillo.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Armadillo/CMake/ArmadilloLibraryDepends.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Armadillo/CMake/ArmadilloLibraryDepends.cmake"
         "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/CMakeFiles/Export/d5f1c8e7a348faccefbb4ca918240c99/ArmadilloLibraryDepends.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Armadillo/CMake/ArmadilloLibraryDepends-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Armadillo/CMake/ArmadilloLibraryDepends.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Armadillo/CMake" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/CMakeFiles/Export/d5f1c8e7a348faccefbb4ca918240c99/ArmadilloLibraryDepends.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Armadillo/CMake" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/CMakeFiles/Export/d5f1c8e7a348faccefbb4ca918240c99/ArmadilloLibraryDepends-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/installed/share/Armadillo/CMake/ArmadilloConfig.cmake;/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/installed/share/Armadillo/CMake/ArmadilloConfigVersion.cmake")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/installed/share/Armadillo/CMake" TYPE FILE FILES
    "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/InstallFiles/ArmadilloConfig.cmake"
    "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/InstallFiles/ArmadilloConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/otdel412/Desktop/AMS/Mushroom/mushroom/build/Desktop-Release/libKriging/dependencies/armadillo-code/tmp/misc/armadillo.pc")
endif()

