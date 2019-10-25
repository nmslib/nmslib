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

list(REMOVE_ITEM TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/bunit.cc)
list(REMOVE_ITEM TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/test_integr.cc)

add_executable (bunit bunit.cc    ${TEST_SRC_FILES})
add_executable (test_integr       test_integr.cc)

add_dependencies (bunit           NonMetricSpaceLib)
add_dependencies (test_integr     NonMetricSpaceLib)


if (WITH_EXTRAS) 
endif()

target_link_libraries (bunit        NonMetricSpaceLib   ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (test_integr  NonMetricSpaceLib   ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
