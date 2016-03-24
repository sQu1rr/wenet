file (GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file (GLOB wenetsrcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ../src/*.cpp)

foreach (fileSrc ${srcs})
    get_filename_component (fileName ${fileSrc} NAME_WE)
    get_filename_component (filePath ${fileSrc} DIRECTORY)

    add_executable (benchmark_${fileName} ${fileSrc} ${wenetsrcs})

    target_link_libraries (benchmark_${fileName} enet)
    set_target_properties (benchmark_${fileName} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${BENCH_BIN}/${filePath})
endforeach (fileSrc)
