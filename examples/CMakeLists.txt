file (GLOB_RECURSE testSrcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file (GLOB_RECURSE srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ../src/*.cpp)

foreach (fileSrc ${testSrcs})
    get_filename_component (fileName ${fileSrc} NAME_WE)
    get_filename_component (filePath ${fileSrc} DIRECTORY)

    add_executable (${fileName} ${fileSrc} ${srcs})
    target_link_libraries (${fileName} enet)

    set_target_properties (${fileName} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${EXAMPLE_BIN}/${filePath})
endforeach (fileSrc)
