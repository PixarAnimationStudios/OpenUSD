set(PXR_PREFIX pxr/usdImaging)
# XXX: PXR_PACKAGE must be set to 'usdrecord_script' to match the target created 
# by the call to pxr_python_bin() below.  
set(PXR_PACKAGE usdrecord_script)

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

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

if (APPLE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on macOS")
    return()
endif()

if (WIN32)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on Windows")
    return()
endif()

# usdrecord tests require plugin mechanism to find different renderer
# plugins, which we do not support for static build configurations due to 
# potential conflict in client applications linking against static build USD
if (NOT BUILD_SHARED_LIBS)
    message(STATUS "Skipping ${PXR_PACKAGE} tests on static build configuration")
    return()
endif()

pxr_install_test_dir(
    SRC testenv/RecordTest
    DEST testUsdRecordSingleFrame
)

pxr_install_test_dir(
    SRC testenv/RecordTest
    DEST testUsdRecordMultipleFrames
)

pxr_install_test_dir(
    SRC testenv/RecordTest
    DEST testUsdRecordSessionLayer
)

pxr_install_test_dir(
    SRC testenv/SmallObjectTest
    DEST SmallObjectTest
)


pxr_register_test(testUsdRecordSingleFrame
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord AnimCube.usda AnimCube.1.png"
    IMAGE_DIFF_COMPARE
        AnimCube.1.png
    FAIL 0.5
    FAIL_PERCENT 0.005
    WARN 0.05
    WARN_PERCENT 0.0075
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdRecordSingleFrame
)

pxr_register_test(testUsdRecordSingleFrame_NoGpu
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord --disableGpu AnimCube.usda AnimCubeNoGpu.1.png"
    IMAGE_DIFF_COMPARE
        AnimCubeNoGpu.1.png
    FAIL 0.5
    FAIL_PERCENT 0.005
    WARN 0.05
    WARN_PERCENT 0.0075
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdRecordSingleFrame
)

pxr_register_test(testUsdRecordMultipleFrames
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord AnimCube.usda AnimCube.#.png --frames 1,5,10"
    IMAGE_DIFF_COMPARE
        AnimCube.1.png
        AnimCube.5.png
        AnimCube.10.png
    FAIL 0.5
    FAIL_PERCENT 0.005
    WARN 0.05
    WARN_PERCENT 0.0075
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdRecordMultipleFrames
)

pxr_register_test(testUsdRecordMultipleFrames_NoGpu
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord --disableGpu AnimCube.usda AnimCubeNoGpu.#.png --frames 1,5,10"
    IMAGE_DIFF_COMPARE
        AnimCubeNoGpu.1.png
        AnimCubeNoGpu.5.png
        AnimCubeNoGpu.10.png
    FAIL 0.5
    FAIL_PERCENT 0.005
    WARN 0.05
    WARN_PERCENT 0.0075
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdRecordMultipleFrames
)

pxr_register_test(testUsdRecordSessionLayer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord --sessionLayer overrideDisplayColor.usda AnimCube.usda AnimCube_overrides.1.png"
    IMAGE_DIFF_COMPARE
        AnimCube_overrides.1.png
    FAIL 0.5
    FAIL_PERCENT 0.005
    WARN 0.05
    WARN_PERCENT 0.0075
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdRecordSessionLayer
)

pxr_register_test(testUsdRecordSmallObject
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdrecord test.usda out.png"
    IMAGE_DIFF_COMPARE
        out.png
    FAIL 0.5
    FAIL_PERCENT 0.005
    WARN 0.05
    WARN_PERCENT 0.0075
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdRecordSmallObject
)

