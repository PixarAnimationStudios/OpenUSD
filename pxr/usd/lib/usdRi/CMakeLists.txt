set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRi)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(usdRi
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdShade
        usdGeom
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        tokens
        statements
        lookAPI
        rmanUtilities
        rslShader
        risIntegrator
        risBxdf
        risObject
        risOslPattern
        risPattern
        
    PUBLIC_HEADERS
        api.h

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
        moduleDeps.cpp
        typeUtils

    PYMODULE_CPPFILES
        module.cpp
        wrapLookAPI.cpp
        wrapRmanUtilities.cpp
        wrapRisBxdf.cpp
        wrapRisIntegrator.cpp
        wrapRisObject.cpp
        wrapRisOslPattern.cpp
        wrapRisPattern.cpp
        wrapRslShader.cpp
        wrapStatements.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

