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
        sdfTypeIndicator

    PUBLIC_HEADERS
        api.h
        nodeDiscoveryResult.h

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
        wrapSdfTypeIndicator.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)
