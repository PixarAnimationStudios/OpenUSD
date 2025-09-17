set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRender)

pxr_library(usdRender
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        gf
        tf
        usd
        usdGeom
        usdShade

    PUBLIC_CLASSES
        spec

    PUBLIC_HEADERS
        api.h

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
        images/aperture.svg
        images/dataWindow.svg
        images/nonSquarePixels.svg
        images/pixelAspectRatio.svg
        doxygen/renderSettings.usda
)
