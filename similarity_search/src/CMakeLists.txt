#
# Non-metric Space Library
#
# Authors: Bilegsaikhan Naidan, Leonid Boytsov.
#
# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
#

file(GLOB HDR_FILES ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/method/*.h ${PROJECT_SOURCE_DIR}/include/space/*.h)
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cc ${PROJECT_SOURCE_DIR}/src/space/*.cc ${PROJECT_SOURCE_DIR}/src/method/*.cc)
file(GLOB SRC_FACTORY_FILES ${PROJECT_SOURCE_DIR}/src/factory/method/*.cc ${PROJECT_SOURCE_DIR}/src/factory/space/*.cc)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cc)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/tune_vptree.cc)
# The dummy application file also needs to be removed from the list
# of library source files:
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/dummy_app.cc)


include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/include/method ${PROJECT_SOURCE_DIR}/include/space)
message(STATUS "Header files: ${HDR_FILES}")
message(STATUS "Source files: ${SRC_FILES}")

include_directories (${PROJECT_SOURCE_DIR}/lshkit/include ${PROJECT_SOURCE_DIR}/src ${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

find_package (Threads)
if (Threads_FOUND)
    message (STATUS "Found Threads.")
else (Threads_FOUND)
    message (STATUS "Could not locate Threads.")
endif (Threads_FOUND)

add_library (NonMetricSpaceLib ${SRC_FILES} ${HDR_FILES})
add_dependencies (NonMetricSpaceLib lshkit)
add_executable (experiment main.cc ${SRC_FACTORY_FILES})
add_executable (tune_vptree tune_vptree.cc ${SRC_FACTORY_FILES})
# The following line is necessary to create an executable for the dummy application:
add_executable (dummy_app dummy_app.cc ${SRC_FACTORY_FILES})

target_link_libraries (experiment NonMetricSpaceLib lshkit ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (tune_vptree NonMetricSpaceLib lshkit ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
# What are the libraries that we need to link with for dummy_app?
target_link_libraries (dummy_app NonMetricSpaceLib lshkit ${Boost_LIBRARIES} 
                                                          ${GSL_LIBRARIES} 
                                                          ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
