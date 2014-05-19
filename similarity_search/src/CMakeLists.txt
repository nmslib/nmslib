#
# Non-metric Space Library
#
# Authors: Bilegsaikhan Naidan, Leonid Boytsov.
#
# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
#

file(GLOB HDR_FILES ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/method/*.h ${PROJECT_SOURCE_DIR}/include/space/*.h ${Boost_INCLUDE_DIR})
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cc ${PROJECT_SOURCE_DIR}/src/space/*.cc ${PROJECT_SOURCE_DIR}/src/method/*.cc)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cc)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/tune_vptree.cc)
# The dummy application file also needs to be removed from the list
# of library source files:
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/dummy_app.cc)

# NO lshkit for Win32
if (WIN32)
  list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/method/lsh.cc)
  list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/method/lsh_multiprobe.cc)
  list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/method/lsh_space.cc)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
message(STATUS "Header files: ${HDR_FILES}")
message(STATUS "Source files: ${SRC_FILES}")

link_directories(${Boost_LIBRARY_DIRS})

find_package (Threads)
if (Threads_FOUND)
    message (STATUS "Found Threads.")
else (Threads_FOUND)
    message (STATUS "Could not locate Threads.")
endif (Threads_FOUND)

add_library (NonMetricSpaceLib ${SRC_FILES} ${HDR_FILES})
if (NOT WIN32)
  add_dependencies (NonMetricSpaceLib lshkit)
  set(LSHKIT_LIB "lshkit")
else()
# NO lshkit for Win32
  set(LSHKIT_LIB "")
endif()
add_executable (experiment main.cc)
add_executable (tune_vptree tune_vptree.cc)
# The following line is necessary to create an executable for the dummy application:
add_executable (dummy_app dummy_app.cc)

target_link_libraries (experiment NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (tune_vptree NonMetricSpaceLib ${LSHKIT_LIB} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
# What are the libraries that we need to link with for dummy_app?
target_link_libraries (dummy_app NonMetricSpaceLib ${LSHKIT_LIB} 
                                                          ${Boost_LIBRARIES} 
                                                          ${GSL_LIBRARIES} 
                                                          ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()
