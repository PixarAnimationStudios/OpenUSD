set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glf)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

set(optionalPublicClasses "")

pxr_library(glf
    LIBRARIES
        ar
        arch
        garch
        gf
        hf
        hio
        plug
        tf
        trace
        sdf

    PUBLIC_CLASSES
        contextCaps
        diagnostic
        glContext
        glRawContext
        info
        simpleLight
        simpleLightingContext
        simpleMaterial
        simpleShadowArray
        testGLContext
        ${optionalPublicClasses}

    PRIVATE_CLASSES
        debugCodes
        glContextRegistry

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        module.cpp
        wrapDiagnostic.cpp
        wrapSimpleLight.cpp
        wrapSimpleMaterial.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)
