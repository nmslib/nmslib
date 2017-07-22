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

if(NOT WIN32)
    CXX_COMPILER_DUMPVERSION(CXX_COMPILER_VERSION)
endif()
#message(FATAL_ERROR "stopping... compiler version is: ${CMAKE_CXX_COMPILER_ID} ${CXX_COMPILER_VERSION}")


if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    # require at least gcc 4.7
    if (CXX_COMPILER_VERSION VERSION_LESS 4.7)
        message(FATAL_ERROR "GCC version must be at least 4.7!")
    endif()
    # Uncomment the following lines to see how the code compiles without AVX,SSE4.2 and/or SSE2
    #set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -fpic -march=x86-64")
    #set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -fpic -march=core2")
    #set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Ofast -lm -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -fpic -msse4.2")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Wcast-align -Ofast -lm -lrt -DNDEBUG -std=c++11 -pthread  -DHAVE_CXX0X -march=native -Wl,--no-as-needed -fpic")
    set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wunreachable-code -Wcast-align -ggdb  -lm -lrt -DNDEBUG -std=c++11 -pthread  -DHAVE_CXX0X -march=native -Wl,--no-as-needed -fpic")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    if (CXX_COMPILER_VERSION VERSION_LESS 14.0.1)
        message(FATAL_ERROR "Intel version must be at least 14.0.1!")
    endif()
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Ofast -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -march=native -fpic")
    set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wunreachable-code -ggdb  -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -march=native -fpic")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if (CXX_COMPILER_VERSION VERSION_LESS 4.2.1)
        message(FATAL_ERROR "Clang version must be at least 3.4 (GCC >= 4.2.1 equivalent)!")
    endif()
    if (CMAKE_SYSTEM_NAME MATCHES Darwin)
        # MACOSX
        set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Wcast-align -O3 -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -fpic -march=native")
        set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wunreachable-code -Wcast-align -ggdb  -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -fpic -march=native")
    else()
        set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wunreachable-code -Wcast-align -O3 -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -march=native -fpic")
        set (CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wunreachable-code -Wcast-align -ggdb  -DNDEBUG -std=c++11 -DHAVE_CXX0X -pthread -march=native -fpic")
    endif()
    #message(FATAL_ERROR "CLANG ${CMAKE_SYSTEM_NAME}")
elseif(WIN32)
    # TODO how can we add support for later versions? There seems to be no flag with the semantics "version x or higher"
    if(NOT MSVC12 AND NOT MSVC14)
         message(FATAL_ERROR "On Windows, only MSVC version 12 and 14 are supported!") 
    endif()
else ()
    message(FATAL_ERROR "Unrecognized compiler (use GCC, Clang, Intel compiler, or MSVC (on Windows)!")
endif()

if (WITH_EXTRAS)
    message(STATUS "******************************")
    message(STATUS "Will build with extra stuff...")
    message(STATUS "******************************")
    add_definitions (-DWITH_EXTRAS=1)
endif()

if (WIN32)
    # With MSVC build types are useless, it's all handled by MSVC itself,
    # which creates build-specific output folders. However, they will all
    # be placed inside the debug subfolder.
    # To avoid this, we set these output variables.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
else()
    if (NOT CMAKE_BUILD_TYPE)
        set (CMAKE_BUILD_TYPE Release CACHE STRING "Build types: Release Debug" FORCE)
    endif (NOT CMAKE_BUILD_TYPE)
    message (STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if (WITH_EXTRAS)
  find_package (GSL REQUIRED)
  if (GSL_FOUND)
      message (STATUS "Found GSL.")
      include_directories (${GSL_INCLUDE_DIRS} ${GSLCBLAS_INCLUDE_DIRS})
      set (LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})
  else (GSL_FOUND)
      message (FATAL_ERROR "Could not locate GSL.")
  endif (GSL_FOUND)

  include_directories (${PROJECT_SOURCE_DIR}/lshkit/include)
  add_subdirectory (lshkit)

  find_package(Eigen3 3 REQUIRED)
  if (EIGEN3_FOUND)
    message (STATUS "Found Eigen3.")
    include_directories (${EIGEN3_INCLUDE_DIR})
  else ()
    message (FATAL_ERROR "Could not locate Eigen3.")
  endif ()

  # Can be used to hardwire boost location.
  #set (BOOST_ROOT $ENV{HOME}/boost_1_48_0)
  #set (Boost_INCLUDE_DIR ${BOOST_ROOT})
  #set (Boost_LIBRARY_DIR ${BOOST_ROOT}/stage/lib)

  # Alternatively one can do:
  # export BOOST_ROOT=<A location where boost is downloaded and compiled>
  # cmake . -DCMAKE_LIBRARY_PATH=<Boost and other library dir> -DCMAKE_INCLUDE_PATH=<Boost and other library include path> -DBoost_NO_SYSTEM_PATHS=true

  find_package (Boost 1.48 COMPONENTS system filesystem REQUIRED)

  if (Boost_FOUND)
    message (STATUS "Found BOOST.")
    include_directories (${Boost_INCLUDE_DIR})
    include_directories (${PROJECT_SOURCE_DIR}/lshkit/include ${PROJECT_SOURCE_DIR}/src ${Boost_INCLUDE_DIR})
    link_directories (${Boost_LIBRARY_DIR})
  else ()
    message (FATAL_ERROR "Could not locate BOOST.")
  endif ()
endif()

add_subdirectory (src)
add_subdirectory (apps)
if (NOT WITHOUT_TESTS)
    add_subdirectory (test)
endif()

message (STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CXX_COMPILER_VERSION}")
