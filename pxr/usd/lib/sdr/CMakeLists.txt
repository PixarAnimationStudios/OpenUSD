set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdr)

pxr_library(sdr
    LIBRARIES
        tf
        vt
        ar
        ndr
        sdf
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h
        declare.h

    PUBLIC_CLASSES
        registry
        shaderMetadataHelpers
        shaderNode
        shaderProperty

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapRegistry.cpp
        wrapShaderNode.cpp
        wrapShaderProperty.cpp

    PYMODULE_FILES
        __init__.py
        shaderParserTestUtils.py
)

pxr_test_scripts(
    testenv/testSdrRegistry.py
)

pxr_build_test_shared_lib(TestSdrRegistry
    CREATE_FRAMEWORK
    INSTALL_PREFIX SdrPlugins
    LIBRARIES
        tf
        ndr
        sdr
    CPPFILES
        testenv/TestSdrRegistry_argsParserPlugin.cpp
        testenv/TestSdrRegistry_discoveryPlugin.cpp
        testenv/TestSdrRegistry_oslParserPlugin.cpp
)

pxr_register_test(testSdrRegistry
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrRegistry"
    ENV
        PXR_DISABLE_STANDARD_PLUG_SEARCH_PATH=1
)
