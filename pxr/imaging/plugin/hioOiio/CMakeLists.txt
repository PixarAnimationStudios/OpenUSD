set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hioOiio)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

if (NOT PXR_IMAGING_HIO_OIIO_FORMATS)
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_IMAGING_HIO_OIIO_FORMATS is empty")
    return()
endif()

# Use the import targets set by Imath's package config
if (Imath_FOUND)
    set(__OIIO_IMATH_LIBS "Imath::Imath")
else()
    set(__OIIO_IMATH_INCLUDE ${OPENEXR_INCLUDE_DIRS})
    set(__OIIO_IMATH_LIBS ${OPENEXR_LIBRARIES})
endif()

set(JSON_HIO_OIIO_FORMATS ${PXR_IMAGING_HIO_OIIO_FORMATS})
list(TRANSFORM JSON_HIO_OIIO_FORMATS REPLACE "[a-zA-Z0-9]+" "\"\\0\"")
list(JOIN JSON_HIO_OIIO_FORMATS ", " JSON_HIO_OIIO_FORMATS)

file(READ "${CMAKE_CURRENT_LIST_DIR}/plugInfo.json.in" content)
string(REPLACE "<JSON_HIO_OIIO_FORMATS>" "${JSON_HIO_OIIO_FORMATS}" content "${content}")
string(REPLACE "<PXR_IMAGING_HIO_OIIO_PRECEDENCE>" "${PXR_IMAGING_HIO_OIIO_PRECEDENCE}" content "${content}")
file(WRITE "${CMAKE_CURRENT_LIST_DIR}/plugInfo.json" "${content}")

pxr_plugin(hioOiio
    LIBRARIES
        ar
        arch
        gf
        hio
        tf
        ${OIIO_LIBRARIES}
        ${__OIIO_IMATH_LIBS}

    INCLUDE_DIRS
        ${OIIO_INCLUDE_DIRS}
        ${__OIIO_IMATH_INCLUDE}

    CPPFILES
        oiioImage.cpp
        metadata.cpp

    RESOURCE_FILES
        plugInfo.json
)
