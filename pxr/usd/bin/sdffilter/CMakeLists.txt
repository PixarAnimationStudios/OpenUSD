set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(sdffilter
    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
 
    LIBRARIES
        arch
        tf
        sdf
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
