set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_BUILD_USDVIEW)
    return()
endif()

pxr_python_bin(usdview
    DEPENDENCIES
        usdviewq
)
