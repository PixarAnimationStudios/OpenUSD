set(DIRS
    usdImaging
    usdImagingGL
    usdProcImaging
    usdRiPxrImaging
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
