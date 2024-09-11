set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glf)


if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Building minimal ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
endif ()

if (TARGET OpenGL::GL)
    list(APPEND optionalLibs OpenGL::GL)
endif()

set(optionalPublicClasses "")
if (X11_FOUND)
    list(APPEND optionalLibs ${X11_LIBRARIES})
    list(APPEND optionalPublicClasses testGLContext)
endif()

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
        ${optionalLibs}

    PUBLIC_CLASSES
        bindingMap
        contextCaps
        diagnostic
        drawTarget
        glContext
        glRawContext
        info
        simpleLight
        simpleLightingContext
        simpleMaterial
        simpleShadowArray
        texture
        uniformBlock
        utils
        ${optionalPublicClasses}

    PRIVATE_CLASSES
        debugCodes
        glContextRegistry

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapDiagnostic.cpp
        wrapDrawTarget.cpp
        wrapSimpleLight.cpp
        wrapSimpleMaterial.cpp
        wrapTexture.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/pcfShader.glslfx
        shaders/simpleLighting.glslfx

    DOXYGEN_FILES
        overview.dox
)
