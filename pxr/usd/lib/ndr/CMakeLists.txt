set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE ndr)

pxr_library(ndr
    LIBRARIES
        tf
        plug
        vt
        work
        ar
        sdf
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        debugCodes
        declare
        discoveryPlugin
        filesystemDiscovery
        filesystemDiscoveryHelpers
        node
        parserPlugin
        property
        registry

    PUBLIC_HEADERS
        api.h
        nodeDiscoveryResult.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapDeclare.cpp
        wrapDiscoveryPlugin.cpp
        wrapFilesystemDiscovery.cpp
        wrapFilesystemDiscoveryHelpers.cpp
        wrapNode.cpp
        wrapNodeDiscoveryResult.cpp
        wrapProperty.cpp
        wrapRegistry.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

pxr_test_scripts(
    testenv/testNdrFilesystemDiscovery.py
    testenv/testNdrVersion.py
)

pxr_install_test_dir(
    SRC testenv/testNdrFilesystemDiscovery.testenv
    DEST testNdrFilesystemDiscovery
)

pxr_register_test(testNdrFilesystemDiscovery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testNdrFilesystemDiscovery"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testNdrVersion
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testNdrVersion"
    EXPECTED_RETURN_CODE 0
)