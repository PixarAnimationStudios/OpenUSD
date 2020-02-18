set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdTiny)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        hd
        tf

    PUBLIC_CLASSES
        mesh
        renderDelegate
        rendererPlugin
        renderPass

    RESOURCE_FILES
       plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

pxr_build_test(testHdTiny
    LIBRARIES
        hdTiny
        hdx
    
    CPPFILES
        testenv/testHdTiny.cpp
)

pxr_install_test_dir(
    SRC testenv/testHdTiny
    DEST testHdTiny
)

# This test only runs on MacOS and Linux for now.
if (APPLE)
pxr_register_test(testHdTiny
    ENV
        DYLD_INSERT_LIBRARIES=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/hdTiny.dylib
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdTiny" 
    STDOUT_REDIRECT output.txt
    DIFF_COMPARE output.txt
)
elseif (UNIX)
pxr_register_test(testHdTiny
    ENV
        LD_PRELOAD=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/hdTiny.so
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdTiny" 
    STDOUT_REDIRECT output.txt
    DIFF_COMPARE output.txt
)
endif()
