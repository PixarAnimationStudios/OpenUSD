set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdMtlx)

pxr_library(hdMtlx
    LIBRARIES
        gf
        hd
        sdf
        sdr
        tf
        trace
        usdMtlx
        vt
        MaterialXCore
        MaterialXFormat

    PUBLIC_CLASSES
        hdMtlx

    PUBLIC_HEADERS
        api.h
)

pxr_build_test(testHdMtlx
        LIBRARIES
        gf
        tf
        vt
        geomUtil
        pxOsd
        hdMtlx
        hdSt
        usdImaging
        CPPFILES
        testenv/testHdMtlx.cpp
)

pxr_install_test_dir(
        SRC testenv/testHdMtlx
        DEST testHdMtlx
)

pxr_register_test(testHdMtlx
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdMtlx"
        DIFF_COMPARE hdMtlx_log.txt
)
