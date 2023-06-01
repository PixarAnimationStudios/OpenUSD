set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glf)
set(optionalLibs "")
set(optionalIncludeDirs "")

set(optionalPublicClasses "")
if (X11_FOUND)
    list(APPEND optionalPublicClasses testGLContext)
endif()

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Building minimal ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    if (NOT APPLEIOS)
        list(APPEND optionalLibs ${OPENGL_gl_LIBRARY})
    endif()
else ()
    list(APPEND optionalLibs ${Boost_PYTHON_LIBRARY})
    list(APPEND optionalLibs ${X11_LIBRARIES})
    list(APPEND optionalLibs ${OPENGL_gl_LIBRARY})
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

INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
    ${optionalIncludeDirs}

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

CPPFILES
    ${optionalCppFiles}

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