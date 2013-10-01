#
# Non-metric Space Library
#
# Authors: Bilegsaikhan Naidan, Leonid Boytsov.
#
# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
#

cmake_minimum_required (VERSION 2.8)

project (NonMetricSpaceLib)

set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -msse2 -msse4.1 -mssse3")
set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -ggdb  -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -msse2 -msse4.1 -mssse3")

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release CACHE STRING "Build types: Release Debug" FORCE)
endif (NOT CMAKE_BUILD_TYPE)
message (STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#set (Boost_DEBUG TRUE)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package (GSL REQUIRED)
if (GSL_FOUND)
    message (STATUS "Found GSL.")
    include_directories (${GSL_INCLUDE_DIRS} ${GSLCBLAS_INCLUDE_DIRS})
    set (LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})
else (GSL_FOUND)
    message (STATUS "Could not locate GSL.")
endif (GSL_FOUND)

#set (BOOST_ROOT $ENV{HOME}/boost_1_53_0)
set (Boost_INCLUDE_DIR ${BOOST_ROOT})
set (Boost_LIBRARY_DIR ${BOOST_ROOT}/stage/lib)
find_package (Boost 1.49 COMPONENTS system filesystem program_options REQUIRED)
include_directories (${Boost_INCLUDE_DIR})
link_directories (${Boost_LIBRARY_DIR})

add_subdirectory (lshkit)
add_subdirectory (src)
add_subdirectory (test)


