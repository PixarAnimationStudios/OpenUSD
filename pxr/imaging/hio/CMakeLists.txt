set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hio)

set (optionalCppFiles)
set (optionalPrivateHeaders)
if (PXR_IMAGING_HIO_STB_FORMATS)
    list(APPEND optionalCppFiles
        stbImage.cpp
    )
    list(APPEND optionalPrivateHeaders
        stb/stb_image.h
        stb/stb_image_resize2.h
        stb/stb_image_write.h
    )
endif()
if (PXR_IMAGING_HIO_OPENEXR_FORMATS)
    list(APPEND optionalCppFiles
        OpenEXRImage.cpp
        OpenEXR/openexr-c.c
    )
    list(APPEND optionalPrivateHeaders
        OpenEXR/openexr-c.h
    )
endif()

set(JSON_HIO_STB_FORMATS ${PXR_IMAGING_HIO_STB_FORMATS})
list(TRANSFORM JSON_HIO_STB_FORMATS REPLACE "[a-zA-Z0-9]+" "\"\\0\"")
list(JOIN JSON_HIO_STB_FORMATS ", " JSON_HIO_STB_FORMATS)

set(JSON_HIO_OPENEXR_FORMATS ${PXR_IMAGING_HIO_OPENEXR_FORMATS})
list(TRANSFORM JSON_HIO_OPENEXR_FORMATS REPLACE "[a-zA-Z0-9]+" "\"\\0\"")
list(JOIN JSON_HIO_OPENEXR_FORMATS ", " JSON_HIO_OPENEXR_FORMATS)

file(READ "${CMAKE_CURRENT_LIST_DIR}/plugInfo.json.in" content)
string(REPLACE "<JSON_HIO_OPENEXR_FORMATS>" "${JSON_HIO_OPENEXR_FORMATS}" content "${content}")
string(REPLACE "<PXR_IMAGING_HIO_OPENEXR_PRECEDENCE>" "${PXR_IMAGING_HIO_OPENEXR_PRECEDENCE}" content "${content}")
string(REPLACE "<JSON_HIO_STB_FORMATS>" "${JSON_HIO_STB_FORMATS}" content "${content}")
string(REPLACE "<PXR_IMAGING_HIO_STB_PRECEDENCE>" "${PXR_IMAGING_HIO_STB_PRECEDENCE}" content "${content}")
file(WRITE "${CMAKE_CURRENT_LIST_DIR}/plugInfo.json" "${content}")

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
        rankedTypeMap.h
        ${optionalPrivateHeaders}

    CPPFILES
        ${optionalCppFiles}

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
