set(DIRS
    usdcat
    usddiff
    usddumpcrate
    usdgenschemafromsdr
    usdedit
    usdresolve
    usdstitch
    usdstitchclips
    sdfdump
    sdffilter
    usdtree
    usdupdatecrate
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (PXR_BUILD_DRACO_PLUGIN)
    add_subdirectory(usdcompress)
endif()

if (PXR_BUILD_PMC_PLUGIN)
    add_subdirectory(usdcrush)
endif()
