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
if (PXR_ENABLE_PTEX_SUPPORT)
    list(APPEND optionalPublicClasses ptexMipmapTextureLoader)
    list(APPEND optionalLibs ${PTEX_LIBRARY})
    list(APPEND optionalIncludeDirs ${PTEX_INCLUDE_DIR})
endif()

set (optionalCppFiles "")
set (plugInfo plugInfo_NoOIIO.json)
if (PXR_BUILD_OPENIMAGEIO_PLUGIN)
    set (plugInfo plugInfo.json)
    list(APPEND optionalCppFiles oiioImage.cpp)
    list(APPEND optionalLibs ${OIIO_LIBRARIES})
    list(APPEND optionalIncludeDirs ${OIIO_INCLUDE_DIRS})
endif()
pxr_library(glf
    LIBRARIES
        arch
        garch
        gf
        hf
        js
        plug
        tf
        trace
        sdf
        ${Boost_PYTHON_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}
        ${GLEW_LIBRARY}
        ${X11_LIBRARIES}
        ${OPENEXR_LIBRARY}
        ${optionalLibs}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        ${OPENEXR_INCLUDE_DIRS}
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        arrayTexture
        baseTexture
        baseTextureData
        bindingMap
        contextCaps
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
        
    PRIVATE_HEADERS
        rankedTypeMap.h
        stb/stb_image.h
        stb/stb_image_resize.h
        stb/stb_image_write.h

    CPPFILES
        dictionary.cpp
        ${optionalCppFiles}
        stbImage.cpp

    PYTHON_CPPFILES
        moduleDeps.cpp

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

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        ${plugInfo}:plugInfo.json 
        shaders/ptexTexture.glslfx
        shaders/pcfShader.glslfx
        shaders/simpleLighting.glslfx
        shaders/simpleShadowMapShader.glslfx
)
