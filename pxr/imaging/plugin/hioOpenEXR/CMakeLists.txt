set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hioOpenEXR)

pxr_plugin(hioOpenEXR
    LIBRARIES
        ar
        arch
        gf
        hio
        tf

    PRIVATE_HEADERS
        OpenEXR/openexr-c.h
        OpenEXR/OpenEXRCoreUnity.h

    CPPFILES
        OpenEXR/openexr-c.c
        OpenEXRImage.cpp

    RESOURCE_FILES
        plugInfo.json
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if (TARGET hioOpenEXR)
        # Hide symbols and inlines in the C code
        target_compile_options(hioOpenEXR PRIVATE
             $<$<COMPILE_LANGUAGE:C>:-fvisibility=hidden>)
    endif()
endif()

pxr_build_test(testHioOpenEXR
    LIBRARIES
        hio
    CPPFILES
        testenv/testHioOpenEXR.cpp
)

# Tests relying on plugins don't run in static builds
if (BUILD_SHARED_LIBS)
    pxr_register_test(testHioOpenEXR
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHioOpenEXR"
        TESTENV testenv/testHioOpenEXR
        EXPECTED_RETURN_CODE 0
    )
endif()

