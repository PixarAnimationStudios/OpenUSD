set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdcheck
    DEPENDENCIES
        tf
        sdf
)
