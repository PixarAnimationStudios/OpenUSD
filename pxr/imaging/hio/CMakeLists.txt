set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hio)

pxr_library(hio
    LIBRARIES
        arch
        js
        plug
        tf
        vt
        trace
        ar
        hf

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        fieldTextureData
        glslfx
        glslfxConfig
        glslfxResourceLayout
        image
        imageRegistry
        types

    PRIVATE_CLASSES
        debugCodes
        dictionary

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        OpenEXR/openexr-c.h
        rankedTypeMap.h
        stb/stb_image.h
        stb/stb_image_resize.h
        stb/stb_image_write.h

    CPPFILES
        OpenEXRImage.cpp
        stbImage.cpp
        OpenEXR/openexr-c.c

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

set(TARGET_NAME "hio")
if(PXR_ENABLE_JS_SUPPORT)
    set(TARGET_NAME "hio_internal")
endif()

target_compile_definitions("${TARGET_NAME}" PRIVATE
    ILMTHREAD_THREADING_ENABLED
    OPENEXR_C_STANDALONE
    OPENEXR_EXPORT=static)
