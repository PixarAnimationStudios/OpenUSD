set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdrOsl)

if(NOT PXR_ENABLE_OSL_SUPPORT)
    return()
endif()

if (Imath_FOUND)
    set(__OSL_IMATH_LIBS "Imath::Imath")
else()
    set(__OSL_IMATH_INCLUDE ${OPENEXR_INCLUDE_DIRS})
endif()

pxr_plugin(sdrOsl
    LIBRARIES
        gf
        tf
        vt
        ar
        ndr
        sdr
        ${__OSL_IMATH_LIBS}
        ${OSL_QUERY_LIBRARY}
        ${OIIO_LIBRARIES}

    INCLUDE_DIRS
        ${__OSL_IMATH_INCLUDE}
        ${OSL_INCLUDE_DIR}
        ${OIIO_INCLUDE_DIRS}

    PUBLIC_CLASSES
        oslParser

    PRIVATE_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapOslParser.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

pxr_test_scripts(
    testenv/testOslParser.py
)

pxr_install_test_dir(
    SRC testenv/testOslParser.testenv
    DEST testOslParser
)

pxr_register_test(testOslParser
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testOslParser"
    EXPECTED_RETURN_CODE 0
)
