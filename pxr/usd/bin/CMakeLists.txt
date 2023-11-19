if (APPLEIOS)
    message(STATUS
            "Skipping ${PXR_PACKAGE} because iOS doesn't support utility binaries")
    return()
endif()

set(DIRS
    usdcat
    usdchecker
    usddiff
    usddumpcrate
    usdgenschemafromsdr
    usdedit
    usdfixbrokenpixarschemas
    usdresolve
    usdstitch
    usdstitchclips
    usdzip
    sdfdump
    sdffilter
    usdtree
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (PXR_BUILD_DRACO_PLUGIN)
    add_subdirectory(usdcompress)
endif()

