set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_ENABLE_PYTHON_SUPPORT)
    message(STATUS "Not building usdview because PXR_ENABLE_PYTHON_SUPPORT=OFF")
    return()
elseif (NOT TARGET usdviewq)
    message(WARNING "Not building usdview because of missing dependency: usdviewq")
    return()
endif()

pxr_python_bin(usdview
    DEPENDENCIES
        usdviewq
)
