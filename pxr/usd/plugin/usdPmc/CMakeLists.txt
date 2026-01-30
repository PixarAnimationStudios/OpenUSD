set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdPmc)

cmake_policy(SET CMP0144 NEW)

pxr_library(usdPmc
    LIBRARIES
        # List dependency libraries
        tf
        sdf
        usd
        usdGeom
        usdUtils
        Pmc::Pmc

    INCLUDE_DIRS

    CPPFILES
        # List non-schema cpp files
        usdPmcDecoder.cpp
        usdPmcEncoder.cpp
        usdPmcFileFormat.cpp
        usdPmcEncodeSession.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    PUBLIC_HEADERS
        api.h
        usdPmcEncoder.hpp
)
