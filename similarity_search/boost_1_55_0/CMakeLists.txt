include_directories(${Boost_INCLUDE_DIR})

file(GLOB boost_SRCS ${Boost_HOME_DIR}/libs/program_options/src/*.cpp)

add_library(boost_1_55_0 ${boost_SRCS})
target_link_libraries(boost_1_55_0 ${CMAKE_REQUIRED_LIBRARIES})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/release/")
else ()
    set (LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
    set (EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/debug/")
endif ()

