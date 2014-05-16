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

add_dependencies (NonMetricSpaceLib lshkit)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/test*.cc)

add_executable (bunit bunit.cc ${TEST_SRC_FILES})

target_link_libraries (bunit NonMetricSpaceLib lshkit ${GSL_LIBRARIES} )

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
