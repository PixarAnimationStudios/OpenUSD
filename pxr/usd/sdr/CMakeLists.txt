set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdr)

pxr_library(sdr
    LIBRARIES
        arch
        plug
        trace
        tf
        vt
        work
        ar
        sdf

    PUBLIC_HEADERS
        api.h
        shaderNodeDiscoveryResult.h

    PUBLIC_CLASSES
        debugCodes
        declare
        discoveryPlugin
        filesystemDiscovery
        filesystemDiscoveryHelpers
        parserPlugin
        registry
        sdfTypeIndicator
        shaderMetadataHelpers
        shaderNode
        shaderNodeMetadata
        shaderNodeQuery
        shaderNodeQueryUtils
        shaderProperty
        shaderPropertyMetadata

    PYMODULE_CPPFILES
        module.cpp
        wrapDeclare.cpp
        wrapDiscoveryPlugin.cpp
        wrapFilesystemDiscovery.cpp
        wrapFilesystemDiscoveryHelpers.cpp
        wrapRegistry.cpp
        wrapSdfTypeIndicator.cpp
        wrapShaderMetadataHelpers.cpp
        wrapShaderNode.cpp
        wrapShaderNodeDiscoveryResult.cpp
        wrapShaderNodeMetadata.cpp
        wrapShaderNodeQuery.cpp
        wrapShaderNodeQueryUtils.cpp
        wrapShaderProperty.cpp
        wrapShaderPropertyMetadata.cpp

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
    testenv/testSdrShaderNodeQuery.py
    testenv/testSdrShownIfConversion.py
    testenv/testSdrVersion.py
)

pxr_register_test(testSdrFilesystemDiscovery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrFilesystemDiscovery"
    TESTENV testenv/testSdrFilesystemDiscovery.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testSdrShaderNodeQuery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrShaderNodeQuery"
    TESTENV testenv/testSdrShaderNodeQuery.testenv
    ENV
        PXR_SDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY=1
        PXR_SDR_SKIP_PARSER_PLUGIN_DISCOVERY=1
    EXPECTED_RETURN_CODE 0
)


pxr_build_test_shared_lib(TestSdrRegistry
    INSTALL_PREFIX SdrPlugins
    LIBRARIES
        tf
        sdr
    CPPFILES
        testenv/TestSdrRegistry_argsParserPlugin.cpp
        testenv/TestSdrRegistry_discoveryPlugin.cpp
        testenv/TestSdrRegistry_oslParserPlugin.cpp
)

pxr_build_test(testSdrShaderMetadata
    LIBRARIES
        gf
        sdf
        sdr
        tf
        vt
    CPPFILES
        testenv/testSdrShaderMetadata.cpp
)

pxr_build_test(testSdrParseValue
    LIBRARIES
        gf
        sdf
        sdr
        tf
        vt
    CPPFILES
        testenv/testSdrParseValue.cpp
)

pxr_register_test(testSdrShaderMetadata
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrShaderMetadata"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testSdrParseValue
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrParseValue"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testSdrRegistry
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrRegistry"
    ENV
        PXR_SDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY=1
        PXR_SDR_SKIP_PARSER_PLUGIN_DISCOVERY=1
)

pxr_register_test(testSdrShownIfConversion
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrShownIfConversion"
    TESTENV testenv/testSdrShownIfConversion.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testSdrVersion
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdrVersion"
    EXPECTED_RETURN_CODE 0
)
