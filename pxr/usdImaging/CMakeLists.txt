set(DIRS
    usdImaging
    usdImagingGL
    usdImagingMtlx
    usdProcImaging
    usdRiImaging
    usdSkelImaging
    usdVolImaging
    usdAppUtils
    usdviewq
    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
