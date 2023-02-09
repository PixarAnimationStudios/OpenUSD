if (APPLEIOS)
    message(STATUS
            "Skipping ${PXR_PACKAGE} because iOS doesn't support utility binaries")
    return()
endif()

set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(sdfdump
    LIBRARIES
        arch
        plug
        tf
        sdf
)
