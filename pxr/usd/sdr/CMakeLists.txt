set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdr)

pxr_library(sdr
    LIBRARIES
        arch
        plug
        trace
        tf
        vt
        ar
        ndr
        sdf

    PUBLIC_HEADERS
        api.h
        sdfTypeIndicator.h
        shaderNodeDiscoveryResult.h

    PUBLIC_CLASSES
        debugCodes
        declare
        discoveryPlugin
        filesystemDiscovery
        filesystemDiscoveryHelpers
        parserPlugin
        registry
        shaderMetadataHelpers
        shaderNode
        shaderProperty

    PYMODULE_CPPFILES
        module.cpp
        wrapDeclare.cpp
        wrapDiscoveryPlugin.cpp
        wrapFilesystemDiscovery.cpp
        wrapFilesystemDiscoveryHelpers.cpp
        wrapRegistry.cpp
        wrapSdfTypeIndicator.cpp
        wrapShaderNode.cpp
        wrapShaderNodeDiscoveryResult.cpp
        wrapShaderProperty.cpp

    PYMODULE_FILES
        __init__.py
        shaderParserTestUtils.py

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testSdrFilesystemDiscovery.py
    testenv/testSdrRegistry.py
    testenv/testSdrVersion.py
)

pxr_install_test_dir(
    SRC testenv/testSdrFilesystemDiscovery.testenv
    DEST testSdrFilesystemDiscovery
)

pxr_register_test(testSdrFilesystemDiscovery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrFilesystemDiscovery"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test_shared_lib(TestSdrRegistry
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
        PXR_NDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY=1
        PXR_NDR_SKIP_PARSER_PLUGIN_DISCOVERY=1
)

pxr_register_test(testSdrVersion
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrVersion"
    EXPECTED_RETURN_CODE 0
)
