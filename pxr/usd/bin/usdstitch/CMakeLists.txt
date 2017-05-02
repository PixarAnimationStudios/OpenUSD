set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdstitch
    DEPENDENCIES
        tf
        sdf
        usdUtils
)
