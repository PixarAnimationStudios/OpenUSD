set(DIRS
    usdcat
    usdchecker
    usddiff
    usddumpcrate
    usdedit
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

