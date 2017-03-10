set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdcat
    DEPENDENCIES
        ar
        sdf
        usd
)
