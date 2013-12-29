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

#
# Runs compiler with "-dumpversion" and parses major/minor
# version with a regex.
#
# Taken&Modified from Boost.cmake
#
function(CXX_COMPILER_DUMPVERSION _OUTPUT_VERSION)

  exec_program(${CMAKE_CXX_COMPILER}
    ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
    OUTPUT_VARIABLE COMPILER_VERSION
  )
  #string(REGEX REPLACE "([0-9])\\.([0-9])(\\.[0-9])?" "\\1\\2"
  #   COMPILER_VERSION ${COMPILER_VERSION})

  set(${_OUTPUT_VERSION} ${COMPILER_VERSION} PARENT_SCOPE)
endfunction()

CXX_COMPILER_DUMPVERSION(CXX_COMPILER_VERSION)


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # require at least gcc 4.7
    if (CXX_COMPILER_VERSION VERSION_LESS 4.7)
        message(FATAL_ERROR "GCC version must be at least 4.7!")
    endif()
    # Uncomment the following lines to see how the code compiles without AVX,SSE4.2 and/or SSE2
    #set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -march=x86-64")
    #set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -march=core2")
    #set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -msse4.2")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wcast-align -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -march=native")
    set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wcast-align -ggdb  -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -march=native")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    if (CXX_COMPILER_VERSION VERSION_LESS 14.0.1)
        message(FATAL_ERROR "Intel version must be at least 14.0.1!")
    endif()
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X  -march=native")
    set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -ggdb  -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X  -march=native")
else ()
    message(FATAL_ERROR "Please, use GCC or the Intel compiler!")
endif()


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

# Can be use to hardwire boost location.
#set (BOOST_ROOT $ENV{HOME}/boost_1_48_0)
set (Boost_INCLUDE_DIR ${BOOST_ROOT})
set (Boost_LIBRARY_DIR ${BOOST_ROOT}/stage/lib)
find_package (Boost 1.48 COMPONENTS system filesystem program_options REQUIRED)
include_directories (${Boost_INCLUDE_DIR})
link_directories (${Boost_LIBRARY_DIR})

add_subdirectory (lshkit)
add_subdirectory (src)
add_subdirectory (test)


