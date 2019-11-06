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

add_executable (experiment                      main.cc)
add_executable (tune_vptree                     tune_vptree.cc)
add_executable (bench_distfunc                  bench_distfunc.cc)
# The following line is necessary to create an executable for the dummy application:
add_executable (dummy_app dummy_app.cc)

add_dependencies (experiment          NonMetricSpaceLib)
add_dependencies (tune_vptree         NonMetricSpaceLib)
add_dependencies (bench_distfunc      NonMetricSpaceLib)
# The following line is necessary to create an executable for the dummy application:
add_dependencies (dummy_app           NonMetricSpaceLib)

target_link_libraries (experiment       NonMetricSpaceLib ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (tune_vptree      NonMetricSpaceLib  ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (bench_distfunc   NonMetricSpaceLib  ${CMAKE_THREAD_LIBS_INIT})
# The following line is necessary to create an executable for the dummy application:
target_link_libraries (dummy_app        NonMetricSpaceLib   ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
