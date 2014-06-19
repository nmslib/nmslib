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
list(REMOVE_ITEM TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/bench_distfunc.cc)

add_executable (bunit bunit.cc ${TEST_SRC_FILES})
add_executable (bench_distfunc bench_distfunc.cc)
add_executable (test_integr test_integr.cc)

add_dependencies (test_integr NonMetricSpaceLib)
add_dependencies (bunit NonMetricSpaceLib)
add_dependencies (bench_distfunc NonMetricSpaceLib)

if (NOT WIN32)
  add_dependencies (bunit lshkit)
  add_dependencies (bench_distfunc lshkit)
  set(LSHKIT_LIB "lshkit")
  set(GSL_LIB    ${GSL_LIBRARIES})
  set(BOOST_LIB  ${Boost_LIBRARIES})
else()
# NO lshkit for Win32
  set(LSHKIT_LIB "")
  set(GSL_LIB "")
  set(BOOST_LIB "")
endif()

target_link_libraries (bunit NonMetricSpaceLib ${LSHKIT_LIB} ${GSL_LIB} )
target_link_libraries (bench_distfunc NonMetricSpaceLib ${LSHKIT_LIB} ${GSL_LIB} )
target_link_libraries (test_integr NonMetricSpaceLib ${LSHKIT_LIB} ${GSL_LIB} ${BOOST_LIB})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
