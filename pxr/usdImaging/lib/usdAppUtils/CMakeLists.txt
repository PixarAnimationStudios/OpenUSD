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
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        camera

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapCamera.cpp

    PYMODULE_FILES
        __init__.py
        cameraArgs.py
        colorArgs.py
        complexityArgs.py
        framesArgs.py
)

pxr_test_scripts(
    testenv/testUsdAppUtilsCamera.py
    testenv/testUsdAppUtilsCmdlineArgs.py
    testenv/testUsdAppUtilsComplexity.py
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
