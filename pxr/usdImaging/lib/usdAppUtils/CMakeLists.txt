set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdAppUtils)

pxr_library(usdAppUtils
    LIBRARIES
        garch
        gf
        glf
        sdf
        tf
        usd
        usdGeom
        usdImagingGL
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        camera
        frameRecorder

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapCamera.cpp
        wrapFrameRecorder.cpp

    PYMODULE_FILES
        __init__.py
        cameraArgs.py
        colorArgs.py
        complexityArgs.py
        framesArgs.py
        rendererArgs.py
)

pxr_test_scripts(
    testenv/testUsdAppUtilsCamera.py
    testenv/testUsdAppUtilsCmdlineArgs.py
    testenv/testUsdAppUtilsComplexity.py
    testenv/testUsdAppUtilsFrameRecorder.py
)

pxr_install_test_dir(
    SRC testenv/CameraTest
    DEST testUsdAppUtilsCamera
)
pxr_register_test(testUsdAppUtilsCamera
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAppUtilsCamera"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAppUtilsCmdlineArgs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAppUtilsCmdlineArgs"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAppUtilsComplexity
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAppUtilsComplexity"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/FrameRecorderTest
    DEST testUsdAppUtilsFrameRecorder
)
if (NOT PXR_HEADLESS_TEST_MODE)
    pxr_register_test(testUsdAppUtilsFrameRecorder
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAppUtilsFrameRecorder"
        EXPECTED_RETURN_CODE 0
    )
endif()
