set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glf)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

set(optionalPublicClasses "")
if (X11_FOUND)
    list(APPEND optionalPublicClasses testGLContext)
endif()

set(optionalLibs "")
set(optionalIncludeDirs "")

if (PXR_ENABLE_OPENVDB_SUPPORT)
    list(APPEND optionalPublicClasses vdbTextureData)
    list(APPEND optionalLibs ${OPENEXR_Half_LIBRARY} ${OPENVDB_LIBRARY})
    list(APPEND optionalIncludeDirs ${OPENVDB_INCLUDE_DIR})
endif()

pxr_library(glf
    LIBRARIES
        ar
        arch
        garch
        gf
        hf
        hio
        js
        plug
        tf
        trace
        sdf
        ${Boost_PYTHON_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${X11_LIBRARIES}
        ${optionalLibs}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        arrayTexture
        baseTexture
        baseTextureData
        bindingMap
        contextCaps
        diagnostic
        drawTarget
        field3DTextureDataBase
        glContext
        glRawContext
        info
        simpleLight
        simpleLightingContext
        simpleMaterial
        simpleShadowArray
        texture
        textureHandle
        textureRegistry
        uniformBlock
        utils
        uvTexture
        uvTextureData
        uvTextureStorage
        uvTextureStorageData
        ${optionalPublicClasses}

    PRIVATE_CLASSES
        debugCodes
        glContextRegistry

    PUBLIC_HEADERS
        api.h
        fieldTextureData.h

    CPPFILES
        ${optionalCppFiles}

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBaseTexture.cpp
        wrapDiagnostic.cpp
        wrapDrawTarget.cpp
        wrapSimpleLight.cpp
        wrapSimpleMaterial.cpp
        wrapTexture.cpp
        wrapTextureRegistry.cpp
        wrapUVTexture.cpp
        wrapUVTextureStorage.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/pcfShader.glslfx
        shaders/simpleLighting.glslfx

    DOXYGEN_FILES
        overview.dox
)
