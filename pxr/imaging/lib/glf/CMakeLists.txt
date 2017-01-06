set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glf)

set(optionalPublicClasses "")
if (X11_FOUND)
    list(APPEND optionalPublicClasses testGLContext)
endif()

pxr_shared_library(glf
    LIBRARIES
        arch
        garch
        gf
        js
        plug
        tf
        tracelite
        sdf
        ${Boost_PYTHON_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}
        ${OIIO_LIBRARIES}
        ${PTEX_LIBRARY}
        ${GLEW_LIBRARY}
        ${X11_LIBRARIES}

    INCLUDE_DIRS
        ${OIIO_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        ${PTEX_INCLUDE_DIR}

    PUBLIC_CLASSES
        arrayTexture
        baseTexture
        baseTextureData
        bindingMap
        diagnostic
        drawTarget
        glew
        glslfx
        glslfxConfig
        glContext
        glRawContext
        image
        imageRegistry
        info
        package
        ptexTexture
        ptexMipmapTextureLoader
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

    PRIVATE_HEADERS
        rankedTypeMap.h

    CPPFILES
        dictionary.cpp
        moduleDeps.cpp
        oiioImage.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBaseTexture.cpp
        wrapDiagnostic.cpp
        wrapDrawTarget.cpp
        wrapGlew.cpp
        wrapSimpleLight.cpp
        wrapSimpleMaterial.cpp
        wrapTexture.cpp
        wrapTextureRegistry.cpp
        wrapUVTexture.cpp
        wrapUVTextureStorage.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/ptexTexture.glslfx
        shaders/pcfShader.glslfx
        shaders/simpleLighting.glslfx
        shaders/simpleShadowMapShader.glslfx
)
