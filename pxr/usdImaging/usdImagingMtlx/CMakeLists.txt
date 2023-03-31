set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingMtlx)

if(NOT ${PXR_ENABLE_MATERIALX_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_MATERIALX_SUPPORT is OFF")
    return()
endif()

pxr_library(usdImagingMtlx
    LIBRARIES
        usdMtlx
        usdImaging

    PUBLIC_CLASSES
        adapter

    PUBLIC_HEADERS
        api.h

    DISABLE_PRECOMPILED_HEADERS
)

pxr_build_test(testUsdImagingMtlxAdapter
    LIBRARIES
        usdImagingMtlx
    CPPFILES
        testenv/testUsdImagingMtlxAdapter.cpp
)
pxr_install_test_dir(
    SRC testenv/testUsdImagingMtlxAdapter.testenv
    DEST testUsdImagingMtlxAdapter
)
pxr_register_test(testUsdImagingMtlxAdapter
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingMtlxAdapter"
    EXPECTED_RETURN_CODE 0
)
