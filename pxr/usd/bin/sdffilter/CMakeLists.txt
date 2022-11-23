if (APPLEIOS)
    message(STATUS
            "Skipping ${PXR_PACKAGE} because iOS doesn't support utility binaries")
    return()
endif()

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
