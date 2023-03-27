set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(sdffilter
    INCLUDE_DIRS
 
    LIBRARIES
        arch
        tf
        sdf
)
