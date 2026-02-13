set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrmanXpuLoader)

pxr_plugin(hdPrmanXpuLoader
    LIBRARIES
        arch
        plug
        tf
        hd
        hdPrmanLoader

    PUBLIC_CLASSES
        rendererPlugin

    RESOURCE_FILES
        plugInfo.json
    
    DISABLE_PRECOMPILED_HEADERS
)

if(TARGET hdPrmanXpuLoader)
    # XXX:
    # The build system does not expect plugins to link against other
    # plugins, so it does not automatically set up an RPATH that lets
    # us find hdPrmanLoader at runtime. We do that manually here,
    # assuming that hdPrmanLoader will be installed into the same
    # directory.
    get_target_property(rpath ${PXR_PACKAGE} INSTALL_RPATH)
    if (rpath)
        if (APPLE)
            list(APPEND rpath "@loader_path")
        else()
            list(APPEND rpath "$ORIGIN")
        endif()

        set_target_properties(${PXR_PACKAGE}
            PROPERTIES
                INSTALL_RPATH "${rpath}"
        )
    endif()

    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()

