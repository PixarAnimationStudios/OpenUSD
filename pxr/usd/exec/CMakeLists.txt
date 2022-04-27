set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE exec)

pxr_library(exec
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
        debugCodes
        registry
        execMetadataHelpers
        execNode
        execProperty

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapRegistry.cpp
        wrapExecNode.cpp
        wrapExecProperty.cpp

    PYMODULE_FILES
        __init__.py
        execParserTestUtils.py

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testExecRegistry.py
)

pxr_build_test_shared_lib(TestExecRegistry
    CREATE_FRAMEWORK
    INSTALL_PREFIX ExecPlugins
    LIBRARIES
        tf
        ndr
        exec
    CPPFILES
        testenv/TestExecRegistry_argsParserPlugin.cpp
        testenv/TestExecRegistry_discoveryPlugin.cpp
        testenv/TestExecRegistry_oslParserPlugin.cpp
)

pxr_register_test(testExecRegistry
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecRegistry"
    ENV
        PXR_EXEC_SKIP_DISCOVERY_PLUGIN_DISCOVERY=1
        PXR_EXEC_SKIP_PARSER_PLUGIN_DISCOVERY=1
)
