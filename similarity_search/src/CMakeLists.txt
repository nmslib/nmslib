#
# Non-metric Space Library
#
# Authors: Bilegsaikhan Naidan, Leonid Boytsov.
#
# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
#

file(GLOB PROJ_HDR_FILES ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/method/*.h ${PROJECT_SOURCE_DIR}/include/space/*.h ${PROJECT_SOURCE_DIR}/include/factory/*.h ${PROJ_HDR_FILES}/include/factory/*/*.h)
file(GLOB OTH_HDR_FILES)
set(HDR_FILES ${PROJ_HDR_FILES} ${OTH_HDR_FILES})
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cc ${PROJECT_SOURCE_DIR}/src/space/*.cc ${PROJECT_SOURCE_DIR}/src/method/*.cc ${PROJECT_SOURCE_DIR}/src/method/*.c)

if (NOT WITH_EXTRAS)
  # Extra spaces
  list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/space/space_sqfd.cc)
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

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()

if (EMAX7)
    add_definitions (-DEMAX7)
    if (EMAXNC)
        add_definitions (-DEMAXNC)
    else ()
        add_definitions (-DARMZYNQ -DFPDDMA)
        set(IMAX_SOURCES ${CMAKE_SOURCE_DIR}/include/conv-c2d/emax7.h ${CMAKE_SOURCE_DIR}/include/conv-c2d/emax7lib.h ${CMAKE_SOURCE_DIR}/include/method/hnsw_distfunc_mv.h)

        # Set the path for intermediate files
        set(TEMP_DIR ${CMAKE_BINARY_DIR}/tmp)

        file(MAKE_DIRECTORY ${TEMP_DIR})

        add_custom_command(
            OUTPUT ${TEMP_DIR}/hnsw_distfunc_mv-emax7.c
            DEPENDS ${CMAKE_SOURCE_DIR}/src/method/hnsw_distfunc_mv.c
            COMMAND ${CMAKE_SOURCE_DIR}/include/conv-mark/conv-mark ${CMAKE_SOURCE_DIR}/src/method/hnsw_distfunc_mv.c > ${TEMP_DIR}/hnsw_distfunc_mv.c-mark.c
            COMMAND cpp -P -DEMAX7 -DARMZYNQ -DFPDDMA -I${CMAKE_SOURCE_DIR}/include ${TEMP_DIR}/hnsw_distfunc_mv.c-mark.c > ${TEMP_DIR}/hnsw_distfunc_mv.c-cppo.c
            COMMAND ${CMAKE_SOURCE_DIR}/include/conv-c2d/conv-c2d ${TEMP_DIR}/hnsw_distfunc_mv.c-cppo.c
        )

        add_custom_target(IMAX_KERNEL_GEN DEPENDS ${TEMP_DIR}/hnsw_distfunc_mv-emax7.c)

        target_sources(IMAX_KERNEL_GEN PRIVATE ${TEMP_DIR}/hnsw_distfunc_mv-emax7.c)

        set(IMAX_SOURCES ${IMAX_SOURCES} ${TEMP_DIR}/hnsw_distfunc_mv-emax7.c)
        set(IMAX_EXTRA_FLAGS -DEMAX7 -DARMZYNQ -DFPDDMA)
        
        list(FILTER SRC_FILES EXCLUDE REGEX ".*hnsw_distfunc_mv.c$")
    endif()
endif()

add_library (NonMetricSpaceLib ${SRC_FILES} ${HDR_FILES} ${IMAX_SOURCES})

string(LENGTH ${PROJECT_SOURCE_DIR} PREFIX_LEN) 
MATH(EXPR PREFIX_LEN "${PREFIX_LEN}+1")
foreach(F ${PROJ_HDR_FILES}) 
  GET_FILENAME_COMPONENT(FP ${F} PATH)
  string(SUBSTRING ${FP} ${PREFIX_LEN} -1 FS)
  #message(${PREFIX_LEN} ":" ${FS})
  install(FILES "${F}" DESTINATION ${FS})
endforeach(F)

install(TARGETS NonMetricSpaceLib   
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library
)

