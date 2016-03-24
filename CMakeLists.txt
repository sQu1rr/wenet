cmake_minimum_required (VERSION 2.8)

project (wenet)

set (FLAGS "-Wall -Werror -std=c++14 -pedantic-errors -pipe -lz -pthread")
set (FLAGS "${FLAGS} -fno-stack-protector") # Some linux distributions have it
set (FLAGS "${FLAGS} -march=native") # Building for this PC only

if (UNIX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
endif ()

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add (
    belks
    PREFIX ${CMAKE_BINARY_DIR}/belks
    GIT_REPOSITORY https://github.com/sQu1rr/belks.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON)
ExternalProject_Get_Property(belks source_dir)
include_directories (${source_dir}/include)

ExternalProject_Add (
    gsl
    PREFIX ${CMAKE_BINARY_DIR}/gsl
    GIT_REPOSITORY https://github.com/Microsoft/GSL.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON)
ExternalProject_Get_Property(gsl source_dir)
include_directories (${source_dir}/include)

ExternalProject_Add (
    convw
    PREFIX ${CMAKE_BINARY_DIR}/convw
    GIT_REPOSITORY https://github.com/sQu1rr/convw.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON)
ExternalProject_Get_Property(convw source_dir)
include_directories (${source_dir}/include)


enable_testing (true)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set (TEST_BIN ${CMAKE_SOURCE_DIR}/test-bin)
set (BENCH_BIN ${CMAKE_SOURCE_DIR}/benchmark-bin)
set (EXAMPLE_BIN ${CMAKE_SOURCE_DIR}/example-bin)

include_directories (src)
include_directories (include)

add_subdirectory (tests)
add_subdirectory (examples)
add_subdirectory (benchmarks)
