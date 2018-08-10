set(PXR_PACKAGE vmp_usd)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        arch
        tf
        work
        usd
        usdUtils
        glf
        usdImaging
        usdImagingGL
        katanaPluginApi
        usdKatana

    CPPFILES
        debugCodes.cpp
        usdVMP.cpp
)

