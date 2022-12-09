set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(sdfdump
    LIBRARIES
        arch
        plug
        tf
        sdf
)
