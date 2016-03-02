file (GLOB_RECURSE testSrcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file (GLOB_RECURSE srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ../src/*.cpp)

foreach (fileSrc ${testSrcs})
    get_filename_component (fileName ${fileSrc} NAME_WE)
    get_filename_component (filePath ${fileSrc} DIRECTORY)

    add_executable (test_${fileName} ${fileSrc} ${srcs})
    target_link_libraries (test_${fileName} enet)

    set_target_properties (test_${fileName} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${EXAMPLE_BIN}/${filePath})
endforeach (fileSrc)
