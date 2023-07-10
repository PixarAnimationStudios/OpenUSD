set(DIRS
    usdImaging
    usdImagingGL
    usdProcImaging
    usdRiPxrImaging
    usdSkelImaging
    usdVolImaging
    usdAppUtils
    usdviewq
    plugin
    hdEmscripten
    bin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
