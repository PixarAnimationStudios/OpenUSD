set(PXR_PREFIX "")
set(PXR_PACKAGE rmanDiscovery)

set(optionalLibs "")
if(PXR_VERSION AND PXR_VERSION LESS 2508)
    list(APPEND optionalLibs ndr)
endif()

pxr_plugin(rmanDiscovery
    LIBRARIES
        arch
        gf
        plug
        tf
        vt
        ar
        sdr
        rmanArgsParser
        ${optionalLibs}

    PRIVATE_CLASSES
        rmanDiscovery

    RESOURCE_FILES
        plugInfo.json
)

if(TARGET rmanDiscovery)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
        "${PROJECT_SOURCE_DIR}/third_party/renderman/plugin"
    )

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
endif()
