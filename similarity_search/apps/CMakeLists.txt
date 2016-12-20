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
add_executable (report_intr_dim                 report_intr_dim.cc)
add_executable (test_clust                      test_clust.cc)
add_executable (bench_projection                bench_projection.cc)
add_executable (knn_stat                        knn_stat.cc)
# The following line is necessary to create an executable for the dummy application:
add_executable (dummy_app dummy_app.cc)

add_dependencies (experiment          NonMetricSpaceLib)
add_dependencies (tune_vptree         NonMetricSpaceLib)
add_dependencies (bench_distfunc      NonMetricSpaceLib)
add_dependencies (report_intr_dim     NonMetricSpaceLib)
add_dependencies (test_clust          NonMetricSpaceLib)
add_dependencies (bench_projection    NonMetricSpaceLib)
add_dependencies (knn_stat            knn_stat)
# The following line is necessary to create an executable for the dummy application:
add_dependencies (dummy_app           NonMetricSpaceLib)

if (WITH_EXTRAS) 
  add_dependencies (experiment      lshkit)

  set(LSHKIT_LIB "lshkit")
endif()

target_link_libraries (experiment       NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (tune_vptree      NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (bench_distfunc   NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (report_intr_dim  NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (test_clust       NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (bench_projection NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (knn_stat         NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
# The following line is necessary to create an executable for the dummy application:
target_link_libraries (dummy_app        NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
