set(DIRS
    usdcat
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

