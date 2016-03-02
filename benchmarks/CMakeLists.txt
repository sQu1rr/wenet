ExternalProject_Add (
    benchmark
    PREFIX ${CMAKE_BINARY_DIR}/benchmark
    GIT_REPOSITORY https://github.com/google/benchmark
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON)

ExternalProject_Get_Property(benchmark install_dir)
include_directories(${install_dir}/src/benchmark/include)

file (GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

foreach (fileSrc ${srcs})
    get_filename_component (fileName ${fileSrc} NAME_WE)
    get_filename_component (filePath ${fileSrc} DIRECTORY)

    add_executable (benchmark_${fileName} ${fileSrc})

    add_dependencies (benchmark_${fileName} benchmark)
    target_link_libraries (benchmark_${fileName}
        ${install_dir}/src/benchmark-build/src/libbenchmark.a)

    set_target_properties (benchmark_${fileName} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${BENCH_BIN}/${filePath})
endforeach (fileSrc)
