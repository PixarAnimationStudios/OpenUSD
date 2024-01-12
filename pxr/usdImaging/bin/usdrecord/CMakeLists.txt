set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdrecord)

# usdrecord has usdAppUtils as a dependency, which is only built when GPU
# support is enabled.
if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

pxr_python_bin(usdrecord
    DEPENDENCIES
        usd
        usdAppUtils)

pxr_install_test_dir(
    SRC testenv/RecordTest
    DEST testUsdRecordSingleFrame
)

pxr_install_test_dir(
    SRC testenv/RecordTest
    DEST testUsdRecordMultipleFrames
)

pxr_install_test_dir(
    SRC testenv/SmallObjectTest
    DEST testUsdRecordSmallObject
)

if (NOT PXR_HEADLESS_TEST_MODE)

    pxr_register_test(testUsdRecordSingleFrame
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord AnimCube.usda AnimCube.png"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdRecordMultipleFrames
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord AnimCube.usda AnimCube.#.png --frames 1,5,10"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdRecordSmallObject
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord test.usda out.png"
        EXPECTED_RETURN_CODE 0
    )

endif()
