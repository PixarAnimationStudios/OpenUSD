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
        stb/stb_image_resize2.h
        stb/stb_image_write.h

    CPPFILES
        OpenEXRImage.cpp
        OpenEXR/openexr-c.c
        stbImage.cpp

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

pxr_build_test(testHioImage
    LIBRARIES
        ar
        hio
        plug
        tf

   CPPFILES
        testenv/testHioImage.cpp
)

pxr_register_test(testHioImage
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHioImage"
)
