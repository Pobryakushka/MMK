#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libKriging::Kriging" for configuration "Release"
set_property(TARGET libKriging::Kriging APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libKriging::Kriging PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libKriging.so.0.8.3"
  IMPORTED_SONAME_RELEASE "libKriging.so.0"
  )

list(APPEND _cmake_import_check_targets libKriging::Kriging )
list(APPEND _cmake_import_check_files_for_libKriging::Kriging "${_IMPORT_PREFIX}/lib/libKriging.so.0.8.3" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
