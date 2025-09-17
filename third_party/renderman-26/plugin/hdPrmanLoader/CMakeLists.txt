set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrmanLoader)

pxr_plugin(hdPrmanLoader
   LIBRARIES
        arch
        plug
        tf
        hd

    PUBLIC_CLASSES
        rendererPlugin

    PUBLIC_HEADERS
        api.h
        rendererPlugin.h

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

if(TARGET hdPrmanLoader)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()
