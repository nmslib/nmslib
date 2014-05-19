#
# Non-metric Space Library
#
# Authors: Bilegsaikhan Naidan, Leonid Boytsov.
#
# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
#

include_directories (${NonMetricSpaceLib_SOURCE_DIR}/include ${NonMetricSpaceLib_SOURCE_DIR}/include/space ${NonMetricSpaceLib_SOURCE_DIR}/include)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/test*.cc)

add_executable (bunit bunit.cc ${TEST_SRC_FILES})

add_dependencies (bunit NonMetricSpaceLib)

if (NOT WIN32)
  add_dependencies (bunit lshkit)
  set(LSHKIT_LIB "lshkit")
else()
# NO lshkit for Win32
  set(LSHKIT_LIB "")
endif()

target_link_libraries (bunit NonMetricSpaceLib ${LSHKIT_LIB} ${GSL_LIBRARIES} )

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
