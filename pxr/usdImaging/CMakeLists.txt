set(DIRS
    usdImaging
    usdExecImaging
    usdImagingGL
    usdProcImaging
    usdSkelImaging
    usdVolImaging
    usdIrImaging
    usdAppUtils
    usdviewq
    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
